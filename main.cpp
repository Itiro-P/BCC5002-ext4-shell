#include <iostream>

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        std::cerr << "Uso: " << argv[0] << " <imagem ext4>\n";
        return 1;
    }

    std::string image_path = argv[1];

    return 0;
}