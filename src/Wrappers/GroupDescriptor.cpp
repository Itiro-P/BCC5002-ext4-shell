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

uint16_t Ext4::Wrappers::GroupDescriptor::get_used_dirs_count() const {
    return _is_64
        ? Utils::concatenate(raw.bg_used_dirs_count_lo, raw.bg_used_dirs_count_hi)
        : raw.bg_used_dirs_count_lo;
}