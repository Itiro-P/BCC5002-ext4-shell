#include "../include/Ext4.hpp"
#include "../include/Utils.hpp"
#include <print>
#include <string>
#include <stdexcept>
#include <cstring>
#include <span>

using namespace Ext4;

Wrappers::Image::Image(const std::string& image_path) {
    // Tentamos abrir o arquivo de imagem em modo leitura e escrita, e em formato binário.
    this->image_file.open(image_path, std::ios::in | std::ios::out| std::ios::binary);
    
    if (!this->image_file.is_open()) {
        throw std::runtime_error("Erro ao abrir a imagem: O arquivo '" + image_path + "' não existe ou está inacessível.");
    }

    // Tentamos colocar o cursor de leitura no offset fixo onde o superbloco deve residir; e
    // lemos o superbloco da imagem para validar a assinatura mágica e garantir que é um sistema de arquivos EXT4 válido.
    Ext4::Raw::SuperBlock super_block{};
    this->read_offset(Ext4::Constants::SUPERBLOCK_OFFSET, Utils::as_span(super_block));

    if (super_block.s_magic != Ext4::Constants::EXT_MAGIC) {
        throw std::runtime_error("Erro: A imagem fornecida não contem um superbloco válido do EXT4 (Assinatura mágica incorreta).");
    }

    // Armazenamos o superbloco lido para uso futuro.
    this->super_block = super_block;

    // Verifica se a flag de 64 bits está ativa.
    this->is_64 = (super_block.s_feature_incompat & Flags::SuperBlockFlags::INCOMPAT_64BIT) != 0;

    // Calculamos o tamanho do bloco a partir do valor lido no superbloco.
    this->block_size = 1024 << super_block.s_log_block_size;
    this->block_count = this->is_64 ? Utils::concatenate(super_block.s_blocks_count_lo, super_block.s_blocks_count_hi) : super_block.s_blocks_count_lo;

    this->inode_size = super_block.s_inode_size;
    this->inode_count = super_block.s_inodes_count;

    this->blocks_per_group = super_block.s_blocks_per_group;
    this->inodes_per_group = super_block.s_inodes_per_group;

    // Agora tentaremos ver onde está a GDT.
    uint64_t gdt_offset = (super_block.s_first_data_block + 1) * this->block_size; // O GDT começa no próximo bloco após o superbloco.

    // Calculamos o número de grupos.
    // E aproveitamos para popular os bitmaps e tabelas de inodes, já que vamos precisar deles para ler os descritores de grupo.
    uint32_t group_count = (this->block_count + this->blocks_per_group - 1) / this->blocks_per_group;
    uint16_t desc_size = super_block.s_desc_size > 0 ? super_block.s_desc_size : 32;

    this->inode_table_offsets.clear();

    for (uint32_t i = 0; i < group_count; i++) {
        Raw::GroupDescriptor gd{};
        std::streamoff offset = gdt_offset + (static_cast<uint64_t>(i) * desc_size);
        this->read_offset(offset, Utils::as_span(gd));
        // Recupera o endereço base do bloco da tabela de inodes deste grupo
        uint64_t inode_table_block = this->is_64 ? Utils::concatenate(gd.bg_inode_table_lo, gd.bg_inode_table_hi) : gd.bg_inode_table_lo;
        
        // Converte o endereço de blocos para um offset absoluto em bytes e armazena
        this->inode_table_offsets.push_back(inode_table_block * this->block_size);
        this->group_descriptors.push_back(gd);
    }
    this->current_inode = this->get_inode(2); // O diretório raiz está sempre no Inode 2.
    std::println("Imagem lida com sucesso! Assinatura mágica verificada: 0x{:04X}\nBem-vindo ao EXT4shell!", super_block.s_magic);
}

void Wrappers::Image::seek(std::streamoff offset) {
    this->image_file.clear(); 

    this->image_file.seekg(offset, std::ios::beg);

    this->image_file.seekp(offset, std::ios::beg);

    if (!this->image_file) {
        throw std::runtime_error(std::format("Seek falhou no offset {}.", offset));
    }
}

