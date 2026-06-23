#pragma once

#include "../Raw/GroupDescriptor.hpp"
#include "../../Utils.hpp"
#include <cstdint>

namespace Ext4::Wrappers {
    class GroupDescriptor {
        Raw::GroupDescriptor raw;
        uint32_t _group_number;
        uint16_t _desc_size;
        bool _is_64;
    public:
        GroupDescriptor(const Raw::GroupDescriptor &raw, const uint32_t group_number, const uint16_t desc_size, const bool is_64): raw(raw), _group_number(group_number), _desc_size(desc_size), _is_64(is_64) {}

        /**
         * @brief Retorna o tamanho do grupo de descritores.
         * @return Um inteiro de 16 bits sem sinal que corresponde ao tamanho da estrutura GD.
         */
        uint16_t get_desc_size() const {
            return this->_desc_size;
        }

        /**
         * @brief Retorna o checksum dos metadados gravados (caso tenha)
         */
        uint16_t get_checksum() const {
            return this->raw.bg_checksum;
        }

        /**
         * @brief Retorna o número que representa esste grupo.
         * @return Um inteiro de 32 bits sem sinal que corresponde ao ID desse grupo.
         */
        uint32_t get_group_number() const {
            return this->_group_number;
        }

        /**
         * @brief Retorna o número do bloco físico que contém o bitmap de blocos deste grupo. Para imagens EXT4 de 64 bits, este valor é composto por bg_block_bitmap_lo e bg_block_bitmap_hi. Para imagens sem suporte a 64 bits, apenas bg_block_bitmap_lo é usado.
         * Este valor é crucial para calcular o offset absoluto do bitmap de blocos no disco.
         */
        uint64_t get_block_bitmap_block() const;

        /**
         * @brief Retorna o número do bloco físico que contém o bitmap de inodes deste grupo. Para imagens EXT4 de 64 bits, este valor é composto por bg_inode_bitmap_lo e bg_inode_bitmap_hi. Para imagens sem suporte a 64 bits, apenas bg_inode_bitmap_lo é usado.
         * Este valor é crucial para calcular o offset absoluto do bitmap de inodes no disco.
         */
        uint64_t get_inode_bitmap_block() const;

        /**
         * @brief Retorna o número do primeiro bloco físico que inicia a tabela de inodes deste grupo.
         * Para imagens EXT4 de 64 bits, este valor é composto por bg_inode_table_lo e bg_inode_table_hi. Para imagens sem suporte a 64 bits, apenas bg_inode_table_lo é usado.
         * Este valor é crucial para calcular o offset absoluto dos inodes no disco, já que a tabela de inodes é onde os metadados dos arquivos e diretórios são
         */
        uint64_t get_inode_table_block() const;

        /**
         * @brief Retorna a quantidade de blocos livres neste grupo. Para imagens EXT4 de 64 bits, este valor é composto por bg_free_blocks_count_lo e bg_free_blocks_count_hi. Para imagens sem suporte a 64 bits, apenas bg_free_blocks_count_lo é usado.
         * Este valor é exibido em `info` e deve ser decrementado ao alocar blocos para arquivos ou diretórios.
         */
        uint32_t get_free_blocks_count() const;

        /**
         * @brief Retorna a quantidade de inodes livres neste grupo. Para imagens EXT4 de 64 bits, este valor é composto por bg_free_inodes_count_lo e bg_free_inodes_count_hi. Para imagens sem suporte a 64 bits, apenas bg_free_inodes_count_lo é usado.
         * Este valor é exibido em `info` e deve ser decrementado ao alocar inodes para arquivos ou diretórios.
         */
        uint32_t get_free_inodes_count() const;

        /**
         * @brief Retorna a quantidade de inodes não usados no final da tabela.
         */
        uint32_t get_itable_unused() const;

        /**
         * @brief Retorna a quantidade de inodes que são diretórios neste grupo. Para imagens EXT4 de 64 bits, este valor é composto por bg_used_dirs_count_lo e bg_used_dirs_count_hi. Para imagens sem suporte a 64 bits, apenas bg_used_dirs_count_lo é usado.
         * Este valor é exibido em `info` para dar uma ideia da quantidade de diretórios presentes no grupo, mas não é usado para controle de alocação.
         */
        uint16_t get_used_dirs_count() const;

        /**
         * @brief Flags — bg_flags indica se o grupo tem bitmap/inode table inicializados 
         * útil para detectar grupos não inicializados em imagens esparsas ou corrompidas.
         */
        uint16_t get_flags() const { 
            return raw.bg_flags; 
        }

        /**
         * @brief Retorna o checksum do bitmap de inodes
         */
        uint32_t get_inode_bitmap_checksum() const;

        /**
         * @brief Retorna o checksum do bitmap de blocos
         */
        uint32_t get_block_bitmap_checksum() const;


        /**
         * Verifica se o bitmap de inodes do grupo NÃO está inicializado.
         */
        bool is_inode_uninit() const { 
            return raw.bg_flags  & Flags::BG_INODE_UNINIT; 
        }
        /**
         * Verifica se o bitmap de blocos do grupo NÃO está inicializado.
         */
        bool is_block_uninit() const { 
            return raw.bg_flags  & Flags::BG_BLOCK_UNINIT; 
        }

        /**
         * @brief Retorna a estrutura interna do GD.
         */
        Raw::GroupDescriptor get_raw() const { return raw; }

        /**
         * @brief Troca a estrutura relacionada.
         */
        void set_raw(const Raw::GroupDescriptor &gd) { this->raw = gd; }
    };
}