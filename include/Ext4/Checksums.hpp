#pragma once

#include <numeric>
#include <cstdint>
#include <bitset>
#include "Raw.hpp"
#include "Wrappers.hpp"

// O valor máximo que um `uint16_t` pode representar
inline constexpr uint16_t MAX_16BIT = std::numeric_limits<uint16_t>().max();

// O valor máximo que um `uint32_t` pode representar
inline constexpr uint32_t MAX_32BIT = std::numeric_limits<uint32_t>().max();

// O polinomial usado no CRC16 (CRC16 CCITT)
inline constexpr uint16_t POLY_16 = 0x1021;

// O polinomial usado no CRC32c (Castagnoli) (0x1EDC6F41 ^ 1)
inline constexpr uint32_t POLY_32c = 0x82F63B78; 

/**
 * Aqui te apresento as magias do C++:
 * Aparentemente, os algoritmos do CRC sofrem apenas pequenas mudanças entre si. MAS, o mecanismo em si é o mesmo.
 * Podemos nos aproveitar disso e fazer um template para generalizar a lógica :)
 * Para a função ser contruída, usamos o Concept do C++ 20.
 * Para chamar a função, 
 */
template <typename T>
concept CRC_config = requires {
    typename T::type;
    // Precisamos dizer a quantidade de bits
    { T::bits } -> std::convertible_to<size_t>;
    // Precisamos dizer qual polinomial usamos
    { T::poly } -> std::convertible_to<typename T::type>;
    // Se precisamos inverter os bits no final
    { T::do_flip } -> std::convertible_to<bool>;
    // Semente padrão caso não seja fornecida uma (que normalmente é o UUID)
    { T::default_seed } -> std::convertible_to<typename T::type>;
    // Se a iteração dos bits deve ser considerando o bit mais significativo ou o inverso
    { T::reflected } -> std::convertible_to<bool>;
} && std::unsigned_integral<typename T::type>; // Garante que é uint16_t, uint32_t, etc.

/**
 * @brief Configuração usada para o CRC16
 */
struct CRC16_Config {
    using type = uint16_t;

    static constexpr size_t bits = 16;
    static constexpr type poly = POLY_16;
    static constexpr bool reflected = false;
    static constexpr bool do_flip = false;
    static constexpr type default_seed = MAX_16BIT;
};

/**
 * @brief Configuração usada para o CRC32c
 */
struct CRC32c_Config {
    using type = uint32_t;

    static constexpr size_t bits = 32;
    static constexpr type poly = POLY_32c;
    static constexpr bool reflected = true;
    static constexpr bool do_flip = false;
    static constexpr type default_seed = MAX_32BIT;
};

template <CRC_config Config>
typename Config::type crc_generic(std::span<const std::byte> data, typename Config::type seed = Config::default_seed) {
    using T = typename Config::type;

    T crc = seed;

    for (std::byte b : data) {
        if constexpr (Config::reflected) {

            crc ^= static_cast<T>(std::to_integer<uint8_t>(b));

            for (unsigned i = 0; i < 8; ++i) {
                crc = (crc & static_cast<T>(1))
                    ? (crc >> 1) ^ Config::poly
                    : (crc >> 1);
            }

        } else {

            crc ^= static_cast<T>(std::to_integer<uint8_t>(b))
                << (Config::bits - 8);

            constexpr T msb = static_cast<T>(1) << (Config::bits - 1);

            for (unsigned i = 0; i < 8; ++i) {
                crc = (crc & msb)
                    ? (crc << 1) ^ Config::poly
                    : (crc << 1);
            }
        }
    }

    if constexpr (Config::do_flip) crc = ~crc;

    return crc;
}

/**
 * @cite http://www.ross.net/crc/download/crc_v3.txt
 * @cite https://en.wikipedia.org/wiki/Cyclic_redundancy_check#Implementations
 */
namespace Ext4::Checksums {
    /**
     * @brief Calcula o CRC16 do vetor de bytes fornecido (Refletido)
     * @param data Um vetor de bytes para leitura dos dados.
     * @param seed A semente usada no CRC (padrão = 0xFFFF).
     * @returns O checksum relacionado ao vetor de bytes.
     */
    uint16_t crc_16(const std::span<const std::byte> data, const uint16_t seed = MAX_16BIT);

    /**
     * @brief Calcula o CRC32c do vetor de bytes fornecido
     * @param data Um vetor de bytes para leitura dos dados.
     * @param seed A semente usada no CRC (padrão = 0xFFFFFFFF).
     * @returns O checksum relacionado ao vetor de bytes.
     */
    uint32_t crc_32c(const std::span<const std::byte> data, const uint32_t seed = MAX_32BIT);

    /**
     *  Calcula o checksum do superbloco
     *  @param super: O superbloco do ext4
     *  @returns  Inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_super_block(const Wrappers::SuperBlock &super);

    /**
     *  Calcula o checksum do descritor de grupos
     *  @param group O descritor de grupo
     *  @param csm_metadata Se a flag `RO_COMPAT_METADATA_CSUM` está ligada 
     *  @param seed A semente inicial para ser usada
     *  @returns  inteiro 16 bits que corresponde ao checksum
     */
    uint16_t checksum_group(const Wrappers::GroupDescriptor &group, const bool csm_metadata, const uint32_t seed);

    /**
     *  Calcula o checksum do mapa de bits
     *  @param bitmap Um vetor de bytes que correponde ao bitmap
     *  @param seed A semente inicial para ser usada
     *  @returns  inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_bitmap(const std::span<const std::byte> bitmap, const uint32_t seed);

    /**
     *  Calcula o checksum do inode
     *  @param inode O inode
     * @param seed A semente inicial para ser usada no lugar do UUID
     *  @returns  inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_inode(const Wrappers::Inode &inode, const uint32_t seed);

    /**
     * Calcula o checksum do diretório
     * @param inode O inode
     * @param dir_block O vetor de bytes que corresponde ao bloco com o diretório
     * @param block_size O tamanho do bloco
     * @param seed A semente inicial para ser usada no lugar do UUID
     * @returns  inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_dir(const Wrappers::Inode &inode, const std::span<const std::byte> dir_block, const uint32_t block_size, const uint32_t seed);

    /**
     * Calcula o checksum dos extents
     * @param inode: O inode
     * @param extent_block O vetor de bytes que corresponde ao bloco com o extent
     * @param block_size O tamanho do bloco
     * @param seed A semente inicial para ser usada no lugar do UUID
     * @returns Um inteiro 32 bits que corresponde ao checksum
     */
    uint32_t checksum_extent(const Wrappers::Inode &inode, const std::span<const std::byte> extent_block, const uint32_t block_size, const uint32_t seed);
}