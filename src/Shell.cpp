#include "../include/Shell.hpp"
#include <iostream>
#include <vector>
#include <ranges>

short Shell::run() {
    // Alguma maracutaia deve acontecer aqui para conseguirmos a imagem montada e o diretório atual.
    std::string current_directory = "/";

    std::string line;

    short exit_code = 0;

    while (true) {
        print_prompt(current_directory);

        if (!std::getline(std::cin, line)) break;  // EOF (Ctrl+D)

        // Usamos ranges para dividir a string de comando em argumentos, usando o espaço como delimitador.
        // Colocamos a string alvo, em um std::string_view para evitar cópias desnecessárias, e depois transformamos cada argumento em std::string.
        std::vector<std::string> args = line | 
            // Quebramos a linha em argumentos
            std::views::split(' ') | 
            // Eliminamos (filtramos) os argumentos vazios ou que contenham apenas espaços.
            std::views::filter([](auto&& arg) { 
                return !arg.empty() && std::none_of(arg.begin(), arg.end(), isspace); 
            }) | 
            // Transformamos cada argumento em uma string normal.
            std::views::transform([](auto&& arg) { 
                return std::string(arg.begin(), arg.end()); 
            }) |
            // E colocamos em um vetor de strings.
            std::ranges::to<std::vector>();

        if(args.empty()) continue;

        if(this->exit_commands.find(args[0]) != this->exit_commands.end()) break;  // sai limpo aqui

        // Adoro o C++
        if(auto it = this->command_map.find(args[0]); it != this->command_map.end()) {
            exit_code = it->second(this->image, args);
        } else {
            std::println(std::cerr, "ext4shell: comando não encontrado: {}", args[0]);
        }
    }
    return exit_code;
}