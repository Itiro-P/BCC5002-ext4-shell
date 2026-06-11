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

#include "../Ext4.hpp"
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
    /* calculo dos checksums para ext4 */
    uint32_t checksum_superblock(char* super);
    uint16_t checksum_group(char* uuid, int32_t group_number, char* group);
    uint32_t checksum_bitmap(char* uuid, char* bitmap, int size);
    uint32_t checksum_inode(char* uuid, uint32_t inode_number, uint32_t inode_gen, char* inode);
    uint32_t checksum_dir(char* uuid, uint32_t inode_number, uint32_t inode_gen, char* dir, int blocksize);
    uint32_t checksum_extent (char* uuid, uint32_t inode_number, uint32_t inode_gen, char* extent, int blocksize);
}