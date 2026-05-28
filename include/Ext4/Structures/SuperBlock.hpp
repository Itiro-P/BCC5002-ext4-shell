#pragma once

#include <array>
#include <cstdint>

namespace Ext4::Structures {

    /**
     * @brief Espaço de nomes dedicado a constantes globais de localização e identificação do Superbloco.
     */
    namespace SuperBlockNS {
        // Número mágico universal que identifica um sistema de arquivos da família EXT (0xEF53).
        static inline constexpr uint16_t MAGIC = 0xEF53;
        
        // Deslocamento fixo (em bytes) a partir do início do disco/partição onde o Superbloco reside.
        // Este espaço inicial de 1024 bytes é estritamente reservado para o x86 Bootloader/MBR.
        static inline constexpr uint32_t SUPERBLOCK_OFFSET = 1024;
    }

    #pragma pack(push, 1)
    /**
     * @brief Estrutura do Superbloco do EXT4 (struct ext4_super_block).
     * Contém o registro mestre de toda a configuração, geometria, estado de integridade,
     * contadores de blocos/inodes livres e a lista de recursos (features) ativos na imagem.
     */
    struct SuperBlock {
        // Geometria Básica e Contadores (0x00) 
        uint32_t    s_inodes_count;           // Quantidade total de inodes configurados no sistema de arquivos.
        uint32_t    s_blocks_count_lo;        // Bits inferiores (0-31) da contagem total de blocos do sistema.
        uint32_t    s_r_blocks_count_lo;      // Bits inferiores (0-31) da contagem de blocos reservados ao superusuário (root).
        uint32_t    s_free_blocks_count_lo;   // Bits inferiores (0-31) da contagem global de blocos livres.
        uint32_t    s_free_inodes_count;      // Quantidade global de inodes livres.
        uint32_t    s_first_data_block;       // O ID do primeiro bloco de dados físico (0 para blocos de 4KB, 1 para blocos de 1KB).
        uint32_t    s_log_block_size;         // Tamanho do bloco, expresso como potência: tamanho = 1024 << s_log_block_size.
        uint32_t    s_log_cluster_size;       // Tamanho do cluster (para alocação de múltiplos blocos/bigalloc), expresso como potência.
        uint32_t    s_blocks_per_group;       // Quantidade de blocos lógicos contidos em cada Grupo de Blocos.
        uint32_t    s_clusters_per_group;     // Quantidade de clusters contidos em cada Grupo de Blocos.
        uint32_t    s_inodes_per_group;       // Quantidade de inodes alocados para cada Grupo de Blocos.
        
        // Timestamps e Contadores de Montagem (0x2C) 
        uint32_t    s_mtime;                  // Carimbo de data/hora (Timestamp Unix) da última montagem do volume.
        uint32_t    s_wtime;                  // Carimbo de data/hora (Timestamp Unix) da última escrita/sincronização no bloco.
        uint16_t    s_mnt_count;              // Quantidade de vezes que o volume foi montado desde a última verificação estrutural.
        uint16_t    s_max_mnt_count;          // Limite máximo de montagens permitido antes que uma verificação forçada (fsck) ocorra.
        uint16_t    s_magic;                  // Assinatura mágica de validação (Deve ser idêntica a SuperBlockNS::MAGIC).
        uint16_t    s_state;                  // Estado do sistema de arquivos (1 = Desmontado de forma limpa, 2 = Erros detectados, etc.).
        uint16_t    s_errors;                 // Comportamento do kernel em caso de erro (1 = Continuar, 2 = Remontar Read-Only, 3 = Panic).
        uint16_t    s_minor_rev_level;        // Nível de revisão secundário.
        uint32_t    s_lastcheck;              // Carimbo de data/hora (Timestamp Unix) da última verificação estrutural completa (fsck).
        uint32_t    s_checkinterval;          // Tempo máximo (em segundos) permitido entre verificações completas.
        uint32_t    s_creator_os;             // Identificador do SO que formatou o volume (0 = Linux, 1 = Hurd, etc.).
        uint32_t    s_rev_level;              // Nível de revisão principal (0 = Formato legado/original, 1 = Formato dinâmico moderno).
        uint16_t    s_def_resuid;             // ID de Usuário (UID) padrão que pode utilizar os blocos reservados (Geralmente 0/root).
        uint16_t    s_def_resgid;             // ID de Grupo (GID) padrão que pode utilizar os blocos reservados.
        
