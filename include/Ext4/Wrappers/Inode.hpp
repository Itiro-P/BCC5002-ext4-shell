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
        // O número global do inode, começando em 2 para o diretório raiz.
        uint32_t inode_id = 0;
        // O inode encapsulado, contendo os metadados físicos do ficheiro ou diretório.
        Raw::Inode inode{};

    public:
        Inode() = default;
        
        Inode(uint32_t id, const Raw::Inode &inode_data) : inode_id(id), inode(inode_data) {}

        uint32_t get_inode_id() const { return this->inode_id; }

        const Raw::Inode &get_inode() const { return this->inode; }

        uint16_t get_type() const {
            return static_cast<uint16_t>(this->inode.i_mode  &Constants::S_IFMT);
        }

        bool is_dir() const {
            return this->get_type() == Flags::S_IFDIR;
        }

        bool is_file() const {
            return this->get_type() == Flags::S_IFREG;
        }

        uint64_t get_size() const {
            return (static_cast<uint64_t>(this->inode.i_size_high) << 32) | this->inode.i_size_lo;
        }

        uint16_t get_links_count() const { 
            return this->inode.i_links_count; 
        }

        uint32_t get_flags() const { 
            return this->inode.i_flags; 
        }

        /**
         * @brief Verifica se o Inode utiliza a estrutura de árvore de extents para mapear os blocos de dados, ou se utiliza o esquema tradicional de blocos diretos/indiretos.
         * @return `true` se o Inode utiliza extents, ou `false` se utiliza blocos diretos/indiretos.
         */
        bool has_extents() const {
            return (this->inode.i_flags  &Flags::EXT4_EXTENTS_FL) != 0;
        }

        /**
         * @brief Retorna o bloco de dados do inode como um `std::array<std::byte, 60>`.
         */
        std::array<std::byte, 60> get_i_block() const {
            return this->inode.i_block;
        }
    };
}