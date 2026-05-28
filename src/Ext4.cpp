#include "../include/Ext4.hpp"
#include <print>
#include <string>
#include <stdexcept>

Ext4::Image::Image(const std::string& image_path) {
    // Tentamos abrir o arquivo de imagem em modo leitura e escrita, e em formato binário.
    this->image_file.open(image_path, std::ios::in | std::ios::out| std::ios::binary);
    
    if (!this->image_file.is_open()) {
        throw std::runtime_error("Erro ao abrir a imagem: O arquivo '" + image_path + "' não existe ou está inacessível.");
    }

    // Evitar muita verbosidade.
    using namespace Ext4::Structures;

    // Tentamos colocar o cursosr de leitura no offset fixo onde o superbloco deve residir.
    if (!this->image_file.seekg(SuperBlockNS::SUPERBLOCK_OFFSET, std::ios::beg)) {
        throw std::runtime_error("Erro ao posicionar o ponteiro de leitura no offset do superbloco.");
    }

    // Lemos o superbloco da imagem para validar a assinatura mágica e garantir que é um sistema de arquivos EXT4 válido.
    SuperBlock super_block;
    this->image_file.read(reinterpret_cast<char*>(&super_block), sizeof(SuperBlock));
    
    if (this->image_file.gcount() != sizeof(SuperBlock)) {
        throw std::runtime_error(
            "Erro ao ler o superbloco: quantidade de bytes lida diferente do esperado: " 
            + std::to_string(this->image_file.gcount()) 
            + " bytes lidos, mas esperados " 
            + std::to_string(sizeof(SuperBlock)) + " bytes."
        );
    }

    if (super_block.s_magic != SuperBlockNS::MAGIC) {
        throw std::runtime_error("Erro: A imagem fornecida não contem um superbloco válido do EXT4 (Assinatura mágica incorreta).");
    }

    this->super_block = super_block; // Armazenamos o superbloco lido para uso futuro.

    // Calculamos o tamanho do bloco a partir do valor lido no superbloco.
    this->block_size = 1024 << super_block.s_log_block_size;
    this->block_count = (static_cast<uint64_t>(super_block.s_blocks_count_hi) << 32) | super_block.s_blocks_count_lo;

    this->inode_size = super_block.s_inode_size;
    this->inode_count = super_block.s_inodes_count;

    this->blocks_per_group = super_block.s_blocks_per_group;
    this->inodes_per_group = super_block.s_inodes_per_group;
    std::println("Superbloco lido com sucesso! Assinatura mágica verificada: 0x{:04X}\nBem-vindo ao EXT4shell!", super_block.s_magic);
}