void Wrappers::Image::read_offset(std::streamoff offset, std::span<std::byte> buffer) {
    this->seek(offset);
    
    // Executa a leitura binária convertendo o span de bytes para char*
    this->image_file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
    std::streamsize read_bytes = this->image_file.gcount();

    if (read_bytes != static_cast<std::streamsize>(buffer.size())) {
        // Se falhou, limpa o estado de erro para não travar os próximos comandos da aplicação
        this->image_file.clear(); 
        
        throw std::runtime_error(std::format(
            "Erro ao ler da imagem: {} bytes lidos, esperados {}. (Offset: {})",
            read_bytes, buffer.size(), offset
        ));
    }
}

void Wrappers::Image::read_block(uint64_t block_num, std::span<std::byte> buffer) {
    std::streamoff offset = block_num * this->block_size;
    this->read_offset(offset, buffer);
}

Ext4::Raw::Inode Wrappers::Image::get_raw_inode(const uint32_t inode_num) {

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
    // ATENÇÃO: Lemos apenas o tamanho físico real indicado pelo superbloco (this->inode_size)
    // para evitar invadir memória de estruturas vizinhas se o sizeof da struct Inode for diferente que o inode_size configurado no superbloco.
    // Aloca um buffer do tamanho real do disco
    std::vector<std::byte> buffer(this->inode_size);
    this->read_offset(final_inode_offset, Utils::as_span(buffer.data(), buffer.size()));

    // Copia apenas o que a struct comporta
    Ext4::Raw::Inode inode{};
    std::memcpy(&inode, buffer.data(), std::min(sizeof(inode), static_cast<size_t>(this->inode_size)));
    return inode;
}

Wrappers::Inode Wrappers::Image::get_inode(const uint32_t inode_num) {
    Raw::Inode inode = this->get_raw_inode(inode_num);
    Wrappers::Inode wrapper = Wrappers::Inode(inode_num, inode);

    return wrapper;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_leafs(const Wrappers::Inode& inode, const Raw::ExtentHeader& header) {
    std::vector<uint64_t> blocks;
    auto leafs = std::span<const Raw::ExtentLeaf>(
        reinterpret_cast<const Raw::ExtentLeaf*>(inode.get_i_block().data() + sizeof(Raw::ExtentHeader)), 
        header.eh_entries
    );
    for (const auto& leaf : leafs) {
        uint64_t start_block = leaf.getStartBlock();
        uint16_t length = leaf.getRealLength();
        for (uint16_t i = 0; i < length; ++i) {
            blocks.push_back(start_block + i);
        }
    }
    return blocks;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_index(const Wrappers::Inode& inode, const uint64_t index_block, const uint16_t depth) {
    std::vector<uint64_t> blocks;
    // Implementação da leitura de blocos a partir de um nó de índice
    return blocks;
}

std::vector<std::byte> Wrappers::Image::read_file(const Wrappers::Inode& inode) {
    std::vector<std::byte> file_data;
    return file_data;
}

std::vector<uint64_t> Wrappers::Image::get_blocks(Wrappers::Inode& inode) {
    std::vector<uint64_t> data_blocks;
    Raw::ExtentHeader header{};
    std::memcpy(&header, inode.get_i_block().data(), sizeof(Raw::ExtentHeader));
    if (header.eh_magic != Constants::EXTENT_MAGIC)
        throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico de extents inválido. O Inode pode estar corrompido ou não utilizar extents.");
    
    if (header.eh_depth == 0) {
        // Nó folha contendo dados reais. As entradas de blocos estão diretamente após o cabeçalho.
        data_blocks = this->read_blocks_from_leafs(inode, header);
    } else {
        // Nó interno de indexação. As entradas de blocos são ponteiros para outros nós de extents.
        data_blocks = this->read_blocks_from_index(inode, 0, header.eh_depth);
    }
    return data_blocks;
}