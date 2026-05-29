#pragma once

namespace Ext4::Inode {
    // Modo do arquivo e permissões (i_mode)
    enum InodeMode : uint16_t {
        // Permissões de Acesso
        S_IXOTH = 0x0001, // Outros utilizadores têm permissão de execução
        S_IWOTH = 0x0002, // Outros utilizadores têm permissão de escrita
        S_IROTH = 0x0004, // Outros utilizadores têm permissão de leitura
        S_IXGRP = 0x0008, // Membros do grupo têm permissão de execução
        S_IWGRP = 0x0010, // Membros do grupo têm permissão de escrita
        S_IRGRP = 0x0020, // Membros do grupo têm permissão de leitura
        S_IXUSR = 0x0040, // O proprietário tem permissão de execução
        S_IWUSR = 0x0080, // O proprietário tem permissão de escrita
        S_IRUSR = 0x0100, // O proprietário tem permissão de leitura
        
        // Atributos Especiais
        S_ISVTX = 0x0200, // Sticky bit
        S_ISGID = 0x0400, // Set GID
        S_ISUID = 0x0800, // Set UID
        
        // Tipos de Ficheiro (Mutuamente Exclusivos)
        S_IFIFO  = 0x1000, // FIFO (Pipe nomeado)
        S_IFCHR  = 0x2000, // Dispositivo de caracteres
        S_IFDIR  = 0x4000, // Diretório
        S_IFBLK  = 0x6000, // Dispositivo de blocos
        S_IFREG  = 0x8000, // Ficheiro regular
        S_IFLNK  = 0xA000, // Ligação simbólica (Symbolic link)
        S_IFSOCK = 0xC000  // Socket
    };

    // Máscara para extrair estritamente o tipo de ficheiro de i_mode
    static constexpr uint16_t S_IFMT = 0xF000;

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
    
    #pragma pack(push, 1)
    // Estruturas dependentes do Sistema Operativo Criador (Mapeamento Linux)
    struct Osd1Linux {
        uint32_t l_i_version;       // Versão do Inode (ou parte superior do refcount de EA_INODE)
    };
    #pragma pack(pop)

    #pragma pack(push, 1)
    struct Osd2Linux {
        uint16_t l_i_blocks_high;   // 16 bits superiores do contador de blocos
        uint16_t l_i_file_acl_high; // 16 bits superiores do bloco de atributos estendidos (ACL)
        uint16_t l_i_uid_high;      // 16 bits superiores do UID do proprietário
        uint16_t l_i_gid_high;      // 16 bits superiores do GID do grupo
        uint16_t l_i_checksum_lo;   // 16 bits inferiores do Checksum do inode
        uint16_t l_i_reserved;      // Espaço não utilizado
    };
    #pragma pack(pop)

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
        uint32_t  i_flags;          // Flags de comportamento do ficheiro (ver InodeFlags)
        
        Osd1Linux i_osd1;           // Campos específicos do SO criador (Mapeado para Linux)
        
        // Mapa de blocos (12 diretos, 1 indireto, 1 duplo, 1 triplo) OU Raiz da árvore de Extents
        uint8_t   i_block[60];      // 60 bytes de armazenamento inline para mapeamento de dados
        
        uint32_t  i_generation;     // Versão do ficheiro (utilizado principalmente para exportações NFS)
        uint32_t  i_file_acl_lo;    // 32 bits inferiores do bloco de atributos estendidos (ACL)
        uint32_t  i_size_high;      // 32 bits superiores do tamanho do ficheiro (anteriormente i_dir_acl)
        uint32_t  i_obso_faddr;     // Endereço de fragmento obsoleto
        
        Osd2Linux i_osd2;           // Campos específicos adicionais do SO (Mapeado para Linux)

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

    /**
     * @brief Encapsula um `Inode`. Operações de conveniência.
     */
    class InodeWrapper {
    private:
        // O número global do inode, começando em 2 para o diretório raiz.
        uint32_t inode_id = 0;
        // O inode encapsulado, contendo os metadados físicos do ficheiro ou diretório.
        Inode inode{};

    public:
        InodeWrapper() = default;
        
        InodeWrapper(uint32_t id, const Inode& inode_data) 
            : inode_id(id), inode(inode_data) {}

        uint32_t get_inode_id() const { return this->inode_id; }
        const Inode& get_inode() const { return this->inode; }

        InodeMode get_type() const {
            return static_cast<InodeMode>(this->inode.i_mode & S_IFMT);
        }        
    };
}