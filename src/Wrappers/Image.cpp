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
#include <algorithm>
#include <tuple>

/**
 * @file    Image.cpp
 * @brief   Implementação da classe Image — acesso ao disco EXT4.
 * @author  Pedro Itiro Nagao
 * @date    2026-06-16
 *
 * Responsável por toda I/O sobre o arquivo de imagem: leitura e escrita
 * de blocos, inodes e descritores de grupo.
 */

using namespace Ext4;

Wrappers::Image::Image(const std::string &image_path) {
    // Tentamos abrir o arquivo de imagem em modo leitura e escrita, e em formato binário.
    this->image_file.open(image_path, std::ios::in | std::ios::out| std::ios::binary);
    
    if (!this->image_file.is_open()) {
        throw std::runtime_error(std::format("Erro ao abrir a imagem: O arquivo {} não existe ou está inacessível.", image_path));
    }

    // Tentamos colocar o cursor de leitura no offset fixo onde o superbloco deve residir; e
    // lemos o superbloco da imagem para validar a assinatura mágica e garantir que é um sistema de arquivos EXT4 válido.
    Raw::SuperBlock super_block{};
    this->read_offset(Constants::SUPERBLOCK_OFFSET, Utils::as_byte_span(super_block));

    // Armazenamos o superbloco lido para uso futuro.
    this->super_block = Wrappers::SuperBlock(super_block);
    this->super_block.validate();

    // Agora tentaremos ver onde está a GDT.
    uint64_t gdt_offset = this->super_block.get_gdt_offset();

    // Calculamos o número de grupos.
    // E aproveitamos para popular os bitmaps e tabelas de inodes, já que vamos precisar deles para ler os descritores de grupo.
    // O número de grupos de blocos varia e vem de acordo com a formatação do disco
    uint32_t group_count = this->super_block.get_group_count();
    // O grupo de descritores têm tamanho variável e é igual a capacidade de bits do disco.
    // - Em discos 32 bits, a estrutura vai até `bg_checksum` que usa CRC16 legado (feito para compatibilidade com o EXT3)
    // - Em discos 64 bits, a estrutura cria novos campos `_hi` para estender os limites dos campos `_lo` antigos e o checksum
    //   é os 16 bits inferiores de um CRC32c (polinomial de Castagnoli).
    uint16_t desc_size = this->super_block.get_desc_size();
    bool is_64 = this->super_block.is_64bit();
    // Agora lemos os grupos de descritores
    // Como eles são contíguos, seus IDS vêem de acordo com sua posição lida.
    for (uint32_t i = 0; i < group_count; i++) {
        // Lemos a struct (uso de {} garante que campos não lidos estejam zerados para evitar erros)
        Raw::GroupDescriptor gd{};
        std::streamoff offset = gdt_offset + (static_cast<uint64_t>(i) * desc_size);
        this->read_offset(offset, Utils::as_byte_span(gd, desc_size));
        Wrappers::GroupDescriptor gdt = Wrappers::GroupDescriptor(gd, i, desc_size, is_64);

        // Checagem de checksums
        Checksums::validate_checksum(
            gdt.get_checksum(), 
            Checksums::checksum_group(gdt, this->super_block.has_metadata_csum(), this->super_block.get_checksum_seed()),
            std::format("Grupo de descritores de ID {}", i)
        );

        // Agora (finalmente) colocamos o GD no nosso programa
        this->group_descriptors.push_back(gdt);
    }
    // Normalmente o diretório raiz / está no inode de ID 2, os primeiros 10 inodes são especiais para uso do sistema
    // Do ID 11 para frente temos diretórios e arquivos normais.
    this->current_inode = this->get_inode(2);
    this->root_inode = this->current_inode;
    std::println("Imagem lida com sucesso!\nBem-vindo ao EXT4shell!");
}

void Wrappers::Image::seek(const std::streamoff offset) {
    // Limpamos quaisquer erros anteriores
    this->image_file.clear();

    // Colocamos o cursor na posição desejada
    this->image_file.seekg(offset, std::ios::beg);
    this->image_file.seekp(offset, std::ios::beg);

    // Se deu erro
    if (!this->image_file) {
        throw std::runtime_error(std::format("Seek falhou no offset {}.", offset));
    }
}

void Wrappers::Image::read_offset(const std::streamoff offset, std::span<std::byte> buffer) {
    // Colocamos o cursor na posição (deslocamento) desejado
    this->seek(offset);
    
    // Executa a leitura binária convertendo o span de bytes para char*
    this->image_file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
    
    // Contamos os bytes lidos para checagem de erros
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
    // Primeiro, vemos se estamos lendo um bloco válido. Se não for, lançamos exceção.
    if (block_num < this->super_block.get_first_data_block() || block_num >= this->super_block.get_blocks_count()) {
        throw std::out_of_range(std::format("Erro: O bloco de ID {} está fora dos limites válidos do sistema de arquivos.", block_num));
    }
    try {
        Wrappers::GroupDescriptor gd = this->get_group_descriptor(this->get_block_group(block_num));
        uint32_t bit_pos = this->get_block_bit_pos(block_num);
        uint32_t bitmap_block = gd.get_block_bitmap_block();
        std::vector<std::byte> bitmap_buffer(this->super_block.get_block_size());
        auto bitmap_span = Utils::as_byte_span(bitmap_buffer);
        this->read_offset(bitmap_block * this->super_block.get_block_size(), bitmap_span);
        if (!Utils::test_bit(bitmap_span, bit_pos)) {
            throw std::out_of_range(std::format("O bloco de ID {} está livre e não pode ser lido.", block_num));
        } else if(gd.is_block_uninit()) {
            throw std::out_of_range(std::format("O bloco de ID {} pertence a um grupo de blocos não inicializado.", block_num));
        }
    } catch (const std::out_of_range&) {
        throw std::out_of_range(std::format("O bloco de ID {} pertence a um grupo de descritores não existente.", block_num));
    }

    // O deslocamento que se encontra o bloco vem do fato do EXT4 usar alocação contígua.
    // Então multiplicamos o número do bloco pelo tamanho de um bloco para saber sua posição no disco.
    std::streamoff offset = block_num * this->super_block.get_block_size();
    // Agora lemos
    this->read_offset(offset, buffer);
}

void Wrappers::Image::write_offset(const std::streamoff offset, const std::span<const std::byte> buffer) {
    // Colocamos o cursor na posição desejada e escrevemos
    this->seek(offset);
    this->image_file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

    // Checagem de (possíveis) erros
    if (!this->image_file) {
        this->image_file.clear();
        throw std::runtime_error(std::format(
            "Erro ao escrever na imagem: falha na escrita. (Offset: {})",
            offset
        ));
    }

    // Atualiza (sincroniza) o buffer do `fstream` para que trabalhemos sempre com a imagem atualizada.
    this->image_file.flush();
}

void Wrappers::Image::write_block(const uint64_t block_num, const std::span<const std::byte> buffer) {
    // O deslocamento que se encontra o bloco vem do fato do EXT4 usar alocação contígua.
    // Então multiplicamos o número do bloco pelo tamanho de um bloco para saber sua posição no disco.
    std::streamoff offset = block_num * this->super_block.get_block_size();
    // Agora escrevemos
    this->write_offset(offset, buffer);
}

uint32_t Ext4::Wrappers::Image::get_inode_group(const uint32_t ino) {
    // Os IDS dos inodes começam do ID 1, deslocamos para zero para evitar deslocamento problemático
    return (ino - 1) / this->super_block.get_inodes_per_group();
}

