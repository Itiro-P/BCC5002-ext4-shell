#include "../include/Shell.hpp"
#include <iostream>
#include <vector>

short Shell::run() {
    std::string line;

    short exit_code = 0;

    while (true) {
        print_prompt();

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