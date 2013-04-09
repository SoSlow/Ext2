

typedef  _int16 Int16;
typedef  _int32 Int32;

typedef unsigned _int16 UInt16;
typedef unsigned _int32 UInt32;
 
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
	UInt32   s_reserved[256*2+235];        /* Padding to the end of the block */
} Ext2SuperBlock;

//typedef struct
//{
//	UInt32 s_inodes_count; //общее число inode в файловой системе;
//	UInt32 s_blocks_count; //общее число блоков в файловой системе;
//	UInt32 s_free_blocks_count; //количество свободных блоков;
//	UInt32 s_free_inodes_count; //количество свободных inode;
//	UInt32 s_first_data_block; //номер первого блока данных (номер блока, в котором находится суперблок);
//	UInt32 s_log_block_size; //это значение используется для вычисления размера блока. Размер блока определяется по формуле: block size = 1024 << s_log_block_size;
//	UInt32 s_blocks_per_group; //количество блоков в группе;
//	UInt32 s_inodes_per_group; //количество inode в группе;
//	UInt16 s_magic; //идентификатор файловой системы ext2 (сигнатура 0xEF53);
//	UInt16 s_inode_size; //размер информационного узла (inode);
//	UInt32 s_first_ino; //номер первого незарезервированного inode.
//} Ext2SuperBlock;


const UInt16 INODE_TYPE_SYMBOL_LINK = 0xA000; //символическая ссылка;
const UInt16 INODE_TYPE_FILE = 0x8000; //обычный файл;
const UInt16 INODE_TYPE_DEV = 0x6000; //файл блочного устройства;
const UInt16 INODE_TYPE_DIRECTORY = 0x4000; //каталог;
const UInt16 INODE_TYPE_CHAR_DEV = 0x2000; //файл символьного устройства;
const UInt16 INODE_TYPE_FIFO = 0x1000; //канал FIFO.


typedef struct
{
	UInt16 i_mode; //тип файла и права доступа к нему. Тип файла определяют биты 12-15 этого поля: 
	UInt32 i_size; //размер в байтах;
	UInt32 i_atime; //время последнего доступа к файлу;
	UInt32 i_ctime; //время создания файла;
	UInt32 i_mtime; //время последней модификации;
	UInt32 i_blocks; //количество блоков, занимаемых файлом;
	UInt32 *i_block; //адреса информационных блоков (включая все косвенные ссылки).
} Ext2Inode;


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
}Ext2GroupDescriptorTable;