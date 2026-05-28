#pragma once

#include <cstdint>
#include <array>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes e enumeradores internos da árvore de extents.
     */
    namespace ExtentsNS {
        // Número mágico universal que identifica um cabeçalho de nó válido na árvore de extents (0xF30A).
        static inline constexpr uint16_t EXT_MAGIC = 0xF30A;
    }

    #pragma pack(push, 1)

    /**
     * @brief Cabeçalho do nó da árvore de extents (struct ext4_extent_header).
     * Mapeia o início da árvore. Se estiver no interior do Inode, ocupa os primeiros 12 bytes 
     * do campo i_block, ditando como os 48 bytes restantes serão interpretados.
     */
    struct ExtentHeader {
        uint16_t eh_magic;      // Número mágico de validação (Deve ser igual a ExtentsNS::EXT_MAGIC).
        uint16_t eh_entries;    // Quantidade de entradas válidas (índices ou folhas) que se seguem a este cabeçalho.
        uint16_t eh_max;        // Capacidade máxima de entradas que este nó consegue armazenar fisicamente.
        uint16_t eh_depth;      // Profundidade na árvore. (0 = Nó folha contendo dados reais, >0 = Nó interno de indexação).
        uint32_t eh_generation; // Geração da árvore de extents (Atualmente não utilizada pelo kernel, geralmente zero).
    };

    /**
     * @brief Nó Interno ou de Índice da árvore de extents (struct ext4_extent_idx).
     * Utilizado estritamente quando eh_depth > 0. Aponta para um bloco de metadados descendente na árvore.
     */
    struct ExtentIndex {
        uint32_t ei_block;   // O maior número de bloco lógico (offset no ficheiro) que este nó indexa.
        uint32_t ei_leaf_lo; // Os 32 bits inferiores do número do bloco físico do nó filho em disco.
        uint16_t ei_leaf_hi; // Os 16 bits superiores do número do bloco físico do nó filho em disco (Suporte 64-bits).
        uint16_t ei_unused;  // Bytes de alinhamento nulo (Padding).

        /**
         * @brief Concatena os bits inferiores e superiores para extrair o endereço físico real.
         * @return uint64_t Endereço absoluto de 48 bits do bloco físico filho em disco.
         */
        uint64_t getLeafBlock() const noexcept { 
            return (static_cast<uint64_t>(ei_leaf_hi) << 32) | ei_leaf_lo; 
        }
    };

    /**
     * @brief Nó Folha contendo o mapeamento de blocos de dados (struct ext4_extent).
     * Utilizado estritamente quando eh_depth == 0. Mapeia um intervalo sequencial de blocos lógicos a físicos.
     */
    struct ExtentLeaf {
        uint32_t ee_block;    // O primeiro número de bloco lógico do ficheiro coberto por este extent.
        uint16_t ee_len;      // Comprimento do extent (Contagem de blocos sequenciais). Se > 32768, o bloco não está inicializado.
        uint16_t ee_start_hi; // Os 16 bits superiores do endereço de bloco físico correspondente (Suporte 64-bits).
        uint32_t ee_start_lo; // Os 32 bits inferiores do endereço de bloco físico correspondente.

        /**
         * @brief Verifica se o intervalo de blocos atual foi pré-alocado mas ainda não foi inicializado com dados.
         * @return true se ee_len for maior que 32768, significando que leituras devem retornar blocos de zeros.
         */
        bool isUninitialized() const noexcept { return ee_len > 32768; }

        /**
         * @brief Trata a flag de inicialização para retornar a quantidade real de blocos do extent.
         * @return uint16_t Quantidade limpa de blocos alocados no intervalo.
         */
        uint16_t getRealLength() const noexcept { 
            return isUninitialized() ? (ee_len - 32768) : ee_len; 
        }

        /**
         * @brief Concatena os bits inferiores e superiores para obter o início do bloco físico de dados.
         * @return uint64_t Endereço de início absoluto de 48 bits do bloco de dados físico em disco.
         */
        uint64_t getStartBlock() const noexcept { 
            return (static_cast<uint64_t>(ee_start_hi) << 32) | ee_start_lo; 
        }
    };

    /**
     * @brief Cauda de verificação opcional para blocos de extents externos (struct ext4_extent_tail).
     * Fica posicionada estritamente no final de blocos de extents completos alocados fora do inode.
     */
    struct ExtentTail { 
        uint32_t eb_checksum; // Checksum de integridade do bloco de extents inteiro (Calculado via CRC32c).
    };

    #pragma pack(pop)

    // Garantia de integridade física e alinhamento de memória nativa do sistema de ficheiros
    static_assert(sizeof(ExtentHeader) == 12, "A estrutura ExtentHeader deve medir exatamente 12 bytes!");
    static_assert(sizeof(ExtentIndex)  == 12, "A estrutura ExtentIndex deve medir exatamente 12 bytes!");
    static_assert(sizeof(ExtentLeaf)   == 12, "A estrutura ExtentLeaf deve medir exatamente 12 bytes!");
    static_assert(sizeof(ExtentTail)   == 4,  "A estrutura ExtentTail deve medir exatamente 4 bytes!");
}