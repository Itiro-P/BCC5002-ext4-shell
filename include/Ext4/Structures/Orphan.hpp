#pragma once

#include <cstdint>
#include <vector>
#include <array>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes internas do ficheiro de órfãos.
     */
    namespace OrphanNS {
        // Valor mágico protetor gravado estritamente no final de cada bloco de órfãos (0x0B10CA04).
        static inline constexpr uint32_t ORPHAN_BLOCK_MAGIC = 0x0B10CA04;
    }

    #pragma pack(push, 1)
    /**
     * @brief Estrutura física da cauda de um bloco de ficheiro de órfãos (struct ext4_orphan_block_tail).
     * Ocupa obrigatoriamente os últimos 8 bytes de qualquer bloco associado ao ficheiro de órfãos,
     * independentemente do tamanho do bloco (1KB, 2KB, 4KB, etc.) definido no Superbloco.
     */
    struct OrphanBlockTail {
        uint32_t ob_magic;     // Assinatura mágica de validação (Deve ser igual a OrphanNS::ORPHAN_BLOCK_MAGIC).
        uint32_t ob_checksum;  // Checksum de integridade de metadados para este bloco específico (Calculado via CRC32c).
    };
    #pragma pack(pop)

    // Asserção estática para garantir o alinhamento de 8 bytes da cauda exigido pela especificação do kernel
    static_assert(sizeof(OrphanBlockTail) == 8, "A cauda OrphanBlockTail deve medir exatamente 8 bytes!");

    /**
     * @brief Classe utilitária responsável por decodificar dinamicamente o conteúdo de um bloco do Orphan File.
     * Este recurso substitui a lista ligada global legada por blocos de indexação paralela, melhorando a
     * escalabilidade e evitando gargalos em concorrência de I/O em grandes volumes.
     */
    class OrphanBlockParser {
    public:
        /**
         * @brief Varre o buffer de um bloco de órfãos e extrai todos os IDs de inodes válidos ativos.
         * * @param blockBuffer Vetor contendo os bytes puros (raw data) do bloco lido do disco (ex: 4096 bytes).
         * @return std::vector<uint32_t> Lista contendo os números dos inodes órfãos recuperados para processamento.
         */
        static std::vector<uint32_t> parseInodes(const std::vector<uint8_t>& blockBuffer) {
            std::vector<uint32_t> foundInodes;
            size_t blocksize = blockBuffer.size();

            // Segurança: O bloco deve ser grande o suficiente para conter pelo menos a cauda de metadados
            if (blocksize < sizeof(OrphanBlockTail)) return foundInodes;

            // 1. Mapeia o ponteiro para os últimos 8 bytes do bloco a fim de extrair a cauda
            const OrphanBlockTail* tail = reinterpret_cast<const OrphanBlockTail*>(
                &blockBuffer[blocksize - sizeof(OrphanBlockTail)]
            );

            // 2. Valida o número mágico do bloco. Se estiver incorreto ou zerado, aborta a leitura
            using namespace Ext4::Structures::OrphanNS;
            if (tail->ob_magic != ORPHAN_BLOCK_MAGIC) return foundInodes;

            // 3. Calcula o limite máximo de entradas (inteiros de 32 bits) que cabem no corpo utilizável.
            // O corpo de dados válidos estende-se do offset 0 até (blocksize - 8 bytes da cauda).
            size_t maxEntries = (blocksize - sizeof(OrphanBlockTail)) / sizeof(uint32_t);
            const uint32_t* entries = reinterpret_cast<const uint32_t*>(blockBuffer.data());
            
            // 4. Varre o array linear de entradas adicionando os IDs de inodes detetados
            for (size_t i = 0; i < maxEntries; ++i) {
                uint32_t inodeNum = entries[i];
                // Cada slot pode estar vazio (0) ou conter o ID de um inode que precisa ser recuperado/truncado
                if (inodeNum != 0) {
                    foundInodes.push_back(inodeNum);
                }
            }
            
            return foundInodes;
        }
    };
}