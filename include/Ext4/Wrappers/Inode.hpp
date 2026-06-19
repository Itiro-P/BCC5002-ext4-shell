#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <span>
#include <optional>
#include "../../Ext4/Raw.hpp"

namespace Ext4::Wrappers {
    /**
     * @brief Encapsula um `Inode`. Operações de conveniência.
     */
    class Inode {
    private:
        // O número global do raw, começando em 2 para o diretório raiz.
        uint32_t inode_id = 0;
        // O uuid do volume.
        std::string _uuid;
        // O tamanho da estrutura do inode
        uint32_t _s_inode_size;
        // O inode encapsulado, contendo os metadados físicos do ficheiro ou diretório.
        Raw::Inode raw{};
        // Os bytes excedentes que vão além dos 160 bytes padrão do inode que o superbloco coloca para funcionalidades extras
        std::vector<std::byte> _excess_bytes;
    public:
        Inode() = default;
        
        Inode(uint32_t id, 
            const std::string &uuid, 
            const uint32_t s_inode_size, 
            const Raw::Inode &inode_data,
            const std::vector<std::byte> &excess_bytes) : inode_id(id), _uuid(uuid), _s_inode_size(s_inode_size), raw(inode_data), _excess_bytes(excess_bytes) {}
        
        /**
         * @brief Retorna o UUID do volume do inode em bytes. Esta é uma função de conveniência apenas.
         */
        std::span<const std::byte> get_uuid_bytes() const {
            return Utils::as_byte_span(this->_uuid, std::optional<unsigned int>(16));
        }

        /**
         * @brief Os bytes excedentes que vão além dos 160 bytes padrão do inode que o superbloco coloca para funcionalidades extras
         */
        std::vector<std::byte> get_excess_bytes() const {
            return _excess_bytes;
        }

        /**
         * @brief Retorna o checksum dos metadados gravados (caso tenha)
         */
        uint32_t get_checksum() const {
            return Utils::concatenate(this->raw.i_osd2.l_i_checksum_lo, this->raw.i_checksum_hi);
        }

        /**
         * @brief Retorna o identificador do inode.
         */
        uint32_t get_inode_id() const { return this->inode_id; }

        /**
         * @brief Retorna o tamanho da estrutura lida no disco.
         */
        uint32_t get_struct_size() const { return this->_s_inode_size; }

        /**
         * @brief Retorna a versão do ficheiro (utilizado principalmente para exportações NFS
         */
        uint32_t get_inode_generation() const { return this->raw.i_generation; }

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

        uint64_t get_size() const { return (static_cast<uint64_t>(this->raw.i_size_hi) << 32) | this->raw.i_size_lo; }

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