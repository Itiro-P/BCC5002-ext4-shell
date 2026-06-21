#include "../include/Command.hpp"
#include <iostream>
#include <fstream>
#include <charconv>
#include <ranges>
#include <algorithm>
#include <cstring>

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
auto by_name = [](const std::string &name) {
    return [&name](const Wrappers::DirectoryEntry &entry) {
        return entry.get_name() == name;
    };
};

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

    // Resolvemos o diretório para um inode (procura que pode ser recursiva)
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());
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
    auto [parent_dir, _] = img.resolve_path(target_path, img.get_current_inode());

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
    const std::string source_path = args.size() > 1 ? args[1] : "";
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if (source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: import <arquivo do SO> <diretório na imagem>");
        return 1;
    }

    // Vemos se o arquivo alvo já existe na imagem. Só ver o nome é só suficiente, certo? :)
    auto [dst_path, dst_file] = Utils::split_path(dest_path);
    auto [dst_inode, _] = img.resolve_path(dst_path, img.get_root_inode());
    if(std::ranges::any_of(img.list_dir(dst_inode), by_name(dst_file))) {
        std::println(std::cerr, "Arquivo alvo {} já existe na imagem.", dst_file);
        return 1;
    }

    // Abrimos o arquivo do SO para transferir como binário
    std::ifstream src_file_stream(source_path, std::ios::binary | std::ios::ate);
    if(!src_file_stream.is_open()) {
        std::println(std::cerr, "Não foi possível abrir o arquivo alvo.");
        return 1;    
    }

    // Pegamos o tamanho do arquivo
    uint64_t file_size = static_cast<uint64_t>(src_file_stream.tellg());
    // Movemos o cursor para o início e (finalmente) lemos o buffer
    src_file_stream.seekg(0, std::ios::beg);
    // Lemos o arquivo em um buffer
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
        .i_extra_isize = img.get_inode(2).get_raw().i_extra_isize, // Aqui só usamos o que o superbloco manda para evitar inconsistências

    };
    // Colocamos o tamanho do arquivo
    Utils::split(file_size, new_inode.i_size_lo, new_inode.i_size_hi);

    // Tentamos alocar blocos e criar a árvore de extents
    uint32_t block_size = img.get_superblock().get_block_size();
    // Quantos blocos precisamos para o arquivo (truque de inteiros aqui)
    uint32_t blocks_needed = (file_size + block_size - 1) / block_size;
    std::vector<Raw::ExtentLeaf> leafs;
    uint32_t logical = 0;
    uint32_t remaining = blocks_needed;

    while(remaining > 0) {
        auto [got, start] = img.alloc_contiguous_blocks(remaining);

        Raw::ExtentLeaf leaf{};
        leaf.ee_block = logical;
        leaf.ee_len = got;
        leaf.ee_start_lo = start;
        leaf.ee_start_hi = 0;

        leafs.push_back(leaf);
        logical += got;
        remaining -= got;
    }

    // Dá para alocar tudo no i_block
    if(leafs.size() <= 4) {
        // Inicializa extent header dentro do i_block
        Raw::ExtentHeader extent_header{
            .eh_magic      = Constants::EXTENT_MAGIC,
            .eh_entries    = static_cast<uint16_t>(leafs.size()),
            .eh_max        = 4, // cabe 4 extents diretos no i_block
            .eh_depth      = 0,
            .eh_generation = 0,
        };
        // Escrevemos na imagem
        Utils::write_to(new_inode.i_block, extent_header);
        size_t file_offset = 0;
        for(int i = 0; i < leafs.size(); ++i) {
            for(int j = 0; j < leafs[i].ee_len; ++j) {
                img.write_block(leafs[i].get_start_block() + j, Utils::as_byte_span(buffer, file_offset, block_size));
                file_offset += block_size;
            }
            Utils::write_to(new_inode.i_block, leafs[i], sizeof(extent_header) + (sizeof(Raw::ExtentLeaf) * i));
        }
    } else {
        // Aqui fodeu: precisamos fazer uma árvore de extents
        // Quantos leafs cabem por bloco de índice
        size_t leafs_per_block = (block_size - sizeof(Raw::ExtentTail)) / sizeof(Raw::ExtentLeaf) - 1; // -1 para o header
        size_t indexes_needed  = (leafs.size() + leafs_per_block - 1) / leafs_per_block;

        // Aloca blocos para os nós de índice
        std::vector<uint32_t> index_blocks;
        for (size_t i = 0; i < indexes_needed; ++i) index_blocks.push_back(img.alloc_block());

        // Escreve cada bloco de índice com seus leafs
        for (size_t i = 0; i < indexes_needed; ++i) {
            size_t leaf_start = i * leafs_per_block;
            size_t leaf_end = std::min(leaf_start + leafs_per_block, leafs.size());
            std::vector<Raw::ExtentLeaf> block_leafs(leafs.begin() + leaf_start, leafs.begin() + leaf_end);

            Raw::ExtentHeader child_header{
                .eh_magic = Constants::EXTENT_MAGIC,
                .eh_entries = static_cast<uint16_t>(block_leafs.size()),
                .eh_max = static_cast<uint16_t>(leafs_per_block),
                .eh_depth = 0,
                .eh_generation = 0,
            };

            std::vector<std::byte> index_block_buf(block_size, std::byte{0});
            Utils::write_to(Utils::as_byte_span(index_block_buf), child_header);
            for (size_t j = 0; j < block_leafs.size(); ++j)
                Utils::write_to(Utils::as_byte_span(index_block_buf), block_leafs[j],
                    sizeof(Raw::ExtentHeader) + j * sizeof(Raw::ExtentLeaf));

            // Tail de checksum
            if (Wrappers::SuperBlock sb = img.get_superblock(); sb.has_metadata_csum()) {
                // inode ainda não está no disco, então construímos o wrapper temporário
                Wrappers::Inode temp(new_inode_id, img.get_volume_uuid(), 
                    sb.get_inode_size(), new_inode,
                    std::vector<std::byte>(sb.get_inode_size() - sizeof(Raw::Inode), std::byte{0}));
                Raw::ExtentTail tail{
                    .eb_checksum = Checksums::checksum_extent(
                        temp, Utils::as_byte_span(index_block_buf), 
                        block_size, sb.get_checksum_seed())
                };
                Utils::write_to(Utils::as_byte_span(index_block_buf), tail, block_size - sizeof(Raw::ExtentTail));
            }
            img.write_block(index_blocks[i], Utils::as_byte_span(index_block_buf));
        }

        // Monta o i_block com os ExtentIndex apontando para os blocos de índice
        Raw::ExtentHeader root_header{
            .eh_magic   = Constants::EXTENT_MAGIC,
            .eh_entries = static_cast<uint16_t>(indexes_needed),
            .eh_max     = 4,
            .eh_depth   = 1,
            .eh_generation = 0,
        };
        Utils::write_to(new_inode.i_block, root_header);
        for (size_t i = 0; i < indexes_needed; i++) {
            Raw::ExtentIndex idx{
                .ei_block   = static_cast<uint32_t>(i * leafs_per_block), // primeiro bloco lógico coberto
                .ei_leaf_lo = index_blocks[i],
                .ei_leaf_hi = 0,
                .ei_unused  = 0,
            };
            Utils::write_to(new_inode.i_block, idx, sizeof(Raw::ExtentHeader) + i * sizeof(Raw::ExtentIndex));
        }
    }

    img.write_inode(Wrappers::Inode(
        new_inode_id,
        img.get_volume_uuid(),
        img.get_superblock().get_inode_size(),
        new_inode,
        std::vector<std::byte>(img.get_superblock().get_inode_size() - sizeof(Raw::Inode), std::byte{0}))
    );

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
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if (source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: export <arquivo da imagem> <diretório no SO>");
        return 1;
    }

    // Vemos se o arquivo alvo existe na imagem. Só ver o nome é só suficiente, certo? :)
    auto [src_path, src_file] = Utils::split_path(source_path);
    auto [src_inode, _] = img.resolve_path(src_path, img.get_current_inode());
    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(src_inode);
    auto target_entry = std::ranges::find_if(entries, by_name(src_file));
    if(target_entry == entries.end()) {
        std::println(std::cerr, "Arquivo alvo {} não existe na imagem.", src_file);
        return 1;
    }

    Wrappers::Inode file_inode = img.get_inode(target_entry->get_inode());

    auto [dst_path, dst_file] = Utils::split_path(dest_path);
    std::string full_path = dst_path;
    if(!full_path.empty() && full_path.back() != '/') full_path += '/';
    full_path += dst_file.empty() ? src_file : dst_file;
    // Vemos se o arquivo já existe no SO
    // Seria mais seguro usar `std::filesystem` aqui, mas não quero deixar parecendo que usamos a STL para coisas mais profundas :)
    if(std::ifstream test_file(full_path); test_file.is_open()) {
        std::println(std::cerr, "Arquivo já existe no SO.");
        return 1;
    }

    // Abrimos o arquivo do SO para transferir como binário
    std::ofstream dst_file_stream(full_path, std::ios::binary);
    if(!dst_file_stream.is_open()) {
        std::println(std::cerr, "Não foi possível abrir o arquivo alvo.");
        return 1; 
    }

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
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());


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

    // Inicializa extent header dentro do i_block
    Raw::ExtentHeader eh{
        .eh_magic      = Constants::EXTENT_MAGIC,
        .eh_entries    = 0,
        .eh_max        = 4, // cabe 4 extents diretos no i_block
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
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: mkdir <diretório>");
        return 1;
    }

    auto [path, path_name] = Utils::split_path(dir_path);
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());

    if(std::ranges::any_of(img.list_dir(parent_dir), by_name(path_name))) {
        std::println("Diretório alvo existe e é um arquivo/diretório");
        return 1;
    }

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
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());

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
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: rmdir <diretório>");
        return 1;
    }

    auto [path, path_name] = Utils::split_path(dir_path);
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());

    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);

    auto target = std::ranges::find_if(entries, by_name(path_name));

    if (target == entries.end()) {
        std::println(std::cerr, "Erro: Diretório '{}' não encontrado.", path_name);
        return 1;
    }

    if (!target->is_dir()) {
        std::println(std::cerr, "Erro: '{}' não é um diretório. Use rm.", path_name);
        return 1;
    }

    // Validando se o diretório está realmente vazio
    Wrappers::Inode target_inode = img.get_inode(target->get_inode());
    std::vector<Wrappers::DirectoryEntry> target_entries = img.list_dir(target_inode);
    
    // Um diretório vazio tem no máximo 2 entradas ("." e "..")
    for (const auto &e : target_entries) {
        if (e.get_name() != "." && e.get_name() != "..") {
            std::println(std::cerr, "Erro: O diretório '{}' não está vazio.", path_name);
            return 1;
        }
    }

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
    auto [parent_dir, _] = img.resolve_path(path, img.get_current_inode());
    // Separamos também o diretório alvo do novo nome do arquivo
    auto [new_path, new_name] = Utils::split_path(new_file_name);
    // Pegamos também o diretório alvo do novo arquivo
    auto [new_dir, _] = img.resolve_path(new_path, img.get_current_inode());

    // Vemos se o arquivo existe
    if (std::ranges::any_of(img.list_dir(parent_dir), by_name(file_name))) {
        std::println(std::cerr, "Erro: '{}' não encontrado.", file_name);
        return 1;
    }

    // Vemos se já existe o arquivo alvo
    if (std::ranges::any_of(img.list_dir(new_dir), by_name(new_name))) {
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
