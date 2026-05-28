#include "include/Shell.hpp"
#include <print>
#include <iostream>

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::println(std::cerr, "Uso: {} <imagem ext4>", argv[0]);
        return 1;
    } else if(argc > 2) {
        std::println(std::cout, "Warn: Muitos argumentos fornecidos. Foi considerado apenas o primeiro.");
    }

    std::string image_path = argv[1];

    Shell shell = Shell(image_path);

    return shell.run();
}