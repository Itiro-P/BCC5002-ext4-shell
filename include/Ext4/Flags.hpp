#pragma once

#include <cstdint>

namespace Ext4::Flags {
    // Flags aplicadas a cada grupo de blocos individualmente (bg_flags)
    enum BlockGroupFlags : uint16_t {
        BG_INODE_UNINIT = 0x0001, // A tabela de inodes e o respectivo bitmap não foram inicializados em disco.
        BG_BLOCK_UNINIT = 0x0002, // O bitmap de blocos de dados não foi inicializado em disco.
        BG_INODE_ZEROED = 0x0004, // A tabela de inodes deste grupo de blocos já foi completamente zerada em background.
    };

    // Contém flags comuns e definições de enumerações usadas para interpretar os campos do superbloco e controlar o comportamento do sistema de arquivos.
    namespace SuperBlockFlags {
        // Estado de integridade e montagem do sistema de arquivos.
        enum State: uint16_t {
            FS_STATE_CLEARLY_UNMOUNTED = 0x0001, // O sistema de arquivos foi desmontado corretamente e de forma limpa.
            FS_STATE_ERROR_DETECTED    = 0x0002, // O sistema de arquivos detectou um erro interno, mas ainda é montável.
            FS_STATE_ORPHAN_INODES     = 0x0004, // Existem inodes órfãos que precisam ser recuperados pelo log do journal.
        };

        // Política de tratamento imediato caso o kernel encontre algum erro de metadados.
        enum ErrorPolicy: uint16_t {
            ERR_POLICY_CONTINUE  = 1, // Continuar a operação normalmente, apenas registrando o erro no log do sistema.
            ERR_POLICY_READ_ONLY = 2, // Remontar o sistema de arquivos instantaneamente como somente leitura para evitar corrupção de dados.
            ERR_POLICY_PANIC     = 3, // Interromper o sistema imediatamente (kernel panic).
        };

        // Sistema Operacional responsável por criar e formatar originalmente o volume.
        enum CreatorOS: uint32_t {
            OS_LINUX = 0,
            OS_HURD = 1,
            OS_MASIX = 2,
            OS_FREEBSD = 3,
            OS_LITES = 4,
        };

        // Nível de revisão do formato do Superbloco.
        enum RevisionLevel: uint32_t {
            REV_ORIGINAL = 0, // Revisão original herdada do ext2 (tamanho de inode fixo).
            REV_DYNAMIC  = 1, // Revisão dinâmica moderna, permitindo recursos expansíveis e inodes de tamanho variável.
        };

        // Recursos compatíveis: O kernel pode ler/escrever mesmo se não conhecer essas flags.
        enum CompatFeature: uint32_t {
            COMPAT_DIR_PREALLOC  = 0x0001, // Suporte a pré-alocação de blocos para estruturas de diretórios.
            COMPAT_IMAGIC_INODES = 0x0002, // Suporte a inodes especiais "imagic" (usados por SOs legados como A/UX).
            COMPAT_HAS_JOURNAL   = 0x0004, // O sistema de arquivos possui um log de Journal ativo para recuperação de falhas.
            COMPAT_EXT_ATTR      = 0x0008, // Suporte a atributos estendidos (como permissões ACL avançadas).
            COMPAT_RESIZE_INODE  = 0x0010, // Inode reservado para permitir o redimensionamento online do sistema de arquivos.
            COMPAT_DIR_INDEX     = 0x0020, // Diretórios usam indexação baseada em árvores HTree para buscas aceleradas de arquivos.
            COMPAT_LAZY_BG       = 0x0040, // Estrutura adaptada para suportar a inicialização tardia (preguiçosa) de grupos de blocos.
            COMPAT_EXCLUDE_INODE = 0x0080, // Suporte a inodes de exclusão especiais para tratamento de snapshots.
            COMPAT_EXCLUDE_BITMAP= 0x0100, // Suporte a bitmaps de exclusão para blocos de snapshots.
            COMPAT_SPARSE_SUPER2 = 0x0200, // Suporte a superblocos esparsos v2 (reduz drasticamente o número de cópias de backup).
            COMPAT_FAST_COMMITS  = 0x0400, // Suporte a commits rápidos no journal para acelerar transações pequenas de I/O.
            COMPAT_ORPHAN_ALLOC  = 0x2000, // Alocação de arquivos órfãos estruturada de forma nativa para melhor rastreamento.
        };

