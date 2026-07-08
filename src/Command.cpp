#include "../include/Command.hpp"
#include <iostream>
#include <fstream>
#include <charconv>
#include <ranges>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>

/**
 * @file    Command.cpp
 * @brief   Implementação dos comandos do projeto.
 * 
 * Implementação dos comandos especificados do projeto.
 */

using Ext4::Wrappers::Image;

/**
 * @brief Predicado de conveniência para ver se a entrada do diretório existe com tal nome.
 */
constexpr auto by_name = [](const std::string &name) {
    return [&name](const Wrappers::DirectoryEntry &entry) {
        return entry.get_name() == name;
    };
};

/**
 * @brief Função recursiva que cria uma árvore de extents caso a profundidade seja maior que 1.
 * @returns O ID do bloco alocado para o nível.
 * @author Pedro Itiro Nagao
 */
uint32_t build_extent_tree(
    Wrappers::Image &img,
    Wrappers::Inode &inode,
    const bool has_metadata_csum,
    const uint16_t max_entries,
    const uint16_t max_per_block,
    const uint16_t depth,
    std::span<std::byte> block_bytes,
    std::span<Raw::ExtentLeaf> leafs
) {
    uint32_t blk_id = img.alloc_block();
    if(blk_id == 0) return 0; // Sem espaço: nenhum bloco livre disponível.
    uint32_t block_size = img.get_superblock().get_block_size();
    std::vector<std::byte> buf(block_size);
    std::span<std::byte> buf_bytes = Utils::as_byte_span(buf);
    // O cabeçalho já começa com o que é comum entre os dois
    Raw::ExtentHeader header{
        .eh_magic = Constants::EXTENT_MAGIC,
        .eh_max = max_entries,
    };
    // Caso base
    if(depth <= 0) {
        header.eh_depth = 0;
        header.eh_entries = static_cast<uint16_t>(leafs.size());
        // Escrevemos o cabeçalho
        Utils::write_to(buf_bytes, header);
        // Agora (finalmente) escrevemos as folhas e seus conteúdos
        for(size_t i = 0; i < leafs.size(); ++i) {
            // Escrevemos a folha em si
            Utils::write_to(buf_bytes, leafs[i], sizeof(header) + (i * sizeof(leafs[i])));
            // Agora escrevemos os dados da folha
            for(uint16_t j = 0; j < leafs[i].ee_len; ++j) {
                // Calcula a posição exata deste bloco dentro do arquivo global
                size_t file_offset = static_cast<size_t>(leafs[i].ee_block + j) * block_size;
                img.write_block(leafs[i].get_start_block() + j, block_bytes.subspan(file_offset, block_size));
            }
        }
        // Caso tenhamos checksums
        if(has_metadata_csum) {
            Raw::ExtentTail tail{
                .eb_checksum = Checksums::checksum_extent(inode, buf_bytes, block_size, img.get_superblock().get_checksum_seed())
            };
            Utils::write_to(buf_bytes, tail, buf_bytes.size() - sizeof(tail));
        }
        // Agora (finalmente) escrevemos o bloco com as folhas
        img.write_block(blk_id, buf_bytes);
    } else {
        // Aqui chamamos o caso recursivo com depth-1
        // Precisamos saber quantas folhas colocamos no nível.
        // Em um nível n temos max_per_block^depth
        uint32_t leafs_per_child = std::pow(max_per_block, depth);
        std::vector<Raw::ExtentIndex> index_entries{};
        size_t leaf_offset = 0;

        while(leaf_offset < leafs.size() && index_entries.size() < max_entries) {
            // Pega o pedaço de folhas que vai pertencer a este filho
            size_t chunk_size = std::min(leafs_per_child, static_cast<uint32_t>(leafs.size() - leaf_offset));
            std::span<Raw::ExtentLeaf> child_leafs = leafs.subspan(leaf_offset, chunk_size);

            // Chamada recursiva: o filho constrói a subárvore dele e nos devolve o bloco físico onde ele se salvou
            // Note que o filho (child) NÃO estará na raiz do Inode, então o max_entries dele será 'max_per_block' (um número bem maior)
            uint32_t child_phys_block = build_extent_tree(
                img, inode, has_metadata_csum, 
                max_per_block, max_per_block, 
                depth - 1, block_bytes, child_leafs
            );

            if(child_phys_block == 0) {
                std::println(std::cerr, "Erro: Não foi possível alocar blocos para a árvore de extents.");
                return 0;
            } else {
                // alocamos 1 bloco de índice, então precisamos atualizar o contador de blocos do inode
                Raw::Inode new_inode = inode.get_raw();
                uint64_t total_sectors = inode.get_sectors() + (block_size / 512);
                new_inode.i_blocks_lo = static_cast<uint32_t>(total_sectors & MAX_32BIT);
                new_inode.i_osd2.l_i_blocks_high = static_cast<uint16_t>((total_sectors >> 32) & MAX_16BIT);   
                inode.set_raw(new_inode);
            }

            // Criamos o índice apontando para esse filho
            Raw::ExtentIndex idx{
                .ei_block = child_leafs[0].ee_block,
                .ei_leaf_lo = child_phys_block,
                .ei_leaf_hi = 0,  // uint32_t nunca tem bits acima de 31
            };
            index_entries.push_back(idx);

            leaf_offset += chunk_size;
        }

        // Agora montamos o cabeçalho deste bloco de índice e escrevemos no buffer
        header.eh_entries = static_cast<uint16_t>(index_entries.size());
        header.eh_max = max_entries;
        header.eh_depth = depth;
        Utils::write_to(buf_bytes, header);

        // Escrevemos os índices gerados no buffer
        for (size_t i = 0; i < index_entries.size(); ++i) {
            Utils::write_to(buf_bytes, index_entries[i], sizeof(header) + (i * sizeof(Raw::ExtentIndex)));
        }

        // Caso tenhamos checksums
        if(has_metadata_csum) {
            Raw::ExtentTail tail{
                .eb_checksum = Checksums::checksum_extent(inode, buf_bytes, block_size, img.get_superblock().get_checksum_seed())
            };
            Utils::write_to(buf_bytes, tail, buf_bytes.size() - sizeof(tail));
        }
        
        img.write_block(blk_id, buf_bytes);
    }
    return blk_id;
}

/**
 * @brief Formata um timestamp Unix (uint32_t) em uma string legível "AAAA-MM-DD HH:MM:SS".
 * Se o timestamp for 0, retorna "-" (indicando campo não usado/vazio).
 */
