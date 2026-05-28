#pragma once

#include <cstdint>
#include <cstddef>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes dos descritores de grupo.
     */
    namespace GroupDescriptorNS {
        // Tamanho físico em bytes de um descritor de grupo moderno de 64 bits (64 bytes).
        static inline constexpr size_t SIZE = 64;
    }

    #pragma pack(push, 1)
    /**
     * @brief Estrutura do Descritor de Grupo de Blocos (struct ext4_group_desc).
     * Mapeia os metadados e a localização das tabelas de um grupo de blocos específico.
     * Os campos divididos em '_lo' (32 ou 16 bits inferiores) e '_hi' (32 ou 16 bits superiores) 
     * são concatenados dinamicamente pelo driver se a flag de 64 bits estiver ativa no Superbloco.
     */
    struct GroupDescriptor {
        // 32 Bytes Inferiores (Equivalente ao layout legado do EXT3)
        uint32_t    bg_block_bitmap_lo;       // ID do bloco físico que contém o bitmap de blocos deste grupo (Bits 0-31).
        uint32_t    bg_inode_bitmap_lo;       // ID do bloco físico que contém o bitmap de inodes deste grupo (Bits 0-31).
        uint32_t    bg_inode_table_lo;        // ID do primeiro bloco físico que inicia a tabela de inodes deste grupo (Bits 0-31).
        uint16_t    bg_free_blocks_count_lo;  // Quantidade de blocos livres no grupo (Bits 0-15).
        uint16_t    bg_free_inodes_count_lo;  // Quantidade de inodes livres no grupo (Bits 0-15).
        uint16_t    bg_used_dirs_count_lo;    // Quantidade de inodes que são diretórios neste grupo (Bits 0-15).
        uint16_t    bg_flags;                 // Flags de estado do grupo (ex: bitmaps não inicializados, tabela zerada).
        uint32_t    bg_exclude_bitmap_lo;     // ID do bloco físico do bitmap de exclusão para snapshots (Bits 0-31).
        uint16_t    bg_block_bitmap_csum_lo;  // Checksum do bitmap de blocos (Bits 0-15).
        uint16_t    bg_inode_bitmap_csum_lo;  // Checksum do bitmap de inodes (Bits 0-15).
        uint16_t    bg_itable_unused_lo;      // Quantidade de inodes não inicializados no final da tabela (Bits 0-15).
        uint16_t    bg_checksum;              // Checksum do próprio descritor de grupo (Algoritmo CRC16 legado).

        // 32 Bytes Superiores (Campos de extensão para suporte a 64 bits do EXT4)
        uint32_t    bg_block_bitmap_hi;       // Bits superiores (32-63) do endereço do bitmap de blocos.
        uint32_t    bg_inode_bitmap_hi;       // Bits superiores (32-63) do endereço do bitmap de inodes.
        uint32_t    bg_inode_table_hi;        // Bits superiores (32-63) do endereço de início da tabela de inodes.
        uint16_t    bg_free_blocks_count_hi;  // Bits superiores (16-31) da contagem de blocos livres.
        uint16_t    bg_free_inodes_count_hi;  // Bits superiores (16-31) da contagem de inodes livres.
        uint16_t    bg_used_dirs_count_hi;    // Bits superiores (16-31) da contagem de diretórios.
        uint16_t    bg_itable_unused_hi;      // Bits superiores (16-31) da contagem de inodes não utilizados.
        uint32_t    bg_exclude_bitmap_hi;     // Bits superiores (32-63) do endereço do bitmap de exclusão.
        uint16_t    bg_block_bitmap_csum_hi;  // Bits superiores (16-31) do checksum do bitmap de blocos.
        uint16_t    bg_inode_bitmap_csum_hi;  // Bits superiores (16-31) do checksum do bitmap de inodes.
        uint32_t    bg_reserved;              // Espaço de alinhamento / Padding nulo da estrutura.
    };
    #pragma pack(pop)

    // Asserção para garantir compatibilidade estrutural milimétrica com a tabela de descritores em disco
    static_assert(sizeof(GroupDescriptor) == GroupDescriptorNS::SIZE, 
                  "A estrutura GroupDescriptor deve ter exatamente 64 bytes!");
}