uint32_t Ext4::Wrappers::Image::get_inode_bit_pos(const uint32_t ino) {
    // Os IDS dos inodes começam do ID 1, deslocamos para zero para evitar deslocamento problemático.
    // O módulo é usado pois o tamanho do bitmap não é infinito e normalmente é o tamanho de um bloco.
    // Assim, blocos de 1024B, um inode 1025 estará no bit 2.
    return (ino - 1) % this->super_block.get_inodes_per_group();
}

uint32_t Ext4::Wrappers::Image::get_block_group(const uint32_t blk) {
    // Mesma coisa do inode mas agora para o grupo que o bloco pertence.
    // A posição do primeiro bloco de dados pode variar, então usamos um getter para evitar problemas de
    // compatibilidade entre sistemas de arquivos com blocos de tamanhos diferentes.
    return (blk - this->super_block.get_first_data_block()) / this->super_block.get_blocks_per_group();
}

uint32_t Ext4::Wrappers::Image::get_block_bit_pos(const uint32_t blk) {
    // Mesma coisa do inode mas agora para o grupo que o bloco pertence.
    // A posição do primeiro bloco de dados pode variar, então usamos um getter para evitar problemas de
    // compatibilidade entre sistemas de arquivos com blocos de tamanhos diferentes.
    return (blk - this->super_block.get_first_data_block()) % this->super_block.get_blocks_per_group();
}

std::streamoff Ext4::Wrappers::Image::get_inode_offset(const uint32_t inode_num) {
    // Validação preventiva: Inodes válidos no EXT4 começam obrigatoriamente no índice 1
    if (inode_num < 1 || inode_num > this->super_block.get_inodes_count()) {
        throw std::logic_error(std::format("Erro: Número de inode inválido ou fora dos limites: {}", inode_num));
    }

    // Descobrir a qual Block Group este inode pertence
    uint32_t group = this->get_inode_group(inode_num);

    // Descobrir o índice local do inode dentro da tabela daquele grupo
    uint32_t index = this->get_inode_bit_pos(inode_num);
    
    // Buscar o offset em bytes de onde começa a tabela de inodes do grupo correspondente
    uint64_t table_base_offset = this->group_descriptors[group].get_inode_table_block() * this->super_block.get_block_size();
    
    // Calcular a posição absoluta do inode alvo
    std::streamoff final_inode_offset = table_base_offset + (static_cast<std::streamoff>(index) * this->super_block.get_inode_size());
    
    return final_inode_offset;
}