static std::string format_time(const uint32_t timestamp) {
    if (timestamp == 0) return "-";
    std::time_t tt = static_cast<std::time_t>(timestamp);
    std::tm tm_buf{};
    localtime_r(&tt, &tm_buf);
    std::ostringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

/**
 * @brief Converte o campo i_mode em uma string estilo `ls -l` (ex: "drwxr-xr-x").
 */
static std::string mode_to_string(const uint16_t mode) {
    std::string result(10, '-');

    switch (mode & Constants::S_IFMT) {
        case Flags::S_IFDIR:  result[0] = 'd'; break;
        case Flags::S_IFLNK:  result[0] = 'l'; break;
        case Flags::S_IFCHR:  result[0] = 'c'; break;
        case Flags::S_IFBLK:  result[0] = 'b'; break;
        case Flags::S_IFIFO:  result[0] = 'p'; break;
        case Flags::S_IFSOCK: result[0] = 's'; break;
        default:              result[0] = '-'; break;
    }

    if (mode & Flags::S_IRUSR) result[1] = 'r';
    if (mode & Flags::S_IWUSR) result[2] = 'w';
    if (mode & Flags::S_IXUSR) result[3] = 'x';
    if (mode & Flags::S_IRGRP) result[4] = 'r';
    if (mode & Flags::S_IWGRP) result[5] = 'w';
    if (mode & Flags::S_IXGRP) result[6] = 'x';
    if (mode & Flags::S_IROTH) result[7] = 'r';
    if (mode & Flags::S_IWOTH) result[8] = 'w';
    if (mode & Flags::S_IXOTH) result[9] = 'x';

    if (mode & Flags::S_ISUID) result[3] = (result[3] == 'x') ? 's' : 'S';
    if (mode & Flags::S_ISGID) result[6] = (result[6] == 'x') ? 's' : 'S';
    if (mode & Flags::S_ISVTX) result[9] = (result[9] == 'x') ? 't' : 'T';

    return result;
}

/**
 * @brief Retorna uma descrição textual do tipo de arquivo a partir do campo i_mode.
 */
static std::string_view type_to_string(const uint16_t mode) {
    switch (mode & Constants::S_IFMT) {
        case Flags::S_IFDIR:  return "Diretório";
        case Flags::S_IFREG:  return "Arquivo regular";
        case Flags::S_IFLNK:  return "Link simbólico";
        case Flags::S_IFCHR:  return "Dispositivo de caracteres";
        case Flags::S_IFBLK:  return "Dispositivo de blocos";
        case Flags::S_IFIFO:  return "FIFO";
        case Flags::S_IFSOCK: return "Socket";
        default:               return "Desconhecido";
    }
}

short Command::help() {
    for (const auto &[cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::info(Image &img) {
    Wrappers::SuperBlock sb = img.get_superblock();

    std::println("=== Informações do Sistema de Arquivos ===");
    std::println("Nome do volume:            {}", sb.get_volume_name());
    std::println("UUID:                      {}", sb.get_uuid());
    std::println("Estado:                    {}", sb.get_state() == Flags::SuperBlockFlags::FS_STATE_CLEARLY_UNMOUNTED ? "Limpo" : "Com erros/Sujo");
    std::println("Nível de revisão:          {}", sb.get_rev_level());
    std::println();

    std::println("Suporte a 64 bits:         {}", sb.is_64bit() ? "Sim" : "Não");
    std::println("Suporte a extents:         {}", sb.has_extents() ? "Sim" : "Não");
    std::println("Diretórios com HTree:      {}", sb.has_dir_htree() ? "Sim" : "Não");
    // std::println("Arquivos grandes (>2GiB):  {}", sb.has_large_file() ? "Sim" : "Não");
    // std::println("Arquivos enormes:          {}", sb.has_huge_file() ? "Sim" : "Não");
    std::println("Checksum de metadados:     {}", sb.has_metadata_csum() ? "Sim" : "Não");
    std::println("Checksum de GDT (legado):  {}", sb.has_compat_gdt_csum() ? "Sim" : "Não");
    std::println();

    std::println("Tamanho do bloco:          {} bytes", sb.get_block_size());
    std::println("Tamanho do inode:          {} bytes", sb.get_inode_size());
    std::println("Tamanho do descritor:      {} bytes", sb.get_desc_size());
    std::println("Primeiro bloco de dados:   {}", sb.get_first_data_block());
    std::println("Blocos por grupo:          {}", sb.get_blocks_per_group());
    std::println("Inodes por grupo:          {}", sb.get_inodes_per_group());
    std::println("Clusters por grupo:        {}", sb.get_clusters_per_group());
    std::println("Quantidade de grupos:      {}", sb.get_group_count());
    std::println();

    std::println("Total de inodes:           {}", sb.get_inodes_count());
    std::println("Inodes livres:             {}", sb.get_free_inodes_count());
    std::println("Primeiro inode não-reserv.: {}", sb.get_first_ino());
    std::println();

    std::println("Total de blocos:           {}", sb.get_blocks_count());
    std::println("Blocos livres:             {}", sb.get_free_blocks_count());
    std::println("Blocos reservados (root):  {}", sb.get_reserved_blocks_count());
    std::println();

    std::println("Criado:                    {}", format_time(sb.get_mkfs_time()));
    std::println("Última montagem:           {}", format_time(sb.get_mtime()));
    std::println("Última escrita:            {}", format_time(sb.get_wtime()));
    std::println("Última verificação:        {}", format_time(sb.get_lastcheck()));
    std::println("Contagem de montagens:     {} / {}", sb.get_mnt_count(), sb.get_max_mnt_count());
    std::println();

    std::println("=== Grupos de Blocos ({} grupo(s)) ===", sb.get_group_count());
    for (const Wrappers::GroupDescriptor &gd : img.get_group_descriptors()) {
        std::println("--- Grupo {} ---", gd.get_group_number());
        std::println("  Bitmap de blocos (bloco):  {}", gd.get_block_bitmap_block());
        std::println("  Bitmap de inodes (bloco):  {}", gd.get_inode_bitmap_block());
        std::println("  Tabela de inodes (bloco):  {}", gd.get_inode_table_block());
        std::println("  Blocos livres:             {}", gd.get_free_blocks_count());
        std::println("  Inodes livres:             {}", gd.get_free_inodes_count());
        std::println("  Inodes não utilizados:     {}", gd.get_itable_unused());
        std::println("  Diretórios neste grupo:    {}", gd.get_used_dirs_count());
        std::println("  Flags:                     0x{:04x} (INODE_UNINIT={}, BLOCK_UNINIT={})",
            gd.get_flags(), gd.is_inode_uninit(), gd.is_block_uninit());
    }

    return 0;
}

short Command::cat(Image &img, const std::span<const std::string> args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";
    if (file_path.empty()) {
        std::println(std::cerr, "Uso: cat <arquivo>");
        return 1;
    }

    // Separamos o diretório do arquivo
    auto [path, file_name] = Utils::split_path(file_path);
    // Resolvemos o diretório para um inode
    auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());
    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);

    // Cria a view filtrada para ver se há um arquivo aqui.
    auto target_file = std::ranges::find_if(entries, by_name(file_name));

    // Verifica se o arquivo realmente foi encontrado antes de extrair para a variável
    if (target_file == entries.end()) {
        std::println(std::cerr, "Erro: Arquivo '{}' não encontrado.", file_name);
        return 1;
    }

    // Se for um diretório, saímos
    if (target_file->is_dir()) {
        std::println(std::cerr, "Erro: Arquivo '{}' na verdade é um diretório.", file_name);
        return 1;
    }

    // Agora (finalmente) listamos o conteúdo do arquivo
    Wrappers::Inode file_inode = img.get_inode(target_file->get_inode());
    std::vector<std::byte> bytes = img.read_file(file_inode);

    std::println("{}", std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    return 0;
}

short Command::attr(Image &img, const std::span<const std::string> args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: attr <arquivo/diretório>");
        return 1;
    }

    Wrappers::Inode target_inode;
    std::string display_name;
    uint32_t inode_id;

    // Caso especial: a raiz não possui uma entrada de diretório "visível" em si mesma dentro do pai
    if (target_path == "/") {
        target_inode = img.get_root_inode();
        display_name = "/";
        inode_id = target_inode.get_inode_id();
    } else {
        // Separamos o diretório do nome do alvo
        auto [path, name] = Utils::split_path(target_path);
        // Resolvemos o diretório pai
        auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());

        std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);
        auto target = std::ranges::find_if(entries, by_name(name));

        if (target == entries.end()) {
            std::println(std::cerr, "Erro: '{}' não encontrado.", name);
            return 1;
        }

        inode_id = target->get_inode();
        target_inode = img.get_inode(inode_id);
        display_name = name;
    }

    const uint16_t mode = target_inode.get_mode();
    const Wrappers::SuperBlock sb = img.get_superblock();

    std::println("=== Atributos de '{}' ===", display_name);
    std::println("Inode:                     {}", inode_id);
    std::println("Tipo:                      {}", type_to_string(mode));
    std::println("Permissões:                {} (0{:o})", mode_to_string(mode), mode & 0xFFF);
    std::println("UID:                       {}", target_inode.get_uid());
    std::println("GID:                       {}", target_inode.get_gid());
    std::println("Tamanho:                   {} bytes", target_inode.get_size());
    std::println("Contagem de links:         {}", target_inode.get_links_count());
    std::println("Blocos alocados (512B):    {}", target_inode.get_raw().i_blocks_lo);
    std::println("Geração:                   {}", target_inode.get_inode_generation());
    std::println();

    std::println("Último acesso:             {}", format_time(target_inode.get_atime()));
    std::println("Última modificação:        {}", format_time(target_inode.get_mtime()));
    std::println("Última alteração (ctime):  {}", format_time(target_inode.get_ctime()));
    std::println();

    std::println("Usa extents:               {}", target_inode.has_extents() ? "Sim" : "Não");
    std::println("Flags (i_flags):           0x{:08x}", target_inode.get_flags());

    if (sb.has_metadata_csum()) {
        uint32_t seed = sb.get_checksum_seed();
        uint32_t calc_checksum = Checksums::checksum_inode(target_inode, seed);
        std::println("Checksum gravado:          0x{:08x}", target_inode.get_checksum());
        std::println("Checksum calculado:        0x{:08x}", calc_checksum);
        std::println("Checksum válido:           {}", target_inode.get_checksum() == calc_checksum ? "Sim" : "Não");
    }

    return 0;
}

