#include "include/Shell.hpp"
#include <print>
#include <iostream>

/**
 * @brief Ponto de entrada do programa, onde a imagem do sistema de arquivos EXT4 é carregada e o shell é iniciado.
 * @author Pedro Itiro Nagao
 */
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::println(std::cerr, "Uso: {} <imagem ext4>", argv[0]);
        return 1;
    } else if (argc > 2) {
        std::println(std::cout, "Warn: Muitos argumentos fornecidos. Foi considerado apenas o primeiro.");
    }

    std::string image_path = argv[1];

    try {
        Shell shell = Shell(image_path);

        return shell.run();
    } catch (const std::exception& e) {
        std::println(std::cerr, "Erro ao inicializar a imagem: {}", e.what());
        return 1;
    }
}
