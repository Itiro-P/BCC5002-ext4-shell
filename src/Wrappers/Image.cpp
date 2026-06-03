#include "../../include/Ext4.hpp"
#include "../../include/Utils.hpp"
#include <print>
#include <string>
#include <stdexcept>
#include <cstring>
#include <span>
#include <vector>
#include <iostream>
#include <ranges>

using namespace Ext4;

Wrappers::Image::Image(const std::string &image_path) {
    // Tentamos abrir o arquivo de imagem em modo leitura e escrita, e em formato binário.
    this->image_file.open(image_path, std::ios::in | std::ios::out| std::ios::binary);
    
    if (!this->image_file.is_open()) {
        throw std::runtime_error("Erro ao abrir a imagem: O arquivo '" + image_path + "' não existe ou está inacessível.");
    }

    // Tentamos colocar o cursor de leitura no offset fixo onde o superbloco deve residir; e
    // lemos o superbloco da imagem para validar a assinatura mágica e garantir que é um sistema de arquivos EXT4 válido.
    Raw::SuperBlock super_block{};
    this->read_offset(Constants::SUPERBLOCK_OFFSET, Utils::as_span(super_block));

    // Armazenamos o superbloco lido para uso futuro.
    this->super_block = Wrappers::SuperBlock(super_block);
    this->super_block.validate();

    // Agora tentaremos ver onde está a GDT.
    uint64_t gdt_offset = this->super_block.get_gdt_offset();

    // Calculamos o número de grupos.
    // E aproveitamos para popular os bitmaps e tabelas de inodes, já que vamos precisar deles para ler os descritores de grupo.
    uint32_t group_count = this->super_block.get_group_count();
    uint16_t desc_size = this->super_block.get_desc_size();

    this->inode_table_offsets.clear();

    for (uint32_t i = 0; i < group_count; i++) {
        Raw::GroupDescriptor gd{};
        std::streamoff offset = gdt_offset + (static_cast<uint64_t>(i) * desc_size);
        this->read_offset(offset, Utils::as_span(gd));
        // Recupera o endereço base do bloco da tabela de inodes deste grupo
        uint64_t inode_table_block = this->super_block.is_64bit() ? Utils::concatenate(gd.bg_inode_table_lo, gd.bg_inode_table_hi) : gd.bg_inode_table_lo;
        
        // Converte o endereço de blocos para um offset absoluto em bytes e armazena
        this->inode_table_offsets.push_back(inode_table_block * this->super_block.get_block_size());
        this->group_descriptors.push_back(Wrappers::GroupDescriptor(gd, this->super_block.is_64bit()));
    }
    this->current_inode = this->get_inode(2); // O diretório raiz está sempre no Inode 2.
    this->root_inode = this->current_inode;
    std::println("Imagem lida com sucesso! Assinatura mágica verificada: 0x{:04X}\nBem-vindo ao EXT4shell!", super_block.s_magic);
}

void Wrappers::Image::seek(const std::streamoff offset) {
    this->image_file.clear(); 

    this->image_file.seekg(offset, std::ios::beg);

    this->image_file.seekp(offset, std::ios::beg);

    if (!this->image_file) {
        throw std::runtime_error(std::format("Seek falhou no offset {}.", offset));
    }
}