short Command::cd(Image &img, const std::span<const std::string> args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: cd <diretório>");
        return 1;
    }

    // Procuramos o diretório
    auto [parent_dir, resolved_path] = img.resolve_path(target_path, img.get_current_inode());

    // Checamos se não chegamos no mesmo diretório
    if (img.get_current_path().ends_with(resolved_path)) {
        std::println(std::cerr, "Diretório alvo é o mesmo do atual.");
        return 1;
    }

    // Agora trocamos o diretório corrente
    img.set_current_inode(parent_dir);
    img.set_current_path(resolved_path);

    return 0;
}

short Command::ls(Image &img, const std::span<const std::string> args) {
    const std::string target_path = args.size() > 1 ? args[1] : img.get_current_path();
    // Pegamos o diretório no sistema
    auto [parent_dir, resolved_path] = img.resolve_path(target_path, img.get_current_inode());

    // Agora só listamos eles
    for (const auto &entry: img.list_dir(parent_dir)) {
        bool is_dir = entry.is_dir();
        std::println("{}{}{}", is_dir ? "\033[32m" : "", entry.get_name(), is_dir ? "\033[0m" : "");
    }

    return 0;
}

short Command::test_inode(Image &img, const std::span<const std::string> args) {
    const std::string inode_str = args.size() > 1 ? args[1] : "";

    if (inode_str.empty()) {
        std::println(std::cerr, "Uso: testi <id do inode>");
        return 1;
    }

    // C++ (ainda) não tem um método como `std::stoi` para inteiro 32 bis sem sinal.
    // Então usamos `std::from_chars`.
    uint32_t inode_id{};
    auto [ptr, ec] = std::from_chars(inode_str.data(), inode_str.data() + inode_str.size(), inode_id);

    if (ec == std::errc::invalid_argument) {
        std::println(std::cerr, "Erro: O argumento para testi deve ser um número inteiro representando o inode.");
        return 1;
    } else if (ec == std::errc::result_out_of_range) {
        std::println(std::cerr, "Erro: O número do inode fornecido está fora do intervalo permitido (inteiro sem sinal de 32 bits).");
        return 1;
    }

    // Inodes são numerados de 1 até s_inodes_count (inclusive)
    const uint32_t max_inode_id = img.get_superblock().get_inodes_count();
    if (inode_id < 1 || inode_id > max_inode_id) {
        std::println(std::cerr, "Erro: ID de inode {} fora do intervalo válido (1 a {}).", inode_id, max_inode_id);
        return 1;
    }

    try {
        // Se conseguimos obter o inode, ele está alocado
        img.get_inode(inode_id);
        std::println("Inode {} alocado.", inode_id);
    } catch (const std::out_of_range&) {
        std::println("Inode {} livre (não alocado).", inode_id);
    }

    return 0;
}

