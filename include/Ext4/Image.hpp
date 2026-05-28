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

        // O superbloco lido da imagem, armazenado para uso futuro.
        Structures::SuperBlock super_block;

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
    };
}
