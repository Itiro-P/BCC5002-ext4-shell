#include "../include/Ext4.hpp"

Ext4::Image::Image(const std::string& image_path) {
    try {
        this->image_file.open(image_path, std::ios::binary);
    } catch (const std::exception& e) {
        throw std::runtime_error("Erro ao abrir a imagem: " + std::string(e.what()));
    }
}