short Command::test_block(Image &img, const std::span<const std::string> args) {
    const std::string block_str = args.size() > 1 ? args[1] : "";

    if (block_str.empty()) {
        std::println(std::cerr, "Uso: testb <id do bloco>");
        return 1;
    }

    // C++ (ainda) não tem um método como `std::stoi` para inteiro 32 bis sem sinal.
    // Então usamos `std::from_chars`.
    uint32_t block_id{};
    auto [ptr, ec] = std::from_chars(block_str.data(), block_str.data() + block_str.size(), block_id);

    if (ec == std::errc::invalid_argument) {
        std::println(std::cerr, "Erro: O argumento para testb deve ser um número inteiro representando o bloco.");
        return 1;
    } else if (ec == std::errc::result_out_of_range) {
        std::println(std::cerr, "Erro: O número do bloco fornecido está fora do intervalo permitido (inteiro sem sinal de 32 bits).");
        return 1;
    }

    // Blocos são numerados de 0 até total_blocks - 1
    const auto &sb = img.get_superblock();
    const uint64_t total_blocks = sb.get_blocks_count();
    if (block_id >= total_blocks) {
        std::println(std::cerr, "Erro: ID de bloco {} fora do intervalo válido (0 a {}).", block_id, total_blocks - 1);
        return 1;
    }

    try {
        // Tentativa de leitura: se não ocorrer exceção, consideramos o bloco alocado
        const uint32_t block_size = img.get_superblock().get_block_size();
        std::vector<std::byte> block(block_size);
        img.read_block(block_id, Utils::as_byte_span(block));
        std::println("Bloco {} alocado.", block_id);
    } catch (const std::out_of_range&) {
        std::println("Bloco {} livre (não alocado).", block_id);
    }

    return 0;
}

