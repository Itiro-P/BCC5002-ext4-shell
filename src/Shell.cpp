#include "../include/Shell.hpp"
#include <iostream>
#include <vector>

/**
 * @file    Shell.cpp
 * @brief   Implementação do shell interativo
 * @author  Pedro Itiro Nagao
 * @date    2026-06-11
 *
 * Responsável por passar os argumentos da entrada do usuário para as respectivas funções.
 */


short Shell::run() {
    std::string line;

    short exit_code = 0;

    while (true) {
        this->print_prompt(exit_code);

        if (!std::getline(std::cin, line)) break;  // EOF (Ctrl+D)

        std::vector<std::string> args = Utils::tokenize(line);

        if (args.empty()) continue;

        if (exit_commands.find(args[0]) != exit_commands.end()) break;  // sai limpo aqui

        // Adoro o C++
        if (auto it = command_map.find(args[0]); it != command_map.end()) {
            exit_code = it->second(this->image, args);
        } else {
            std::println(std::cerr, "ext4shell: comando não encontrado: {}", args[0]);
        }
    }
    return exit_code;
}