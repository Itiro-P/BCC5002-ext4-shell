#pragma once

#include <array>
#include <cstdint>

namespace Ext4::Journal {
    static inline constexpr uint32_t JBD2_MAGIC_NUMBER = 0xC03A39A2;
    // Tipos de blocos internos que compõem o Journal JBD2 (j_blocktype).
    enum JournalBlockType : uint32_t {
        JBD2_DESCRIPTOR_BLOCK = 1,
        JBD2_COMMIT_BLOCK     = 2,
        JBD2_SUPERBLOCK_V1    = 3,
        JBD2_SUPERBLOCK_V2    = 4,
        JBD2_REVOKE_BLOCK     = 5,
    };
    
    #pragma pack(push, 1)
    struct JournalHeader {
        uint32_t    h_magic;         // Número mágico do Journal
        uint32_t    h_blocktype;     // Identificador do tipo de bloco (JournalBlockType)
        uint32_t    h_sequence;      // Número de sequência global da transação
    };
    
    struct JournalSuperBlock {
        JournalHeader header;
        uint32_t    j_blocksize;
        uint32_t    j_maxlen;
        uint32_t    j_first;
        uint32_t    j_sequence;
        uint32_t    j_start;
        uint32_t    j_errno;
        uint32_t    j_feature_compat;
        uint32_t    j_feature_incompat;
        uint32_t    j_feature_ro_compat;
        std::array<uint8_t, 16> j_uuid;
        uint32_t    j_nr_users;
        uint32_t    j_dynpad;
        std::array<uint32_t, 4> j_ids;
        uint8_t     j_checksum_type;
        std::array<uint8_t, 3> j_padding2;
        uint32_t    j_checksum_seed;
        uint32_t    j_checksum;
        std::array<uint8_t, 924> j_padding;
    };
    #pragma pack(pop)

    static_assert(sizeof(JournalHeader) == 12, "O tamanho de JournalHeader deve ser de 12 bytes!");
    static_assert(sizeof(JournalSuperBlock) == 1024, "O tamanho da estrutura JournalSuperBlock deve ser exatamente 1024 bytes!");
}
