#include "../../include/Ext4.hpp"
#include "../../include/Utils.hpp"
#include <bitset>

/**
 * @file    GroupDescriptor.cpp
 * @brief   Implementação da classe GroupDescriptor — manipulação dos grupos de descritores
 * @author  Pedro Itiro Nagao
 * @date    2025-06-04
 *
 * Classe de conveniência para não precisar calcular manualmente bits concatenados dos campos.
 */

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

uint32_t Ext4::Wrappers::GroupDescriptor::get_itable_unused() const {
    return _is_64
        ? Utils::concatenate(raw.bg_itable_unused_lo, raw.bg_itable_unused_hi)
        : raw.bg_itable_unused_lo;
}

uint16_t Ext4::Wrappers::GroupDescriptor::get_used_dirs_count() const {
    return _is_64
        ? Utils::concatenate(raw.bg_used_dirs_count_lo, raw.bg_used_dirs_count_hi)
        : raw.bg_used_dirs_count_lo;
}

uint32_t Ext4::Wrappers::GroupDescriptor::get_inode_bitmap_checksum() const {
    return _is_64
        ? Utils::concatenate(raw.bg_inode_bitmap_csum_lo, raw.bg_inode_bitmap_csum_hi)
        : raw.bg_inode_bitmap_csum_lo;
}

uint32_t Ext4::Wrappers::GroupDescriptor::get_block_bitmap_checksum() const {
    return _is_64
        ? Utils::concatenate(raw.bg_block_bitmap_csum_lo, raw.bg_block_bitmap_csum_hi)
        : raw.bg_block_bitmap_csum_lo;
}