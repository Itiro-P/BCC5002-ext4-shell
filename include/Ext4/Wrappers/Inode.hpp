#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <span>
#include "../../Ext4/Raw.hpp"

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

        bool is_dir() const {
            return this->get_type() == Flags::S_IFDIR;
        }

        bool is_file() const {
            return this->get_type() == Flags::S_IFREG;
        }

        uint64_t get_size() const {
            return (static_cast<uint64_t>(this->raw.i_size_high) << 32) | this->raw.i_size_lo;
        }

        uint16_t get_links_count() const { 
            return this->raw.i_links_count; 
        }

        uint32_t get_flags() const { 
            return this->raw.i_flags; 
        }

        /**
         * @brief Verifica se o Inode utiliza a estrutura de árvore de extents para mapear os blocos de dados, ou se utiliza o esquema tradicional de blocos diretos/indiretos.
         * @return `true` se o Inode utiliza extents, ou `false` se utiliza blocos diretos/indiretos.
         */
        bool has_extents() const {
            return (this->raw.i_flags & Flags::EXT4_EXTENTS_FL) != 0;
        }

        /**
         * @brief Retorna o bloco de dados do raw como um `std::array<std::byte, 60>`.
         */
        std::array<std::byte, 60> get_i_block() const {
            return this->raw.i_block;
        }
    };
}