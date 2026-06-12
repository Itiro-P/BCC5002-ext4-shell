#pragma once

#include "../Raw/SuperBlock.hpp"
#include "../Flags.hpp"
#include "../../Utils.hpp"
#include <cstdint>
#include <string>

namespace Ext4::Wrappers {

class SuperBlock {
    Raw::SuperBlock raw;
    bool _is_64 = false;

public:
    SuperBlock() = default;
    SuperBlock(const Raw::SuperBlock &raw_super_block);

    /**
     * @brief Retorna a estrutura vinculada ao Wrappers
     */
    Raw::SuperBlock get_raw() const;

    /**
     * @brief Valida a assinatura mágica e campos críticos do SuperBlock.
     * @throws `std::runtime_error` se a assinatura mágica for diferente de `0xEF53`.
     */
    void validate() const;

    /**
     * @brief Indica se a imagem usa campos de 64 bits no GroupDescriptor.
     * Controla se os campos _hi do GDT devem ser lidos.
     */
    bool is_64bit() const;

    /**
     * @brief Indica se os inodes usam extent tree (EXT4_FEATURE_INCOMPAT_EXTENTS).
     * O projeto assume que esta flag está sempre ativa — validar na abertura da imagem.
     */
    bool has_extents() const;

    /**
     * @brief Indica se diretórios usam árvore hash (HTree) para busca.
     * O projeto não suporta HTree — útil para validar que a imagem é compatível.
     */
    bool has_dir_htree() const;

    /**
     * @brief Indica se arquivos grandes (> 2GiB) são suportados (i_size_hi válido).
     * Se ativo, o tamanho do arquivo é composto por i_size_lo | (i_size_hi << 32).
     */
    bool has_large_file() const;

    /**
     * @brief Indica se i_blocks é contado em unidades de block_size em vez de 512 bytes.
     * Afeta a interpretação de i_blocks_lo no inode.
     */
    bool has_huge_file() const;

    /**
     * @brief Indica se checksums de metadados estão ativos (`s_feature_ro_compat`).
     */
    bool has_metadata_csum() const;

    /**
     * @brief Indica se checksums de descritores de grupo estão ativos (`RO_COMPAT_GDT_CSUM`).
     */
    bool has_compat_gdt_csum() const;

    /**
     * @brief Total de inodes no filesystem.
     * Usado para validar números de inode em get_raw_inode().
     */
    uint32_t get_inodes_count() const;

    /**
     * @brief Total de inodes livres.
     * Exibido em `info`. Deve ser decrementado em touch/mkdir.
     */
    uint32_t get_free_inodes_count() const;

    /**
     * @brief Total de blocos no filesystem (32 ou 64 bits conforme is_64bit()).
     * Usado para calcular group_count().
     */
    uint64_t get_blocks_count() const;

    /**
     * @brief Total de blocos livres (32 ou 64 bits conforme is_64bit()).
     * Exibido em `info`. Deve ser decrementado ao alocar blocos.
     */
    uint64_t get_free_blocks_count() const;

    /**
     * @brief Blocos reservados para o superusuário (root).
     * Exibido em `info`. Esses blocos não são alocáveis por usuários normais.
     */
    uint64_t get_reserved_blocks_count() const;

    /**
     * @brief Tamanho do bloco em bytes: 1024 << s_log_block_size.
     * Valores possíveis: 1024, 2048, 409Usado em todo cálculo de offset.
     */
    uint32_t get_block_size() const;

    /**
     * @brief Número de blocos por grupo.
     * Usado para calcular em qual grupo um bloco reside: bloco / blocks_per_group.
     */
    uint32_t get_blocks_per_group() const;

    /**
     * @brief Número de inodes por grupo.
     * Usado para calcular em qual grupo um inode reside: (ino-1) / inodes_per_group.
     */
    uint32_t get_inodes_per_group() const;

    /**
     * @brief Tamanho de cada inode no disco em bytes (tipicamente 128 ou 256).
     * Usado para calcular o offset de um inode dentro da tabela de inodes.
     */
    uint32_t get_inode_size() const;

    /**
     * @brief Tamanho do GroupDescriptor em bytes (32 sem 64-bit, 64 com 64-bit).
     * Retorna 32 se s_desc_size for zero (imagens antigas sem o campo).
     */
    uint16_t get_desc_size() const;

    /**
     * @brief Número do primeiro bloco de dados (0 para blocos >= 2K, 1 para blocos de 1K).
     * Usado para calcular o offset do GDT: (s_first_data_block + 1) * block_size.
     */
    uint32_t get_first_data_block() const;

    /**
     * @brief Offset absoluto em bytes do início do GDT no disco.
     * Derivado de first_data_block e block_size — centraliza esse cálculo.
     */
    uint64_t get_gdt_offset() const;

    /**
     * @brief Número total de grupos de blocos no filesystem.
     * Derivado de block_count e blocks_per_group.
     */
    uint32_t get_group_count() const;

    /**
     * @brief Número do primeiro inode não reservado pelo sistema (tipicamente 11).
     * Inodes 1–10 são reservados (bad blocks, root, ACL, etc.).
     * Usado como ponto de partida na busca por inodes livres em touch/mkdir.
     */
    uint32_t get_first_ino() const;

    /**
     * @brief Número de inodes reservados por grupo para expansão da tabela de inodes.
     * Inodes além de (inodes_per_group - itable_unused) estão disponíveis para uso.
     * Relevante para alloc_inode nas operações de escrita.
     */
    uint16_t get_min_extra_isize() const;

    /**
     * @brief Nome do volume (até 16 caracteres, pode não ser null-terminated).
     * Exibido em `info`.
     */
    std::string get_volume_name() const;

    /**
     * @brief UUID do filesystem no formato padrão com hífens.
     * Exibido em `info`.
     */
    std::string get_uuid() const;

    /**
     * @brief UUID do filesystem como bytes.
     */
    std::span<const std::byte, 16> get_uuid_bytes() const;

    /**
     * @brief Timestamp Unix da criação do filesystem (mkfs).
     * Exibido em `info`.
     */
    uint32_t get_mkfs_time() const;

    /**
     * @brief Timestamp Unix da última montagem do filesystem.
     * Exibido em `info`.
     */
    uint32_t get_mtime() const;

    /**
     * @brief Timestamp Unix da última escrita no filesystem.
     * Exibido em `info`.
     */
    uint32_t get_wtime() const;

    /**
     * @brief Timestamp Unix da última verificação com e2fsck.
     * Exibido em `info`.
     */
    uint32_t get_lastcheck() const;

    /**
     * @brief Estado do filesystem (1 = limpo, 2 = com erros).
     * Exibido em `info`. Se diferente de 1, a imagem pode estar inconsistente.
     */
    uint16_t get_state() const;

    /**
     * @brief Número de vezes que o filesystem foi montado desde o último fsck.
     * Exibido em `info`.
     */
    uint16_t get_mnt_count() const;

    /**
     * @brief Número máximo de montagens antes de forçar um fsck.
     * Exibido em `info`.
     */
    uint16_t get_max_mnt_count() const;

    /**
     * @brief Nível de revisão do filesystem (0 = original, 1 = dinâmico com inode_size variável).
     * Imagens ext4 modernas usam rev_level = 1.
     */
    uint32_t get_rev_level() const;
};
}