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
    
    /**
     * @brief Estrutura do Superbloco do Journal (struct journal_superblock_t).
     * O JBD2 (Journaling Block Device v2) gerencia as transações de metadados do EXT4.
     * Este bloco regista o estado da fila circular do log do journal, tamanhos lógicos,
     * sequências ativas para replay pós-queda e ponteiros de transação.
     */
    struct JournalSuperBlock {
        //  Cabeçalho Comum de Metadados (Geralmente contém o Magic do Journal 0xC03B3998) 
        JournalHeader header;           // Metadados iniciais e de identificação de bloco do JBD2.

        //  Geometria Base Estática (0x0C) 
        uint32_t                 j_blocksize;          // Tamanho do bloco do journal em bytes (Deve coincidir com o do EXT4).
        uint32_t                 j_maxlen;             // Quantidade total de blocos alocados para o ficheiro de journal.
        uint32_t                 j_first;              // ID do primeiro bloco lógico interno onde começam os registos de transação.

        //  Estado Dinâmico de Transação (0x18) 
        uint32_t                 j_sequence;           // Número de sequência esperado para a próxima transação mestre.
        uint32_t                 j_start;              // ID do bloco lógico onde inicia a primeira transação ativa não confirmada (Fila Circular).
        uint32_t                 j_errno;              // Último código de erro gravado de forma persistente em caso de falha de I/O do driver.

        //  Máscaras de Compatibilidade de Recursos (0x24) 
        uint32_t                 j_feature_compat;     // Flags de recursos compatíveis (O driver manipula sem quebrar).
        uint32_t                 j_feature_incompat;   // Flags de recursos incompatíveis (O driver recusa se não conhecer, ex: Checksum v3).
        uint32_t                 j_feature_ro_compat;  // Flags de recursos somente leitura tolerados pelo motor.

        //  Identidade e Integração (0x30) 
        std::array<uint8_t, 16>  j_uuid;               // Identificador Único Universal (UUID) da própria instância do Journal.
        uint32_t                 j_nr_users;           // Quantidade de sistemas de ficheiros que partilham este journal (Geralmente 1).
        uint32_t                 j_dynpad;             // Alinhamento dinâmico de blocos de disco adicionais.

        //  IDs de Backup e Criptografia (0x48) 
        std::array<uint32_t, 4>  j_ids;                // Cópia de segurança/IDs específicos do Superbloco do sistema de ficheiros utilizador.
        
        //  Integridade e Metadados Modernos (0x58) 
        uint8_t                  j_checksum_type;      // Tipo de algoritmo de checksum usado nos blocos (1 para CRC32c).
        std::array<uint8_t, 3>   j_padding2;           // Bytes nulos de alinhamento estrutural para garantir 32 bits.
        uint32_t                 j_checksum_seed;      // Semente pré-calculada combinando o UUID para acelerar cálculos do hash CRC32c.
        uint32_t                 j_checksum;           // Checksum final do próprio bloco do superbloco do journal.

        //  Preenchimento de Cauda (Padding) (0x64) 
        // Reserva exata de 924 bytes livres para assegurar que a estrutura ocupe integralmente 
        // os limites físicos de um bloco padrão e garanta compatibilidade retroativa.
        std::array<uint8_t, 924> j_padding;
    };
    #pragma pack(pop)

    static_assert(sizeof(JournalHeader) == 12, "O tamanho de JournalHeader deve ser de 12 bytes!");
    static_assert(sizeof(JournalSuperBlock) == 1024, "O tamanho da estrutura JournalSuperBlock deve ser exatamente 1024 bytes!");
}
