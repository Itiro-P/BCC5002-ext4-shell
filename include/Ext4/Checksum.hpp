/**
 * @file Checksum.hpp
 * @author Rodrigo Campiolo (@rcampiolo), Pedro Itiro Nagao
 * @brief Calcula o checksum (crc32c) para estruturas do ext4. Versão modificada para se adequar aos padrões do Ext4Shell.
 * @version 0.1
 * @date 2023-10-12, 2026-06-10 (Atualizado, PIN)
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

#include <span>
#include <cryptopp/crc.h>
#include <cstdint>

/**
 * Converte um array byte em int32 little endian
 * @param b: vetor de tamanho 4 que representa o checksum em bytes
 * @returns inteiro 32 bits que corresponde ao checksum
 */
uint32_t bytearray_to_int32_le(std::span<const std::byte, 4> b);

/**
 * Função helper para converter um `std::span<std::byte, 4>` para `CryptoPP::byte*`.
 * @param bytes Um `std::span<std::byte, 4>` com bytes.
 * @returns Um ponteiro representando o span de bytes.
 */
inline constexpr CryptoPP::byte* to_crc_byte(std::span<std::byte, 4> bytes);

namespace Ext4 {
    /**
     *  Calcula o checksum do superbloco
     *  @param super: `std::span<const std::byte>` que representa o superbloco do ext4
     *  @returns  inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_superblock(std::span<const std::byte> super);
    
    /**
     *  Calcula o checksum do descritor de grupos
     *  @param uuid: vetor de bytes (tamanho 16) que corresponde ao uuid do superbloco
     *  @param group_number: número do grupo
     *  @param group: `std::span<const std::byte>` que corresponde ao descritor do grupo group_number
     *  @returns  inteiro 16 bits que corresponde ao checksum
     */
    uint16_t checksum_group(std::span<const std::byte> uuid, int32_t group_number, std::span<const std::byte> group);
    
    /**
     * Calcula o checksum do bitmap
     * @param uuid: `std::span<std::byte>` correspondente ao UUID do superbloco
     * @param bitmap: span dinâmico contendo os bytes do bitmap (bloco ou inode)
     * @returns inteiro de 32 bits correspondente ao checksum
     */
    uint32_t checksum_bitmap(std::span<const std::byte> uuid, std::span<const std::byte> bitmap);

    /**
     * Calcula o checksum do inode
     * @param uuid: `std::span<std::byte>` correspondente ao UUID do superbloco
     * @param inode_number: número do inode
     * @param inode_gen: campo do inode i_generation
     * @param inode: span contendo os bytes do inode (deve ter pelo menos 256 bytes)
     * @returns inteiro de 32 bits correspondente ao checksum
     */
    uint32_t checksum_inode(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> inode);
    
    /**
     * Calcula o checksum do diretório
     * @param uuid: `std::span<std::byte>` correspondente ao UUID do superbloco
     * @param inode_number: número do inode
     * @param inode_gen: campo do inode i_generation
     * @param dir: span contendo os bytes do bloco do diretório (tamanho dinâmico)
     * @returns inteiro de 32 bits correspondente ao checksum
     */
    uint32_t checksum_dir(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> dir);

    /**
     * Calcula o checksum dos extents
     * @param uuid: `std::span<std::byte>` correspondente ao UUID do superbloco
     * @param inode_number: número do inode
     * @param inode_gen: campo do inode i_generation
     * @param extent: span contendo os bytes do bloco de extents (tamanho dinâmico)
     * @returns inteiro de 32 bits correspondente ao checksum
     */
    uint32_t checksum_extent(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> extent);
}