#pragma once

#include <fstream>
#include <string>
#include "../../Ext4/Raw.hpp"
#include "Inode.hpp"

using namespace Ext4;

namespace Ext4::Wrappers {
    /**
     * @brief Classe utilitária responsável por encapsular a manipulação de arquivos de imagem de disco.
     * @author Pedro Itiro Nagao
     */
    class Image {
        // A imagem carrega um arquivo de imagem de disco e mantém um fluxo de leitura e escrita.
        std::fstream image_file;

        // O Inode raíz (/).
        Wrappers::Inode root_inode;

        // O inode atual, começando no diretório raiz (Inode 2).
        Wrappers::Inode current_inode;

        // O diretório atual, representado como uma string de caminho (ex: "/home/user/docs").
        std::string current_path = "/";

        // O superbloco lido da imagem, armazenado para uso futuro.
        Wrappers::SuperBlock super_block;

        // O vetor contendo a tabela de descritores de grupos, lida a partir do próximo bloco após o superbloco.
        std::vector<Wrappers::GroupDescriptor> group_descriptors;
        
        /**
         * @brief Método auxiliar para posicionar o ponteiro de leitura/escrita da imagem em um offset específico a partir do início.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @throws `std::runtime_error` Se ocorrer um erro ao tentar posicionar o ponteiro de leitura/escrita.
         */
        void seek(const std::streamoff offset);

        public:
        /**
         * @brief Construtor que tenta abrir o arquivo de imagem especificado e ler o superbloco para validar a imagem.
         * @param image_path O caminho para o arquivo de imagem a ser aberto.
         * @throws `std::runtime_error` Se o arquivo não puder ser aberto ou se o superbloco não for válido (assinatura mágica incorreta).
         */
        explicit Image(const std::string &image_path);

        // Desabilita a cópia para evitar problemas de gerenciamento de recursos.
        Image(const Image&) = delete;
        Image &operator=(const Image&) = delete;
        
        // Permite a movimentação para facilitar o gerenciamento de recursos.
        Image(Image&&) = default;
        Image &operator=(Image&&) = default;
        ~Image() { 
            if (image_file.is_open()) image_file.close(); 
        }
        
        /**
         * @brief Retorna o offset do inode alvo.
         * @param inode_um O número do inode.
         */
        std::streamoff get_inode_offset(const uint32_t inode_num);

        /**
         * @brief Método auxiliar para ler um offset de dados da imagem, garantindo que a quantidade de bytes lida seja a esperada.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void read_offset(const std::streamoff offset, std::span<std::byte> buffer);
        
        /**
         * @brief Método auxiliar para ler um bloco de dados da imagem, garantindo que a quantidade de bytes lida seja a esperada.
         * @param block_num O número do bloco a ser lido.
         * @param buffer O buffer onde os dados lidos serão armazenados.
         * @throws `std::runtime_error` Se a quantidade de bytes lida for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void read_block(const uint64_t block_num, std::span<std::byte> buffer);

        /**
         * @brief Método auxiliar para escrever um offset de dados da imagem, garantindo que a quantidade de bytes escrita seja a esperada.
         * @param offset O deslocamento em bytes a partir do início da imagem para onde o ponteiro deve ser movido.
         * @param buffer O buffer onde os dados escritos estão.
         * @throws `std::runtime_error` Se a quantidade de bytes escrita for diferente do esperado ou se ocorrer um erro de escrita.
         */
        void write_offset(const std::streamoff offset, const std::span<const std::byte> buffer);
        
        /**
         * @brief Método auxiliar para escrever um bloco de dados da imagem, garantindo que a quantidade de bytes escrita seja a esperada.
         * @param block_num O número do bloco a ser escrito.
         * @param buffer O buffer onde os dados escritos estão.
         * @throws `std::runtime_error` Se a quantidade de bytes escrita for diferente do esperado ou se ocorrer um erro de leitura.
         */
        void write_block(const uint64_t block_num, const std::span<const std::byte> buffer);

        /**
         * @brief Retorna o inode root encapsulado em um `Inode`, que fornece métodos de conveniência para acessar os metadados do inode.
         * @returns O `Inode` do inode atual.
         */
        Wrappers::Inode get_root_inode() const {
            return this->root_inode;
        }

