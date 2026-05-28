#pragma once

#include <fstream>
#include <string>

namespace Ext4 {
    /**
     * @brief Classe utilitária responsável por encapsular a manipulação de arquivos de imagem de disco.
     */
    class Image {
        std::fstream image_file;
    public:
        explicit Image(const std::string& image_path);
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        Image(Image&&) = default;
        Image& operator=(Image&&) = default;
        ~Image() { if(image_file.is_open()) image_file.close(); }
    };
}