Wrappers::Inode Wrappers::Image::get_inode(const uint32_t inode_num) {
    // Primeiro, validamos se o inode está livre ou não. Se estiver livre, lançamos exceção.
    try {
        Wrappers::GroupDescriptor gd = this->get_group_descriptor(this->get_inode_group(inode_num));
        uint32_t bit_pos = this->get_inode_bit_pos(inode_num);
        uint32_t bitmap_block = gd.get_inode_bitmap_block();
        std::vector<std::byte> bitmap_buffer(this->super_block.get_block_size());
        auto bitmap_span = Utils::as_byte_span(bitmap_buffer);
        this->read_block(bitmap_block, bitmap_span);
        if (!Utils::test_bit(bitmap_span, bit_pos)) {
            throw std::out_of_range(std::format("O inode de ID {} está livre e não pode ser lido.\n", inode_num));
        } else if(gd.is_inode_uninit()) {
            throw std::out_of_range(std::format("O inode de ID {} pertence a um grupo de blocos não inicializado.", inode_num));
        }
    } catch (const std::out_of_range&) {
        throw std::out_of_range(std::format("O inode de ID {} pertence a um grupo de descritores não existente.", inode_num));
    }

    // Alocar a struct e ler do disco
    // Normalmente, leríamos o tamanho indicado pelo superbloco.
    // PORÉM, os bytes excedentes da estrutura não têm semântica fixa e apenas mapeias funcionalidades opcionais
    // que não estão em nenhuma das imagens de teste (além de `ext_attr`, ao que parece)
    // Então só copiaremos os bytes excedentes para calculo do checksum
    std::vector<std::byte> buffer(this->super_block.get_inode_size());
    this->read_offset(this->get_inode_offset(inode_num), Utils::as_byte_span(buffer));
    // Lemos a struct
    Raw::Inode inode = Utils::copy<Raw::Inode>(buffer);

    // Agora lemos os byte extras.
    // As imagens não usam EXT4_INLINE, então os inodes não guardam nada relevante depois de 160 bytes.
    // Mas, creio que que há algo do Xattr ainda lá. Só ignoraremos por enquanto pois não usamos nesse projeto 
    //  (mas ainda copiamos eles para validar e refazer o cheksum).
    std::vector<std::byte> excess_bytes;
    // Lemos os byte extras e colocamos no wrapper
    excess_bytes.append_range(Utils::as_span<std::byte>(buffer, sizeof(Raw::Inode)));
    Wrappers::Inode wrapper = Wrappers::Inode(inode_num, this->get_volume_uuid(), this->super_block.get_inode_size(), inode, excess_bytes);

    // Checagem de checksums
    if(this->super_block.has_metadata_csum()) {
        uint32_t calculated = Checksums::checksum_inode(wrapper, this->super_block.get_checksum_seed());
        uint32_t stored = wrapper.get_checksum();

        bool has_checksum_hi = wrapper.has_checksum_hi();

        uint32_t expected = has_checksum_hi ? calculated : (calculated & 0xFFFF);

        Checksums::validate_checksum(
            stored,
            expected,
            std::format("Inode de ID {}", wrapper.get_inode_id())
        );
    }
    return wrapper;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_leafs(std::span<const std::byte> node_data, const Raw::ExtentHeader &header) {
    // Quando o sistema usa extents, pode ocorrer duas coisas (sendo uma):
    // - O ExtentHeader sinalizar que os próximos bytes são ExtentLeafs (então só lemos os dados)
    std::vector<uint64_t> blocks;
    // Usamos `std::span` aqui para ler após os 12 bytes do ExtentHeader (melhor que manipulação de ponteiros eu acho)
    std::span<const Raw::ExtentLeaf> leafs = Utils::as_span<const Raw::ExtentLeaf>(
        node_data, 
        sizeof(Raw::ExtentHeader), 
        header.eh_entries * sizeof(Raw::ExtentLeaf)
    );
    // Agora lemos os IDS dos blocos que as folhas apontam
    for (const auto &leaf : leafs) {
        uint64_t start_block = leaf.get_start_block();
        uint16_t length = leaf.get_real_length();
        for (uint16_t i = 0; i < length; ++i) {
            blocks.push_back(start_block + i);
        }
    }
    return blocks;
}

std::vector<uint64_t> Wrappers::Image::read_blocks_from_index(const Wrappers::Inode &inode, 
    std::span<const std::byte> node_data, 
    const Raw::ExtentHeader &header) {
    
    // Vemos se o número mágico é válido
    if (header.eh_magic != Constants::EXTENT_MAGIC)
        throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico de extents inválido.");

    // Quando o sistema usa extents, pode ocorrer duas coisas (sendo uma):
    // - O ExtentHeader sinalizar que os próximos bytes são ExtentIndex. 
    //     Então devemos ir até o bloco que o ExtentIndex aponta e fazer o processo recursivamente até achar um ExtentHeader onde eh_depth == 0
    //     e os próximos bytes forem folhas que agora finalmente apontam para dados.
    std::vector<uint64_t> blocks{};
    std::span<const Raw::ExtentIndex> index_entries = Utils::as_span<const Raw::ExtentIndex>(
        node_data, 
        sizeof(Raw::ExtentHeader), 
        header.eh_entries * sizeof(Raw::ExtentIndex)
    );
    
    // Criamos um vetor alocado com o tamanho de um bloco para otimizar alocação.
    std::vector<std::byte> buffer(this->super_block.get_block_size());
    std::span<std::byte> buffer_bytes = Utils::as_byte_span(buffer);
    for (const auto &index : index_entries) {
        uint64_t child_block = index.get_leaf_block();
        
        // Lemos o bloco para o nosso buffer local
        this->read_block(child_block, buffer_bytes);
        Raw::ExtentHeader child_header = Utils::copy<Raw::ExtentHeader>(buffer);
        
        // Checagem de número mágico
        if (child_header.eh_magic != Constants::EXTENT_MAGIC) {
            throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico inválido no nó filho.");
        }
        
        // Checagem de checksums
        if (this->super_block.has_metadata_csum()) {
            // Lemos os últimos 4 bytes do bloco para colocar em `ExtentTail` que contém o checksum de metadados
            Raw::ExtentTail tail = Utils::copy<Raw::ExtentTail>(buffer_bytes, buffer.size() - sizeof(Raw::ExtentTail));
            Checksums::validate_checksum(
                tail.eb_checksum,
                Checksums::checksum_extent(inode, buffer_bytes, buffer.size(), this->super_block.get_checksum_seed()),
                std::format("Extent de Inode {}", inode.get_inode_id())
            );
        }
        // Aqui vemos se fazemos o processo recursivo ou não
        if (child_header.eh_depth == 0) {
            // Passa por valor. read_blocks_from_leafs processa e joga os dados no vector 'child_blocks'
            auto child_blocks = this->read_blocks_from_leafs(buffer_bytes, child_header);
            blocks.insert(blocks.end(), child_blocks.begin(), child_blocks.end());
        } else {
            // Chamada recursiva segura: child_span é válido enquanto o pai executa esta linha
            auto child_blocks = this->read_blocks_from_index(inode, buffer_bytes, child_header);
            blocks.insert(blocks.end(), child_blocks.begin(), child_blocks.end());
        }
    }


    return blocks; // Retorno por valor (Move semantics do C++ garante que é eficiente e seguro)
}

std::vector<uint64_t> Wrappers::Image::get_blocks(const Wrappers::Inode &inode) {
    std::vector<uint64_t> data_blocks;
    // Aqui é o array com dados do inode.
    // Consideramos que o sistema de arquivos sempre usa extents. Então teremos a árvore de extents aqui
    std::array<std::byte, 60> i_blocks = inode.get_i_block();
    // Copiamos só o cabeçalho (`ExtentIndex`) para ver como devemos interpretar os próximos dados
    Raw::ExtentHeader header = Utils::copy<Raw::ExtentHeader>(i_blocks);
    // Checagem de segurança
    if (header.eh_magic != Constants::EXTENT_MAGIC)
        throw std::runtime_error("Erro ao ler os blocos do Inode: Número mágico de extents inválido. O Inode pode estar corrompido ou não utilizar extents.");

    // Criamos um `std::span` para evitar cópia na (possível) recursão
    std::span<std::byte> blocks_span = Utils::as_byte_span(i_blocks);

    if (header.eh_depth == 0) {
        // Nó folha contendo dados reais. As entradas de blocos estão diretamente após o cabeçalho.
        data_blocks = this->read_blocks_from_leafs(blocks_span, header);
    } else {
        // Nó interno de indexação. As entradas de blocos são ponteiros para outros nós de extents.
        data_blocks = this->read_blocks_from_index(inode, blocks_span, header);
    }
    return data_blocks;
}

std::vector<std::byte> Wrappers::Image::read_file(const Wrappers::Inode &inode) {
    std::vector<std::byte> file_data;
    // Pegamos os blocos dos dados do inode para montar os bytes do ficheiro
    std::vector<uint64_t> data_blocks = this->get_blocks(inode);
    for (const auto &block_num : data_blocks) {
        // Lemos cada bloco e colocamos no nosso vetor de bytes
        std::vector<std::byte> buffer(this->super_block.get_block_size());
        this->read_block(block_num, Utils::as_byte_span(buffer));
        file_data.insert(file_data.end(), buffer.begin(), buffer.end());
    }
    // Diretórios precisam manter o bloco inteiro (tail de checksum fica no final)
    if (!inode.is_dir()) file_data.resize(inode.get_size());
    return file_data;
}


std::pair<Wrappers::Inode, std::string> Wrappers::Image::resolve_path(const std::string &path, const Wrappers::Inode &base) {
    // Evitar burradas do usuário
    if (path.empty() || path == "." || this->get_current_path().ends_with(path)) return {base, this->get_current_path()};
    if (path == "/") return {this->get_root_inode(), "/"};

    bool is_root = (path[0] == '/');

    // Se o path começar com /, então devemos começar a montagem do diretório do inode da raíz
    Wrappers::Inode inode = is_root ? this->get_root_inode() : base;
    std::vector<std::string> paths = Utils::filter_split(path, "/");
    
    // Começamos com o caminho base atual
    std::string final_path = is_root ? "/" : this->get_current_path();

    // Percorremos cada diretório para montar o caminho.
    for (const auto &it : paths) {
        // Será que temos um inode válido?
        if (inode.get_type() != Flags::S_IFDIR) {
            // Não um inode válido aqui
            return std::pair{base, this->get_current_path()};
        }
        // Agora sabemos que temos um inode que representa um diretório...
        auto entries = this->list_dir(inode);
        bool found = false;
        // Agora vemos se o inode contém uma parte do caminho.
        for (const auto &entry : entries) {
            if (entry.get_name() == it) {
                // Achamos!
                inode = this->get_inode(entry.get_inode());
                found = true;

                // Caso queremos que volte para o pai.
                if (it == "..") {
                    size_t last_slash = final_path.find_last_of('/');
                    // Voltamos para o diretório anterior (vinculado ao ..)
                    if (last_slash != std::string::npos && last_slash > 0) {
                        final_path = final_path.substr(0, last_slash);

                    // std::string::npos indica que não achamos um diretório válido. Damos um fallback para o root.
                    } else {
                        final_path = "/";
                    }
                // Pode ocorrer? Pode ocorrer, mas não entendo como alguém vai fazer '/root/././././pasta'
                } else if (it != ".") {
                    if (final_path.back() != '/') final_path += "/";
                    final_path += entry.get_name();
                }
                break;
            }
        }

        if(!found) {
            // Não achamos um inode que corresponde ao diretório
            return {base, this->get_current_path()};
        }
    }

    return {inode, final_path};
}

std::vector<Wrappers::DirectoryEntry> Wrappers::Image::list_dir(const Wrappers::Inode &inode) {
    std::vector<Wrappers::DirectoryEntry> entries{};
    // Só diretórios têm entries
    if (!inode.is_dir()) return entries;
    // Pegamos os dados do inode
    std::vector<std::byte> bytes = this->read_file(inode);
    size_t offset = 0;

    // Checagem de checkums
    // Aqui temos problemas
    // Muito por compatibilidade, quando usamos diretórios com checksum de metadados, 
    //  os últimos 12 bytes são "ocultos" do sistemas de arquivos e indicam uma nova estrutura `DirrectoryEntryTail` que:
    // - Têm 4 bytes que são zero (para o sistema de arquivos achar que não tem algo lá)
    // - Têm 2 bytes de rec_len (sempre 12)
    // - Têm 1 byte que é zero
    // - Têm 1 bytes que indicam o tipo de arquivo (0xDE: EXT4_FT_DIR_CSUM)
    // - Têm 4 bytes que formam o checksum
    // Devemos ler essa estrutura com cuidado para evitar de ler dados corruptos.
    if (this->super_block.has_metadata_csum()) {
        uint32_t block_size = this->super_block.get_block_size();
        Raw::DirectoryEntryTail tail = Utils::copy<Raw::DirectoryEntryTail>(Utils::as_byte_span(bytes, block_size - sizeof(Raw::DirectoryEntryTail)));
        // Vemos se a saída é realmente a cauda de checksum
        if (
            (tail.det_reserved_zero1 + tail.det_reserved_zero2) == 0 &&
            tail.det_rec_len == 12 && 
            tail.det_reserved_ft == Raw::DirectoryFileType::EXT4_FT_DIR_CSUM) {
            Checksums::validate_checksum(
                tail.det_checksum,
                Checksums::checksum_dir(inode, Utils::as_byte_span(bytes), block_size, this->super_block.get_checksum_seed()),
                std::format("Entrada de diretório de Inode {}", inode.get_inode_id())
            );
        } else {
            // Provavelmente o disco está corrompido.
            throw std::runtime_error(
                "A flag de checksum de metadados está ativada, mas a cauda da lista de diretórios não é válida...\n"
            );
        }
    }

    // A leitura das entradas do diretório não é linearmente constante.
    // Pois o nome da entrada pode variar e é necessário que não haja padding entre entradas.
    // Então lemos a estrutura crua primeiro e depois (com a ajuda de seus dados) lemos a string que representa seu nome.
    while (offset < inode.get_size()) {
        std::span<const std::byte> current_view(bytes.data() + offset, bytes.size() - offset);
        // Lemos a estrutura crua da entrada atual
        Raw::DirectoryEntry entry = Utils::copy<Raw::DirectoryEntry>(current_view);
        // rec_len é o tamanho da estrutura + tamanho do nome (então deve ser maior que 0)
        if (entry.rec_len == 0) break;

        // Vemos se chegamos em uma entrada válida
        if (entry.inode != 0) {
            // Lemos o nome, botamos num `std::string` e colocamos no nosso vetor resultante
            std::span<const std::byte> name_span = current_view.subspan(sizeof(Raw::DirectoryEntry), entry.name_len);
            entries.push_back({entry, std::string{
                reinterpret_cast<const char*>(name_span.data()), 
                name_span.size()
            }});
        }
        // Pulamos de rec_len em rec_len
        offset += entry.rec_len;
    }
    return entries;
}

void Ext4::Wrappers::Image::write_inode(const Wrappers::Inode &inode) {
    std::streamoff offset = this->get_inode_offset(inode.get_inode_id());
    Raw::Inode new_raw = inode.get_raw();
    
    // Se temos cálculo de checksums
    if (this->super_block.has_metadata_csum()) {
        uint32_t check = Checksums::checksum_inode(inode, this->super_block.get_checksum_seed());
        Utils::split(check, new_raw.i_osd2.l_i_checksum_lo, new_raw.i_checksum_hi);
    }

    // Monta o buffer completo: struct (160 bytes) + excess_bytes (resto até inode_size)
    std::vector<std::byte> buffer(inode.get_struct_size(), std::byte{0});
    std::span<std::byte> buffer_bytes = Utils::as_byte_span(buffer);
    Utils::write_to(buffer_bytes, new_raw);
    
    // Caso temos bytes de excesso que não são lidos
    if (std::vector<std::byte> excess = inode.get_excess_bytes(); !excess.empty()) {
        std::memcpy(buffer.data() + sizeof(new_raw), excess.data(), excess.size());
    }

    this->write_offset(offset, Utils::as_byte_span(buffer));
}

void Ext4::Wrappers::Image::write_gdt(const Wrappers::GroupDescriptor &gd) {
    uint16_t desc_size = this->super_block.get_desc_size();
    std::streamoff offset = this->super_block.get_gdt_offset() + (static_cast<uint64_t>(gd.get_group_number()) * desc_size);
    
    // Cálculo do checksum para gravação
    // Calculamos o novo checksum e colocamos na struct para escrita
    Wrappers::GroupDescriptor copy = gd;
    Raw::GroupDescriptor new_raw = copy.get_raw();
    new_raw.bg_checksum = Checksums::checksum_group(copy, this->super_block.has_metadata_csum(), this->super_block.get_checksum_seed());
    copy.set_raw(new_raw);
    // Agora escrevemos e atualizamos nosso vetor de GDs
    this->write_offset(offset, Utils::as_byte_span(new_raw, desc_size));
    this->group_descriptors[copy.get_group_number()] = copy;
}

void Ext4::Wrappers::Image::write_superblock(const Wrappers::SuperBlock &sb) {
    // Cálculo de checksums
    if(sb.has_metadata_csum()) {
        // Calculamos o novo checksum e colocamos na struct para escrita
        Raw::SuperBlock sb_raw = sb.get_raw();
        sb_raw.s_checksum = Checksums::checksum_super_block(sb);
        Wrappers::SuperBlock new_sp = Wrappers::SuperBlock(sb_raw);
        this->super_block = new_sp;
        this->write_offset(Constants::SUPERBLOCK_OFFSET, Utils::as_byte_span(sb_raw));
        return;
    }
    // Código que ocorre caso a flag de checksum de metadados esteja desativada
    this->super_block = sb;
    this->write_offset(Constants::SUPERBLOCK_OFFSET, Utils::as_byte_span(sb.get_raw()));
}

uint64_t Ext4::Wrappers::Image::get_absolute_block_offset(const Wrappers::Inode &inode, const uint32_t relative_offset) {
    // Traduz o offset lógico do arquivo para o offset absoluto em bytes do disco físico
    size_t block_size = this->super_block.get_block_size();
    size_t block_index = relative_offset / block_size;
    size_t offset_within_block = relative_offset % block_size;

    // Busca o bloco físico correspondente
    uint64_t phys_block_num = this->get_blocks(inode)[block_index];
    return (phys_block_num * block_size) + offset_within_block;
}

uint32_t Ext4::Wrappers::Image::alloc_inode(const bool is_dir) {
    bool stop = false;
    std::vector<Wrappers::GroupDescriptor> &gds = this->group_descriptors;

    size_t gd_id = 0, bit_pos = 0;

    // Possível novo checksum do bitmap
    uint32_t new_bitmap_csum = 0;
    // Procuramos pelo primeiro grupo de descritores que contém 1 bit desativado em seu bitmap
    for (size_t i = 0; i < gds.size(); i++) {
        if (stop) break;
        if (gds[i].is_inode_uninit()) {
            // O bitmap de inodes deste grupo não está inicializado no disco.
            // Pule para evitar erro de checksum.
            continue; 
        }

        std::vector<std::byte> inode_bitmap(this->super_block.get_block_size());
        std::span<std::byte> full_span = Utils::as_byte_span(inode_bitmap);
        // Nem sempre o bloco inteiro é de bitmaps, então dividimos por `inode_per_group` para descobrir até onde vão os bits válidos
        auto bitmap_span = full_span.subspan(0, this->super_block.get_inodes_per_group() / 8);

        if (gds[i].is_inode_uninit()) {
            // Bitmap "virtual": todos os inodes deste grupo estão livres.
            // Não lemos do disco (pode ser lixo) nem validamos checksum ainda.
            std::fill(bitmap_span.begin(), bitmap_span.end(), std::byte{0});
        } else {
            this->read_block(gds[i].get_inode_bitmap_block(), full_span);
            if (this->super_block.has_metadata_csum()) {
                Checksums::validate_checksum(
                    gds[i].get_inode_bitmap_checksum(),
                    Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed()),
                    std::format("Checksum para o bitmap de inodes do GDT {}", i)
                );
            }
        }
        // Iteramos BIT-A-BIT para achar um bit zerado
        for (size_t j = 0; j < bitmap_span.size() * 8; ++j) {
            if (!Utils::test_bit(bitmap_span, j)) {
                // Achamos um bit!
                // Ativamos ele e escrevemos o novo bitmap
                Utils::set_bit(bitmap_span, j, 1);

                gd_id = i;
                bit_pos = j;
                this->write_block(gds[gd_id].get_inode_bitmap_block(), full_span);

                // Criamos um novo checksum do bitmap
                if (this->super_block.has_metadata_csum())
                    new_bitmap_csum = Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed());

                stop = true;
                break;
            }
        }
    }

    if(!stop) return 0; // Sem espaço: nenhum inode livre disponível.

    // Atualizamos as estruturas relacionadas (decrementamos a quantidade de inodes livres)
    Wrappers::GroupDescriptor to_change = gds[gd_id];
    Raw::GroupDescriptor new_raw = to_change.get_raw();
    if(this->super_block.has_metadata_csum())
        Utils::split(new_bitmap_csum, new_raw.bg_inode_bitmap_csum_lo, new_raw.bg_inode_bitmap_csum_hi);
    
    uint32_t new_inode_count = to_change.get_free_inodes_count() - 1;
    Utils::split(new_inode_count, new_raw.bg_free_inodes_count_lo, new_raw.bg_free_inodes_count_hi);
    
    // Se pegamos um inode no final da tabela, atualizamos o itable
    if(uint32_t unused = to_change.get_itable_unused(); bit_pos >= this->super_block.get_inodes_per_group() - unused) {
        uint32_t new_itable_unused = to_change.get_itable_unused() - 1;
        Utils::split(new_itable_unused, new_raw.bg_itable_unused_lo, new_raw.bg_itable_unused_hi);
    }

    // Caso seja um diretório, temos que incrementar o contador de diretórios do GDT
    if(is_dir) {
        uint32_t used_dirs_count = to_change.get_used_dirs_count() + 1;
        Utils::split(used_dirs_count, new_raw.bg_used_dirs_count_lo, new_raw.bg_used_dirs_count_hi);
    }

    if(to_change.is_inode_uninit()) {
        new_raw.bg_flags &= ~Ext4::Flags::BG_INODE_UNINIT; // limpa o bit da flag
    }

    to_change.set_raw(new_raw);
    this->write_gdt(to_change);

    Raw::SuperBlock sb_raw = this->super_block.get_raw();
    sb_raw.s_free_inodes_count--;
    this->write_superblock(Wrappers::SuperBlock(sb_raw));

    return gd_id * this->super_block.get_inodes_per_group() + bit_pos + 1;
}