short Command::to_in(Image &img, const std::span<const std::string> args) {
    std::filesystem::path source_path(args.size() > 1 ? args[1] : "");
    
    // Usamos `std::filesystem` para perguntar ao SO se o arquivo existe
    if(!std::filesystem::exists(source_path)) {
        std::println(std::cerr, "Uso: import <arquivo do SO> <diretório/arquivo na imagem>");
        return 1;
    }
    // Usamos `std::filesystem` para perguntar ao SO se o arquivo é na verdade um diretório
    if(std::filesystem::is_directory(source_path)) {
        std::println(std::cerr, "O arquivo alvo na verdade é um diretório.");
        return 1;
    }

    // Caso o diretório de destino não seja dado nós usamos o diretório atual + nome do alvo
    const std::string dest_path = (args.size() > 2 ? 
        args[2] : 
        img.get_current_path() + (img.get_current_path().ends_with("/") ? "" : "/") + source_path.filename().string()
    );

    // Vemos se o arquivo alvo já existe na imagem. Só ver o nome é só suficiente, certo? :)
    auto [dst_path, dst_file] = Utils::split_path(dest_path);
    auto [dst_inode, dst_resolved_path] = img.resolve_path(dst_path, img.get_root_inode());
    if(std::ranges::any_of(img.list_dir(dst_inode), by_name(dst_file))) {
        std::println(std::cerr, "Arquivo alvo {} já existe na imagem.", dst_file);
        return 1;
    }

    // Abrimos o arquivo do SO para transferir como binário
    std::ifstream src_file_stream(source_path, std::ios::binary | std::ios::ate);
    if(!src_file_stream.is_open()) {
        std::println(std::cerr, "Erro ao abrir o arquivo do SO.");
        return 1;
    }
    // Pegamos o tamanho do arquivo
    uint64_t file_size = static_cast<uint64_t>(src_file_stream.tellg());
    // Tentamos alocar blocos e criar a árvore de extents
    uint32_t block_size = img.get_superblock().get_block_size();
    // Quantos blocos precisamos para o arquivo (truque de inteiros aqui)
    uint64_t blocks_needed = (file_size + block_size - 1) / block_size;
    
    // Caso aconteça de um arquivo muito grande ser importado: não queremos isso :)
    if(uint64_t free_blocks = img.get_superblock().get_free_blocks_count(); free_blocks < blocks_needed) {
        std::println("O arquivo alvo é muito grande para ser importado");
        std::println(" - Número de blocos necessários: {}", blocks_needed);
        std::println(" - Números de blocos livres: {}", free_blocks);
        return 1;
    }
    
    // Movemos o cursor para o início e (finalmente) lemos o buffer
    src_file_stream.seekg(0, std::ios::beg);
    std::vector<std::byte> buffer(file_size);
    src_file_stream.read(reinterpret_cast<char*>(buffer.data()), file_size);

    // Alocamos um inode e criamos a estrutura para escrita
    uint32_t new_inode_id = img.alloc_inode();
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    
    if(new_inode_id == 0) {
        std::println(std::cerr, "Erro: Não foi possível alocar um novo inode. A imagem está provavelmente cheia.");
        return 1;
    }

    // Inicializa o inode
    Raw::Inode new_inode{
        .i_mode  = Flags::InodeMode::S_IFREG | 0644,  // arquivo regular + permissões 644
        // Timestamps
        .i_atime = now,
        .i_ctime = now,
        .i_mtime = now,
        .i_links_count = 1, // 1 hard link para ele mesmo
        .i_flags = Flags::InodeFlags::EXT4_EXTENTS_FL,  // EXT4_EXTENTS_FL (usamos extents)
        // Aqui só usamos o que o superbloco manda para evitar inconsistências
        .i_extra_isize = img.get_inode(2).get_raw().i_extra_isize,
    };
    // Colocamos o tamanho do arquivo
    Utils::split(file_size, new_inode.i_size_lo, new_inode.i_size_hi);
    // O contador i_blocks conta com setores de 512 bytes. Então atualizamos ele.
    uint64_t total_sectors = blocks_needed * (block_size / 512);
    new_inode.i_blocks_lo = static_cast<uint32_t>(total_sectors & MAX_32BIT);
    new_inode.i_osd2.l_i_blocks_high = static_cast<uint16_t>((total_sectors >> 32) & MAX_16BIT);

    std::vector<Raw::ExtentLeaf> leafs{};
    uint32_t logical = 0, remaining = blocks_needed;
    
    // Tentamos alocar blocos até não precisarmos mais e colocamos em folhas
    while(remaining > 0) {
        auto [allocated_blocks, start_block] = img.alloc_contiguous_blocks(remaining);
        
        Raw::ExtentLeaf leaf{};
        leaf.ee_block = logical;
        leaf.ee_len = allocated_blocks;
        leaf.ee_start_lo = start_block;
        // Apostarei que nunca usaremos esse bits :)
        leaf.ee_start_hi = 0;
        
        leafs.push_back(leaf);
        logical += allocated_blocks;
        remaining -= allocated_blocks;
    }
    std::span<Raw::ExtentLeaf> leafs_span = Utils::as_span<Raw::ExtentLeaf>(leafs);

    Wrappers::Inode new_wrapper(
        new_inode_id,
        img.get_volume_uuid(),
        img.get_superblock().get_inode_size(),
        new_inode,
        std::vector<std::byte>(img.get_superblock().get_inode_size() - sizeof(Raw::Inode), std::byte{0})
    );

    // Agora distribuímos as folhas em extents

    // Quantos extents conseguimos colocar no `i_block`
    uint16_t max_inline = (sizeof(Raw::Inode::i_block) - sizeof(Raw::ExtentHeader)) / sizeof(Raw::ExtentLeaf);
    // Dá para alocar tudo no i_block
    // Esse é o nosso caso feliz :D
    if(leafs.size() <= 4) {
        // Inicializa extent header dentro do i_block
        Raw::ExtentHeader extent_header{
            .eh_magic      = Constants::EXTENT_MAGIC,
            .eh_entries    = static_cast<uint16_t>(leafs.size()),
            .eh_max        = max_inline,
            .eh_depth      = 0, // Só temos folhas diretas aqui
        };
        // Escrevemos na imagem
        Utils::write_to(new_inode.i_block, extent_header);
        // Onde estamos lendo do arquivo
        size_t file_offset = 0;
        for(size_t i = 0; i < leafs.size(); ++i) {
            for(uint16_t j = 0; j < leafs[i].ee_len; ++j) {
                // Escrevemos partes do arquivo nos blocos
                img.write_block(leafs[i].get_start_block() + j, Utils::as_byte_span(buffer, file_offset, block_size));
                file_offset += block_size;
            }
            // Colocamos os dados no offset certo
            Utils::write_to(new_inode.i_block, leafs[i], sizeof(extent_header) + (sizeof(leafs[i]) * i));
        }
        // Não há necessidade de checksums aqui pois o `i_block` já é coberto pelo checksum do inode
    } else {
        // Aqui fodeu, precisamos criar uma árvore de extents DO ZERO
        bool has_metadata_csum = img.get_superblock().has_metadata_csum();
        uint16_t max_per_block = (block_size - sizeof(Raw::ExtentHeader) - (has_metadata_csum ? sizeof(Raw::ExtentTail) : 0)) / sizeof(Raw::ExtentIndex);
        // Agora que sabemos quantos ExtentIndex/Leaf podemos colocar em um bloco, precisamos desobrir quantos blocos precisamos alocar para os ExtentIndex
        // Para cada nível da árvore temos (max_inline *  max_indexes_per_block^n) onde n é a profundidade.
        // Fazendo manipulação matemática e sabendo que a única coisa que importa aqui é quanto o último nível pode guardar, temos:
        // n = log(folhas/max_inline) / log(max_indexes_per_block)
        // Com isso, podemos fazer recursão onde o caso base ocorre quanto depth == 0
        // Eu realmente espero que isso seja mais rápido que usar loops
        uint16_t depth = static_cast<uint16_t>(
            std::ceil(std::log(static_cast<double>(leafs.size()) / max_inline) / 
            std::log(static_cast<double>(max_per_block)))
        );

        // Quantas folhas cada filho direto da raiz consegue gerenciar abaixo dele
        uint32_t leafs_per_root_child = std::pow(max_per_block, depth);
        std::vector<Raw::ExtentIndex> root_indices{};
        size_t leaf_offset = 0;

        // O loop da raiz roda até processar todas as folhas ou encher o espaço inline do Inode (max_inline = 4)
        while(leaf_offset < leafs.size() && root_indices.size() < max_inline) {
            size_t chunk_size = std::min(static_cast<size_t>(leafs_per_root_child), leafs.size() - leaf_offset);
            std::span<Raw::ExtentLeaf> child_leafs = leafs_span.subspan(leaf_offset, chunk_size);

            // Chamamos a recursão para os blocos externos (passando max_per_block como capacidade)
            uint32_t child_blk = build_extent_tree(
                img, new_wrapper, has_metadata_csum, 
                max_per_block, max_per_block, 
                depth - 1, Utils::as_byte_span(buffer), child_leafs
            );

            if(child_blk == 0) {
                std::println(std::cerr, "Erro: Não foi possível alocar blocos para a árvore de extents.");
                return 1;
            } else {
                // alocamos 1 bloco de índice, então precisamos atualizar o contador de blocos do inode
                total_sectors += (block_size / 512);
                new_inode.i_blocks_lo = static_cast<uint32_t>(total_sectors & MAX_32BIT);
                new_inode.i_osd2.l_i_blocks_high = static_cast<uint16_t>((total_sectors >> 32) & MAX_16BIT);   
            }

            // Monta o índice para colocar na raiz (i_block)
            Raw::ExtentIndex idx{
                .ei_block = child_leafs[0].ee_block,
                .ei_leaf_lo = child_blk,
                .ei_leaf_hi = 0,
            };
            root_indices.push_back(idx);
            leaf_offset += chunk_size;
        }

        // Inicializa o ExtentHeader da raiz DIRETO no i_block do Inode
        Raw::ExtentHeader root_header{
            .eh_magic   = Constants::EXTENT_MAGIC,
            .eh_entries = static_cast<uint16_t>(root_indices.size()),
            .eh_max     = max_inline, // Na raiz o limite estrito é max_inline (4)
            .eh_depth   = depth,      // Altura total da árvore
        };

        // Copia o cabeçalho e os índices gerados para dentro da estrutura i_block do Inode
        Utils::write_to(new_inode.i_block, root_header);
        for (size_t i = 0; i < root_indices.size(); ++i) {
            Utils::write_to(new_inode.i_block, root_indices[i], sizeof(root_header) + (i * sizeof(Raw::ExtentIndex)));
        }
    }
    // Sincroniza o wrapper com o raw inode modificado antes de salvar no disco
    new_wrapper.set_raw(new_inode);
    img.write_inode(new_wrapper);

    img.dir_add_entry(dst_inode, new_inode_id, dst_file, Raw::DirectoryFileType::EXT4_FT_REG_FILE);

    // Modificamos o inode pai para modificar o campo "modificado"
    Raw::Inode dst_raw = dst_inode.get_raw();
    dst_raw.i_mtime = now;
    dst_inode.set_raw(dst_raw);
    img.write_inode(dst_inode);

    return 0;
}

