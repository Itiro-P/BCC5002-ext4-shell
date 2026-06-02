#pragma once

#include <fstream>
#include <string>

namespace Ext4 {
    /**
     * @brief Classe utilitária responsável por encapsular a manipulação de arquivos de imagem de disco.
     * @author Pedro Itiro Nagao
     */
    class Image {
        // A imagem carrega um arquivo de imagem de disco e mantém um fluxo de leitura e escrita.
        std::fstream image_file;

        // O inode atual, começando no diretório raiz (Inode 2).
        Ext4::Inode::Inode current_inode;

        // O diretório atual, representado como uma string de caminho (ex: "/home/user/docs").
        std::string current_path = "/";

        // O superbloco lido da imagem, armazenado para uso futuro.
        Structures::SuperBlock super_block;

        // O vetor contendo a tabela de descritores de grupos, lida a partir do próximo bloco após o superbloco.
        std::vector<Structures::GroupDescriptor> group_descriptors;

        // Se estamos em um sistema de arquivos com suporte a 64 bits.
        bool is_64;

        // O vetor contendo os bitmaps dos blocos.
        std::vector<uint64_t> block_bitmaps;

        // O vetor contendo os bitmaps dos inodes.
        std::vector<uint64_t> inode_bitmaps;

        // O vetor contendo os offsets dos inodes.
        std::vector<uint64_t> inode_table_offsets;

        // O tamanho de um bloco.
        uint32_t block_size;

        // O número de blocos.
        uint64_t block_count;

        // O tamanho de um inode.
        uint32_t inode_size;

        // O número de inodes.
        uint32_t inode_count;

        // Números de blocos por grupo.
        uint32_t blocks_per_group;

        // Números de inodes por grupo.
        uint32_t inodes_per_group;
        
        /**
         * @brief Método auxiliar para posicionar o ponteiro de leitura/escrita da imagem em um offset específico a partir do início.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @throws `std::runtime_error` Se ocorrer um erro ao tentar posicionar o ponteiro de leitura/escrita.
         */
        void seek(std::streamoff offset);

        public:
        /**
         * @brief Construtor que tenta abrir o arquivo de imagem especificado e ler o superbloco para validar a imagem.
         * @param image_path O caminho para o arquivo de imagem a ser aberto.
         * @throws `std::runtime_error` Se o arquivo não puder ser aberto ou se o superbloco não for válido (assinatura mágica incorreta).
         */
        explicit Image(const std::string& image_path);

        // Desabilita a cópia para evitar problemas de gerenciamento de recursos.
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;
        
        // Permite a movimentação para facilitar o gerenciamento de recursos.
        Image(Image&&) = default;
        Image& operator=(Image&&) = default;
        ~Image() { 
            if (image_file.is_open()) image_file.close(); 
        }
        
        /**
         * @brief Método auxiliar para ler um offset de dados da imagem, garantindo que a quantidade de bytes lida seja a esperada.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void read_offset(std::streamoff offset, std::span<std::byte> buffer);
        
        /**
         * @brief Método auxiliar para ler um bloco de dados da imagem, garantindo que a quantidade de bytes lida seja a esperada.
         * @param block_num O número do bloco a ser lido.
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void read_block(uint64_t block_num, std::span<std::byte> buffer);

        /**
         * @brief Retorna o inode atual encapsulado em um `InodeWrapper`, que fornece métodos de conveniência para acessar os metadados do inode.
         * @return O `InodeWrapper` do inode atual.
         */
        Ext4::Inode::Inode get_current_inode() const& {
            return this->current_inode;
        }

        /**
         * @brief Retorna o caminho do diretório atual como uma string. O caminho é atualizado conforme o usuário navega pelo sistema de arquivos.
         * @return O caminho do diretório atual.
         */
        std::string get_current_path() const& {
            return this->current_path;
        }

        /**
         * @brief Lê os metadados de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @return Estrutura preenchida com os dados do disco Inode.
         */
        Ext4::Inode::Inode get_inode(const uint32_t inode_num);

        /**
         * @brief Obtém uma lista de blocos alocados para este Inode. Se o Inode utiliza extents, esta função irá decodificar a estrutura de extents para retornar os blocos físicos. Se o Inode utiliza blocos diretos/indiretos, esta função irá ler os blocos diretos e seguir os ponteiros de blocos indiretos conforme necessário.
         * @param inode O Inode para o qual os blocos alocados devem ser obtidos.
         * @return Um vetor de números de blocos alocados para este Inode.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o Inode estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> get_blocks(Ext4::Inode::Inode& inode);

        /**
         * @brief Lê os metadados brutos de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @return Estrutura preenchida com os dados do disco Inode.
         * @throws `std::runtime_error` se o Inode for inválido ou qualquer tipo de erro.
         */
        Ext4::Inode::Raw::Inode get_raw_inode(const uint32_t inode_num);

        /**
         * @brief Lê os dados do Inode a partir dos blocos alocados. Esta função irá ler os blocos físicos correspondentes aos dados do Inode e concatená-los para retornar o conteúdo completo do ficheiro ou diretório.
         * @param inode O Inode para o qual os dados devem ser lidos.
         * @return Um vetor de bytes contendo os dados lidos.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o Inode estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<std::byte> read_file(const Ext4::Inode::Inode& inode);

        /**
         * @brief Lê os blocos de dados diretamente de um nó folha da árvore de extents. Deve ser chamado apenas quando `eh_depth == 0`.
         * @param inode O Inode que contém o nó de extents a ser lido. Necessário para acessar os dados do bloco de extents.
         * @param header O cabeçalho do nó de extents, necessário para determinar quantas entradas de blocos existem.
         * @return Um vetor de números de blocos físicos alocados para os dados deste nó folha.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o nó estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> read_blocks_from_leafs(const Ext4::Inode::Inode& inode, const Ext4::Structures::ExtentHeader& header);

        /**
         * @brief Lê os blocos de dados a partir de um nó interno de indexação da árvore de extents. Deve ser chamado apenas quando `eh_depth > 0`.
         * @param inode O Inode que contém o nó de extents a ser lido. Necessário para acessar os dados do bloco de extents.
         * @param index_block O número do bloco de índice a ser lido.
         * @param depth A profundidade da árvore de extents.
         * @return Um vetor de números de blocos físicos alocados para os dados indexados por este nó interno.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o nó estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> read_blocks_from_index(const Ext4::Inode::Inode& inode, const uint64_t index_block, const uint16_t depth);
    };
}