uint32_t Ext4::Wrappers::Image::alloc_block() {
    bool stop = false;
    std::vector<Wrappers::GroupDescriptor> &gds = this->group_descriptors;

    uint32_t gd_id = 0, bit_pos = 0;

    // Possível novo checksum de bitmap
    uint32_t new_bitmap_csum = 0;

    // Procuramos pelo primeiro grupo de descritores que contém 1 bit desativado em seu bitmap
    for (size_t i = 0; i < gds.size(); i++) {
        if(stop) break;
        if(gds[i].is_block_uninit()) {
            // O bitmap de blocos deste grupo não está inicializado no disco.
            // Pulamos para evitar erro de checksum.
            continue; 
        }

        std::vector<std::byte> block_bitmap(this->super_block.get_block_size());
        auto full_span = Utils::as_byte_span(block_bitmap);
        this->read_block(gds[i].get_block_bitmap_block(), full_span);
        // Nem sempre o bloco inteiro é de bitmaps, 
        // então dividimos por `blocks_per_group` para descobrir até onde vão os bits válidos
        auto bitmap_span = full_span.subspan(0, this->super_block.get_blocks_per_group() / 8);

        // Checagem de checksums
        if (this->super_block.has_metadata_csum()) {
            Checksums::validate_checksum(
                gds[i].get_block_bitmap_checksum(),
                Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed()),
                std::format("Checksum para o bitmap de blocos do GDT {}", i)
            );
        }

        for (size_t j = 0; j < bitmap_span.size() * 8; ++j) {
            if (!Utils::test_bit(bitmap_span, j)) {
                // Achamos um bit!
                // Ativamos ele e escrevemos o novo bitmap
                Utils::set_bit(bitmap_span, j, 1);

                gd_id = i;
                bit_pos = j;
                this->write_block(gds[gd_id].get_block_bitmap_block(), full_span);

                // Criamos um novo checksum
                if (this->super_block.has_metadata_csum())
                    new_bitmap_csum = Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed());

                stop = true;
                break;
            }
        }
    }

    if(!stop) return 0; // Sem espaço: nenhum bloco livre disponível.

    // Atualizamos as estruturas relacionadas (decrementamos a quantidade de blocos livres)
    Wrappers::GroupDescriptor to_change = gds[gd_id];
    Raw::GroupDescriptor new_raw = to_change.get_raw();
    if(this->super_block.has_metadata_csum())
        Utils::split(new_bitmap_csum, new_raw.bg_block_bitmap_csum_lo, new_raw.bg_block_bitmap_csum_hi);

    uint32_t new_block_count = to_change.get_free_blocks_count() - 1;
    Utils::split(new_block_count, new_raw.bg_free_blocks_count_lo, new_raw.bg_free_blocks_count_hi);

    to_change.set_raw(new_raw);
    this->write_gdt(to_change);

    Raw::SuperBlock sb_raw = this->super_block.get_raw();
    uint64_t global_free_blocks = Utils::concatenate(sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi) - 1;
    Utils::split(global_free_blocks, sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi);
    this->write_superblock(Wrappers::SuperBlock(sb_raw));

    uint32_t first_data_block = this->super_block.get_first_data_block();
    return (gd_id * this->super_block.get_blocks_per_group()) + bit_pos + first_data_block;
}

