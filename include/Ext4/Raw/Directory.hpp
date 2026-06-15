#pragma once

#include <cstdint>
#include <array>

namespace Ext4::Raw {
    /**
     * @brief Tipos de ficheiro utilizados no campo file_type das entradas de diretório (DirectoryEntry).
     * Mapeia nativamente o formato do driver EXT4 para identificação rápida sem ler o inode.
     */
    enum DirectoryFileType : uint8_t {
        EXT4_FT_UNKNOWN  = 0, // Tipo desconhecido.
        EXT4_FT_REG_FILE = 1, // Ficheiro regular (Regular file).
        EXT4_FT_DIR      = 2, // Diretório (Directory).
        EXT4_FT_CHRDEV   = 3, // Dispositivo de caracteres (Character device).
        EXT4_FT_BLKDEV   = 4, // Dispositivo de blocos (Block device).
        EXT4_FT_FIFO     = 5, // Fila FIFO / Pipe nomeado.
        EXT4_FT_SOCK     = 6, // Socket Unix.
        EXT4_FT_SYMLINK  = 7,  // Ligação simbólica (Symbolic link).
        EXT4_FT_DIR_CSUM = 0xDE  // Entrada falsa de tail de checksum
    };

    #pragma pack(push, 1)
    /**
     * @brief Entrada de diretório linear clássica/moderna v2 (struct ext4_dir_entry_2).
     * Esta estrutura possui tamanho variável em disco baseado no campo rec_len.
     * O nome do ficheiro segue-se em linha na memória imediatamente após a estrutura física.
     */
    struct DirectoryEntry {
        uint32_t inode;        // Número do inode para o qual esta entrada aponta. Se 0, a entrada está vazia.
        uint16_t rec_len;      // Tamanho total deste registo em disco (Deve ser múltiplo de 4 bytes).
        uint8_t  name_len;     // Tamanho real (em bytes) da string correspondente ao nome do ficheiro.
        uint8_t  file_type;    // Tipo do ficheiro mapeado (Ver mapeamento em DirectoryFileType).
        // O nome do arquivo segue em linha na memória: char name[name_len]
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    /**
     * @brief Cauda presente de forma oculta que armazena o checksum do diretório.
     */
    struct DirectoryEntryTail {
        uint32_t det_reserved_zero1;  // Sempre 0 (para parecer um inode inválido)
        uint16_t det_rec_len;         // Tamanho deste registro (sempre 12)
        uint8_t  det_reserved_zero2;  // Sempre 0
        uint8_t  det_reserved_ft;     // Tipo de arquivo especial (0xDE - EXT4_FT_DIR_CSUM)
        uint32_t det_checksum;        // O CHECKSUM REAL (CRC32c) de 32 bits
    };
    #pragma pack(pop)

    // Asserções estáticas para assegurar conformidade milimétrica com o layout do Kernel Linux
    static_assert(sizeof(DirectoryEntry) == 8, "A estrutura base DirectoryEntry deve medir exatamente 8 bytes!");
    static_assert(sizeof(DirectoryEntryTail) == 12, "DirectoryEntryTail deve ter exatamente 12 bytes!");
}