short Command::to_out(Image &img, const std::span<const std::string> args) {
    const std::string source_path = args.size() > 1 ? args[1] : "";
    if (source_path.empty()) {
        std::println(std::cerr, "Uso: export <arquivo da imagem> <diretório/arquivo no SO>");
        return 1;
    }
    auto [src_path, src_file] = Utils::split_path(source_path);
    // Vemos se o arquivo já existe no SO
    // Usamos `std::filesystem` para ver se o arquivo está no SO perguntando ao SO
    std::filesystem::path dest_path(args.size() > 2 ? args[2] : "");

    // O uso do operador  / é para colocar o separador de diretório do SO (só usaremos linux aqui, mas é mais semântico)
    // Se não foi dado destino, usa o diretório atual do SO + nome do arquivo
    if(dest_path.empty()) dest_path = std::filesystem::current_path() / src_file;
    // Se foi dado um diretório existente, concatena o nome do arquivo
    if(std::filesystem::is_directory(dest_path)) dest_path /= src_file;

    // Agora sim checa se o arquivo final já existe
    if(std::filesystem::exists(dest_path)) {
        std::println(std::cerr, "Arquivo '{}' já existe no SO.", dest_path.string());
        return 1;
    }

    // Vemos se o arquivo alvo existe na imagem. Só ver o nome é só suficiente, certo? :)
    auto [src_inode, src_resolved_path] = img.resolve_path(src_path, img.get_current_inode());
    if(!src_inode.is_dir()) {
        std::println(std::cerr, "Diretório alvo na verdade é um arquivo.");
        return 1;
    }
    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(src_inode);
    auto target_entry = std::ranges::find_if(entries, by_name(src_file));
    if(target_entry == entries.end()) {
        std::println(std::cerr, "Arquivo alvo {} não existe na imagem.", src_file);
        return 1;
    }

    Wrappers::Inode file_inode = img.get_inode(target_entry->get_inode());
    // Abrimos o arquivo do SO para transferir como binário
    std::ofstream dst_file_stream(dest_path, std::ios::binary);
    if(!dst_file_stream.is_open()) {
        std::println(std::cerr, "Não foi possível abrir o arquivo alvo.");
        return 1; 
    }

    // Colocamos os bytes no arquivo de destino
    std::vector<std::byte> file_bytes = img.read_file(file_inode);
    dst_file_stream.seekp(0, std::ios::beg);
    dst_file_stream.write(reinterpret_cast<char *>(file_bytes.data()), file_bytes.size());

    return 0;
}

short Command::pwd(Image &img) {
    // Sempre guardamos em `img`, então é só pegar de volta a informação do diretório atual
    std::println("{}", img.get_current_path());
    return 0;
}

short Command::touch(Image &img, const std::span<const std::string> args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if(file_path.empty()) {
        std::println(std::cerr, "Uso: touch <arquivo>");
        return 1;
    }

    // Pegamos o diretório
    auto [path, file_name] = Utils::split_path(file_path);
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());

    // Antes de mais nada, temos que ver se não temos um nome de arquivo muito grande.
    // O campo `name_len` do diretório é um uint8_t, então o tamanho máximo de um nome de arquivo é 255 bytes.
    if(file_name.size() > std::numeric_limits<uint8_t>::max()) {
        std::println(std::cerr, "Erro: Nome de arquivo muito grande ({} bytes). Máximo permitido: {} bytes.", file_name.size(), std::numeric_limits<uint8_t>::max());
        return 1;
    }

    // Vemos se o arquivo já existe. (Usamos ranges para iterar)
    if(std::ranges::any_of(img.list_dir(parent_dir), by_name(file_name))) {
        std::println(std::cerr, "Erro: Arquivo '{}' já existe.", file_name);
        return 1;
    }

    // Alocamos um inode e criamos a estrutura para escrita
    uint32_t new_inode_id = img.alloc_inode();
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));

    if(new_inode_id == 0) {
        std::println(std::cerr, "Erro: Não foi possível alocar um novo inode. A imagem está provavelmente cheia.");
        return 1;
    }

    // Inicializa o inode
    Raw::Inode new_inode{
        .i_mode      = Flags::InodeMode::S_IFREG | 0644,  // arquivo regular + permissões 644
        .i_size_lo   = 0,
        // Timestamps
        .i_atime = now,
        .i_ctime = now,
        .i_mtime = now,
        .i_links_count = 1, // 1 hard link para ele mesmo
        .i_flags     = Flags::InodeFlags::EXT4_EXTENTS_FL,  // EXT4_EXTENTS_FL (usamos extents)
        .i_size_hi   = 0,
        .i_extra_isize = img.get_inode(2).get_raw().i_extra_isize, // Aqui só usamos o que o superbloco manda para evitar inconsistências

    };
    uint16_t max_inline = (new_inode.i_block.max_size() - sizeof(Raw::ExtentHeader)) / sizeof(Raw::ExtentLeaf);
    // Inicializa extent header dentro do i_block
    Raw::ExtentHeader eh{
        .eh_magic      = Constants::EXTENT_MAGIC,
        .eh_entries    = 0,
        .eh_max        = max_inline,
        .eh_depth      = 0,
        .eh_generation = 0,
    };

    // Finalmente escrevemos na imagem
    Utils::write_to(new_inode.i_block, eh);
    img.write_inode(Wrappers::Inode(
        new_inode_id,
        img.get_volume_uuid(),
        img.get_superblock().get_inode_size(),
        new_inode,
        std::vector<std::byte>(img.get_superblock().get_inode_size() - sizeof(Raw::Inode), std::byte{0}))
    );

    img.dir_add_entry(parent_dir, new_inode_id, file_name, Raw::DirectoryFileType::EXT4_FT_REG_FILE);

    // Modificamos o inode pai para modificar o campo "modificado"
    Raw::Inode parent_raw = parent_dir.get_raw();
    parent_raw.i_mtime = now;
    parent_dir.set_raw(parent_raw);
    img.write_inode(parent_dir);
    return 0;
}