std::pair<uint32_t, uint32_t> Ext4::Wrappers::Image::alloc_contiguous_blocks(const uint32_t amount) {
    std::vector<Wrappers::GroupDescriptor> &gds = this->group_descriptors;
    
    // Variáveis para rastrear o melhor lugar encontrado globalmente
    int best_gd_id = -1;
    size_t global_best_start = 0;
    uint32_t global_best_len = 0;

    // Busca o melhor grupo
    for (size_t i = 0; i < gds.size(); i++) {
        if (gds[i].is_block_uninit()) {
            // O bitmap de blocos deste grupo não está inicializado no disco.
            // Pulamos para evitar erro de checksum.
            continue; 
        }
        std::vector<std::byte> block_bitmap(this->super_block.get_block_size());
        auto full_span = Utils::as_byte_span(block_bitmap);
        this->read_block(gds[i].get_block_bitmap_block(), full_span);
        auto bitmap_span = full_span.subspan(0, this->super_block.get_blocks_per_group() / 8);

        // Checagem de checksums do bitmap que lemos
        if (this->super_block.has_metadata_csum()) {
            Checksums::validate_checksum(
                gds[i].get_block_bitmap_checksum(),
                Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed()),
                std::format("Checksum para o bitmap de blocos do GDT {}", i)
            );
        }

        size_t total_bits = bitmap_span.size() * 8;
        size_t best_start = 0, best_len = 0;  // melhor sequência encontrada neste grupo
        size_t run_start = 0, run_len = 0;    // sequência atual sendo contada

        for (size_t j = 0; j < total_bits; ++j) {
            if (!Utils::test_bit(bitmap_span, j)) {
                if (run_len == 0) run_start = j;
                run_len++;

                if (run_len >= amount) {
                    best_start = run_start;
                    best_len = amount;
                    break; // Achou o ideal neste grupo!
                }
                if (run_len > best_len) {
                    best_start = run_start;
                    best_len = run_len;
                }
            } else {
                run_len = 0;
            }
        }

        // Compara com o melhor global
        if (best_len > global_best_len) {
            global_best_len = best_len;
            global_best_start = best_start;
            best_gd_id = i;

            // Se achou exatamente a quantidade que pediu, não precisa testar os próximos grupos
            if (global_best_len == amount) {
                break;
            }
        }
    }

    if (global_best_len == 0 || best_gd_id == -1) {
        throw std::runtime_error("Sem espaço: nenhum bloco livre disponível em toda a imagem.");
    }

    // Aloca de fato no melhor grupo (best_gd_id)
    std::vector<std::byte> block_bitmap(this->super_block.get_block_size());
    auto full_span = Utils::as_byte_span(block_bitmap);
    this->read_block(gds[best_gd_id].get_block_bitmap_block(), full_span);
    auto bitmap_span = full_span.subspan(0, this->super_block.get_blocks_per_group() / 8);

    for (size_t k = global_best_start; k < global_best_start + global_best_len; ++k) {
        Utils::set_bit(bitmap_span, k, 1);
    }

    this->write_block(gds[best_gd_id].get_block_bitmap_block(), full_span);

    // Atualizando metadados e checksums
    Wrappers::GroupDescriptor to_change = gds[best_gd_id];
    Raw::GroupDescriptor new_raw = to_change.get_raw();

    if (this->super_block.has_metadata_csum()) {
        uint32_t new_bitmap_csum = Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed());
        Utils::split(new_bitmap_csum, new_raw.bg_block_bitmap_csum_lo, new_raw.bg_block_bitmap_csum_hi);
    }

    uint32_t new_block_count = to_change.get_free_blocks_count() - global_best_len;
    Utils::split(new_block_count, new_raw.bg_free_blocks_count_lo, new_raw.bg_free_blocks_count_hi);
    to_change.set_raw(new_raw);
    this->write_gdt(to_change);

    Raw::SuperBlock sb_raw = this->super_block.get_raw();
    uint64_t global_free_blocks = Utils::concatenate(sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi) - global_best_len;
    Utils::split(global_free_blocks, sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi);

    this->write_superblock(Wrappers::SuperBlock(sb_raw));

    // Cálculo do bloco lógico inicial
    uint32_t first_data_block = this->super_block.get_first_data_block();
    uint32_t first_block = (best_gd_id * this->super_block.get_blocks_per_group()) + global_best_start + first_data_block;

    return {global_best_len, first_block};
}

