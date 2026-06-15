#include "../include/Command.hpp"
#include <iostream>
#include <charconv>
#include <ranges>
#include <algorithm>

using Ext4::Wrappers::Image;

short Command::help() {
    for (const auto &[cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::info(Image &img) {
    return 0;
}

short Command::cat(Image &img, const std::vector<std::string> &args) {
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
    auto target_file = std::ranges::find_if (entries, [&](const auto &entry) {
        return entry.get_name() == file_name;
    });

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

    std::string_view content(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    std::println("{}", content);
    return 0;
}

short Command::attr(Image &img, const std::vector<std::string> &args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: attr <arquivo/diretório>");
        return 1;
    }

    // necessário implementar

    return 0;
}

short Command::cd(Image &img, const std::vector<std::string> &args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: cd <diretório>");
        return 1;
    }
    
    // Procuramos o diretório
    auto [parent_dir, resolved_path] = img.resolve_path(target_path, img.get_current_inode());

    // Checamos se não chegamos no memso diretório
    if (img.get_current_path().ends_with(resolved_path)) {
        std::println("Diretório alvo é o mesmo do atual.");
        return 1;
    }

    // Agora trocamos o diretório corrente
    img.set_current_inode(parent_dir);
    img.set_current_path(resolved_path);

    return 0;
}

short Command::ls(Image &img, const std::vector<std::string> &args) {
    const std::string target_path = args.size() > 1 ? args[1] : img.get_current_path();
    auto [parent_dir, _] = img.resolve_path(target_path, img.get_current_inode());

    for (const auto &entry: img.list_dir(parent_dir)) {
        bool is_dir = entry.is_dir();
        std::println("{}{}{}", is_dir ? "\033[32m" : "", entry.get_name(), is_dir ? "\033[0m" : "");
    }

    return 0;
}

short Command::test_inode(Image &img, const std::vector<std::string> &args) {
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

short Command::test_block(Image &img, const std::vector<std::string> &args) {
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

short Command::to_in(Image &img, const std::vector<std::string> &args) {
    const std::string source_path = args.size() > 1 ? args[1] : "";
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if (source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: import <arquivo do SO> <diretório da imagem>");
        return 1;
    }

    // necessário implementar

    return 0;
}

short Command::to_out(Image &img, const std::vector<std::string> &args) {
    const std::string source_path = args.size() > 1 ? args[1] : "";
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if (source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: export <arquivo da imagem> <diretório do SO>");
        return 1;
    }

    // necessário implementar

    return 0;
}


short Command::pwd(Image &img) {
    std::println("{}", img.get_current_path());
    return 0;
}

short Command::touch(Image &img, const std::vector<std::string> &args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: touch <arquivo>");
        return 1;
    }

    auto [path, file_name] = Utils::split_path(file_path);

    auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());

    // Vemos se o arquivo já existe.
    bool file_exists = std::ranges::any_of(img.list_dir(parent_dir), [&](const auto &entry) {
        return entry.get_name() == file_name;
    });

    // Verifica se o arquivo realmente foi encontrado antes de extrair para a variável e se existe
    if (file_exists) {
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
        .i_links_count = 1,
        .i_flags     = Flags::InodeFlags::EXT4_EXTENTS_FL,  // EXT4_EXTENTS_FL
        .i_size_hi   = 0,
        .i_extra_isize = img.get_inode(2).get_raw().i_extra_isize,

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
        std::vector<std::byte>(img.get_superblock().get_inode_size() - sizeof(Raw::Inode), std::byte{0})));

    img.dir_add_entry(parent_dir, new_inode_id, file_name, Raw::DirectoryFileType::EXT4_FT_REG_FILE);

    // Modificamos o inode pai para modificar os campos "modificado"
    Raw::Inode parent_raw = parent_dir.get_raw();
    parent_raw.i_mtime = now;
    img.write_inode(parent_dir);
    return 0;
}

short Command::mkdir(Image &img, const std::vector<std::string> &args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: mkdir <diretório>");
        return 1;
    }

    auto [path, path_name] = Utils::split_path(dir_path);

    auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());

    // vemos se o diretório já existe
    bool path_exists = std::ranges::any_of(img.list_dir(parent_dir), [&](const Wrappers::DirectoryEntry &entry){
        return entry.get_name() == path_name;
    });

    if (path_exists) {
        std::println("Diretório alvo existe e é um arquivo/diretório");
        return 1;
    }

    return 0;
}

short Command::rm(Image &img, const std::vector<std::string> &args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: rm <arquivo>");
        return 1;
    }

    auto [path, file_name] = Utils::split_path(file_path);

    auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());

    std::vector<Wrappers::DirectoryEntry> entries = img.list_dir(parent_dir);

    // Vemos se o arquivo não existe mais
    auto target = std::ranges::find_if (entries, [&](const auto &e) {
        return e.get_name() == file_name;
    });

    if (target == entries.end()) {
        std::println(std::cerr, "Erro: Arquivo '{}' não encontrado.", file_name);
        return 1;
    }

    if (target->is_dir()) {
        std::println(std::cerr, "Erro: '{}' é um diretório. Use rmdir.", file_name);
        return 1;
    }

    img.dir_remove_entry(parent_dir, file_name);

    return 0;
}

short Command::rmdir(Image &img, const std::vector<std::string> &args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: rmdir <diretório>");
        return 1;
    }

    // necessário implementar

    return 0;
}

short Command::rename(Image &img, const std::vector<std::string> &args) {
    const std::string file = args.size() > 1 ? args[1] : "";
    const std::string new_file_name = args.size() > 2 ? args[2] : "";

    if (file.empty() || new_file_name.empty()) {
        std::println(std::cerr, "Uso: rename <arquivo> <novo nome do arquivo>");
        return 1;
    }

    auto [path, file_name] = Utils::split_path(file);

    auto [parent_dir, resolved_path] = img.resolve_path(path, img.get_current_inode());
    
    // Vemos se o arquivo existe.
    auto entries = img.list_dir(parent_dir);

    bool file_exists = false;
    bool name_taken  = false;

    // Aqui fazer um loop manual tende a ser mais eficiente já que estamos procurando 2 valores em si
    for (const auto &e : entries) {
        if (e.get_name() == file_name)    file_exists = true;
        if (e.get_name() == new_file_name) name_taken  = true;
        if (file_exists && name_taken) break;
    }

    if (!file_exists) {
        std::println(std::cerr, "Erro: '{}' não encontrado.", file_name);
        return 1;
    }
    if (name_taken) {
        std::println(std::cerr, "Erro: '{}' já existe.", new_file_name);
        return 1;
    }

    img.dir_rename_entry(parent_dir, file_name, new_file_name);

    return 0;
}

short Command::clear() {
    // Usamos CSI J 2 para limpar a tela e CSI H para mover o cursor para a posição inicial.
    std::print("\033[2J\033[H");
    return 0;
}