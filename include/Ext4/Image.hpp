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
        // Desabilita a cópia para evitar problemas de gerenciamento de recursos.
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        
        // Permite a movimentação para facilitar o gerenciamento de recursos.
        Image(Image&&) = default;
        Image& operator=(Image&&) = default;
        ~Image() { if(image_file.is_open()) image_file.close(); }
    };
}
