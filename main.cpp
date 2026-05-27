#include <iostream>
#include "include/Shell.hpp"

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cerr << "Uso: " << argv[0] << " <imagem ext4>\n";
        return 1;
    } else if(argc > 2) {
        std::cout << "Warn: Muitos argumentos fornecidos. Foi considerado apenas o primeiro.\n";
    }

    std::string image_path = argv[1];

    Shell shell = Shell();

    return shell.run(image_path);
}