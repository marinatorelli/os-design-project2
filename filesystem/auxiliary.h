
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	auxiliary.h
 * @brief 	Headers for the auxiliary functions required by filesystem.c.
 * @date	Last revision 01/04/2020
 *
 */
 
int write_metadata();

int read_metadata();

int ialloc();

int balloc();

int ifree(int inode_id);

int bfree(int block_id);

int namei(char *fname);

int bmap(int inode_id, int offset);

int add_data_block(int inode_id);

int remove_links(int inode_id);
