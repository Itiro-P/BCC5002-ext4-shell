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
    };
};