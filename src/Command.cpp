#include "../include/Command.hpp"
#include <iostream>

short Command::info() {
    for(const auto& [cmd, desc] : Command::command_info) {
        std::println("- {:<32} - {}", cmd, desc);
    }
    return 0;
}

short Command::cat(const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if(file_path.empty()) {
        std::println(std::cerr, "Uso: cat <arquivo>");
        return 1;
    }

    return 0;
}

short Command::attr(const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if(target_path.empty()) {
        std::println(std::cerr, "Uso: attr <arquivo/diretório>");
        return 1;
    }

    return 0;
}

short Command::cd(const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if(target_path.empty()) {
        std::println(std::cerr, "Uso: cd <diretório>");
        return 1;
    }

    return 0;
}

short Command::ls(const std::vector<std::string>& args) {
    const std::string target_path = args.size() > 1 ? args[1] : "";

    if(target_path.empty()) {
        // Listar o diretório atual
    } else {
        // Listar o diretório especificado
    }

    return 0;
}

short Command::test_inode(const std::vector<std::string>& args) {
    const std::string inode_str = args.size() > 1 ? args[1] : "";

    if(inode_str.empty()) {
        std::println(std::cerr, "Uso: test_inode <inode>");
        return 1;
    }

    try {
        const int inode = std::stoi(inode_str);
        // Faça sua mágica!


    } catch(std::invalid_argument& e) {
        std::println(std::cerr, "Erro: O argumento para testi deve ser um número inteiro representando o inode.");
        return 1;
    } catch(std::out_of_range& e) {
        std::println(std::cerr, "Erro: O número do inode fornecido está fora do intervalo permitido (inteiro com sinal de 32 bits).");
        return 1;
    }

    return 0;
}

short Command::test_block(const std::vector<std::string>& args) {
    const std::string block_str = args.size() > 1 ? args[1] : "";

    if(block_str.empty()) {
        std::println(std::cerr, "Uso: test_block <bloco>");
        return 1;
    }

    try {
        const int block = std::stoi(block_str);
        // Faça sua mágica!


    } catch(std::invalid_argument& e) {
        std::println(std::cerr, "Erro: O argumento para testb deve ser um número inteiro representando o bloco.");
        return 1;
    } catch(std::out_of_range& e) {
        std::println(std::cerr, "Erro: O número do bloco fornecido está fora do intervalo permitido (inteiro com sinal de 32 bits).");
        return 1;
    }

    return 0;
}

short Command::cp(const std::vector<std::string>& args) {
    const std::string source_path = args.size() > 1 ? args[1] : "";
    const std::string dest_path = args.size() > 2 ? args[2] : "";

    if(source_path.empty() || dest_path.empty()) {
        std::println(std::cerr, "Uso: cp <arquivo1> <arquivo2>");
        return 1;
    }

    return 0;
}

short Command::pwd() {
    return 0;
}

short Command::touch(const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if(file_path.empty()) {
        std::println(std::cerr, "Uso: touch <arquivo>");
        return 1;
    }

    return 0;
}

short Command::mkdir(const std::vector<std::string>& args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if(dir_path.empty()) {
        std::println(std::cerr, "Uso: mkdir <diretório>");
        return 1;
    }

    return 0;
}

short Command::rm(const std::vector<std::string>& args) {
    const std::string file_path = args.size() > 1 ? args[1] : "";

    if(file_path.empty()) {
        std::println(std::cerr, "Uso: rm <arquivo>");
        return 1;
    }

    return 0;
}

short Command::rmdir(const std::vector<std::string>& args) {
    const std::string dir_path = args.size() > 1 ? args[1] : "";

    if(dir_path.empty()) {
        std::println(std::cerr, "Uso: rmdir <diretório>");
        return 1;
    }

    return 0;
}

short Command::rename(const std::vector<std::string>& args) {
    const std::string file = args.size() > 1 ? args[1] : "";
    const std::string new_file_name = args.size() > 2 ? args[2] : "";

    if(file.empty() || new_file_name.empty()) {
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