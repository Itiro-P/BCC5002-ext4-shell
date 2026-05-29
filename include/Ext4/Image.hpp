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
        Ext4::Inode::InodeWrapper current_inode;

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
         * @brief Método auxiliar para posicionar o ponteiro de leitura/escrita da imagem em um offset específico.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @param dir A direção do deslocamento (`std::ios::beg, std::ios::cur, std::ios::end`).
         * @throws `std::runtime_error` Se ocorrer um erro ao tentar posicionar o ponteiro de leitura/escrita.
         */
        void seek(std::streamoff offset, std::ios_base::seekdir dir);
        
        /**
         * @brief Método auxiliar para ler um bloco de dados da imagem, garantindo que a quantidade de bytes lida seja a esperada.
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @param size O número de bytes a serem lidos.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void read(char *buffer, std::streamsize size);
        
        /**
         * @brief Método auxiliar que combina a funcionalidade de `seek` e `read` para ler dados de um offset específico na imagem.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido antes da leitura.
         * @param dir A direção do deslocamento (`std::ios::beg, std::ios::cur, std::ios::end`).
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @param size O número de bytes a serem lidos.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void seek_and_read(std::streamoff offset, std::ios_base::seekdir dir, char *buffer, std::streamsize size);
        
        /**
         * @brief Concatena duas metades de bits (_lo e _hi) em um tipo inteiro maior de 64 bits.
         * Usa conceitos do C++20 para garantir que apenas tipos inteiros sejam aceitos.
         * @param lo A parte inferior (bits 0-31 ou 0-15) do valor a ser concatenado.
         * @param hi A parte superior (bits 32-63 ou 16-31) do valor a ser concatenado.
         * @return O valor concatenado resultante, com os bits de `hi` deslocados para a posição correta e combinados com `lo`.
         */
        template<typename T>
        constexpr uint64_t concatenate(T lo, T hi) requires std::is_integral_v<T> {
            // Quantos bits a estrutura original 'T' possui? (Ex: se for uint32_t, bits = 32)
            constexpr std::size_t bits = sizeof(T) * 8;
            
            // Fazemos o cast do 'hi' para 64 bits ANTES do shift, 
            // para evitar que os bits saiam do limite do tipo original.
            return (static_cast<uint64_t>(hi) << bits) | static_cast<uint64_t>(lo);
        }
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
            if(image_file.is_open()) image_file.close(); 
        }

        Ext4::Inode::InodeWrapper get_current_inode() const& {
            return this->current_inode;
        }

        /**
         * @brief Lê os metadados de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @return Estrutura preenchida com os dados do disco Inode.
         */
        Ext4::Inode::InodeWrapper get_inode(const uint32_t inode_num);

        /**
         * @brief Lê os metadados brutos de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @return Estrutura preenchida com os dados do disco Inode.
         */
        Ext4::Inode::Inode get_raw_inode(const uint32_t inode_num);
    };
}