void Wrappers::Image::read_offset(const std::streamoff offset, std::span<std::byte> buffer) {
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

void Wrappers::Image::read_block(const uint64_t block_num, std::span<std::byte> buffer) {
    std::streamoff offset = block_num * this->super_block.get_block_size();
    this->read_offset(offset, buffer);
}

void Wrappers::Image::write_offset(const std::streamoff offset, const std::span<const std::byte> buffer) {
    this->seek(offset);
    
    // Executa a leitura binária convertendo o span de bytes para char*
    this->image_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    
    std::streamsize read_bytes = this->image_file.gcount();

    if (read_bytes != static_cast<std::streamsize>(buffer.size())) {
        // Se falhou, limpa o estado de erro para não travar os próximos comandos da aplicação
        this->image_file.clear(); 
        
        throw std::runtime_error(std::format(
            "Erro ao escrever na imagem: {} bytes lidos, esperados {}. (Offset: {})",
            read_bytes, buffer.size(), offset
        ));
    }

    this->image_file.flush();
}

void Wrappers::Image::write_block(const uint64_t block_num, const std::span<const std::byte> buffer) {
    std::streamoff offset = block_num * this->super_block.get_block_size();
    this->write_offset(offset, buffer);
}

std::streamoff Ext4::Wrappers::Image::get_inode_offset(const uint32_t inode_num) {
    // Validação preventiva: Inodes no EXT4 começam obrigatoriamente no índice 1
    if (inode_num == 0 || inode_num > this->super_block.get_inodes_count()) {
        throw std::runtime_error(std::format("Erro: Número de inode inválido ou fora dos limites: {}", inode_num));
    }

    // 1. Descobrir a qual Block Group este inode pertence
    uint32_t group = (inode_num - 1) / this->super_block.get_inodes_per_group();

    // 2. Descobrir o índice local do inode dentro da tabela daquele grupo
    uint32_t index = (inode_num - 1) % this->super_block.get_inodes_per_group();

    // 3. Buscar o offset em bytes de onde começa a tabela de inodes do grupo correspondente
    uint64_t table_base_offset = this->inode_table_offsets.at(group);

    // 4. Calcular a posição absoluta do inode alvo
    std::streamoff final_inode_offset = table_base_offset + (static_cast<uint64_t>(index) * this->super_block.get_inode_size());

    return final_inode_offset;
}

Raw::Inode Wrappers::Image::get_raw_inode(const uint32_t inode_num) {

    // Alocar a struct e ler do disco
    // ATENÇÃO: Lemos apenas o tamanho físico real indicado pelo superbloco (this->inode_size)
    // para evitar invadir memória de estruturas vizinhas se o sizeof da struct Inode for diferente que o inode_size configurado no superbloco.
    // Aloca um buffer do tamanho real do disco
    std::vector<std::byte> buffer(this->super_block.get_inode_size());
    this->read_offset(this->get_inode_offset(inode_num), Utils::as_span(buffer));

    // Copia apenas o que a struct comporta
    auto inode = Utils::copy_bounded<Raw::Inode>(buffer, static_cast<size_t>(this->super_block.get_inode_size()));
    return inode;
}

Wrappers::Inode Wrappers::Image::get_inode(const uint32_t inode_num) {
    Raw::Inode inode = this->get_raw_inode(inode_num);
    Wrappers::Inode wrapper = Wrappers::Inode(inode_num, inode);

    return wrapper;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_leafs(std::span<const std::byte> node_data, const Raw::ExtentHeader &header) {
    std::vector<uint64_t> blocks;
    auto leafs = std::span<const Raw::ExtentLeaf>(
        reinterpret_cast<const Raw::ExtentLeaf*>(node_data.data() + sizeof(Raw::ExtentHeader)), 
        header.eh_entries
    );
    for (const auto &leaf : leafs) {
        uint64_t start_block = leaf.getStartBlock();
        uint16_t length = leaf.getRealLength();
        for (uint16_t i = 0; i < length; ++i) {
            blocks.push_back(start_block + i);
        }
    }
    return blocks;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_index(std::span<const std::byte> node_data, const Raw::ExtentHeader &header) {
    std::vector<uint64_t> blocks;
    
    if (header.eh_magic != Constants::EXTENT_MAGIC) {
        throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico de extents inválido.");
    }
    
    auto index_entries = std::span<const Raw::ExtentIndex>(
        reinterpret_cast<const Raw::ExtentIndex*>(node_data.data() + sizeof(Raw::ExtentHeader)), 
        header.eh_entries
    );

    std::vector<std::byte> buffer(this->super_block.get_block_size());

    for (const auto &index : index_entries) {
        uint64_t child_block = index.getLeafBlock();
        
        // Lemos o bloco para o nosso buffer local
        this->read_block(child_block, Utils::as_span(buffer));
        Raw::ExtentHeader child_header = Utils::copy<Raw::ExtentHeader>(buffer);

        if (child_header.eh_magic != Constants::EXTENT_MAGIC) {
            throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico inválido no nó filho.");
        }

        if (child_header.eh_depth == 0) {
            // Passa por valor. read_blocks_from_leafs processa e joga os dados no vector 'child_blocks'
            auto child_blocks = this->read_blocks_from_leafs(Utils::as_span(buffer), child_header);
            blocks.insert(blocks.end(), child_blocks.begin(), child_blocks.end());
        } else {
            // Chamada recursiva segura: child_span é válido enquanto o pai executa esta linha
            auto child_blocks = this->read_blocks_from_index(Utils::as_span(buffer), child_header);
            blocks.insert(blocks.end(), child_blocks.begin(), child_blocks.end());
        }
    }
    return blocks; // Retorno por valor (Move semantics do C++ garante que é eficiente e seguro)
}

std::vector<std::byte> Wrappers::Image::read_file(const Wrappers::Inode &inode) {
    std::vector<std::byte> file_data;
    std::vector<uint64_t> data_blocks = this->get_blocks(inode);

    for(const auto &block_num : data_blocks) {
        std::vector<std::byte> buffer(this->super_block.get_block_size());
        this->read_block(block_num, Utils::as_span(buffer));
        file_data.insert(file_data.end(), buffer.begin(), buffer.end());
    }
    // Ajusta o tamanho do vetor para o tamanho real do arquivo, removendo bytes extras do último bloco
    file_data.resize(inode.get_size());
    return file_data;
}

std::vector<uint64_t> Wrappers::Image::get_blocks(const Wrappers::Inode &inode) {
    std::vector<uint64_t> data_blocks;
    auto header = Utils::copy<Raw::ExtentHeader>(inode.get_i_block());

    if (header.eh_magic != Constants::EXTENT_MAGIC)
        throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico de extents inválido. O Inode pode estar corrompido ou não utilizar extents.");
    
    std::span blocks_span = Utils::as_span(inode.get_i_block());

    if (header.eh_depth == 0) {
        // Nó folha contendo dados reais. As entradas de blocos estão diretamente após o cabeçalho.
        data_blocks = this->read_blocks_from_leafs(blocks_span, header);
    } else {
        // Nó interno de indexação. As entradas de blocos são ponteiros para outros nós de extents.
        data_blocks = this->read_blocks_from_index(blocks_span, header);
    }
    return data_blocks;
}

std::pair<Wrappers::Inode, std::string> Wrappers::Image::resolve_path(const std::string &path, const Wrappers::Inode &base) {
    if (path.empty() || path == "." || this->get_current_path().ends_with(path)) return {base, this->get_current_path()};
    if (path == "/") return {this->get_root_inode(), "/"};

    bool is_root = path[0] == '/';

    Wrappers::Inode inode = is_root ? this->get_root_inode() : base;
    std::vector<std::string> paths = Utils::filter_split(path, "/");
    
    // Começamos com o caminho base atual
    std::string final_path = is_root ? "/" : this->get_current_path();

    for (const auto &it : paths) {
        if (inode.get_type() != Flags::S_IFDIR) {
            std::println(std::cerr, "Erro: O componente {} não é um diretório.", it);
            return {base, this->get_current_path()};
        }
        auto entries = this->list_dir(inode);
        bool found = false;
        for (const auto &entry : entries) {
            if (entry.get_name() == it) {
                inode = this->get_inode(entry.get_inode());
                found = true;

                if (it == "..") {
                    size_t last_slash = final_path.find_last_of('/');
                    if (last_slash != std::string::npos && last_slash > 0) {
                        final_path = final_path.substr(0, last_slash);
                    } else {
                        final_path = "/";
                    }
                } else if (it != ".") {
                    if (final_path.back() != '/') final_path += "/";
                    final_path += entry.get_name();
                }
                break;
            }
        }

        if (!found) {
            std::println(std::cerr, "Erro: O componente {} não existe.", it);
            return {base, this->get_current_path()};
        }
    }

    return {inode, final_path};
}

std::vector<Wrappers::DirectoryEntry> Wrappers::Image::list_dir(const Wrappers::Inode &inode) {
    std::vector<Wrappers::DirectoryEntry> entries{};
    if(!inode.is_dir()) return entries;
    auto bytes = this->read_file(inode);
    size_t offset = 0;

    while (offset < inode.get_size()) {
        std::span<const std::byte> current_view(bytes.data() + offset, bytes.size() - offset);

        auto entry = Utils::copy<Raw::DirectoryEntry>(current_view);

        if (entry.rec_len == 0) break;

        if (entry.inode != 0) {
            std::string name(
                reinterpret_cast<const char*>(current_view.data() + sizeof(Raw::DirectoryEntry)),
                entry.name_len
            );
            entries.push_back({entry, name});
        }

        offset += entry.rec_len;
    }
    return entries;
}