#pragma once

#include <cstdint>
#include <array>

namespace Ext4::Structures {

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
        EXT4_FT_SYMLINK  = 7  // Ligação simbólica (Symbolic link).
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
     * @brief Informações de controlo da raiz de indexação HTree (struct dx_root_info).
     * Presente apenas se o Inode do diretório possuir a flag EXT4_INDEX_FL ativa.
     * Fica embutida logo após as entradas tradicionais de ponto "." e dois pontos "..".
     */
    struct DxRootInfo {
        uint32_t reserved_zero;   // Espaço reservado obrigatório (Deve ser zero).
        uint8_t  hash_version;    // Versão/Algoritmo do hash utilizado para indexar a árvore HTree.
        uint8_t  info_length;     // Tamanho em bytes desta estrutura de metadados (Geralmente 8).
        uint8_t  indirect_levels; // Quantidade de níveis de indireção/profundidade da árvore (Nível máximo: 2 ou 3).
        uint8_t  unused_flags;    // Flags operacionais não utilizadas.
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    /**
     * @brief Entrada individual de indexação por hash dentro da árvore HTree (struct dx_entry).
     * Mapeia uma chave hash ao bloco do diretório que armazena os nomes correspondentes.
     */
    struct DxEntry { 
        uint32_t hash;            // O valor de hash calculado com base no nome do ficheiro procurado.
        uint32_t block;           // Número do bloco do diretório que contém sub-hashes ou as entradas DirectoryEntry.
    };
    #pragma pack(pop)
    
    #pragma pack(push, 1)
    /**
     * @brief Cauda de validação e integridade do bloco HTree (struct dx_tail).
     * Reside estritamente nos últimos 8 bytes do bloco indexado para verificação de corrupção.
     */
    struct DxTail { 
        uint32_t dt_reserved;     // Espaço reservado (Não utilizado, mas entra no cálculo do checksum).
        uint32_t dt_checksum;     // Valor do Checksum do bloco de diretório HTree inteiro (Calculado via CRC32c).
    };
    #pragma pack(pop)

    // Asserções estáticas para assegurar conformidade milimétrica com o layout do Kernel Linux
    static_assert(sizeof(DirectoryEntry) == 8, "A estrutura base DirectoryEntry deve medir exatamente 8 bytes!");
    static_assert(sizeof(DxRootInfo)     == 8, "A estrutura DxRootInfo deve medir exatamente 8 bytes!");
    static_assert(sizeof(DxEntry)        == 8, "A estrutura DxEntry deve medir exatamente 8 bytes!");
    static_assert(sizeof(DxTail)         == 8, "A estrutura DxTail deve medir exatamente 8 bytes!");
}