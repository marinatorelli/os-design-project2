
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	metadata.h
 * @brief 	Definition of the structures and data types of the file system.
 * @date	Last revision 01/04/2020
 *
 */
#define MAGIC_NUM 1234
#define N_INODES 48
#define NAME_LENGTH 32
#define INODES_BLOCK 16
#define MIN_SIZE_DISK 460*1024
#define MAX_SIZE_DISK 600*1024

#define REGULAR 0
#define SYM_LINK 1


#define bitmap_getbit(bitmap_, i_) (bitmap_[i_ >> 3] & (1 << (i_ & 0x07)))
static inline void bitmap_setbit(char *bitmap_, int i_, int val_) {
  if (val_)
    bitmap_[(i_ >> 3)] |= (1 << (i_ & 0x07));
  else
    bitmap_[(i_ >> 3)] &= ~(1 << (i_ & 0x07));
}

typedef struct Superblock {
    int magic_number;
    int n_inodes; /* Number of inodes on the device */
    int n_blocks_inodes; /* Number of blocks for the inodes */
    int n_data_blocks; /* Number of data blocks on the device */
    int first_data_block; /* Logical address of the first data block */
    int device_size; /* Total device size in bytes*/
    char inode_map[N_INODES/8];  /* Number of blocks of the inode map*/
    char block_map[((MAX_SIZE_DISK/BLOCK_SIZE)-1-N_INODES/INODES_BLOCK)/8]; /* Number of blocks of the data map */
    char padding[BLOCK_SIZE-6*4-N_INODES/8-((MAX_SIZE_DISK/BLOCK_SIZE)-1-N_INODES/INODES_BLOCK)/8]; /* Padding to fill a block */
} Superblock;

typedef struct Inode {
    int type; /* REGULAR or SYM_LINK */
    char name[NAME_LENGTH]; /* Name of the associated file or link */
    int inode; /* Id of referenced inode in case it is a symbolic link */
    int size; /* File size in bytes */
    int direct_block; /* Direct block number */
    int indirect_block1; /* Indirect block number */
    int indirect_block2; /* Indirect block number */
    int indirect_block3; /* Indirect block number */
    int indirect_block4; /* Indirect block number */
    int includes_integrity; /* Whether the file includes integrity or not */
    uint32_t integrity; /* CRC checksum */
    char padding[(BLOCK_SIZE/INODES_BLOCK)-10*4-NAME_LENGTH]; /* Padding, we will have 16 inodes per block so each inode will fill 128 bytes */
} Inode;
