#pragma once

#include <cstdint>

namespace Ext4::Constants {
    // Máscara para extrair estritamente o tipo de ficheiro de i_mode
    inline constexpr uint16_t S_IFMT = 0xF000;
    
    // Número mágico universal que identifica um cabeçalho de nó válido na árvore de extents (0xF30A).
    inline constexpr uint16_t EXTENT_MAGIC = 0xF30A;
    
    // Número mágico universal que identifica um sistema de arquivos da família EXT (`0xEF53`).
    inline constexpr uint16_t EXT_MAGIC = 0xEF53;
    
    // Deslocamento fixo (em bytes) a partir do início do disco/partição onde o Superbloco reside.
    // Este espaço inicial de 1024 bytes é estritamente reservado para o x86 Bootloader/MBR.
    inline constexpr uint32_t SUPERBLOCK_OFFSET = 1024;
}