short Command::mkdir(Image &img, const std::span<const std::string> args) {
    // Pegamos o caminho do diretório a ser criado a partir dos argumentos
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    // Verificamos se o caminho do diretório foi fornecido
    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: mkdir <diretório>");
        return 1;
    }

    // Separamos o diretório alvo do nome do diretório a ser criado
    auto [path, path_name] = Utils::split_path(dir_path);
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());

    // Antes de mais nada, temos que ver se não temos um nome de arquivo muito grande.
    // O campo `name_len` do diretório é um `uint8_t`, então o tamanho máximo de um nome de arquivo é 255 bytes.
    if(path_name.size() > std::numeric_limits<uint8_t>::max()) {
        std::println(std::cerr, "Erro: Nome de diretório muito grande ({} bytes). Máximo permitido: {} bytes.", path_name.size(), std::numeric_limits<uint8_t>::max());
        return 1;
    }

    // Verificamos se já existe um diretório ou arquivo com o mesmo nome no diretório pai
    if(std::ranges::any_of(img.list_dir(parent_dir), by_name(path_name))) {
        std::println("Já existe um arquivo ou diretório com o nome '{}'.", path_name);
        return 1;
    }

    // Pegamos o timestamp atual para usar nos campos de tempo do inode
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));
    uint32_t block_size = img.get_superblock().get_block_size();
    uint32_t inode_size = img.get_superblock().get_inode_size();
    bool has_metadata_csum = img.get_superblock().has_metadata_csum();

    // Inicializa a estrutura do inode
    Raw::Inode new_inode{
        .i_mode      = Flags::InodeMode::S_IFDIR | 0755,  // diretório + permissões 755
        .i_size_lo   = block_size,
        // Timestamps
        .i_atime = now,
        .i_ctime = now,
        .i_mtime = now,
        .i_links_count = 2, // O número de links começa em 2 porque um link é para ele mesmo (".") e outro é para o diretório pai ("..").
        .i_blocks_lo = block_size / 512,
        .i_flags     = Flags::InodeFlags::EXT4_EXTENTS_FL,  // EXT4_EXTENTS_FL (usamos extents)
        .i_size_hi   = 0,
        .i_extra_isize = img.get_inode(2).get_raw().i_extra_isize, // Aqui só usamos o que o superbloco manda para evitar inconsistências  
    };
    
    // Calcula o número máximo de extents inline que cabem no i_block do inode
    uint16_t max_inline = (new_inode.i_block.max_size() - sizeof(Raw::ExtentHeader)) / sizeof(Raw::ExtentLeaf);
    
    // Inicializa extent header dentro do i_block
    Raw::ExtentHeader eh{
        .eh_magic      = Constants::EXTENT_MAGIC,
        .eh_entries    = 1,
        .eh_max        = max_inline,
        .eh_depth      = 0,
    };

    // Alocamos um bloco
    uint32_t id_block = img.alloc_block();
    if(id_block == 0) {
        std::println(std::cerr, "Erro: Não foi possível alocar um novo bloco. A imagem está provavelmente cheia ou só restam blocos não inicializados.");
        return 1;
    }

    // Criamos a folha de extent correspondente ao bloco alocado
    Raw::ExtentLeaf leaf{
        .ee_block = 0,
        .ee_len = 1,
        .ee_start_hi = 0,
        .ee_start_lo = id_block
    };

    // Alocamos um inode
    uint32_t id_inode = img.alloc_inode(true);

    if(id_inode == 0) {
        std::println(std::cerr, "Erro: Não foi possível alocar um novo inode. A imagem está provavelmente cheia.");
        return 1;
    }
    
    // Escrevemos o novo inode na imagem
    // Começamos pelo cabeçalho de extent
    Utils::write_to(new_inode.i_block, eh);

    // Colocamos a folha correspondente
    Utils::write_to(new_inode.i_block, leaf, sizeof(eh));

    // Escrevemos o inode na imagem
    img.write_inode(Wrappers::Inode(
        id_inode,
        img.get_volume_uuid(),
        inode_size,
        new_inode,
        std::vector<std::byte>(inode_size - sizeof(Raw::Inode), std::byte{0}))
    );
    uint16_t dir_entry_size = sizeof(Raw::DirectoryEntry);
    // Criamos uma entrada de diretório para o novo diretório
    Raw::DirectoryEntry dot{
        .inode = id_inode,
        .rec_len = Utils::to_4bit_aligned<uint16_t>(dir_entry_size + 1),
        .name_len = 1,
        .file_type = Raw::DirectoryFileType::EXT4_FT_DIR
    },
    // Criamos uma entrada de diretório para o diretório pai
    dotdot{
        .inode = parent_dir.get_inode_id(),
        .rec_len = Utils::to_4bit_aligned<uint16_t>(block_size - dot.rec_len - (has_metadata_csum ? sizeof(Raw::DirectoryEntryTail) : 0)),
        .name_len = 2,
        .file_type = Raw::DirectoryFileType::EXT4_FT_DIR
    };

    // Criamos um buffer do tamanho do bloco para escrever a entrada de diretório
    std::vector<std::byte> buffer_block(block_size);
    
    // Criamos uma view do buffer para escrever as entradas de diretório
    std::span<std::byte> buffer_span = Utils::as_byte_span(buffer_block);
    //Escrevemos as entradas de diretório no buffer
    Utils::write_to(buffer_span, dot);
    Utils::write_to(buffer_span, ".", sizeof(dot));
    Utils::write_to(buffer_span, dotdot, dot.rec_len);
    Utils::write_to(buffer_span, "..", dot.rec_len + sizeof(dotdot));
    
    // Pegamos o inode que criamos
    Wrappers::Inode target_inode = img.get_inode(id_inode);

    // Verificamos se o superbloco tem checksum de metadados habilitado.
    if(has_metadata_csum) {
        // Se sim, calculamos o checksum do diretório e escrevemos no final do bloco.
        Raw::DirectoryEntryTail tail{
            .det_rec_len = 12,
            .det_reserved_ft = Raw::DirectoryFileType::EXT4_FT_DIR_CSUM,
            .det_checksum = Checksums::checksum_dir(target_inode, buffer_span, block_size, img.get_superblock().get_checksum_seed()),
        };
        Utils::write_to(buffer_span, tail, buffer_span.size() - sizeof(tail));
    }

    // Escrevemos a entrada de diretório no buffer
    img.write_block(id_block, buffer_span);
    
    // Colocamos o novo inode no diretório pai
    img.dir_add_entry(parent_dir, id_inode, dir_path, Raw::DirectoryFileType::EXT4_FT_DIR);

    // Pegamos o diretório pai
    Raw::Inode parent_raw = parent_dir.get_raw();

    // Modificamos o inode pai para modificar o campo "modificado"
    parent_raw.i_mtime = now;

    // Incrementa o número de links do diretório pai (o novo diretório é um link para ele)
    parent_raw.i_links_count += 1; 

    // Atualizamos os dados do diretório pai para refletir a nova entrada
    parent_dir.set_raw(parent_raw);
    
    // Atualizamos o diretório pai
    img.write_inode(parent_dir);

    return 0;
}

