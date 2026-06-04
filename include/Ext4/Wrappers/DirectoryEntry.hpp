#pragma once

#include "../../Ext4/Raw/Directory.hpp"
#include <string>

namespace Ext4::Wrappers {
    /**
     * @brief Encapsula uma entrada de diretório.
     */
    class DirectoryEntry {
        Raw::DirectoryEntry raw;
        std::string name;

        public:
        DirectoryEntry(const Raw::DirectoryEntry &raw, const std::string &name): raw(raw), name(name) {}

        /**
         * @brief Retorna o ID do inode relacionado a esta entrada.
         */
        uint32_t get_inode() const {
            return this->raw.inode;
        }

        /**
         * @brief Retorna a estrutura crua relacionada.
         */
        Raw::DirectoryEntry get_raw() const { return this->raw; }

        /**
         * @brief Troca a estrutura relacionada.
         */
        void set_raw(const Raw::DirectoryEntry &entry) { this->raw = entry; }

        /**
         * @brief Retorna o nome da entrada.
         */
        std::string get_name() const {
            return this->name;
        }

        /**
         * @brief Retorna o tipo do arquivo.
         */
        uint8_t get_type() const {
            return this->raw.file_type;
        }

        /**
         * @brief Retorna se o tipo da entrada é um diretório.
         */
        bool is_dir() const {
            return this->raw.file_type == Raw::DirectoryFileType::EXT4_FT_DIR;
        }

        /**
         * @brief Retorna se o tipo da entrada é um arquivo.
         */
        bool is_file() const {
            return this->raw.file_type == Raw::DirectoryFileType::EXT4_FT_REG_FILE;
        }

        /**
         * @brief Retorna o tamanho usado pela entrada. Alinhado a um múltiplo de 4.
         */
        uint32_t get_used_size() const {
            uint32_t bytes_needed = sizeof(Raw::DirectoryEntry) + static_cast<uint32_t>(this->get_name().size());
            // Truquezinho: Múltiplos de 4 sempre terminam com 00 à direita.
            // Somar 3 a um número faz com que ele entre na "próxima" casa de um múltiplo de 4.
            // Mascarar com o complemento de 1 do 3 resulta na limpeza dos primeiros 2 bits à direita.
            return (bytes_needed + 3) & ~3;
        }

        /**
         * @brief Retorna o tamanho que foi alocado para essa entrada. Esse número costuma sobrar quando a entrada é a última no bloco.
         */
        uint32_t get_allocated_size() const {
            return static_cast<uint32_t>(this->get_raw().rec_len);
        }

        /**
         * @brief Retorna o espaço livre. Que é, basicamente, o padding da última entrada até o final do bloco.
         */
        uint32_t get_free_space() const {
            return this->get_allocated_size() - this->get_used_size();
        }
    };
};