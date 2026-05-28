#pragma once

#include <string>
#include <fstream>

/**
 * @brief Classe responsável por representar a imagem do sistema de arquivos EXT4 e fornecer métodos para acessar seus dados.
 * Foi feita para ser um singleton que representa a imagem montada.
 */
class Ext4Image {
    std::ifstream image_file;
public:
    // Construtor
    explicit Ext4Image(const std::string& image_path): image_file(image_path, std::ios::binary) {
        if(!image_file.is_open()) {
            throw std::runtime_error("Não foi possível abrir a imagem: " + image_path);
        }
    }

    // Não podemos permitir cópias.
    Ext4Image(const Ext4Image&) = delete;
    Ext4Image& operator=(const Ext4Image&) = delete;

    // Move está tudo bem, mantém uma instância única da imagem.
    Ext4Image(Ext4Image&&) = default;
    Ext4Image& operator=(Ext4Image&&) = default;

    ~Ext4Image() {
        if(image_file.is_open()) image_file.close();
    }
};