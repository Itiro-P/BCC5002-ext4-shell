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
         * @brief Retorna o nome da entrada.
         */
        std::string get_name() const& {
            return this->name;
        }
    };
};