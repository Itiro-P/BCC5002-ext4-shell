#include "../include/Ext4.hpp"


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
    return ~crc_32c(sp_bytes);
}

uint16_t Ext4::Checksums::checksum_group(std::span<const std::byte, 16> uuid, const Wrappers::GroupDescriptor &group, const bool csm_metadata) {
    // Pegamos a struct crua e o número do grupo
    Raw::GroupDescriptor group_raw = group.get_raw();
    uint32_t group_num = group.get_group_number();
    uint16_t desc_size = group.get_desc_size();

    if(csm_metadata) {
       
        // Processa o UUID (Semente inicial limpa: MAX_32BIT)
        uint32_t current_crc = crc_32c(uuid, MAX_32BIT);
        
        // Para continuar o cálculo de forma idêntica ao .Update() contínuo,
        // nós NÃO usamos o '~' aqui! Passamos o valor direto.
        // E o group_num DEVE ser de 4 bytes (uint32_t)
        current_crc = crc_32c(Utils::as_byte_span(group_num), current_crc);
        
        // Processa o bloco inteiro do descritor (já com o bg_checksum zerado lá dentro)
        // respeitando o tamanho dinâmico (32 ou 64 bytes)
        auto desc_bytes = std::span<const std::byte>(reinterpret_cast<const std::byte*>(&group_raw), desc_size);
        uint32_t final_crc32 = crc_32c(desc_bytes, current_crc);
        
        // Retorna os 16 bits menos significativos
        return static_cast<uint16_t>(final_crc32 & MAX_16BIT);

    } else {
        uint16_t current_crc = crc_16(uuid, MAX_16BIT);
        
        uint16_t group_num_16 = static_cast<uint16_t>(group_num & 0xFFFF);
        current_crc = crc_16(Utils::as_byte_span(group_num_16), current_crc);
        
        size_t size_to_calculate = desc_size - sizeof(group_raw.bg_checksum);
        auto desc_bytes = std::span<const std::byte>(reinterpret_cast<const std::byte*>(&group_raw), size_to_calculate);
        
        return crc_16(desc_bytes, current_crc);
    }
}

// Arrumar
uint32_t Ext4::Checksums::checksum_bitmap(std::span<const std::byte, 16> uuid, std::span<const std::byte> bitmap) {
    // Simples: Usamos o crc32 para o UUID e depois passamos para o bitmap
    return crc_32c(bitmap, ~crc_32c(uuid));
}

// Arrumar
uint32_t Ext4::Checksums::checksum_inode(const Wrappers::Inode &inode) {
    uint32_t inode_number = inode.get_inode_id();
    uint32_t inode_gen = inode.get_inode_generation();
    Raw::Inode raw_inode = inode.get_raw();
    return 0;
}

// Arrumar
uint32_t Ext4::Checksums::checksum_dir(const Wrappers::Inode &inode, std::span<const std::byte> dir_block, const int32_t block_size) {
    // Calcula as sementes bases
    uint32_t seed = ~crc_32c(inode.get_uuid_bytes());
    seed = ~crc_32c(Utils::as_byte_span(inode.get_inode_id()), seed);
    seed = ~crc_32c(Utils::as_byte_span(inode.get_inode_generation()), seed);

    // Corta EXATAMENTE o tamanho do campo de checksum (4 bytes) do final do bloco
    auto data = dir_block.subspan(0, block_size - sizeof(Raw::DirectoryEntryTail::det_checksum));
    // Calcula o checksum simulando os 4 bytes de zeros que deveriam estar ali no final.        
    return crc_32c(ZEROS, ~crc_32c(data, seed));
}

// Arrumar
uint32_t Ext4::Checksums::checksum_extent(const Wrappers::Inode &inode, std::span<const std::byte> extent_block, const uint32_t block_size) {
    // Calcula as sementes bases
    uint32_t seed = ~crc_32c(inode.get_uuid_bytes());
    seed = ~crc_32c(Utils::as_byte_span(inode.get_inode_id()), seed);
    seed = ~crc_32c(Utils::as_byte_span(inode.get_inode_generation()), seed);

    // Corta EXATAMENTE os 4 bytes do checksum do final do bloco
    // (A cauda do extent_tail contém apenas o campo eb_checksum, que possui 4 bytes)
    auto data = extent_block.subspan(0, block_size - sizeof(Raw::ExtentTail::eb_checksum));       
    // Calcula o checksum simulando os 4 bytes de zeros que deveriam estar ali no final.        
    return crc_32c(ZEROS, ~crc_32c(data, seed));
}