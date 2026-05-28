#pragma once

#include <array>
#include <cstdint>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes internas dos Atributos Estendidos.
     */
    namespace XattrNS {
        // Número mágico universal que valida a assinatura dos Atributos Estendidos (0xEA020000).
        static inline constexpr uint32_t XATTR_MAGIC = 0xEA020000;
    }

    /**
     * @brief Índices de mapeamento que substituem os prefixos textuais comuns das chaves.
     * Poupa espaço físico em disco armazenando apenas um ID numérico em vez da string completa do namespace.
     */
    enum XattrNameIndex : uint8_t {
        EXT4_XATTR_INDEX_USER                  = 1, // Substitui o prefixo "user."
        EXT4_XATTR_INDEX_POSIX_ACL_ACCESS      = 2, // Substitui o prefixo "system.posix_acl_access"
        EXT4_XATTR_INDEX_POSIX_ACL_DEFAULT     = 3, // Substitui o prefixo "system.posix_acl_default"
        EXT4_XATTR_INDEX_TRUSTED               = 4, // Substitui o prefixo "trusted."
        EXT4_XATTR_INDEX_SECURITY              = 6, // Substitui o prefixo "security."
        EXT4_XATTR_INDEX_SYSTEM                = 7, // Substitui o prefixo "system." (Geralmente usado para inline data)
        EXT4_XATTR_INDEX_SYSTEM_RICHACL        = 8  // Substitui o prefixo "system.richacl" (Especificidade de kernels específicos)
    };

    #pragma pack(push, 1)

    /**
     * @brief Cabeçalho do Bloco de Atributos Estendidos (struct ext4_xattr_header).
     * Ocupa os primeiros 32 bytes de um bloco de disco dedicado exclusivamente a armazenar xattrs.
     */
    struct XattrHeader {
        uint32_t h_magic;        // Número mágico de identificação (Deve ser igual a XattrNS::XATTR_MAGIC).
        uint32_t h_refcount;     // Contador de referências. Permite que múltiplos inodes partilhem o mesmo bloco se os atributos forem idênticos.
        uint32_t h_blocks;       // Quantidade de blocos em uso para armazenar estes atributos (Atualmente fixo em 1).
        uint32_t h_hash;         // Hash numérico calculado sobre o valor de todos os atributos para deduplicação rápida.
        uint32_t h_checksum;     // Checksum de integridade do bloco de atributos inteiro (Calculado via CRC32c).
        std::array<uint32_t, 3> h_reserved; // Espaço reservado para expansão futura (Deve ser preenchido com zeros).
    };

    /**
     * @brief Entrada de Chave de Atributo Estendido (struct ext4_xattr_entry).
     * Uma lista encadeada destas estruturas segue-se imediatamente após o cabeçalho XattrHeader.
     * O fim da lista em memória é identificado quando os primeiros 4 campos medem rigorosamente zero.
     */
    struct XattrEntry {
        uint8_t  e_name_len;      // Tamanho em bytes do nome do atributo (Apenas a string sufixo, excluindo o prefixo indexado).
        uint8_t  e_name_index;    // Índice numérico do prefixo do atributo (Ver mapeamento em XattrNameIndex).
        uint16_t e_value_offs;    // Deslocamento relativo (offset em bytes) dentro do bloco onde o valor do dado real começa.
        uint32_t e_value_block;   // O bloco físico de disco onde o valor reside (Se zero, o valor está armazenado neste mesmo bloco).
        uint32_t e_value_size;    // Tamanho real em bytes do valor associado ao atributo.
        uint32_t e_hash;          // Hash calculado individualmente sobre a chave (nome) e o valor para validação.
        // O resto da string do nome do atributo segue imediatamente em linha na memória: char e_name[e_name_len]
    };
    #pragma pack(pop)

    // Asserções estáticas para assegurar conformidade milimétrica com o layout em disco do Kernel Linux
    static_assert(sizeof(XattrHeader) == 32, "A estrutura XattrHeader deve medir exatamente 32 bytes!");
    static_assert(sizeof(XattrEntry)  == 16, "A estrutura base XattrEntry deve medir exatamente 16 bytes!");
}