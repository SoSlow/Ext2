

typedef  _int16 Int16;
typedef  _int32 Int32;

typedef unsigned _int8 UInt8;
typedef unsigned _int16 UInt16;
typedef unsigned _int32 UInt32;
typedef unsigned _int64 UInt64;
 
const UInt16 EXT2_SUPER_MAGIC = 0xEF53;

typedef struct
{
	UInt32   s_inodes_count;         /* Inodes count */
	UInt32   s_blocks_count;         /* Blocks count */
	UInt32   s_r_blocks_count;       /* Reserved blocks count */
	UInt32   s_free_blocks_count;    /* Free blocks count */
	UInt32   s_free_inodes_count;    /* Free inodes count */
	UInt32   s_first_data_block;     /* First Data Block */
	UInt32   s_log_block_size;       /* Block size */
	Int32    s_log_frag_size;        /* Fragment size */
	UInt32   s_blocks_per_group;     /* # Blocks per group */
	UInt32   s_frags_per_group;      /* # Fragments per group */
	UInt32   s_inodes_per_group;     /* # Inodes per group */
	UInt32   s_mtime;                /* Mount time */
	UInt32   s_wtime;                /* Write time */
	UInt16   s_mnt_count;            /* Mount count */
	Int16	 s_max_mnt_count;        /* Maximal mount count */
	UInt16   s_magic;                /* Magic signature */
	UInt16   s_state;                /* File system state */
	UInt16   s_errors;               /* Behaviour when detecting errors */
	UInt16   s_pad;
	UInt32   s_lastcheck;            /* time of last check */
	UInt32   s_checkinterval;        /* max. time between checks */
	UInt32   s_creator_os;           /* OS */
	UInt32   s_rev_level;            /* Revision level */
	UInt16   s_def_resuid;           /* Default uid for reserved blocks */
	UInt16   s_def_resgid;           /* Default gid for reserved blocks */
	UInt32	 s_first_inode;			 /* First inode */
	UInt32	 s_inode_size;			 /* Inodes size */
	UInt32   s_reserved[256*2+233];  /* Padding to the end of the block */
} Ext2SuperBlock;

const UInt16 INODE_TYPE_SYMBOL_LINK = 0xA000; //символьная ссылка;
const UInt16 INODE_TYPE_FILE = 0x8000; //обычный файл;
const UInt16 INODE_TYPE_DEV = 0x6000; //файл блочного устройства;
const UInt16 INODE_TYPE_DIRECTORY = 0x4000; //каталог;
const UInt16 INODE_TYPE_CHAR_DEV = 0x2000; //файл символьного устройства;
const UInt16 INODE_TYPE_FIFO = 0x1000; //канал FIFO.

typedef struct 
{
	UInt32   bg_block_bitmap;        /* Blocks bitmap block */
	UInt32   bg_inode_bitmap;        /* Inodes bitmap block */
	UInt32   bg_inode_table;         /* Inodes table block */
	UInt16   bg_free_blocks_count;   /* Free blocks count */
	UInt16   bg_free_inodes_count;   /* Free inodes count */
	UInt16   bg_used_dirs_count;     /* Directories count */
	UInt16   bg_pad;
	UInt32   bg_reserved[3];
}Ext2GroupDescriptor;

typedef struct
{
    UInt16   i_mode;         /* File mode */
    UInt16   i_uid;          /* Owner Uid */
    UInt32   i_size;         /* Size in bytes */
    UInt32   i_atime;        /* Access time */
    UInt32   i_ctime;        /* Creation time */
    UInt32   i_mtime;        /* Modification time */
    UInt32   i_dtime;        /* Deletion Time */
    UInt16   i_gid;          /* Group Id */
    UInt16   i_links_count;  /* Links count */
    UInt32   i_blocks;       /* Blocks count */
    UInt32   i_flags;        /* File flags */
    union {
            struct {
                    UInt32  l_i_reserved1;
            } linux1;
            struct {
                    UInt32  h_i_translator;
            } hurd1;
            struct {
                    UInt32  m_i_reserved1;
            } masix1;
    } osd1;                         /* OS dependent 1 */
    UInt32   i_block[15];	 /* Pointers to blocks */
    UInt32   i_version;      /* File version (for NFS) */
    UInt32   i_file_acl;     /* File ACL */
    UInt32   i_dir_acl;      /* Directory ACL */
    UInt32   i_faddr;        /* Fragment address */
    union {
            struct {
                    UInt8    l_i_frag;       /* Fragment number */
                    UInt8    l_i_fsize;      /* Fragment size */
                    UInt16   i_pad1;
                    UInt32   l_i_reserved2[2];
            } linux2;
            struct {
                    UInt8    h_i_frag;       /* Fragment number */
                    UInt8    h_i_fsize;      /* Fragment size */
                    UInt16   h_i_mode_high;
                    UInt16   h_i_uid_high;
                    UInt16   h_i_gid_high;
                    UInt32   h_i_author;
            } hurd2;
            struct {
                    UInt8    m_i_frag;       /* Fragment number */
                    UInt8    m_i_fsize;      /* Fragment size */
                    UInt16   m_pad1;
                    UInt32   m_i_reserved2[2];
            } masix2;
    } osd2;                         /* OS dependent 2 */
}Ext2Inode;

//Defined Reserved InodesConstant Name	Value	Description
const UInt32 EXT2_BAD_INO			= 1;//	bad blocks inode
const UInt32 EXT2_ROOT_INO 			= 2;//	root directory inode
const UInt32 EXT2_ACL_IDX_INO		= 3;//	ACL index inode (deprecated?)
const UInt32 EXT2_ACL_DATA_INO		= 4;//	ACL data inode (deprecated?)
const UInt32 EXT2_BOOT_LOADER_INO	= 5;//	boot loader inode
const UInt32 EXT2_UNDEL_DIR_INO		= 6;//	undelete directory inode

typedef struct 
{
	UInt32   inode;                 /* Inode number */
	UInt16   rec_len;               /* Directory entry length */
	UInt8   name_len;				/* Name length */
	UInt8   file_type;				/* File type */
	char    name[256];				/* File name */

}Ext2DirEntry;

//Ext2DirEntry file_types
const UInt8 EXT2_FT_UNKNOWN	= 0;//Unknown File Type
const UInt8 EXT2_FT_REG_FILE= 1;//	Regular File
const UInt8 EXT2_FT_DIR		= 2;//	Directory File
const UInt8 EXT2_FT_CHRDEV	= 3;//	Character Device
const UInt8 EXT2_FT_BLKDEV	= 4;//	Block Device
const UInt8 EXT2_FT_FIFO	= 5;//	Buffer File
const UInt8 EXT2_FT_SOCK	= 6;//	Socket File
const UInt8 EXT2_FT_SYMLINK	= 7;//	Symbolic Link