void Ext4::Wrappers::Image::free_inode(const uint32_t ino, const bool is_dir) {
    // Procuramos o grupo de descritores deste inode e seu bit no bitmap
    uint32_t group = this->get_inode_group(ino);
    uint32_t bit_pos = this->get_inode_bit_pos(ino);
    // Possível novo checksum de bitmap
    uint32_t new_bitmap_csum = 0;

    Wrappers::GroupDescriptor &gd = this->group_descriptors[group];
    std::vector<std::byte> inode_bitmap(this->super_block.get_block_size());
    auto full_span = Utils::as_byte_span(inode_bitmap);
    this->read_block(gd.get_inode_bitmap_block(), full_span);

    auto bitmap_span = full_span.subspan(0, this->super_block.get_inodes_per_group() / 8);
    // Checagem de checksums
    if (this->super_block.has_metadata_csum()) {
        Checksums::validate_checksum(
            gd.get_inode_bitmap_checksum(),
            Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed()),
            std::format("Checksum para o bitmap de inodes do GDT {}", gd.get_group_number())
        );
    }

    // Se está ativado, desativamos ele e calculamos um novo checksum
    if (Utils::test_bit(bitmap_span, bit_pos)) {
        Utils::set_bit(bitmap_span, bit_pos, 0);
        this->write_block(gd.get_inode_bitmap_block(), full_span);

        if (this->super_block.has_metadata_csum()) 
            new_bitmap_csum = Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed());
    } else return;

    // Atualizando estruturas relacionadas
    Wrappers::GroupDescriptor to_change = gd;
    Raw::GroupDescriptor new_raw = to_change.get_raw();
    if (this->super_block.has_metadata_csum())
        Utils::split(new_bitmap_csum, new_raw.bg_inode_bitmap_csum_lo, new_raw.bg_inode_bitmap_csum_hi);
    
    uint32_t new_inodes_count = to_change.get_free_inodes_count() + 1;
    Utils::split(new_inodes_count, new_raw.bg_free_inodes_count_lo, new_raw.bg_free_inodes_count_hi);

    // Se liberamos um inode no final da tabela, atualizamos o itable
    if(uint32_t unused = to_change.get_itable_unused(); bit_pos + 1== this->super_block.get_inodes_per_group() - unused) {
        uint32_t new_itable_unused = to_change.get_itable_unused() + 1;
        Utils::split(new_itable_unused, new_raw.bg_itable_unused_lo, new_raw.bg_itable_unused_hi);
    }

    // Caso seja um diretório, temos que decrementar o contador de diretórios do GDT
    if (is_dir) {
        uint32_t used_dirs_count = to_change.get_used_dirs_count() - 1;
        Utils::split(used_dirs_count, new_raw.bg_used_dirs_count_lo, new_raw.bg_used_dirs_count_hi);
    }
    to_change.set_raw(new_raw);
    this->write_gdt(to_change);

    Raw::SuperBlock sb_raw = this->super_block.get_raw();
    sb_raw.s_free_inodes_count++;
    this->write_superblock(Wrappers::SuperBlock(sb_raw));
}

