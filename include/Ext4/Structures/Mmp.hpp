#pragma once

#include <array>
#include <cstdint>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes e assinaturas mágicas do mecanismo MMP.
     */
    namespace MmpNS {
        // Número mágico universal que valida o bloco MMP (0x004D4D50 -> "MMP" em ASCII).
        static inline constexpr uint32_t MMP_MAGIC = 0x004D4D50;
        
        // Valores sequenciais especiais que indicam estados controlados de libertação do disco.
        static inline constexpr uint32_t MMP_SEQ_CLEAN = 0xFFE44D4D; // O volume foi desmontado de forma limpa e está livre.
        static inline constexpr uint32_t MMP_SEQ_FSCK  = 0xE24D4D50; // O utilitário fsck está a modificar o sistema de ficheiros.
    }

    #pragma pack(push, 1)
    /**
     * @brief Estrutura do Bloco de Proteção de Montagem Múltipla (struct mmp_struct).
     * O bloco MMP atua como um mecanismo de pulsação (heartbeat) dinâmico em discos partilhados (SANs/Clusters).
     * Impede que mais de um nó monte o mesmo sistema de ficheiros em modo leitura-escrita (Read-Write)
     * concorrentemente, prevenindo a corrupção imediata das tabelas de alocação.
     */
    struct MmpStruct {
        uint32_t    mmp_magic;          // Número mágico de validação (Deve ser igual a MmpNS::MMP_MAGIC).
        uint32_t    mmp_seq;            // Número de sequência incremental atualizado periodicamente pelo nó ativo.
        uint64_t    mmp_time;           // Carimbo de data/hora (Timestamp Unix) da última escrita deste bloco.
        
        std::array<char, 64> mmp_nodename; // String contendo o nome legível da máquina (Hostname) que montou o volume.
        std::array<char, 32> mmp_bdevname; // String contendo o nome do dispositivo de blocos no sistema operativo (ex: "sdb1").
        
        uint16_t    mmp_check_interval; // O intervalo sugerido (em segundos) que outros nós devem aguardar antes de re-verificar o bloco.
        uint16_t    mmp_pad1;           // Bytes de alinhamento estrutural (Deve ser preenchido com zero).
        
        std::array<uint32_t, 226> mmp_pad2; // Matriz de preenchimento de segurança para reservar o corpo do bloco (Preenchido com zeros).
        uint32_t    mmp_checksum;       // Checksum de metadados correspondente a todo o bloco MMP (Calculado via CRC32c).
    };
    #pragma pack(pop)

    // Asserção estática para garantir conformidade milimétrica com o tamanho padrão de bloco lógico da especificação
    static_assert(sizeof(MmpStruct) == 1024, "A estrutura MmpStruct deve medir exatamente 1024 bytes!");
}