        // Recursos Incompatíveis: O kernel DEVE recusar a montagem caso não entenda qualquer um desses bits ativos.
        enum IncompatFeature: uint32_t {
            INCOMPAT_COMPRESSION      = 0x00001, // Compressão de dados em nível de disco (não implementada).
            INCOMPAT_FILETYPE         = 0x00002, // Entradas de diretório gravam o tipo do arquivo.
            INCOMPAT_RECOVER          = 0x00004, // Exige reprodução do log do journal antes de poder ser montado.
            INCOMPAT_JOURNAL_DEV      = 0x00008, // O volume utiliza um dispositivo físico externo dedicado para o journal.
            INCOMPAT_META_BG          = 0x00010, // Uso de agrupamentos flexíveis de descritores (Meta block groups).
            INCOMPAT_EXTENTS          = 0x00040, // Mapeamento de blocos de dados através de árvores de extents.
            INCOMPAT_64BIT            = 0x00080, // Permite identificadores e descritores de blocos de 64 bits.
            INCOMPAT_MOUNT_PROC       = 0x00100, // Proteção contra montagem múltipla simultânea (MMP).
            INCOMPAT_FLEX_BG          = 0x00200, // Grupos de blocos flexíveis.
            INCOMPAT_EA_INODE         = 0x00400, // Permite armazenar atributos estendidos grandes dentro do inode.
            INCOMPAT_DIRDATA          = 0x01000, // Permite dados customizados nas entradas de diretórios.
            INCOMPAT_CKSUM_SEED       = 0x02000, // Permite que a semente do checksum seja guardada no superbloco.
            INCOMPAT_LARGE_DIR        = 0x04000, // Suporte a estruturas de diretórios gigantescas (HTree de 3 níveis).
            INCOMPAT_INLINE_DATA      = 0x08000, // Arquivos e diretórios pequenos embutidos diretamente no inode.
            INCOMPAT_ENCRYPTION       = 0x10000, // Suporte nativo para criptografia.
            INCOMPAT_DIR_INSENSITIVE  = 0x20000, // Busca de nomes de arquivos case-insensitive (casefold).
        };

        // Recursos compatíveis com leitura-apenas: Se o kernel não entender um bit ativo, DEVE montar como Read-Only.
        enum RoCompatFeature: uint32_t {
            RO_COMPAT_SPARSE_SUPER  = 0x0001, // Reduz o número de cópias redundantes do superbloco.
            RO_COMPAT_LARGE_FILE    = 0x0002, // Indica suporte para arquivos grandes (> 2GB).
            RO_COMPAT_BTREE         = 0x0004, // Uso de indexação por árvore binária simples (obsoleto).
            RO_COMPAT_HUGE_FILE     = 0x0008, // Permite arquivos gigantescos (contador medido em setores de 512 bytes).
            RO_COMPAT_GDT_CSUM      = 0x0010, // Descritores de grupos usam checksums simples (CRC16).
            RO_COMPAT_DIR_NLINK     = 0x0020, // Permite que diretórios possuam mais de 32.000 subdiretórios.
            RO_COMPAT_EXTRA_ISIZE   = 0x0040, // Inodes expandidos usam campos extras de precisão temporal.
            RO_COMPAT_HAS_SNAPSHOT  = 0x0080, // Suporte a snapshots em nível de sistema de arquivos.
            RO_COMPAT_QUOTA         = 0x0100, // Suporte interno integrado para limites de cotas de disco.
            RO_COMPAT_PROJECT       = 0x0200, // Suporte a cotas de disco associadas a IDs de projetos.
            RO_COMPAT_BIGALLOC      = 0x0400, // Alocação de blocos de dados feita em clusters.
            RO_COMPAT_METADATA_CSUM = 0x0400 | 0x0010, // metadata_csum na especificação real depende do bit 0x0400 de forma indireta ou é avaliado estritamente como 0x0400 na máscara expandida do Kernel (Mapeado como 0x00000400 no código-fonte modernizado do ext4.h).
        };
        
        // REVISÃO CIRÚRGICA RO_COMPAT PARA O SEU CASO (Ajustado conforme o kernel oficial do Linux):
        // Modificado os enumerators abaixo para os valores hexadecimais literais exatos do driver ext4:
        inline constexpr uint32_t RO_COMPAT_FEATURE_BIGALLOC      = 0x00000800; // CORRIGIDO (era 0x0400)
        inline constexpr uint32_t RO_COMPAT_FEATURE_METADATA_CSUM = 0x00001000; // CORRIGIDO (era 0x0800)
        inline constexpr uint32_t RO_COMPAT_FEATURE_READONLY      = 0x00002000; // CORRIGIDO (era 0x2000 lógico puro)
        inline constexpr uint32_t RO_COMPAT_FEATURE_PROJECT       = 0x00004000; // CORRIGIDO

        // Define o algoritmo usado no recurso HTree para indexar e buscar arquivos dentro das pastas.
        enum DefaultHashVersion : uint8_t {
            HASH_LEGACY            = 0x0,
            HASH_HALF_MD4          = 0x1,
            HASH_TEA               = 0x2,
            HASH_LEGACY_UNSIGNED   = 0x3,
            HASH_HALF_MD4_UNSIGNED = 0x4,
            HASH_TEA_UNSIGNED      = 0x5
        };