void Ext4::Wrappers::Image::free_block(const uint32_t blk) {
    // Procuramos o grupo de descritores deste bloco e seu bit no bitmap
    uint32_t group = this->get_block_group(blk);
    uint32_t bit_pos = this->get_block_bit_pos(blk);
    // Possível novo checksum de bitmap
    uint32_t new_bitmap_csum = 0;

    Wrappers::GroupDescriptor &gd = this->group_descriptors[group];
    std::vector<std::byte> block_bitmap(this->super_block.get_block_size());
    auto full_span = Utils::as_byte_span(block_bitmap);
    this->read_block(gd.get_block_bitmap_block(), full_span);
    auto bitmap_span = full_span.subspan(0, this->super_block.get_blocks_per_group() / 8);
    // Checagem de checksums
    if (this->super_block.has_metadata_csum()) {
        Checksums::validate_checksum(
            gd.get_block_bitmap_checksum(),
            Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed()),
            std::format("Checksum para o bitmap de blocos do GDT {}", gd.get_group_number())
        );
    }

    // Se está ativado, desativamos ele e calculamos um novo checksum
    if (Utils::test_bit(bitmap_span, bit_pos)) {
        Utils::set_bit(bitmap_span, bit_pos, 0);
        this->write_block(gd.get_block_bitmap_block(), full_span);

        if (this->super_block.has_metadata_csum()) 
            new_bitmap_csum = Checksums::checksum_bitmap(bitmap_span, this->super_block.get_checksum_seed());
    } else return;

    // Atualizando estruturas relacionadas
    Wrappers::GroupDescriptor to_change = gd;
    Raw::GroupDescriptor new_raw = to_change.get_raw();
    if (this->super_block.has_metadata_csum())
        Utils::split(new_bitmap_csum, new_raw.bg_block_bitmap_csum_lo, new_raw.bg_block_bitmap_csum_hi);

    uint32_t new_block_count = to_change.get_free_blocks_count() + 1;
    Utils::split(new_block_count, new_raw.bg_free_blocks_count_lo, new_raw.bg_free_blocks_count_hi);

    to_change.set_raw(new_raw);
    this->write_gdt(to_change);

    Raw::SuperBlock sb_raw = this->super_block.get_raw();
    uint64_t global_free_blocks = Utils::concatenate(sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi);
    global_free_blocks++;
    Utils::split(global_free_blocks, sb_raw.s_free_blocks_count_lo, sb_raw.s_free_blocks_count_hi);
    this->write_superblock(Wrappers::SuperBlock(sb_raw));
}

uint64_t Ext4::Wrappers::Image::entry_logical_offset(std::span<const Wrappers::DirectoryEntry> entries, size_t idx) const {
    // Acumulamos os `rec_len` das entradas anteriores até chegar no alvo
    return std::ranges::fold_left(
        entries | std::views::take(idx)
                | std::views::transform([](const auto &e) {
                      return static_cast<uint64_t>(e.get_raw().rec_len);
                  }),
        0, std::plus<uint64_t>{}
    );
}

void Ext4::Wrappers::Image::dir_add_entry(const Wrappers::Inode &dir_inode, uint32_t target_ino, const std::string &name, const uint8_t file_type) {
    if (dir_inode.get_inode_id() < 1 || target_ino < 1 || name.empty()) return;
  
    std::vector<Wrappers::DirectoryEntry> entries = this->list_dir(dir_inode);
    if(entries.empty()) return;

    // Descobrir em qual bloco físico a última entrada reside
    uint64_t last_entry_logical_offset = this->entry_logical_offset(Utils::as_span<Wrappers::DirectoryEntry>(entries), entries.size() - 1);
    uint32_t block_size = this->super_block.get_block_size();
    
    uint64_t last_entry_phys_offset = this->get_absolute_block_offset(dir_inode, last_entry_logical_offset);
    uint64_t block_number = (last_entry_phys_offset / block_size);

    // Ler o bloco COMPLETO para a memória antes de modificar
    std::vector<std::byte> block_buffer(block_size);
    std::span<std::byte> buffer_bytes = Utils::as_byte_span(block_buffer);
    this->read_block(block_number, buffer_bytes);

    // Encontrar o offset local (dentro do buffer de 1K) da última entrada
    size_t local_last_entry_offset = last_entry_phys_offset % block_size;

    Raw::DirectoryEntry last_raw = Utils::copy<Raw::DirectoryEntry>(buffer_bytes.subspan(local_last_entry_offset));
    uint16_t old_total_rec_len = last_raw.rec_len;

    // O rec_len encolhido precisa ser alinhado a 4 bytes
    last_raw.rec_len = Utils::to_4bit_aligned<uint16_t>(sizeof(Raw::DirectoryEntry) + last_raw.name_len);

    // Onde a nova entrada vai começar dentro do buffer
    size_t local_new_entry_offset = local_last_entry_offset + last_raw.rec_len;
    
    // Configura a nova entrada no espaço restante
    Raw::DirectoryEntry new_raw{
        .inode = target_ino,
        .rec_len = static_cast<uint16_t>(old_total_rec_len - last_raw.rec_len),
        .name_len = static_cast<uint8_t>(name.size()),
        .file_type = file_type,
    };

    // Copia o nome para logo após a struct da nova entrada
    Utils::write_to(buffer_bytes, last_raw, local_last_entry_offset);
    Utils::write_to(buffer_bytes, new_raw, local_new_entry_offset);
    Utils::write_to(buffer_bytes, name, local_new_entry_offset + sizeof(Raw::DirectoryEntry));

    // Atualizar o tail no buffer antes de assinar
    if (this->super_block.has_metadata_csum()) {
        size_t local_tail_offset = block_size - sizeof(Raw::DirectoryEntryTail);
        Raw::DirectoryEntryTail tail = Utils::copy<Raw::DirectoryEntryTail>(block_buffer, local_tail_offset);
        tail.det_reserved_zero1 = 0;
        tail.det_rec_len = 12;
        tail.det_reserved_zero2 = 0;
        tail.det_reserved_ft = 0xDE; // EXT4_FT_DIR_CSUM
        
        tail.det_checksum = Checksums::checksum_dir(dir_inode, 
            Utils::as_span<std::byte>(block_buffer), 
            block_size, 
            this->super_block.get_checksum_seed()
        );
        Utils::write_to(buffer_bytes, tail, local_tail_offset);
    }

    // Escreve o bloco inteiro atualizado e perfeitamente síncrono no disco
    this->write_block(block_number, buffer_bytes);
}

void Ext4::Wrappers::Image::dir_unlink_entry(const Wrappers::Inode &dir_inode, const std::string &name) {
    std::vector<Wrappers::DirectoryEntry> entries = this->list_dir(dir_inode);
    if (entries.empty()) return;
    uint32_t block_size = this->super_block.get_block_size();

    // Caso especial: primeira entry (".") -> Apenas esvazia marcando inode = 0
    if (entries.front().get_name() == name) {
        uint64_t phys_offset = this->get_absolute_block_offset(dir_inode, 0);
        uint64_t block_number = phys_offset / block_size;

        std::vector<std::byte> block_buffer(block_size);
        std::span<std::byte> buffer_bytes = Utils::as_byte_span(block_buffer);
        this->read_block(block_number, buffer_bytes);

        uint32_t first_entry_offset = phys_offset % block_size;
        Raw::DirectoryEntry first_raw = Utils::copy<Raw::DirectoryEntry>(buffer_bytes, first_entry_offset);
        first_raw.inode = 0;

        Utils::write_to(buffer_bytes, first_raw, first_entry_offset);
        if (this->super_block.has_metadata_csum()) {
            uint32_t tail_offset = block_size - sizeof(Raw::DirectoryEntryTail);
            Raw::DirectoryEntryTail tail = Utils::copy<Raw::DirectoryEntryTail>(buffer_bytes, tail_offset);
            tail.det_checksum = Checksums::checksum_dir(dir_inode, buffer_bytes, block_size, this->super_block.get_checksum_seed());
            Utils::write_to(buffer_bytes, tail, tail_offset);
        }

        this->write_block(block_number, buffer_bytes);
        return;
    }

    // Caso geral: expande rec_len da entry anterior
    // Pegamos o par (antes_alvo, alvo)
    auto processed_view = entries | std::views::adjacent<2>;
    auto it = std::ranges::find_if(processed_view, [&](const auto &p){
        return std::get<1>(p).get_name() == name;
    });

    if (it == processed_view.end()) return;
    auto [before_target, target] = *it;

    size_t before_idx = std::ranges::distance(
        entries.begin(),
        std::ranges::find_if(entries, [&](const auto &e){ return e.get_name() == before_target.get_name(); })
    );

    uint64_t before_offset = this->entry_logical_offset(entries, before_idx);
    uint64_t before_phys = this->get_absolute_block_offset(dir_inode, before_offset);
    uint64_t block_number = before_phys / block_size;

    // Lemos o bloco para alteração contígua
    std::vector<std::byte> block_buffer(block_size);
    std::span<std::byte> buffer_bytes = Utils::as_byte_span(block_buffer);
    this->read_block(block_number, buffer_bytes);

    // Modifica a entrada anterior dentro do buffer
    auto* raw_before = reinterpret_cast<Raw::DirectoryEntry*>(block_buffer.data() + (before_phys % block_size));
    raw_before->rec_len += target.get_raw().rec_len;

    // Atualiza a assinatura baseando-se no estado real modificado em memória
    if (this->super_block.has_metadata_csum()) {
        uint32_t tail_offset = block_size - sizeof(Raw::DirectoryEntryTail);
        Raw::DirectoryEntryTail tail = Utils::copy<Raw::DirectoryEntryTail>(buffer_bytes, tail_offset);
        tail.det_checksum = Checksums::checksum_dir(dir_inode, buffer_bytes, block_size, this->super_block.get_checksum_seed());
        Utils::write_to(buffer_bytes, tail, tail_offset);
    }
    this->write_block(block_number, buffer_bytes);
}