short Command::rm(Image &img, const std::span<const std::string> args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: rm <arquivo>");
        return 1;
    }

    // Separamos o diretório alvo do nome do arquivo
    auto [path, file_name] = Utils::split_path(file_path);
    // Pegamos o diretório
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());

    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);

    // Vemos se o arquivo não existe mais
    auto target = std::ranges::find_if(entries, by_name(file_name));

    if (target == entries.end()) {
        std::println(std::cerr, "Erro: Arquivo '{}' não encontrado.", file_name);
        return 1;
    }

    // Por favor não remova diretórios por aqui :)
    if (target->is_dir()) {
        std::println(std::cerr, "Erro: '{}' é um diretório. Use rmdir.", file_name);
        return 1;
    }

    // Agora só removemos
    img.dir_remove_entry(parent_dir, file_name);

    return 0;
}

short Command::rmdir(Image &img, const std::span<const std::string> args) {
    // Pegamos o caminho do diretório a ser removido a partir dos argumentos
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    // Verificamos se o caminho do diretório foi fornecido
    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: rmdir <diretório>");
        return 1;
    }

    // Separamos o diretório alvo do nome do diretório a ser removido
    auto [path, path_name] = Utils::split_path(dir_path);
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());

    // Pegamos a lista de entradas do diretório pai
    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);

    // Pegamos o diretorio que queremos remover (se existir) da lista de entradas do diretório pai
    auto target = std::ranges::find_if(entries, by_name(path_name));

    // Se não existir, retornamos erro
    if (target == entries.end()) {
        std::println(std::cerr, "Erro: Diretório '{}' não encontrado.", path_name);
        return 1;
    }

    // Verificamos se o alvo é realmente um diretório
    if (!target->is_dir()) {
        std::println(std::cerr, "Erro: '{}' não é um diretório. Use rm.", path_name);
        return 1;
    }

    // Pegamos o inode do diretório alvo
    Wrappers::Inode target_inode = img.get_inode(target->get_inode());

    // Listamos as entradas do diretório alvo
    std::vector<Wrappers::DirectoryEntry> target_entries = img.list_dir(target_inode);
    
    // Um diretório vazio tem no máximo 2 entradas ("." e "..")
    for (const auto &e : target_entries) {
        if (e.get_name() != "." && e.get_name() != "..") {
            std::println(std::cerr, "Erro: O diretório '{}' não está vazio.", path_name);
            return 1;
        }
    }

    // Pegamos o timestamp atual para usar nos campos de tempo do inode
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));

    // Removemos o inode atual do diretório pai e ao mesmo tempo decrementamos o contador de links do diretório pai e 
    // excluimos os blocos do diretorio que queremos excluir
    img.dir_remove_entry(parent_dir, path_name);
    
    // Pegamos o diretório pai
    Raw::Inode parent_raw = parent_dir.get_raw();
    
    // Modificamos o inode pai para mudar o campo "modificado"
    parent_raw.i_mtime = now;
    
    // Também diminuímos o números de links agora que o alvo foi removido
    if(parent_raw.i_links_count > 2) parent_raw.i_links_count--;
    
    // Atualizamos os dados do diretório pai para refletir a nova entrada
    parent_dir.set_raw(parent_raw);
    
    // Atualizamos o diretório pai
    img.write_inode(parent_dir);

    return 0;
}

short Command::rename(Image &img, const std::span<const std::string> args) {
    const std::string file = args.size() > 1 ? args[1] : "";
    const std::string new_file_name = args.size() > 2 ? args[2] : "";

    if (file.empty() || new_file_name.empty()) {
        std::println(std::cerr, "Uso: rename <arquivo> <novo nome do arquivo>");
        return 1;
    }
    // Separamos o diretório alvo do nome do arquivo
    auto [path, file_name] = Utils::split_path(file);
    // Agora pegamos o inode do diretório pai do alvo do arquivo
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());
    // Separamos também o diretório alvo do novo nome do arquivo
    auto [new_path, new_name] = Utils::split_path(new_file_name);
    // Pegamos também o diretório alvo do novo arquivo
    auto [new_dir, new_parent_resolved_path] = img.resolve_path(new_path, img.get_current_inode());

    // Vemos se o arquivo existe
    if(std::ranges::none_of(img.list_dir(parent_dir), by_name(file_name))) {
        std::println(std::cerr, "Erro: '{}' não encontrado.", file_name);
        return 1;
    }

    // Vemos se já existe o arquivo alvo
    if(std::ranges::any_of(img.list_dir(new_dir), by_name(new_name))) {
        std::println(std::cerr, "Erro: '{}' já existe.", new_name);
        return 1;
    }
    // Agora só renomeamos
    img.dir_rename_entry(parent_dir, file_name, new_dir, new_name);

    return 0;
}

short Command::clear() {
    // Usamos CSI J 2 para limpar a tela e CSI H para mover o cursor para a posição inicial.
    std::print("\033[2J\033[H");
    return 0;
}