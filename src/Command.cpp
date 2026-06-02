#include "../include/Command.hpp"
#include <iostream>
#include <charconv>

using Ext4::Wrappers::Image;

short Command::help() {
    for(const auto& [cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::info(Image& img) {
    return 0;
}

short Command::cat(Image& img, const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: cat <arquivo>");
        return 1;
    }

    return 0;
}

short Command::attr(Image& img, const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: attr <arquivo/diretório>");
        return 1;
    }

    return 0;
}

short Command::cd(Image& img, const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        std::println(std::cerr, "Uso: cd <diretório>");
        return 1;
    }

    return 0;
}

short Command::ls(Image& img, const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if (target_path.empty()) {
        // Listar o diretório atual
    } else {
        // Listar o diretório especificado
    }

    return 0;
}

short Command::test_inode(Image& img, const std::vector<std::string>& args) {
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

short Command::test_block(Image& img, const std::vector<std::string>& args) {
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

    // Faça sua mágica!

    return 0;
}

short Command::cp(Image& img, const std::vector<std::string>& args) {
    const std::string source_path = args.size() > 1 ? args[1] : "";
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if (source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: cp <arquivo1> <arquivo2>");
        return 1;
    }

    return 0;
}

short Command::pwd(Image& img) {
    return 0;
}

short Command::touch(Image& img, const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: touch <arquivo>");
        return 1;
    }

    return 0;
}

short Command::mkdir(Image& img, const std::vector<std::string>& args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: mkdir <diretório>");
        return 1;
    }

    return 0;
}

short Command::rm(Image& img, const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if (file_path.empty()) {
        std::println(std::cerr, "Uso: rm <arquivo>");
        return 1;
    }

    return 0;
}

short Command::rmdir(Image& img, const std::vector<std::string>& args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if (dir_path.empty()) {
        std::println(std::cerr, "Uso: rmdir <diretório>");
        return 1;
    }

    return 0;
}

short Command::rename(Image& img, const std::vector<std::string>& args) {
    const std::string file = args.size() > 1 ? args[1] : "";
    const std::string new_file_name = args.size() > 2 ? args[2] : "";

    if (file.empty() || new_file_name.empty()) {
        std::println(std::cerr, "Uso: rename <arquivo> <novo nome do arquivo>");
        return 1;
    }
    return 0;
}

short Command::clear() {
    // Usamos CSI J 2 para limpar a tela e CSI H para mover o cursor para a posição inicial.
    std::print("\033[2J\033[H");
    return 0;
}