        /**
         * @brief Retorna o inode atual encapsulado em um `Inode`, que fornece métodos de conveniência para acessar os metadados do inode.
         * @returns O `Inode` do inode atual.
         */
        Wrappers::Inode get_current_inode() const {
            return this->current_inode;
        }

        /**
         * @brief Altera o inode atual da imagem para o especificado.
         * @param inode o inode.
         */
        void set_current_inode(const Wrappers::Inode &inode) {
            this->current_inode = inode;
        }

        /**
         * @brief Altera o caminho atual da imagem para o especificado.
         * @param str o caminho.
         */
        void set_current_path(const std::string &str) {
            this->current_path= str;
        }

        /**
         * @brief Retorna o caminho do diretório atual como uma string. O caminho é atualizado conforme o usuário navega pelo sistema de arquivos.
         * @returns O caminho do diretório atual.
         */
        std::string get_current_path() const {
            return this->current_path;
        }

        /**
         * @brief Retorna uma lista de entradas de um inode.
         * @param inode o Inode a ser listado.
         * @returns Uma lista de entradas.
         * @throws `std::runtime_error` para quaisquer erros durante a execução.
         */
        std::vector<Wrappers::DirectoryEntry> list_dir(const Wrappers::Inode &inode);

        /**
         * @brief Resolve o diretório alvo e encontra o inode relacionado e ele.
         * Se `path` for vazio, inode do diretório atual da imagem é retornado.
         * @param path O diretório alvo.
         * @param base o Inode base a ser o alvo inicial de procura.
         * @returns O Inode relacionado ao diretório alvo.
         * @throws `std::runtime_error` para quaisquers erros que ocorram na execução.
         */
        std::pair<Wrappers::Inode, std::string> resolve_path(const std::string &path, const Wrappers::Inode &base);

        /**
         * @brief Lê os metadados de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @returns Estrutura preenchida com os dados do disco Inode.
         * @throws `std::runtime_error` se o Inode for inválido ou qualquer tipo de erro.
         */
        Raw::Inode get_raw_inode(const uint32_t inode_num);

        /**
         * @brief Lê os metadados de um inode específico do disco a partir do seu número global.
         * @param inode_num Número do inode (ex: 2 para o diretório raiz).
         * @returns Estrutura preenchida com os dados do disco Inode.
         * @throws `std::runtime_error` se o Inode for inválido ou qualquer tipo de erro.
         */
        Wrappers::Inode get_inode(const uint32_t inode_num);

        /**
         * @brief Obtém uma lista de blocos alocados para este Inode. Se o Inode utiliza extents, esta função irá decodificar a estrutura de extents para retornar os blocos físicos. Se o Inode utiliza blocos diretos/indiretos, esta função irá ler os blocos diretos e seguir os ponteiros de blocos indiretos conforme necessário.
         * @param inode O Inode para o qual os blocos alocados devem ser obtidos.
         * @returns Um vetor de números de blocos alocados para este Inode.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o Inode estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> get_blocks(const Wrappers::Inode &inode);

        /**
         * @brief Lê os dados do Inode a partir dos blocos alocados. Esta função irá ler os blocos físicos correspondentes aos dados do Inode e concatená-los para retornar o conteúdo completo do ficheiro ou diretório.
         * @param inode O Inode para o qual os dados devem ser lidos.
         * @returns Um vetor de bytes contendo os dados lidos.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o Inode estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<std::byte> read_file(const Wrappers::Inode &inode);

        /**
         * @brief Lê os blocos de dados diretamente de um nó folha da árvore de extents. Deve ser chamado apenas quando `eh_depth == 0`.
         * @param node_data Os dados do nó de extents a ser lido.
         * @param header O cabeçalho do nó de extents, necessário para determinar quantas entradas de blocos existem.
         * @returns Um vetor de números de blocos físicos alocados para os dados deste nó folha.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o nó estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> read_blocks_from_leafs(std::span<const std::byte> node_data, const Raw::ExtentHeader &header);

        /**
         * @brief Lê os blocos de dados a partir de um nó interno de indexação da árvore de extents. Deve ser chamado apenas quando `eh_depth > 0`.
         * @param node_data Os dados do nó de extents a ser lido.
         * @param header O cabeçalho do nó de extents, necessário para determinar quantas entradas de blocos existem.
         * @returns Um vetor de números de blocos físicos alocados para os dados indexados por este nó interno.
         * @throws `std::runtime_error` se ocorrer um erro ao ler os blocos (por exemplo, se o nó estiver corrompido ou se houver um erro de leitura do dispositivo).
         */
        std::vector<uint64_t> read_blocks_from_index(std::span<const std::byte> node_data, const Raw::ExtentHeader &header);

