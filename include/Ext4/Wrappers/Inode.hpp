#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <span>
#include "../../Ext4/Raw.hpp"
#include "../../Ext4/Checksum.hpp"

namespace Ext4::Wrappers {
    /**
     * @brief Encapsula um `Inode`. Operações de conveniência.
     */
    class Inode {
    private:
        // O número global do raw, começando em 2 para o diretório raiz.
        uint32_t inode_id = 0;
        // O inode encapsulado, contendo os metadados físicos do ficheiro ou diretório.
        Raw::Inode raw{};

    public:
        Inode() = default;
        
        Inode(uint32_t id, const Raw::Inode &inode_data) : inode_id(id), raw(inode_data) {}

        uint32_t get_inode_id() const { return this->inode_id; }

        uint32_t get_generation() const { return this->raw.i_generation; }

        /**
         * @brief Retorna o checksum guardado no inode.
         */
        uint32_t get_checksum() const {
            return Utils::concatenate(this->raw.i_osd2.l_i_checksum_lo, this->raw.i_checksum_hi);
        }

        /**
         * @brief Calcula o checksum e valida a estrutura interna.
         * @param uuid `std::span<std::byte>` correspondente ao UUID do superbloco
         */
        bool validate_checksum(std::span<const std::byte> uuid) {
            uint32_t checksum = Ext4::checksum_inode(uuid, this->get_inode_id(), this->get_raw().i_generation, Utils::as_byte_span(this->raw));
            return (this->get_checksum() == checksum);
        }

        /**
         * @brief Retorna a estrutura interna do inode.
         */
        Raw::Inode get_raw() const { return raw; }

        /**
         * @brief Troca a estrutura relacionada.
         */
        void set_raw(const Raw::Inode &inode) { this->raw = inode; }

        uint16_t get_type() const {
            return static_cast<uint16_t>(this->raw.i_mode  &Constants::S_IFMT);
        }

        uint16_t get_mode() const { return this->raw.i_mode; }

        uint16_t get_uid() const { return this->raw.i_uid; }

        uint16_t get_gid() const { return this->raw.i_gid; }

        uint32_t get_atime() const { return this->raw.i_atime; }

        uint32_t get_ctime() const { return this->raw.i_ctime; }

        uint32_t get_mtime() const { return this->raw.i_mtime; }

        bool is_dir() const { return this->get_type() == Flags::S_IFDIR; }

        bool is_file() const { return this->get_type() == Flags::S_IFREG; }

        uint64_t get_size() const { return Utils::concatenate(this->raw.i_size_lo, this->raw.i_size_hi); }

        uint16_t get_links_count() const { return this->raw.i_links_count; }

        uint32_t get_flags() const { return this->raw.i_flags; }

        /**
         * @brief Verifica se o Inode utiliza a estrutura de árvore de extents para mapear os blocos de dados, ou se utiliza o esquema tradicional de blocos diretos/indiretos.
         * @returns `true` se o Inode utiliza extents, ou `false` se utiliza blocos diretos/indiretos.
         */
        bool has_extents() const { return (this->raw.i_flags & Flags::EXT4_EXTENTS_FL) != 0; }

        /**
         * @brief Retorna o bloco de dados do raw como um `std::array<std::byte, 60>`.
         */
        std::array<std::byte, 60> get_i_block() const { return this->raw.i_block; }
    };
}