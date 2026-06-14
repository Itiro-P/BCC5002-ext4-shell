#include "../../include/Ext4/Wrappers/SuperBlock.hpp"
#include "../../include/Ext4/Raw/SuperBlock.hpp"
#include "../../include/Ext4/Constants.hpp"
#include "../../include/Ext4/Checksums.hpp"
#include <stdexcept>
#include <string>
#include <format>
#include <cstring>
#include <print>

Ext4::Wrappers::SuperBlock::SuperBlock(const Raw::SuperBlock &raw_super_block) : raw(raw_super_block) {
    this->_is_64 = (raw.s_feature_incompat  &Flags::SuperBlockFlags::IncompatFeature::INCOMPAT_64BIT) != 0;
}

Ext4::Raw::SuperBlock Ext4::Wrappers::SuperBlock::get_raw() const {
    return this->raw;
}

void Ext4::Wrappers::SuperBlock::validate() const {
    if (this->raw.s_magic != Ext4::Constants::EXT_MAGIC) {
        throw std::runtime_error("Erro: SuperBlock inválido - Assinatura mágica incorreta. O sistema de arquivos pode estar corrompido ou não ser um EXT4.");
    }
    // Se temos suporte a 64 bits, então temos checksum de metadados. Checando...
    if (this->_is_64 && this->has_metadata_csum()) {
        uint32_t checksum = Checksums::checksum_super_block(*this);
        if (this->get_raw().s_checksum != checksum) {
            throw std::runtime_error(
                std::format("Erro: SuperBlock inválido - Checksum incorreto. O sistema de arquivos pode estar corrompido ou não ser um EXT4.\nGravado: {}; Obtido: {}\n", this->get_raw().s_checksum, checksum));
        }
    } else {
        std::println("Imagem não suporta checksum de metadados. Pulando verfificação...");
    }
}

bool Ext4::Wrappers::SuperBlock::has_extents() const {
    return raw.s_feature_incompat  &Flags::SuperBlockFlags::IncompatFeature::INCOMPAT_EXTENTS;
}

bool Ext4::Wrappers::SuperBlock::is_64bit() const {
    return this->_is_64;
}

bool Ext4::Wrappers::SuperBlock::has_dir_htree() const {
    return raw.s_feature_compat &Flags::SuperBlockFlags::CompatFeature::COMPAT_DIR_INDEX;
}

bool Ext4::Wrappers::SuperBlock::has_large_file() const {
    return raw.s_feature_ro_compat  &Flags::SuperBlockFlags::RoCompatFeature::RO_COMPAT_LARGE_FILE;
}

bool Ext4::Wrappers::SuperBlock::has_huge_file() const {
    return raw.s_feature_ro_compat  &Flags::SuperBlockFlags::RoCompatFeature::RO_COMPAT_HUGE_FILE;
}

bool Ext4::Wrappers::SuperBlock::has_metadata_csum() const {
    return raw.s_feature_ro_compat  &Flags::SuperBlockFlags::RoCompatFeature::RO_COMPAT_METADATA_CSUM;
}

bool Ext4::Wrappers::SuperBlock::has_compat_gdt_csum() const {
    return raw.s_feature_ro_compat  &Flags::SuperBlockFlags::RoCompatFeature::RO_COMPAT_GDT_CSUM;
}

uint32_t Ext4::Wrappers::SuperBlock::get_inodes_count() const {
    return raw.s_inodes_count;
}

uint32_t Ext4::Wrappers::SuperBlock::get_free_inodes_count() const { 
    return raw.s_free_inodes_count; 

}

uint64_t Ext4::Wrappers::SuperBlock::get_blocks_count() const {
    return _is_64
        ? Utils::concatenate(raw.s_blocks_count_lo, raw.s_blocks_count_hi)
        : raw.s_blocks_count_lo;
}

uint64_t Ext4::Wrappers::SuperBlock::get_free_blocks_count() const {
    return _is_64
        ? Utils::concatenate(raw.s_free_blocks_count_lo, raw.s_free_blocks_count_hi)
        : raw.s_free_blocks_count_lo;
}

