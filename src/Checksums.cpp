#include "../include/Ext4.hpp"
#include <print>

/**
 * @file    Checksums.cpp
 * @brief   Implementação de checksums usados no EXT4
 * @author  Pedro Itiro Nagao
 * @date    2025-06-14
 *
 * Funções de checksum (metadados e CRC16 para os grupos de descritores) para cálculo rápido
 */

uint16_t Ext4::Checksums::crc_16(const std::span<const std::byte> data, const uint16_t seed) {
    return crc_generic<CRC16_Config>(data, seed);
}

uint32_t Ext4::Checksums::crc_32c(const std::span<const std::byte> data, const uint32_t seed) {
    return crc_generic<CRC32c_Config>(data, seed);
}

uint32_t Ext4::Checksums::checksum_super_block(const Wrappers::SuperBlock &super) {
    Raw::SuperBlock raw_sp = super.get_raw();
    // Copiamos o conteúdo do bloco para um vetor de bytes SEM o campo do checksum (os últimos 4 bytes)
    std::span<const std::byte> sp_bytes = Utils::as_byte_span(raw_sp, sizeof(raw_sp) - sizeof(raw_sp.s_checksum));
    return crc_32c(sp_bytes);
}

uint16_t Ext4::Checksums::checksum_group(const Wrappers::GroupDescriptor &group, const bool csm_metadata, const uint32_t seed) {
    // Pegamos a struct crua e o número do grupo
    Raw::GroupDescriptor group_raw = group.get_raw();
    uint32_t group_num = group.get_group_number();
    uint16_t desc_size = group.get_desc_size();
    group_raw.bg_checksum = 0;

    if (csm_metadata) {
        // Processa o UUID (Semente inicial limpa: MAX_32BIT)
        uint32_t current_crc = seed;
        
        // O group_num DEVE ser de 4 bytes (uint32_t)
        current_crc = crc_32c(Utils::as_byte_span(group_num), current_crc);
        // Processa o bloco inteiro do descritor (já com o bg_checksum zerado lá dentro)
        // respeitando o tamanho dinâmico (32 ou 64 bytes)
        uint32_t final_crc32 = crc_32c(Utils::as_byte_span(group_raw, desc_size), current_crc);
        
        // Retorna os 16 bits menos significativos
        return static_cast<uint16_t>(final_crc32 & MAX_16BIT);
    } else {
        // Seção de código não testada, já que não usamos nenhuma imagem com checksum antigo crc16
        uint16_t current_crc = crc_16(Utils::as_byte_span(seed), MAX_16BIT);
        
        uint16_t group_num_16 = static_cast<uint16_t>(group_num & MAX_16BIT);
        current_crc = crc_16(Utils::as_byte_span(group_num_16), current_crc);
        
        size_t size_to_calculate = desc_size - sizeof(group_raw.bg_checksum);      
        return crc_16(Utils::as_byte_span(group_raw, size_to_calculate), current_crc);
    }
}

uint32_t Ext4::Checksums::checksum_bitmap(const std::span<const std::byte> bitmap, const uint32_t seed) {
    return crc_32c(bitmap, seed);
}

uint32_t Ext4::Checksums::checksum_inode(const Wrappers::Inode &inode, const uint32_t seed) {
    Raw::Inode raw_inode = inode.get_raw();
    raw_inode.i_checksum_hi = 0;
    raw_inode.i_osd2.l_i_checksum_lo = 0;

    std::vector<std::byte> raw_bytes;
    raw_bytes.append_range(Utils::as_byte_span(raw_inode));
    raw_bytes.append_range(inode.get_excess_bytes());

    uint32_t current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_id()), seed);
    current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_generation()), current_crc);
    return crc_32c(Utils::as_byte_span(raw_bytes), current_crc);
}

uint32_t Ext4::Checksums::checksum_dir(const Wrappers::Inode &inode, 
        const std::span<const std::byte> dir_block, 
        const uint32_t block_size, const uint32_t seed
    ) {
    // Calcula as sementes bases
    uint32_t current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_id()), seed);
    current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_generation()), current_crc);

    // Corta a cauda do cálculo
    auto data = dir_block.subspan(0, block_size - sizeof(Raw::DirectoryEntryTail));       
    return crc_32c(data, current_crc);
}

uint32_t Ext4::Checksums::checksum_extent(const Wrappers::Inode &inode, 
        const std::span<const std::byte> extent_block, 
        const uint32_t block_size, const uint32_t seed
    ) {
    // Calcula as sementes bases
    uint32_t current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_id()), seed);
    current_crc = crc_32c(Utils::as_byte_span(inode.get_inode_generation()), current_crc);

    // Corta EXATAMENTE os 4 bytes do checksum do final do bloco
    // (A cauda do extent_tail contém apenas o campo eb_checksum, que possui 4 bytes)
    auto data = extent_block.subspan(0, block_size - sizeof(Raw::ExtentTail));       
    // Calcula o checksum simulando os 4 bytes de zeros que deveriam estar ali no final.        
    return crc_32c(data, current_crc);
}