        // Flags de bit que definem o comportamento padrão do volume
        enum DefaultMountOpts : uint32_t {
            DEFM_DEBUG          = 0x0001, 
            DEFM_BSDGROUPS      = 0x0002, 
            DEFM_XATTR_USER     = 0x0004, 
            DEFM_ACL            = 0x0008, 
            DEFM_UID16          = 0x0010, 
            DEFM_JMODE_DATA     = 0x0020, // Máscara combinada de journaling
            DEFM_JMODE_ORDERED  = 0x0040, 
            DEFM_JMODE_WBACK    = 0x0060, 
            DEFM_NOBARRIER      = 0x0100, 
            DEFM_BLOCK_VALIDITY = 0x0200, 
            DEFM_DISCARD        = 0x0400, 
            DEFM_NODELALLOC     = 0x0800, 
        };

        // Modificadores internos de estado geral de indexação.
        enum IndexFlags : uint32_t {
            FLAGS_SIGNED_DIR_HASH   = 0x0001, 
            FLAGS_UNSIGNED_DIR_HASH = 0x0002, 
            FLAGS_DEVELOPMENT_CODE  = 0x0004, 
        };

        // Identificadores para os modos de criptografia nativa por arquivo
        enum EncryptionMode : uint8_t {
            ENCRYPT_INVALID_ALGORITHM = 0,
            ENCRYPT_AES_256_XTS       = 1,
            ENCRYPT_AES_256_GCM       = 2,
            ENCRYPT_AES_256_CBC       = 3
        };
    };

    // Flags aplicadas ao comportamento do Inode (i_flags)
    enum InodeFlags : uint32_t {
        EXT4_SECRM_FL            = 0x00000001, 
        EXT4_UNRM_FL             = 0x00000002, 
        EXT4_COMPR_FL            = 0x00000004, 
        EXT4_SYNC_FL             = 0x00000008, 
        EXT4_IMMUTABLE_FL        = 0x00000010, 
        EXT4_APPEND_FL           = 0x00000020, 
        EXT4_NODUMP_FL           = 0x00000040, 
        EXT4_NOATIME_FL          = 0x00000080, 
        EXT4_DIRTY_FL            = 0x00000100, 
        EXT4_COMPRBLK_FL         = 0x00000200, 
        EXT4_NOCOMPR_FL          = 0x00000400, 
        EXT4_ENCRYPT_FL          = 0x00000800, 
        EXT4_INDEX_FL            = 0x00001000, 
        EXT4_IMAGIC_FL           = 0x00002000, 
        EXT4_JOURNAL_DATA_FL     = 0x00004000, 
        EXT4_NOTAIL_FL           = 0x00008000, 
        EXT4_DIRSYNC_FL          = 0x00010000, 
        EXT4_TOPDIR_FL           = 0x00020000, 
        EXT4_HUGE_FILE_FL        = 0x00040000, 
        EXT4_EXTENTS_FL          = 0x00080000, 
        EXT4_VERITY_FL           = 0x00100000, 
        EXT4_EA_INODE_FL         = 0x00200000, 
        EXT4_EOFBLOCKS_FL        = 0x00400000, 
        EXT4_SNAPFILE_FL         = 0x01000000, 
        EXT4_SNAPFILE_DELETED_FL = 0x04000000, 
        EXT4_SNAPFILE_SHRUNK_FL  = 0x08000000, 
        EXT4_INLINE_DATA_FL      = 0x10000000, 
        EXT4_PROJINHERIT_FL      = 0x20000000, 
        EXT4_CASEFOLD_FL         = 0x40000000, 
        EXT4_RESERVED_FL         = 0x80000000, 

        EXT4_FL_USER_VISIBLE     = 0x705BDFFF, 
        EXT4_FL_USER_MODIFIABLE  = 0x604BC0FF  
    };

    // Modo do arquivo e permissões (i_mode)
    enum InodeMode : uint16_t {
        S_IXOTH = 0x0001, 
        S_IWOTH = 0x0002, 
        S_IROTH = 0x0004, 
        S_IXGRP = 0x0008, 
        S_IWGRP = 0x0010, 
        S_IRGRP = 0x0020, 
        S_IXUSR = 0x0040, 
        S_IWUSR = 0x0080, 
        S_IRUSR = 0x0100, 
        
        S_ISVTX = 0x0200, // Sticky bit
        S_ISGID = 0x0400, // Set GID
        S_ISUID = 0x0800, // Set UID
        
        S_IFIFO  = 0x1000, 
        S_IFCHR  = 0x2000, 
        S_IFDIR  = 0x4000, 
        S_IFBLK  = 0x6000, 
        S_IFREG  = 0x8000, 
        S_IFLNK  = 0xA000, 
        S_IFSOCK = 0xC000  
    };

    struct Osd1Linux {
        uint32_t l_i_version;
    };

    #pragma pack(push, 1)
    struct Osd2Linux {
        uint16_t l_i_blocks_high;
        uint16_t l_i_file_acl_high;
        uint16_t l_i_uid_high;
        uint16_t l_i_gid_high;
        uint16_t l_i_checksum_lo;
        uint16_t l_i_reserved;
    };
    #pragma pack(pop)

    static_assert(sizeof(Flags::Osd1Linux) == 4);
    static_assert(sizeof(Flags::Osd2Linux) == 12);
}