        // Atributos de Revisão Dinâmica (Apenas se s_rev_level == 1) (0x54) 
        uint32_t    s_first_ino;              // ID do primeiro inode utilizável para arquivos comuns (Geralmente 11, os anteriores são especiais).
        uint16_t    s_inode_size;             // Tamanho físico em bytes de cada estrutura de Inode em disco (Geralmente 256 bytes no EXT4).
        uint16_t    s_block_group_nr;         // O número do grupo de blocos que hospeda esta cópia específica do Superbloco.
        uint32_t    s_feature_compat;         // Máscara de bits de recursos compatíveis (O driver pode ler/escrever mesmo sem suporte nativo).
        uint32_t    s_feature_incompat;       // Máscara de bits de recursos incompatíveis (O driver DEVE recusar a montagem se não suportar).
        uint32_t    s_feature_ro_compat;      // Máscara de bits de recursos somente-leitura (Se não suportados, monta apenas como Read-Only).
        std::array<uint8_t, 16> s_uuid;       // Identificador Único Universal (UUID) de 128 bits atribuído ao volume.
        std::array<char, 16>    s_volume_name; // Nome legível (Volume Label) atribuído à partição.
        std::array<char, 64>    s_last_mounted; // Caminho absoluto (Diretório) onde o volume foi montado pela última vez.
        uint32_t    s_algorithm_usage_bitmap; // Usado antigamente para definir algoritmos de compressão legados.
        
        // Otimizações de Desempenho e Pré-alocação (0xDC) 
        uint8_t     s_prealloc_blocks;        // Quantidade de blocos a pré-alocar em arquivos comuns (Legado).
        uint8_t     s_prealloc_dir_blocks;    // Quantidade de blocos a pré-alocar em diretórios (Legado).
        uint16_t    s_reserved_gdt_blocks;    // Quantidade de blocos reservados na tabela de descritores para expansão online do disco.
        
        // Infraestrutura do Journal Interno (JBD2) (0xE4) 
        std::array<uint8_t, 16> s_journal_uuid; // UUID do dispositivo de Journal externo (Se aplicável).
        uint32_t    s_journal_inum;           // Número do Inode associado ao arquivo de Journal interno (Normalmente o Inode 8).
        uint32_t    s_journal_dev;            // Número de dispositivo do Journal externo.
        uint32_t    s_last_orphan;            // Cabeça da lista ligada legada de inodes órfãos (Aponta para o primeiro Inode órfão).
        
        // Diretórios Indexados e Criptografia (0x100) 
        std::array<uint32_t, 4> s_hash_seed;  // Semente criptográfica de 128 bits utilizada no algoritmo de hash das HTrees.
        uint8_t     s_def_hash_version;       // Algoritmo de hash padrão usado para indexação de diretórios.
        uint8_t     s_jnl_backup_type;        // Tipo de backup do Journal.
        uint16_t    s_desc_size;              // Tamanho físico do Descritor de Grupo (Se a flag 64bit estiver ativa, mede 64).
        uint32_t    s_default_mount_opts;     // Opções de montagem padrão pré-configuradas no sistema.
        uint32_t    s_first_meta_bg;          // ID do primeiro meta-grupo de blocos em discos de escala gigantesca.
        uint32_t    s_mkfs_time;              // Carimbo de data/hora (Timestamp Unix) de criação do sistema de arquivos.
        std::array<uint32_t, 17>    s_jnl_blocks; // Cópia de segurança dos blocos de dados do Inode do Journal (Backup).
        
        // Extensões de 64-bits para Grande Escala (0x150) 
        uint32_t    s_blocks_count_hi;        // Bits superiores (32-63) da contagem global de blocos do sistema.
        uint32_t    s_r_blocks_count_hi;      // Bits superiores (32-63) da contagem de blocos reservados ao root.
        uint32_t    s_free_blocks_count_hi;   // Bits superiores (32-63) da contagem de blocos livres.
        uint16_t    s_min_extra_isize;        // Tamanho extra mínimo em bytes que novos inodes devem reservar para metadados estendidos.
        uint16_t    s_want_extra_isize;       // Tamanho extra desejado em bytes a ser alocado para expansão futura do inode.
        uint32_t    s_flags;                  // Flags diversas do superbloco (ex: signed/unsigned char hash, test_io).
        uint16_t    s_raid_stride;            // Geometria RAID: Largura de um chunk medido em contagem de blocos lógicos.
        uint16_t    s_mmp_interval;           // Intervalo padrão de verificação do mecanismo de Proteção de Montagem Múltipla.
        uint64_t    s_mmp_block;              // Número absoluto do bloco físico de disco onde reside a estrutura MmpStruct.
        uint32_t    s_raid_stripe_width;      // Geometria RAID: Número de blocos lógicos em uma stripe completa de dados.
        uint8_t     s_log_groups_per_flex;    // Tamanho do agrupamento flexível de blocos (FlexBG), expresso como potência de 2.
        uint8_t     s_checksum_type;          // Tipo do algoritmo de checksum utilizado no superbloco (1 para CRC32c).
        uint8_t     s_encryption_level;       // Nível/Versão do motor de criptografia nativo ativo.
        uint8_t     s_reserved_pad;           // Byte de alinhamento com zero.
        uint64_t    s_kbytes_written;         // Contador vitalício de Kilobytes totais escritos neste disco ao longo da sua vida útil.
        
