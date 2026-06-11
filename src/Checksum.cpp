#include "../include/Ext4/Checksum.hpp"
#include <bit>

using namespace CryptoPP;

CRC32C crc;

std::array<std::byte, 4> digest{};

uint32_t bytearray_to_int32_le(std::span<const std::byte, 4> b) {
    return std::bit_cast<uint32_t>(digest);
}

inline constexpr CryptoPP::byte* to_crc_byte(std::span<std::byte, 4> bytes) {
    return reinterpret_cast<CryptoPP::byte*>(bytes.data());
}

namespace Ext4 {
    uint32_t checksum_superblock(std::span<const std::byte> super) {
        crc.Restart();
        crc.Update(reinterpret_cast<const byte*>(super.data()), super.size());
        crc.Final(to_crc_byte(digest));

        return (bytearray_to_int32_le(digest) ^ 0xFFFFFFFF);
    }

    uint16_t checksum_group(std::span<const std::byte> uuid, int32_t group_number, std::span<const std::byte> group) {
        uint16_t dummy_csum = 0;
        crc.Restart();
    
        // 1. UUID
        crc.Update(reinterpret_cast<const byte*>(uuid.data()), uuid.size());
        
        // 2. Group Number
        crc.Update(reinterpret_cast<const byte*>(&group_number), sizeof(group_number));
        
        // 3. Primeira parte do grupo (bytes 0 a 29)
        auto first_partition = group.subspan(0, 30);
        crc.Update(reinterpret_cast<const byte*>(first_partition.data()), first_partition.size());
        
        // 4. Checksum zerado
        crc.Update(reinterpret_cast<const byte*>(&dummy_csum), sizeof(dummy_csum));
        
        // 5. Segunda parte do grupo (do byte 32 em diante, pegando 32 bytes)
        auto second_partition = group.subspan(32, 32);
        crc.Update(reinterpret_cast<const byte*>(second_partition.data()), second_partition.size());
        crc.Final(to_crc_byte(digest));

        return ((bytearray_to_int32_le(digest) ^ 0xFFFFFFFF) & 0XFFFF);
    }

    uint32_t checksum_bitmap(std::span<const std::byte> uuid, std::span<const std::byte> bitmap) {
        crc.Restart();
        
        // 1. UUID (estático, sempre 16 bytes)
        crc.Update(reinterpret_cast<const byte*>(uuid.data()), uuid.size());
        
        // 2. Bitmap (dinâmico, usa o tamanho embutido no próprio span)
        crc.Update(reinterpret_cast<const byte*>(bitmap.data()), bitmap.size());
        
        crc.Final(to_crc_byte(digest));

        return (bytearray_to_int32_le(digest) ^ 0xFFFFFFFF);
    }

    uint32_t checksum_inode(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> inode) {
        uint16_t dummy_csum = 0;

        crc.Restart();
        
        // 1. UUID (16 bytes)
        crc.Update(reinterpret_cast<const byte*>(uuid.data()), uuid.size());
        
        // 2. Inode Number (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_number), sizeof(inode_number));
        
        // 3. Inode Generation (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_gen), sizeof(inode_gen));
        
        // 4. Inode: bytes 0 a 123
        auto parte_1 = inode.subspan(0, 124);
        crc.Update(reinterpret_cast<const byte*>(parte_1.data()), parte_1.size());
        
        // 5. Checksum zerado inferior (2 bytes)
        crc.Update(reinterpret_cast<const byte*>(&dummy_csum), sizeof(dummy_csum));
        
        // 6. Inode: bytes 126 a 129
        auto parte_2 = inode.subspan(126, 4);
        crc.Update(reinterpret_cast<const byte*>(parte_2.data()), parte_2.size());
        
        // 7. Checksum zerado superior (2 bytes)
        crc.Update(reinterpret_cast<const byte*>(&dummy_csum), sizeof(dummy_csum));
        
        // 8. Inode: bytes 132 até 255 (124 bytes)
        auto parte_3 = inode.subspan(132, 124);
        crc.Update(reinterpret_cast<const byte*>(parte_3.data()), parte_3.size());
        
        crc.Final(to_crc_byte(digest));

        return (bytearray_to_int32_le(digest) ^ 0xFFFFFFFF);
    }

    uint32_t checksum_dir(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> dir) {
        crc.Restart();
        
        // 1. UUID (16 bytes)
        crc.Update(reinterpret_cast<const byte*>(uuid.data()), uuid.size());
        
        // 2. Inode Number (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_number), sizeof(inode_number));
        
        // 3. Inode Generation (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_gen), sizeof(inode_gen));
        
        // 4. Diretório: Todo o bloco, exceto os últimos 12 bytes da estrutura ext4_dir_entry_tail
        auto dir_data = dir.subspan(0, dir.size() - 12);
        crc.Update(reinterpret_cast<const byte*>(dir_data.data()), dir_data.size());
        
        crc.Final(to_crc_byte(digest));

        return (bytearray_to_int32_le(digest) ^ 0xFFFFFFFF);
    }

    uint32_t checksum_extent(std::span<const std::byte> uuid, uint32_t inode_number, uint32_t inode_gen, std::span<const std::byte> extent) {
        
        crc.Restart();
        
        // 1. UUID (16 bytes)
        crc.Update(reinterpret_cast<const byte*>(uuid.data()), uuid.size());
        
        // 2. Inode Number (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_number), sizeof(inode_number));
        
        // 3. Inode Generation (4 bytes)
        crc.Update(reinterpret_cast<const byte*>(&inode_gen), sizeof(inode_gen));
        
        // 4. Extent: calcula a posição limite do checksum baseada no tamanho do bloco
        size_t tamanho_bloco = extent.size();
        size_t tamanho_calculado = tamanho_bloco - (tamanho_bloco % 12);
        
        auto dados_extent = extent.subspan(0, tamanho_calculado);
        crc.Update(reinterpret_cast<const byte*>(dados_extent.data()), dados_extent.size());
        
        crc.Final(to_crc_byte(digest));

        return (bytearray_to_int32_le(digest) ^ 0xFFFFFFFF);
    }
}