void Ext4::Wrappers::Image::dir_remove_entry(const Wrappers::Inode &dir_inode, const std::string &name) {
    if (name == "." || name == ".." || name.empty()) return;

    std::vector<Wrappers::DirectoryEntry> entries = this->list_dir(dir_inode);
    auto target = std::ranges::find_if (entries, [&](const auto &e){
        return e.get_name() == name;
    });

    if(target == entries.end()) return;
    uint32_t ino = target->get_inode();
    bool is_directory = target->is_dir();

    // Remove a referência física da tabela de diretórios do PAI primeiro
    this->dir_unlink_entry(dir_inode, name);

    Wrappers::Inode inode = this->get_inode(ino);
    Raw::Inode raw = inode.get_raw();

    // Diretórios precisam ter seu i_links_count decrementado
    // Se chegar a 0, liberamos os recursos
    if(is_directory) raw.i_links_count = 0;
    else if(raw.i_links_count > 0) raw.i_links_count--;

    bool should_delete = (raw.i_links_count == 0);
    std::vector<uint64_t> blocks_to_free;
    if(should_delete) blocks_to_free = this->get_blocks(inode);  // coleta ANTES de zerar

    if(should_delete) {
        raw.i_dtime = static_cast<uint32_t>(std::time(nullptr));
        raw.i_mode = 0;
        raw.i_size_lo = 0;
        raw.i_size_hi = 0;
    }

    inode.set_raw(raw);
    this->write_inode(inode);

    if (should_delete) {
        this->free_inode(ino, is_directory);
        for (const auto &blk : blocks_to_free) this->free_block(blk);
    }
}

void Ext4::Wrappers::Image::dir_rename_entry(const Wrappers::Inode &dir_inode, const std::string &old_name, const Wrappers::Inode &new_dir, const std::string &new_name) {
    std::vector<Wrappers::DirectoryEntry> entries = this->list_dir(dir_inode);
    auto target = std::ranges::find_if(entries, [&](const auto &entry){
        return entry.get_name() == old_name;
    });

    uint32_t file_ino = target->get_inode();
    Raw::DirectoryFileType file_type = target->get_type();
    bool is_dir = (file_type == Raw::DirectoryFileType::EXT4_FT_DIR);
    this->dir_unlink_entry(dir_inode, old_name);

    // Se o rename é no mesmo diretório, então os checksums do alvo e atual diretórios são iguais
    if (dir_inode.get_checksum() == new_dir.get_checksum()) {
        // Rename simples — mesmo diretório, ".." não muda
        this->dir_add_entry(dir_inode, file_ino, new_name, file_type);
    } else {
        // Move para outro diretório
        // Caso o alvo seja um diretório
        if (is_dir) {
            // Atualiza ".." do diretório movido
            Wrappers::Inode moved = this->get_inode(file_ino);
            this->dir_rename_dotdot(moved, new_dir.get_inode_id());
            
            // Decrementa links do pai antigo
            Raw::Inode old_raw = dir_inode.get_raw();
            old_raw.i_links_count--;
            old_raw.i_ctime = static_cast<uint32_t>(std::time(nullptr));
            Wrappers::Inode old_updated = dir_inode;
            old_updated.set_raw(old_raw);
            this->write_inode(old_updated);

            // Incrementa links do novo pai
            Raw::Inode new_raw = new_dir.get_raw();
            new_raw.i_links_count++;
            Wrappers::Inode new_updated = new_dir;
            new_updated.set_raw(new_raw);
            this->write_inode(new_updated);

            // Atualiza data de atualização
            Raw::Inode moved_raw = moved.get_raw();
            moved_raw.i_ctime = static_cast<uint32_t>(std::time(nullptr));
            moved.set_raw(moved_raw);
            this->write_inode(moved);
        }
        this->dir_add_entry(new_dir, file_ino, new_name, file_type);
    }
}

void Ext4::Wrappers::Image::dir_rename_dotdot(const Wrappers::Inode &dir_inode, uint32_t new_parent_ino) {
    // Lê o bloco de dados do diretório
    auto blocks = this->get_blocks(dir_inode);
    std::vector<std::byte> buffer(this->super_block.get_block_size());
    std::span<std::byte> buffer_bytes = Utils::as_byte_span(buffer);
    this->read_block(blocks[0], buffer_bytes);

    Raw::DirectoryEntry dot = Utils::copy<Raw::DirectoryEntry>(buffer_bytes);
    // A entry ".." é sempre a segunda entry — pula a primeira (".")
    size_t dotdot_offset = dot.rec_len;
    Raw::DirectoryEntry dotdot = Utils::copy<Raw::DirectoryEntry>(buffer_bytes, dotdot_offset);

    // Atualiza só o campo inode da entry ".."
    dotdot.inode = new_parent_ino;
    Utils::write_to(buffer_bytes, dotdot, dotdot_offset);

    this->write_block(blocks[0], buffer_bytes);

    // Recalcula o checksum do bloco
    if (this->super_block.has_metadata_csum()) {
        uint32_t block_size = this->super_block.get_block_size();
        uint64_t tail_phys_offset = (blocks[0] * block_size) + block_size - sizeof(Raw::DirectoryEntryTail);

        // Relê o bloco já atualizado para calcular o checksum correto
        std::vector<std::byte> updated_block(block_size);
        this->read_block(blocks[0], Utils::as_byte_span(updated_block));

        Raw::DirectoryEntryTail new_tail{
            .det_reserved_zero1 = 0,
            .det_rec_len = 12,
            .det_reserved_zero2 = 0,
            .det_reserved_ft = Raw::DirectoryFileType::EXT4_FT_DIR_CSUM,
            .det_checksum = Checksums::checksum_dir(
                dir_inode, Utils::as_span<std::byte>(updated_block), 
                block_size, this->super_block.get_checksum_seed()
            ),
        };
        this->write_offset(tail_phys_offset, Utils::as_byte_span(new_tail));
    }
}