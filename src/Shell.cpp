#include "../include/Shell.hpp"
#include <iostream>
#include <vector>
#include <ranges>

short Shell::run(const std::string& image_path) {
    // Alguma maracutaia deve acontecer aqui para conseguirmos a imagem montada e o diretório atual.
    std::string current_directory = "/";
    print_prompt(current_directory);

    std::string command;
    std::getline(std::cin, command);

    // Usamos ranges para dividir a string de comando em argumentos, usando o espaço como delimitador.
    // Colocamos a string alvo.
    std::vector<std::string> args = command | 
        // Usamos pipe com split para tokenizar por espaços.
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

    return this->command_map[args[0]](args);
}