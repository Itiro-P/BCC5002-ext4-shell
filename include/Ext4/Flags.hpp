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
            COMPAT_EXT_ATTR      = 0x0008, // Suporte a atributos estendidos estendidos (como permissões ACL avançadas).
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
            INCOMPAT_COMPRESSION      = 0x00001, // Compressão de dados em nível de disco (não implementada no kernel estável).
            INCOMPAT_FILETYPE         = 0x00002, // Entradas de diretório gravam o tipo do arquivo para acelerar a varredura sem ler o inode.
            INCOMPAT_RECOVER          = 0x00004, // O sistema exige reprodução do log do journal antes de poder ser montado em modo leitura-escrita.
            INCOMPAT_JOURNAL_DEV      = 0x00008, // O volume utiliza um dispositivo físico externo e dedicado para o log do journal.
            INCOMPAT_META_BG          = 0x00010, // Uso de agrupamentos flexíveis de descritores (Meta block groups) para redimensionamento em larga escala.
            INCOMPAT_EXTENTS          = 0x00040, // Mapeamento de blocos de dados através de árvores de extents em vez de blocos indiretos. (Crucial para o ext4).
            INCOMPAT_64BIT            = 0x00080, // Permite que o sistema use identificadores e descritores de blocos de 64 bits (suporta volumes > 16 TiB).
            INCOMPAT_MOUNT_PROC       = 0x00100, // Proteção contra montagem múltipla simultânea (MMP) em ambientes de cluster.
            INCOMPAT_FLEX_BG          = 0x00200, // Grupos de blocos flexíveis (reúne tabelas de metadados consecutivamente no disco para ganho de performance).
            INCOMPAT_EA_INODE         = 0x00400, // Permite armazenar atributos estendidos grandes dentro do corpo expandido do próprio inode.
            INCOMPAT_DIRDATA          = 0x01000, // Permite que dados customizados opcionais sejam injetados nas entradas de diretórios.
            INCOMPAT_META_CSUM_CHECK  = 0x02000, // Verificação de integridade dos descritores através de metadados checksum (v2).
            INCOMPAT_LARGE_DIR        = 0x04000, // Suporte a estruturas de diretórios gigantescas que passam de 2GB ou exigem HTree de 3 níveis.
            INCOMPAT_INLINE_DATA      = 0x08000, // Arquivos e diretórios muito pequenos são armazenados diretamente no corpo do inode para economizar I/O.
            INCOMPAT_ENCRYPTION       = 0x10000, // Suporte nativo do sistema de arquivos para criptografia de diretórios e arquivos.
            INCOMPAT_DIR_INSENSITIVE  = 0x20000, // Busca e correspondência de nomes de arquivos insensível a maiúsculas e minúsculas (casefold).
        };

        // Recursos compatíveis com leitura-apenas: Se o kernel não entender um bit ativo, ele DEVE montar obrigatoriamente como Read-Only.
        enum RoCompatFeature: uint32_t {
            RO_COMPAT_SPARSE_SUPER  = 0x0001, // Reduz o número de cópias redundantes do superbloco salvando-as apenas em grupos específicos.
            RO_COMPAT_LARGE_FILE    = 0x0002, // Indica suporte legado para arquivos grandes (mantido por retrocompatibilidade com ext2).
            RO_COMPAT_BTREE         = 0x0004, // Uso de indexação por árvore binária simples em diretórios antigos (obsoleto).
            RO_COMPAT_HUGE_FILE     = 0x0008, // Permite arquivos gigantescos cujo contador de blocos dentro do inode é medido em setores de 512 bytes.
            RO_COMPAT_GDT_CSUM      = 0x0010, // Descritores de grupos de blocos usam checksums simples para otimização do inicializador (lazy init).
            RO_COMPAT_DIR_NLINK     = 0x0020, // Permite que diretórios possuam mais de 32.000 subdiretórios vinculados.
            RO_COMPAT_EXTRA_ISIZE   = 0x0040, // Inodes expandidos usam campos extras de precisão temporal em nanossegundos e tamanhos em 64 bits.
            RO_COMPAT_QUOTA         = 0x0100, // Suporte interno integrado no sistema de arquivos para limites de cotas de disco.
            RO_COMPAT_PROJECT       = 0x0200, // Suporte a cotas de disco associadas a identificadores de projetos específicos.
            RO_COMPAT_BIGALLOC      = 0x0400, // Alocação de blocos de dados feita em conjuntos macro chamados "clusters" (reduz fragmentação).
            RO_COMPAT_METADATA_CSUM = 0x0800, // Ativa a proteção global por checksum CRC32c em praticamente todos os metadados do sistema.
            RO_COMPAT_REPLICA       = 0x1000, // Suporte experimental para árvores de réplicas de leitura para tolerância a falhas.
            RO_COMPAT_READONLY      = 0x2000, // Força o sistema a se comportar de forma estritamente somente-leitura em nível lógico interno.
            RO_COMPAT_ORPHAN_PRESENT= 0x4000, // Indica de maneira estática a presença de arquivos na lista de órfãos pendentes.
        };

        // Define o algoritmo usado no recurso HTree para indexar e buscar arquivos dentro das pastas.
        enum DefaultHashVersion : uint8_t {
            HASH_LEGACY            = 0x0,
            HASH_HALF_MD4          = 0x1,
            HASH_TEA               = 0x2,
            HASH_LEGACY_UNSIGNED   = 0x3,
            HASH_HALF_MD4_UNSIGNED = 0x4,
            HASH_TEA_UNSIGNED      = 0x5
        };

        // Flags de bit que definem o comportamento padrão do volume quando o usuário omite parâmetros no comando mount
        enum DefaultMountOpts : uint32_t {
            DEFM_DEBUG          = 0x0001, // Imprime informações detalhadas de depuração no dmesg ao montar o volume.
            DEFM_BSDGROUPS      = 0x0002, // Novos arquivos herdam o GID do diretório pai (comportamento clássico do BSD).
            DEFM_XATTR_USER     = 0x0004, // Ativa por padrão o mapeamento de atributos estendidos para o espaço do usuário.
            DEFM_ACL            = 0x0008, // Habilita o suporte a Listas de Controle de Acesso POSIX (ACLs).
            DEFM_UID16          = 0x0010, // Desativa mapeamento e emulação para IDs de usuários antigos de 16 bits.
            DEFM_JMODE_DATA     = 0x0020, // Modo Journal total: dados e metadados passam pelo log do journal antes do disco.
            DEFM_JMODE_ORDERED  = 0x0040, // Modo Journal ordenado: garante a escrita dos blocos de dados antes dos metadados no journal.
            DEFM_JMODE_WBACK    = 0x0060, // Modo Journal writeback: escrita assíncrona sem garantia de ordem estrita entre metadados e dados.
            DEFM_NOBARRIER      = 0x0100, // Desativa as barreiras de cache de escrita do hardware (risco em quedas de energia).
            DEFM_BLOCK_VALIDITY = 0x0200, // Ativa checagens extras para rastrear metadados e impedir blocos corrompidos de sobrescrever dados.
            DEFM_DISCARD        = 0x0400, // Emite comandos de liberação de blocos automaticamente (TRIM para SSDs).
            DEFM_NODELALLOC     = 0x0800, // Desativa completamente o recurso de Alocação Atrasada (Delayed Allocation).
        };

        // Modificadores internos de estado geral de indexação.
        enum IndexFlags : uint32_t {
            FLAGS_SIGNED_DIR_HASH   = 0x0001, // Indica o uso de funções de espelhamento hash de diretório com sinal (Signed).
            FLAGS_UNSIGNED_DIR_HASH = 0x0002, // Indica o uso de funções de espelhamento hash de diretório sem sinal (Unsigned).
            FLAGS_DEVELOPMENT_CODE  = 0x0004, // Flag ativa apenas se o volume foi montado sob uma versão de kernel com códigos de teste experimentais.
        };

        // Identificadores para os modos de criptografia nativa por arquivo armazenados na lista do superbloco.
        enum EncryptionMode : uint8_t {
            ENCRYPT_INVALID_ALGORITHM = 0,
            ENCRYPT_AES_256_XTS       = 1,
            ENCRYPT_AES_256_GCM       = 2,
            ENCRYPT_AES_256_CBC       = 3
        };
    };

    // Flags aplicadas ao comportamento do Inode (i_flags)
    enum InodeFlags : uint32_t {
        EXT4_SECRM_FL            = 0x00000001, // Eliminação segura exigida (não implementado)
        EXT4_UNRM_FL             = 0x00000002, // Preservar para recuperação (não implementado)
        EXT4_COMPR_FL            = 0x00000004, // Ficheiro comprimido (não totalmente implementado)
        EXT4_SYNC_FL             = 0x00000008, // Escritas síncronas obrigatórias
        EXT4_IMMUTABLE_FL        = 0x00000010, // Ficheiro imutável
        EXT4_APPEND_FL           = 0x00000020, // Apenas permite escrita no final (Append)
        EXT4_NODUMP_FL           = 0x00000040, // O utilitário dump(1) deve ignorar o ficheiro
        EXT4_NOATIME_FL          = 0x00000080, // Não atualizar o tempo de acesso (atime)
        EXT4_DIRTY_FL            = 0x00000100, // Ficheiro comprimido modificado (não usado)
        EXT4_COMPRBLK_FL         = 0x00000200, // Possui blocos comprimidos (não usado)
        EXT4_NOCOMPR_FL          = 0x00000400, // Não comprimir o ficheiro (não usado)
        EXT4_ENCRYPT_FL          = 0x00000800, // Inode encriptado
        EXT4_INDEX_FL            = 0x00001000, // Diretório possui índices indexados por hash (HTree)
        EXT4_IMAGIC_FL           = 0x00002000, // Diretório mágico AFS
        EXT4_JOURNAL_DATA_FL     = 0x00004000, // Dados passam obrigatoriamente pelo Journal
        EXT4_NOTAIL_FL           = 0x00008000, // O final do ficheiro não deve ser fundido (não usado)
        EXT4_DIRSYNC_FL          = 0x00010000, // Alterações no diretório são síncronas
        EXT4_TOPDIR_FL           = 0x00020000, // Topo da hierarquia de diretórios
        EXT4_HUGE_FILE_FL        = 0x00040000, // Ficheiro gigante (escala os contadores de blocos)
        EXT4_EXTENTS_FL          = 0x00080000, // O Inode utiliza árvore de extents (i_block armazena extents)
        EXT4_VERITY_FL           = 0x00100000, // Ficheiro protegido por Verity
        EXT4_EA_INODE_FL         = 0x00200000, // O Inode armazena um atributo estendido grande nos seus blocos
        EXT4_EOFBLOCKS_FL        = 0x00400000, // Possui blocos alocados para lá do EOF (depreciado)
        EXT4_SNAPFILE_FL         = 0x01000000, // Inode é um snapshot (fora do mainline)
        EXT4_SNAPFILE_DELETED_FL = 0x04000000, // Snapshot em remoção (fora do mainline)
        EXT4_SNAPFILE_SHRUNK_FL  = 0x08000000, // Redução de snapshot concluída (fora do mainline)
        EXT4_INLINE_DATA_FL      = 0x10000000, // O Inode armazena dados embutidos diretamente em i_block
        EXT4_PROJINHERIT_FL      = 0x20000000, // Subdiretórios herdam o mesmo ID de Projeto
        EXT4_CASEFOLD_FL         = 0x40000000, // Diretório com buscas insensíveis a maiúsculas/minúsculas
        EXT4_RESERVED_FL         = 0x80000000, // Reservado para a biblioteca ext4

        EXT4_FL_USER_VISIBLE     = 0x705BDFFF, // Máscara de flags visíveis pelo utilizador
        EXT4_FL_USER_MODIFIABLE  = 0x604BC0FF  // Máscara de flags modificáveis pelo utilizador
    };
}