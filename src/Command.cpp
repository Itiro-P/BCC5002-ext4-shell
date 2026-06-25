#include "../include/Command.hpp"
#include <iostream>
#include <fstream>
#include <charconv>
#include <ranges>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <filesystem>

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
        // Agora (finalmente) escrevemos o bloco com as folhas
        img.write_block(blk_id, buf_bytes);
    } else {
        // Aqui chamamos o caso recursivo com depth-1
        // Precisamos saber quantas folhas colocamos no nível.
        // Em um nível n temos max_per_block^depth
        uint32_t leafs_per_child = std::pow(max_per_block, depth);
        std::vector<Raw::ExtentIndex> index_entries{};
        size_t leaf_offset = 0;

        while (leaf_offset < leafs.size() && index_entries.size() < max_entries) {
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

            // Criamos o índice apontando para esse filho
            Raw::ExtentIndex idx{
                .ei_block = child_leafs[0].ee_block,
                .ei_leaf_lo = static_cast<uint32_t>(child_phys_block & MAX_32BIT),
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

short Command::help() {
    for (const auto &[cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::info(Image &img) {
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

    // necessário implementar

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

    // Faça sua mágica!
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

    // necessário implementar

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
        while (leaf_offset < leafs.size() && root_indices.size() < max_inline) {
            size_t chunk_size = std::min(static_cast<size_t>(leafs_per_root_child), leafs.size() - leaf_offset);
            std::span<Raw::ExtentLeaf> child_leafs = leafs_span.subspan(leaf_offset, chunk_size);

            // Chamamos a recursão para os blocos externos (passando max_per_block como capacidade)
            uint32_t child_blk = build_extent_tree(
                img, new_wrapper, has_metadata_csum, 
                max_per_block, max_per_block, 
                depth - 1, Utils::as_byte_span(buffer), child_leafs
            );

            // Monta o índice para colocar na raiz (i_block)
            Raw::ExtentIndex idx{
                .ei_block = child_leafs[0].ee_block,
                .ei_leaf_lo = static_cast<uint32_t>(child_blk & MAX_32BIT),
                .ei_leaf_hi = 0,  // uint32_t nunca tem bits acima de 31
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

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: touch <arquivo>");
        return 1;
    }

    // Pegamos o diretório
    auto [path, file_name] = Utils::split_path(file_path);
    auto [parent_dir, parent_resolved_path] = img.resolve_path(path, img.get_current_inode());

    // Vemos se o arquivo já existe. (Usamos ranges para iterar)
    if (std::ranges::any_of(img.list_dir(parent_dir), by_name(file_name))) {
        std::println(std::cerr, "Erro: Arquivo '{}' já existe.", file_name);
        return 1;
    }

    // Alocamos um inode e criamos a estrutura para escrita
    uint32_t new_inode_id = img.alloc_inode();
    uint32_t now = static_cast<uint32_t>(std::time(nullptr));

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

    // Pegamos o diretório pai onde o novo diretório será criado
    auto [parent_dir_inode, _] = img.resolve_path(path, img.get_current_inode());

    // Verificamos se já existe um diretório ou arquivo com o mesmo nome no diretório pai
    if(std::ranges::any_of(img.list_dir(parent_dir_inode), by_name(path_name))) {
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

    // Criamos a folha de extent correspondente ao bloco alocado
    Raw::ExtentLeaf leaf{
        .ee_block = 0,
        .ee_len = 1,
        .ee_start_hi = 0,
        .ee_start_lo = id_block
    };

    // Alocamos um inode
    uint32_t id_inode = img.alloc_inode(true);
    
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
        .rec_len = Utils::to_4bit_aligned(dir_entry_size + 1),
        .name_len = 1,
        .file_type = Raw::DirectoryFileType::EXT4_FT_DIR
    },
    // Criamos uma entrada de diretório para o diretório pai
    dotdot{
        .inode = parent_dir_inode.get_inode_id(),
        .rec_len = Utils::to_4bit_aligned(block_size - (has_metadata_csum ? sizeof(Raw::DirectoryEntryTail) : 0)),
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
    img.dir_add_entry(parent_dir_inode, id_inode, dir_path, Raw::DirectoryFileType::EXT4_FT_DIR);

    // Pegamos o diretório pai
    Raw::Inode parent_raw = parent_dir_inode.get_raw();

    // Modificamos o inode pai para modificar o campo "modificado"
    parent_raw.i_mtime = now;

    // Incrementa o número de links do diretório pai (o novo diretório é um link para ele)
    parent_raw.i_links_count += 1; 

    // Atualizamos os dados do diretório pai para refletir a nova entrada
    parent_dir_inode.set_raw(parent_raw);
    
    // Atualizamos o diretório pai
    img.write_inode(parent_dir_inode);

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

    // Pegamos o diretório pai onde o diretório será removido
    auto [parent_dir_inode, _] = img.resolve_path(path, img.get_current_inode());

    // Pegamos a lista de entradas do diretório pai
    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir_inode);

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
    img.dir_remove_entry(parent_dir_inode, path_name);
    
    // Pegamos o diretório pai
    Raw::Inode parent_raw = parent_dir_inode.get_raw();
    
    // Modificamos o inode pai para mudar o campo "modificado"
    parent_raw.i_mtime = now;
    
    // Atualizamos os dados do diretório pai para refletir a nova entrada
    parent_dir_inode.set_raw(parent_raw);
    
    // Atualizamos o diretório pai
    img.write_inode(parent_dir_inode);

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