        // Mecanismo de Snapshots e Logs de Erro (0x190) 
        uint32_t    s_snapshot_inum;          // Número do Inode que gerencia os snapshots ativos do sistema.
        uint32_t    s_snapshot_id;            // ID sequencial do snapshot ativo.
        uint64_t    s_snapshot_r_blocks_count; // Contagem de blocos de disco reservados para uso exclusivo dos snapshots.
        uint32_t    s_snapshot_list;          // ID do inode cabeça da lista de snapshots.
        uint32_t    s_error_count;            // Contador global acumulado de erros de I/O de metadados detectados pelo kernel.
        uint32_t    s_first_error_time;       // Timestamp Unix da primeira falha histórica registrada.
        uint32_t    s_first_error_ino;        // Número do Inode afetado na ocorrência da primeira falha histórica.
        uint64_t    s_first_error_block;      // Número do bloco físico de disco afetado na primeira falha histórica.
        std::array<uint8_t, 32> s_first_error_func; // String contendo o nome da função em C do kernel Linux onde o primeiro erro disparou.
        uint32_t    s_first_error_line;       // Número da linha do código-fonte do kernel Linux associado à primeira falha.
        uint32_t    s_last_error_time;        // Timestamp Unix da última falha isolada ocorrida.
        uint32_t    s_last_error_ino;         // Número do Inode afetado na ocorrência da última falha.
        uint32_t    s_last_error_line;        // Número da linha do código-fonte do kernel Linux associado à última falha.
        uint64_t    s_last_error_block;       // Número do bloco físico de disco afetado na última falha.
        std::array<uint8_t, 32> s_last_error_func; // String contendo o nome da função em C do kernel Linux onde o último erro disparou.
        
        // Quotas e Metadados de Alocação Avançada (0x238) 
        std::array<uint8_t, 64> s_mount_opts; // String contendo as opções textuais passadas ao comando mount (ex: "noatime,nodiratime").
        uint32_t    s_usr_quota_inum;         // ID do Inode oculto que rastreia os limites e regras de quotas de Usuários.
        uint32_t    s_grp_quota_inum;         // ID do Inode oculto que rastreia os limites e regras de quotas de Grupos.
        uint32_t    s_overhead_blocks;        // Quantidade de blocos consumidos inevitavelmente por metadados fixos do sistema de arquivos.
        std::array<uint32_t, 2> s_backup_bgs; // IDs dos blocos que contêm cópias reservas redundantes do Superbloco (Se ativo sparse_super2).
        std::array<uint8_t, 4>  s_encrypt_algos; // Identificadores dos algoritmos de criptografia de arquivos em uso.
        std::array<uint8_t, 16> s_encrypt_pw_salt; // Sal (Salt) criptográfico mestre utilizado para derivação de chaves.
        uint32_t    s_lpf_ino;                // ID do Inode associado à pasta padrão de recuperação "lost+found".
        uint32_t    s_prj_quota_inum;         // ID do Inode oculto que gerencia quotas baseadas em Identificadores de Projeto.
        uint32_t    s_checksum_seed;          // Semente pré-calculada do UUID para agilizar o cálculo de hashes CRC32c de estruturas internas.
        
        // Extensões de Alta Resolução para Timestamps (Campos Hi-Res Epoch) (0x2C4) 
        uint8_t     s_wtime_hi;               // Bits de época adicionais para estender o campo s_wtime além do bug do ano 2038.
        uint8_t     s_mtime_hi;               // Bits de época adicionais para estender o campo s_mtime além do bug do ano 2038.
        uint8_t     s_mkfs_time_hi;           // Bits de época adicionais para estender o campo s_mkfs_time além do bug do ano 2038.
        uint8_t     s_lastcheck_hi;           // Bits de época adicionais para estender o campo s_lastcheck além do bug do ano 2038.
        uint8_t     s_first_error_time_hi;    // Bits de época adicionais para estender o campo s_first_error_time.
        uint8_t     s_last_error_time_hi;     // Bits de época adicionais para estender o campo s_last_error_time.
        uint8_t     s_first_error_errcode;    // Código numérico de erro POSIX associado à primeira falha registrada.
        uint8_t     s_last_error_errcode;     // Código numérico de erro POSIX associado à última falha registrada.
        
        // Configurações Modernas do Kernel e Ficheiro de Órfãos (0x2CC) 
        uint16_t    s_encoding;               // Código identificador da tabela de codificação de caracteres do disco (ex: UTF-8).
        uint16_t    s_encoding_flags;         // Flags operacionais para o motor de tradução de codificação de strings.
        uint32_t    s_orphan_file_inum;       // ID do Inode especial associado ao Orphan File (Mecanismo paralelo de órfãos).
        
        // Preenchimento Final de Segurança (Padding) (0x2D8) 
        // Garante a reserva exata de espaço livre para compatibilidade com futuras expansões do Kernel Linux.
        std::array<uint32_t, 94>    s_reserved; 
        
        // [0x3FC] Valor numérico do Checksum calculado sobre toda a extensão física do Superbloco (via CRC32c).
        uint32_t    s_checksum;
    };
    #pragma pack(pop)

    // O Superbloco deve possuir exatamente 1024 bytes físicos em disco, conforme o padrão POSIX/Linux
    static_assert(sizeof(SuperBlock) == 1024, "A estrutura mestre SuperBlock deve medir exatamente 1024 bytes!");
}