        /**
         * @brief Escreve um inode no disco.
         * @param inode_num O número do inode.
         * @param inode A estrutura `inode`.
         */
        void write_inode(const uint32_t inode_num, const Raw::Inode &inode);

        /**
         * @brief Escreve um grupo de descritores no disco.
         * @param inode_num O número do GroupDescriptor.
         * @param gd A estrutura GroupDescriptor.
         */
        void write_gdt(uint32_t group, const Raw::GroupDescriptor &gd);

        /**
         * @brief Escreve um superbloco no disco.
         * @param sp A estrutura SuperBlock.
         */
        void write_superblock(const Raw::SuperBlock &sp);

        /**
         * @brief Retorna o grupo de blocos que o inode pertence.
         * @param ino O ID do inode.
         * @returns O índice do GroupDescriptor para ser usado.
         */
        uint32_t get_inode_group(const uint32_t ino);

        /**
         * @brief Retorna a posição do bit que o inode pertence.
         * @param ino O ID do inode.
         * @returns A posição do bit no bitmap do GroupDescriptor relacionado.
         */
        uint32_t get_inode_bit_pos(const uint32_t ino);

        /**
         * @brief Retorna o grupo de blocos que o bloco pertence.
         * @param ino O ID do bloco.
         * @returns O índice do GroupDescriptor para ser usado.
         */
        uint32_t get_block_group(const uint32_t blk);

        /**
         * @brief Retorna a posição do bit que o bloco pertence.
         * @param ino O ID do bloco.
         * @returns A posição do bit no bitmap do GroupDescriptor relacionado.
         */
        uint32_t get_block_bit_pos(const uint32_t blk);

        /**
         * @brief Retorna o offset absoluto de um bloco.
         * @param inode O inode em que o bloco está presente.
         * @param relative_offset O offset relativo do bloco.
         * @returns O offset absoluto do bloco.
         */
        uint64_t get_absolute_block_offset(const Wrappers::Inode &inode, const uint32_t relative_offset);

        /**
         * @brief Aloca um novo inode.
         * @returns O ID do novo inode.
         */
        uint32_t alloc_inode();

        /**
         * @brief Aloca um novo bloco.
         * @returns O ID do novo bloco.
         */
        uint32_t alloc_block();

        /**
         * @brief Libera um inode.
         * @param ino O ID do inode.
         */
        void free_inode(const uint32_t ino);

        /**
         * @brief Libera um bloco.
         * @param ino O ID do bloco.
         */
        void free_block(const uint32_t blk);

        /**
         * @brief Relaciona uma um inode a outro.
         * @param dir_ino O ID do inode que será relacionado (o pai).
         * @param target_ino O ID do inode que se relacionará. 
         * @param name O nome do inode (arquivo/diretório).
         * @param file_type O tipo do inode (arquivo/diretório).
         */
        void dir_add_entry(const uint32_t dir_ino, uint32_t target_ino, const std::string &name, const uint8_t file_type);

        /**
         * @brief Remove uma entrada de nome `name`.
         * @param dir_ino O ID do inode.
         * @param name O nome da entrada.
         */
        void dir_remove_entry(const uint32_t dir_ino, const std::string &name);

        /**
         * @brief Renomeia uma entrada de `old_name` para `new_name`.
         * @param dir_ino O ID do inode.
         * @param old_name O nome alvo (o antigo).
         * @param new_name O novo nome.
         */
        void dir_rename_entry(const uint32_t dir_ino, const std::string &old_name, const std::string &new_name);
    };
}