uint64_t Ext4::Wrappers::SuperBlock::get_reserved_blocks_count() const {
    return _is_64
        ? Utils::concatenate(raw.s_r_blocks_count_lo, raw.s_r_blocks_count_hi)
        : raw.s_r_blocks_count_lo;
}

uint32_t Ext4::Wrappers::SuperBlock::get_block_size() const {
    return 1024 << raw.s_log_block_size;
}

uint32_t Ext4::Wrappers::SuperBlock::get_blocks_per_group() const {
    return raw.s_blocks_per_group;
}

uint32_t Ext4::Wrappers::SuperBlock::get_inodes_per_group() const {
    return raw.s_inodes_per_group;
}

uint32_t Ext4::Wrappers::SuperBlock::get_clusters_per_group() const {
    return raw.s_clusters_per_group;
}

uint32_t Ext4::Wrappers::SuperBlock::get_inode_size() const {
    return raw.s_inode_size;
}

uint16_t Ext4::Wrappers::SuperBlock::get_desc_size() const {
    return raw.s_desc_size > 0 ? raw.s_desc_size : 32;
}

uint32_t Ext4::Wrappers::SuperBlock::get_first_data_block() const {
    return raw.s_first_data_block;
}

uint64_t Ext4::Wrappers::SuperBlock::get_gdt_offset() const {
    return static_cast<uint64_t>(raw.s_first_data_block + 1) * get_block_size();
}

uint32_t Ext4::Wrappers::SuperBlock::get_group_count() const {
    return static_cast<uint32_t>(
        (get_blocks_count() + raw.s_blocks_per_group - 1) / raw.s_blocks_per_group
    );
}

uint32_t Ext4::Wrappers::SuperBlock::get_first_ino() const {
    return raw.s_first_ino;
}

uint16_t Ext4::Wrappers::SuperBlock::get_min_extra_isize() const {
    return raw.s_min_extra_isize;
}

std::string Ext4::Wrappers::SuperBlock::get_volume_name() const {
    return std::string(raw.s_volume_name.data(), 
        strnlen(raw.s_volume_name.data(), raw.s_volume_name.size()));
}

std::string Ext4::Wrappers::SuperBlock::get_uuid() const {
    const auto u = std::span<const uint8_t>(raw.s_uuid.data(), raw.s_uuid.size());
    return std::format(
        "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}"
        "-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
        u[0],u[1],u[2],u[3], u[4],u[5], u[6],u[7],
        u[8],u[9], u[10],u[11],u[12],u[13],u[14],u[15]);
}

std::span<const std::byte> Ext4::Wrappers::SuperBlock::get_uuid_bytes() const {
    return Utils::as_byte_span(raw.s_uuid);
}

uint32_t Ext4::Wrappers::SuperBlock::get_checksum_seed() const {
    return this->has_checksum_seed() 
        ? raw.s_checksum_seed 
        : Checksums::crc_32c(Utils::as_byte_span(raw.s_uuid));
}

bool Ext4::Wrappers::SuperBlock::has_checksum_seed() const {
    return (raw.s_feature_incompat & Flags::SuperBlockFlags::IncompatFeature::INCOMPAT_CKSUM_SEED) != 0;
}

uint32_t Ext4::Wrappers::SuperBlock::get_mkfs_time() const {
    return raw.s_mkfs_time;
}

uint32_t Ext4::Wrappers::SuperBlock::get_mtime() const { 
    return raw.s_mtime; 
}

uint32_t Ext4::Wrappers::SuperBlock::get_wtime() const { 
    return raw.s_wtime; 
}

uint32_t Ext4::Wrappers::SuperBlock::get_lastcheck() const { 
    return raw.s_lastcheck; 
}

uint16_t Ext4::Wrappers::SuperBlock::get_state() const { 
    return raw.s_state; 
}

uint16_t Ext4::Wrappers::SuperBlock::get_mnt_count() const { 
    return raw.s_mnt_count; 
}

uint16_t Ext4::Wrappers::SuperBlock::get_max_mnt_count() const { 
    return raw.s_max_mnt_count; 
}

uint32_t Ext4::Wrappers::SuperBlock::get_rev_level() const { 
    return raw.s_rev_level; 
}