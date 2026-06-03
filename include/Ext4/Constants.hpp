#pragma once

#include <cstdint>

namespace Ext4::Constants {
    // Máscara para extrair estritamente o tipo de ficheiro de i_mode
    inline constexpr uint16_t S_IFMT = 0xF000;
    
    // Número mágico universal que identifica um cabeçalho de nó válido na árvore de extents (0xF30A).
    inline constexpr uint16_t EXTENT_MAGIC = 0xF30A;
    
    // Número mágico universal que valida o bloco MMP (0x004D4D50 -> "MMP" em ASCII).
    inline constexpr uint32_t MMP_MAGIC = 0x004D4D50;
    
    // Valores sequenciais especiais que indicam estados controlados de libertação do disco.
    inline constexpr uint32_t MMP_SEQ_CLEAN = 0xFF4D4D50; // O volume foi desmontado de forma limpa e está livre.
    inline constexpr uint32_t MMP_SEQ_FSCK  = 0xE24D4D50; // O utilitário fsck está a modificar o sistema de ficheiros.
    
    // Valor mágico protetor gravado estritamente no final de cada bloco de órfãos (0x0B10CA04).
    inline constexpr uint32_t ORPHAN_BLOCK_MAGIC = 0x0B10CA04;
    
    // Número mágico universal que identifica um sistema de arquivos da família EXT (`0xEF53`).
    inline constexpr uint16_t EXT_MAGIC = 0xEF53;
    
    // Deslocamento fixo (em bytes) a partir do início do disco/partição onde o Superbloco reside.
    // Este espaço inicial de 1024 bytes é estritamente reservado para o x86 Bootloader/MBR.
    inline constexpr uint32_t SUPERBLOCK_OFFSET = 1024;
    
    // Número mágico universal que valida a assinatura dos Atributos Estendidos (0xEA020000).
    inline constexpr uint32_t XATTR_MAGIC = 0xEA020000;

    // Ele CERTAMENTE é usado :)
    inline constexpr uint32_t JBD2_MAGIC_NUMBER = 0xC03A39A2;
}