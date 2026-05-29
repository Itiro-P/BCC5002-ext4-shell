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

    // Tentamos colocar o cursor de leitura no offset fixo onde o superbloco deve residir; e
    // lemos o superbloco da imagem para validar a assinatura mágica e garantir que é um sistema de arquivos EXT4 válido.
    SuperBlock super_block{};
    this->seek_and_read(SuperBlockNS::SUPERBLOCK_OFFSET, std::ios::beg, reinterpret_cast<char*>(&super_block), sizeof(SuperBlock));

    if (super_block.s_magic != SuperBlockNS::MAGIC) {
        throw std::runtime_error("Erro: A imagem fornecida não contem um superbloco válido do EXT4 (Assinatura mágica incorreta).");
    }

    // Armazenamos o superbloco lido para uso futuro.
    this->super_block = super_block;

    // Verifica se a flag de 64 bits está ativa.
    this->is_64 = (super_block.s_feature_incompat & Flags::SuperBlockFlags::INCOMPAT_64BIT) != 0;

    // Calculamos o tamanho do bloco a partir do valor lido no superbloco.
    this->block_size = 1024 << super_block.s_log_block_size;
    this->block_count = this->is_64 ? this->concatenate(super_block.s_blocks_count_lo, super_block.s_blocks_count_hi) : super_block.s_blocks_count_lo;

    this->inode_size = super_block.s_inode_size;
    this->inode_count = super_block.s_inodes_count;

    this->blocks_per_group = super_block.s_blocks_per_group;
    this->inodes_per_group = super_block.s_inodes_per_group;

    // Agora tentaremos ver onde está a GDT.
    uint64_t gdt_offset = (super_block.s_first_data_block + 1) * this->block_size; // O GDT começa no próximo bloco após o superbloco.

    // Calculamos o número de grupos.
    // E aproveitamos para popular os bitmaps e tabelas de inodes, já que vamos precisar deles para ler os descritores de grupo.
    uint32_t group_count = (this->block_count + this->blocks_per_group - 1) / this->blocks_per_group;
    uint16_t desc_size = super_block.s_desc_size;

    this->inode_table_offsets.clear();

    for (uint32_t i = 0; i < group_count; i++) {
        GroupDescriptor gd{};
        std::streamoff offset = gdt_offset + (static_cast<uint64_t>(i) * desc_size);
        this->seek_and_read(offset, std::ios::beg, reinterpret_cast<char*>(&gd), desc_size);

        // Recupera o endereço base do bloco da tabela de inodes deste grupo
        uint64_t inode_table_block = this->is_64 ? this->concatenate(gd.bg_inode_table_lo, gd.bg_inode_table_hi) : gd.bg_inode_table_lo;
        
        // Converte o endereço de blocos para um offset absoluto em bytes e armazena
        this->inode_table_offsets.push_back(inode_table_block * this->block_size);
        this->group_descriptors.push_back(gd);
    }
    this->current_inode = this->get_inode(2); // O diretório raiz está sempre no Inode 2.
    std::println("Imagem lida com sucesso! Assinatura mágica verificada: 0x{:04X}\nBem-vindo ao EXT4shell!", super_block.s_magic);
}

void Ext4::Image::seek(std::streamoff offset, std::ios_base::seekdir dir) {
    if (!this->image_file.seekg(offset, dir)) {
        throw std::runtime_error("Erro ao posicionar o ponteiro de leitura/escrita na imagem.");
    }
}

void Ext4::Image::read(char *buffer, std::streamsize size) {
    this->image_file.read(buffer, size);
    if (this->image_file.gcount() != size) {
        throw std::runtime_error(
            "Erro ao ler da imagem: quantidade de bytes lida diferente do esperado: " 
            + std::to_string(this->image_file.gcount()) 
            + " bytes lidos, mas esperados " 
            + std::to_string(size) + " bytes."
        );
    }
}

void Ext4::Image::seek_and_read(std::streamoff offset, std::ios_base::seekdir dir, char *buffer, std::streamsize size) {
    this->seek(offset, dir);
    this->read(buffer, size);
}

Ext4::Inode::Inode Ext4::Image::get_raw_inode(const uint32_t inode_num) {

    // Validação preventiva: Inodes no EXT4 começam obrigatoriamente no índice 1
    if (inode_num == 0 || inode_num > this->inode_count) {
        throw std::runtime_error("Erro: Número de inode inválido ou fora dos limites: " + std::to_string(inode_num));
    }

    // 1. Descobrir a qual Block Group este inode pertence
    uint32_t group = (inode_num - 1) / this->inodes_per_group;

    // 2. Descobrir o índice local do inode dentro da tabela daquele grupo
    uint32_t index = (inode_num - 1) % this->inodes_per_group;

    // 3. Buscar o offset em bytes de onde começa a tabela de inodes do grupo correspondente
    uint64_t table_base_offset = this->inode_table_offsets.at(group);

    // 4. Calcular a posição absoluta do inode alvo
    std::streamoff final_inode_offset = table_base_offset + (static_cast<uint64_t>(index) * this->inode_size);

    // 5. Alocar a struct e ler do disco
    Ext4::Inode::Inode inode{};
    
    // ATENÇÃO: Lemos apenas o tamanho físico real indicado pelo superbloco (this->inode_size)
    // para evitar invadir memória de estruturas vizinhas se o sizeof da struct Inode for diferente que o inode_size configurado no superbloco.
    this->seek_and_read(final_inode_offset, std::ios::beg, reinterpret_cast<char*>(&inode), this->inode_size);
    return inode;
}

Ext4::Inode::InodeWrapper Ext4::Image::get_inode(const uint32_t inode_num) {
    Ext4::Inode::Inode inode = this->get_raw_inode(inode_num);
    Ext4::Inode::InodeWrapper wrapper = Ext4::Inode::InodeWrapper(inode_num, inode);

    return wrapper;
}