#include "../../include/Ext4.hpp"
#include "../../include/Utils.hpp"
#include <bitset>


uint64_t Ext4::Wrappers::GroupDescriptor::get_block_bitmap_block() const {
    return _is_64
        ? Utils::concatenate(raw.bg_block_bitmap_lo, raw.bg_block_bitmap_hi)
        : raw.bg_block_bitmap_lo;
}

uint64_t Ext4::Wrappers::GroupDescriptor::get_inode_bitmap_block() const {
    return _is_64
        ? Utils::concatenate(raw.bg_inode_bitmap_lo, raw.bg_inode_bitmap_hi)
        : raw.bg_inode_bitmap_lo;
}

uint64_t Ext4::Wrappers::GroupDescriptor::get_inode_table_block() const {
    return _is_64
        ? Utils::concatenate(raw.bg_inode_table_lo, raw.bg_inode_table_hi)
        : raw.bg_inode_table_lo;
}

uint32_t Ext4::Wrappers::GroupDescriptor::get_free_blocks_count() const {
    return _is_64
        ? Utils::concatenate(raw.bg_free_blocks_count_lo, raw.bg_free_blocks_count_hi)
        : raw.bg_free_blocks_count_lo;
}

uint32_t Ext4::Wrappers::GroupDescriptor::get_free_inodes_count() const {
    return _is_64
        ? Utils::concatenate(raw.bg_free_inodes_count_lo, raw.bg_free_inodes_count_hi)
        : raw.bg_free_inodes_count_lo;
}


uint32_t Ext4::Wrappers::GroupDescriptor::get_first_free_inode(std::span<const std::byte> bitmap_block, uint32_t inodes_per_group) const {
    if(this->is_inode_uninit()) return 1;
    
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(bitmap_block.data());
    for (uint32_t i = 0; i < inodes_per_group; ++i) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        // Se o bit for 0, encontramos um espaço livre!
        if ((bytes[byte_idx] & (1 << bit_idx)) == 0) {
            return i + 1; // Retorna índice baseado em 1
        }
    }
    return 0; // Grupo cheio
}

uint64_t Ext4::Wrappers::GroupDescriptor::get_first_free_block(std::span<const std::byte> bitmap_block, uint32_t blocks_per_group) const {
    if(this->is_block_uninit()) return 1;
    
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(bitmap_block.data());
    for (uint32_t i = 0; i < blocks_per_group; ++i) {
        uint32_t byte_idx = i / 8;
        uint32_t bit_idx = i % 8;
        
        if ((bytes[byte_idx] & (1 << bit_idx)) == 0) {
            return i + 1; // Retorna índice baseado em 1
        }
    }
    return 0; // Grupo cheio
}

void Ext4::Wrappers::GroupDescriptor::set_bitmap_bit(std::span<std::byte> bitmap_block, uint32_t local_index, bool occupied) {
    uint32_t i = local_index - 1; // Transforma o índice baseado em 1 de volta para base 0
    uint32_t byte_idx = i / 8;
    uint32_t bit_idx = i % 8;
    
    uint8_t* bytes = reinterpret_cast<uint8_t*>(bitmap_block.data());
    
    if (occupied) bytes[byte_idx] |= (1 << bit_idx);  // Força o bit a virar 1
    else bytes[byte_idx] &= ~(1 << bit_idx); // Força o bit a virar 0
}

uint16_t Ext4::Wrappers::GroupDescriptor::get_used_dirs_count() const {
    return _is_64
        ? Utils::concatenate(raw.bg_used_dirs_count_lo, raw.bg_used_dirs_count_hi)
        : raw.bg_used_dirs_count_lo;
}