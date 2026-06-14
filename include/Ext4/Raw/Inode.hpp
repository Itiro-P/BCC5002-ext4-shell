#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include "../Flags.hpp"

namespace Ext4::Raw {
    #pragma pack(push, 1)
    /**
     * @brief Estrutura correspondente ao nó de índice (struct ext4_inode) do Linux Kernel.
     * Mapeia os metadados físicos exatos de um ficheiro ou diretório no EXT4.
     */
    struct Inode {
        // Estrutura Clássica do EXT2/EXT3 (Primeiros 128 bytes)
        uint16_t  i_mode;           // Modo do ficheiro (Tipo e permissões de acesso)
        uint16_t  i_uid;            // 16 bits inferiores do UID do proprietário
        uint32_t  i_size_lo;        // 32 bits inferiores do tamanho do ficheiro em bytes
        uint32_t  i_atime;          // Último acesso (Timestamp Unix) ou checksum se EA_INODE ativo
        uint32_t  i_ctime;          // Última alteração do inode ou refcount inferior se EA_INODE ativo
        uint32_t  i_mtime;          // Última modificação dos dados ou inode proprietário de EA_INODE
        uint32_t  i_dtime;          // Momento da eliminação (Deletion Time) ou elo da lista orphan
        uint16_t  i_gid;            // 16 bits inferiores do GID do grupo
        uint16_t  i_links_count;    // Contador de ligações físicas (Hard links)
        uint32_t  i_blocks_lo;      // 32 bits inferiores do contador de blocos alocados (setores de 512B)
        uint32_t  i_flags;          // Flags de comportamento do ficheiro (ver `InodeFlags`)
        
        Flags::Osd1Linux i_osd1;           // Campos específicos do SO criador (Mapeado para Linux)
        
        // Mapa de blocos (12 diretos, 1 indireto, 1 duplo, 1 triplo) OU Raiz da árvore de Extents
        std::array<std::byte, 60> i_block;  // 60 bytes de armazenamento inline para mapeamento de dados
        
        uint32_t  i_generation;     // Versão do ficheiro (utilizado principalmente para exportações NFS)
        uint32_t  i_file_acl_lo;    // 32 bits inferiores do bloco de atributos estendidos (ACL)
        uint32_t  i_size_hi;        // 32 bits superiores do tamanho do ficheiro (anteriormente i_dir_acl)
        uint32_t  i_obso_faddr;     // Endereço de fragmento obsoleto
        
        Flags::Osd2Linux i_osd2;           // Campos específicos adicionais do SO (Mapeado para Linux)

        // Campos Estendidos Modernos do EXT4 (Extensão além dos 128 bytes originais)
        // Existem estritamente se o Superbloco indicar sb.s_inode_size > 128 e i_extra_isize for adequado.
        
        uint16_t  i_extra_isize;    // Tamanho em bytes desta estrutura além do limite básico de 128 bytes
        uint16_t  i_checksum_hi;    // 16 bits superiores do Checksum deste Inode
        uint32_t  i_ctime_extra;    // Fração de nanossegundos e bits extras de época para ctime
        uint32_t  i_mtime_extra;    // Fração de nanossegundos e bits extras de época para mtime
        uint32_t  i_atime_extra;    // Fração de nanossegundos e bits extras de época para atime
        uint32_t  i_crtime;         // Data de criação do ficheiro (Timestamp em segundos)
        uint32_t  i_crtime_extra;   // Fração de nanossegundos e bits extras de época para crtime
        uint32_t  i_version_hi;     // 32 bits superiores do número de versão do ficheiro
        uint32_t  i_projid;         // Identificador de Projeto (Quota de Projeto)
    };
    #pragma pack(pop)
    // Garante integridade do tamanho da especificação moderna completa (160 bytes)
    static_assert(sizeof(Inode) == 160, "O tamanho da estrutura básica do Inode deve ser de 160 bytes!");
}