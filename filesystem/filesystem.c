
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	filesystem.c
 * @brief 	Implementation of the core file system funcionalities and auxiliary functions.
 * @date	Last revision 01/04/2020
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "filesystem/filesystem.h" // Headers for the core functionality
#include "filesystem/auxiliary.h"  // Headers for auxiliary functions
#include "filesystem/metadata.h"   // Type and structure declaration of the file system

struct Superblock s_block;
struct Inode i_nodes[N_INODES];
struct inode_x {
  unsigned int open;
  unsigned int open_integrity;
  unsigned int f_seek;
} inode_x[N_INODES];
int mounted = 0;

/*
 * @brief 	Generates the proper file system structure in a storage device, as designed by the student.
 * @return 	0 if success, -1 otherwise.
 */
int mkFS(long deviceSize)
{ 
    /* Check validity of argument */
    if (deviceSize < MIN_SIZE_DISK) {
	perror("Error mkFS: The size of the disk is smaller than the minimum size\n");
	return -1;
    }
    if (deviceSize > MAX_SIZE_DISK) {
	perror("Error makeFS: The size of the disk is larger than the maximum size\n");
	return -1;
    }

    /* Compute the total number of blocks in the device */
    int disk_blocks = deviceSize/BLOCK_SIZE;    

    /* Initialize values of superblock */
    memset(&s_block, 0, BLOCK_SIZE);
    s_block.magic_number = MAGIC_NUM;
    s_block.n_inodes = N_INODES;
    /* We will have 16 inodes per block, so 3 blocks for the inodes in total */
    s_block.n_blocks_inodes = N_INODES/INODES_BLOCK;
    s_block.n_data_blocks = disk_blocks-1-s_block.n_blocks_inodes;
    s_block.first_data_block = 1 + s_block.n_blocks_inodes;
    s_block.device_size = deviceSize;

    /* Initialize all values of i_nodes to 0 */
    memset(i_nodes, 0, sizeof(i_nodes)); 

    /* Run command to create the disk file */
    char command[20];
    sprintf(command, "./create_disk %d", disk_blocks);    
    system(command);

    /* Write file system metadata to disk */
    if (write_metadata() == -1){
        return -1;
    }

    /* Initialize all data blocks to 0 */
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < s_block.n_data_blocks; i++) {
	if (bwrite(DEVICE_IMAGE, i+s_block.first_data_block, buffer) == -1) {
            perror("mkFS: Error initializing data blocks to 0\n");
            return -1;
        }
    }
    return 0;
}

/*
 * @brief 	Mounts a file system in the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int mountFS(void)
{
    /* There's an error if we try to mount a system that is already mounted */
    if (mounted == 1) {
	perror("Error mountFS: The file system is already mounted\n");
        return -1;
    }
    /* Read metadata from disk to memory */
    if (read_metadata() == -1) {
        return -1;
    }
    mounted = 1;
    return 0;
}

/*
 * @brief 	Unmounts the file system from the simulated device.
 * @return 	0 if success, -1 otherwise.
 */
int unmountFS(void)
{
    /* You can't unmount a system that isn't mounted */
    if (mounted == 0) {
	perror("unmountFS: The file system is already unmounted\n");
        return -1;
    }
    /* Write metadata from memory to disk, so that it perdures between unmount and mount */
    if (write_metadata() == -1) {
        return -1;
    }
    /* Delete data from current session */
    memset(inode_x, 0, sizeof(inode_x));
    mounted = 0;
    return 0;
}

/*
 * @brief	Creates a new file, provided it it doesn't exist in the file system.
 * @return	0 if success, -1 if the file already exists, -2 in case of error.
 */
int createFile(char *fileName)
{
    /* Error checking in case the file system isn't mounted or the file already exists */
    if (mounted == 0) {
        perror("createFile: The file system is not mounted\n");
        return -2;
    }
    if (namei(fileName) >= 0) {
        perror("createFile: File name already exists\n");
        return -1;
    }
    /* Allocate an inode and a data block for the file */
    int inode_id, block_id;
    inode_id = ialloc();
    if (inode_id == -1) {
        return -2;
    }
    block_id = balloc();
    if (block_id == -1) {
        ifree(inode_id);
        return -2;
    }
    /* Initialize inode values */
    i_nodes[inode_id].type = REGULAR;
    strncpy(i_nodes[inode_id].name, fileName, NAME_LENGTH);
    /* When we create a file it will only use the direct block, indirect blocks will be initialized to 1 to show that they are not in use */
    i_nodes[inode_id].direct_block = block_id; 
    i_nodes[inode_id].indirect_block1 = -1;
    i_nodes[inode_id].indirect_block2 = -1;
    i_nodes[inode_id].indirect_block3 = -1;	
    i_nodes[inode_id].indirect_block4 = -1;		
    i_nodes[inode_id].size = 0;
    i_nodes[inode_id].includes_integrity = 0;

    /* Initialize values of inode in memory of current session too */
    inode_x[inode_id].f_seek = 0;    
    inode_x[inode_id].open = 0; /* We could change this to open directly a file when it is created */
    inode_x[inode_id].open_integrity = 0;

    return inode_id;
}

/*
 * @brief	Deletes a file, provided it exists in the file system.
 * @return	0 if success, -1 if the file does not exist, -2 in case of error..
 */
int removeFile(char *fileName)
{
    /* Check for errors in case the file system isn't mounted or if the file doesn't exist */
    if (mounted == 0) {
        perror("removeFile: The file system is not mounted\n");
        return -2;
    }
    int inode_id = namei(fileName);
    if (inode_id < 0) {
        perror("removeFile: File does not exist\n");
        return -1;
    } 
    /* We can't use this function to delete symbolic links */
    if (i_nodes[inode_id].type != REGULAR) {
        perror("removeFile: File is not of type regular\n");
        return -2;
    }

    /* Free the blocks, only freeing indirect blocks if they are being used by the inode (they are different than -1) */  
    if (bfree(i_nodes[inode_id].direct_block) == -1) {
        return -2;
    }
    if (i_nodes[inode_id].indirect_block1 != -1) {
        if (bfree(i_nodes[inode_id].indirect_block1) == -1) {
            return -2;
        }
    }
    if (i_nodes[inode_id].indirect_block2 != -1) {
        if (bfree(i_nodes[inode_id].indirect_block2) == -1) {
            return -2;
        }
    }
    if (i_nodes[inode_id].indirect_block3 != -1) {
        if (bfree(i_nodes[inode_id].indirect_block3) == -1) {
            return -2;
        }
    }
    if (i_nodes[inode_id].indirect_block4 != -1) {
        if (bfree(i_nodes[inode_id].indirect_block4) == -1) {
            return -2;
        }
    }
    /* When we delete a file we also delete all the symbolic links to that file */
    if (remove_links(inode_id) == -1) {
	perror("removeFile: Error deleting symbolic links to file\n");
        return -2;		
    }
    /* Free the inode */
    if (ifree(inode_id) == -1) {
        return -2;
    }
    return 0;
}

/*
 * @brief	Opens an existing file.
 * @return	The file descriptor if possible, -1 if file does not exist, -2 in case of error..
 */
int openFile(char *fileName)
{
    /* Check for errors if the file system ins't mounted or the file doesn't exist */
    if (mounted == 0) {
        perror("openFile: The file system is not mounted\n");
        return -2;
    }
    int inode_id = namei(fileName);
    if (inode_id < 0) {
        perror("openFile: File does not exist\n");
        return -1;
    }
    /* When we open a symbolic link we open the file the link points to */
    if (i_nodes[inode_id].type == SYM_LINK) {
        inode_id = i_nodes[inode_id].inode;
    }
    /* Check if the file was already opened with integrity */
    if (inode_x[inode_id].open_integrity == 1) {
        perror("openFile: File is already opened with integrity\n");
    }
    /* Move the pointer f_seek to the beginning of the file and set open field to 1*/
    inode_x[inode_id].f_seek = 0;    
    inode_x[inode_id].open = 1;
    inode_x[inode_id].open_integrity = 0;

    return inode_id;
}

/*
 * @brief	Closes a file.
 * @return	0 if success, -1 otherwise.
 */
int closeFile(int fileDescriptor)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("closeFile: The file system is not mounted\n");
        return -2;
    }
    if (fileDescriptor < 0 || fileDescriptor >= N_INODES) {
        perror("closeFile: File descriptor is not valid\n");
        return -1;
    }
    /* As we don't have the name of the file we have to check that the fileDescriptor corresponds to a created file */
    if (bitmap_getbit(s_block.inode_map, fileDescriptor) == 0) {
	perror("closeFile: File descriptor does not correspond to an existing iNode\n");
        return -1;		
    }
    /* If we close from a symbolic link we close the file it points to */
    if (i_nodes[fileDescriptor].type == SYM_LINK) {
        fileDescriptor = i_nodes[fileDescriptor].inode;
    }
    /* We cannot close without integrity a file that was opened with it */
    if (inode_x[fileDescriptor].open_integrity == 1) {
        perror("closeFile: File was opened with integrity\n");
        return -1;
    }
    /* Set vale of open to 0 and reset f_seek*/
    inode_x[fileDescriptor].open = 0;
    inode_x[fileDescriptor].f_seek = 0;
    return 0;
}

/*
 * @brief	Reads a number of bytes from a file and stores them in a buffer.
 * @return	Number of bytes properly read, -1 in case of error.
 */
int readFile(int fileDescriptor, void *buffer, int numBytes)
{
    /* Check for errors in arguments */
    if (mounted == 0) {
        perror("readFile: The file system is not mounted\n");
        return -1;
    }
    if (fileDescriptor < 0 || fileDescriptor >= N_INODES) {
        perror("readFile: File descriptor is not valid\n");
        return -1;    
    }
    if (bitmap_getbit(s_block.inode_map, fileDescriptor) == 0) {
	perror("readFile: File descriptor does not correspond to an existing iNode\n");
        return -1;		
    }
    /* Size cannot be negative */
    if (numBytes < 0) {
        perror("readFile: Size isn't valid\n");
        return -1;
    }
    /* If we try to read from a symbolic link we access the file it points to */
    if (i_nodes[fileDescriptor].type == SYM_LINK) {
        fileDescriptor = i_nodes[fileDescriptor].inode;
    }
	
    /* If the data the user wants to reads exceeds the size of the file we limit it to the maximum space available */
    if (inode_x[fileDescriptor].f_seek+numBytes > i_nodes[fileDescriptor].size)
        numBytes = i_nodes[fileDescriptor].size - inode_x[fileDescriptor].f_seek;

    char b[BLOCK_SIZE];
    int block_id, block_offset, buffer_offset = 0, bytes_read = 0;
    
    while (numBytes > 0) {
        /* Get in which block and where in the block we have to read */
        block_id = bmap(fileDescriptor, inode_x[fileDescriptor].f_seek);
        block_offset = inode_x[fileDescriptor].f_seek % BLOCK_SIZE;  

        /* As we will read block by block if the number of bytes to read exceeds the size of the block we limit it */
        int toRead;
        if (numBytes <= BLOCK_SIZE-block_offset)
            toRead = numBytes;
        else
            toRead = BLOCK_SIZE-block_offset;

        /* Read the block from the disk and write it into the buffer from the last position we wrote (indicated by the buffer offset) */
        bread(DEVICE_IMAGE, s_block.first_data_block+block_id, b);
        memmove(buffer+buffer_offset, b+block_offset, toRead);

        /* Update positions of pointers and variables */
        buffer_offset += toRead;
        inode_x[fileDescriptor].f_seek += toRead;
        numBytes -= toRead;  
        bytes_read += toRead;      
    }

    return bytes_read;
}

/*
 * @brief	Writes a number of bytes from a buffer and into a file.
 * @return	Number of bytes properly written, -1 in case of error.
 */
int writeFile(int fileDescriptor, void *buffer, int numBytes)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("writeFile: The file system isn't mounted\n");
        return -2;
    }
    if (fileDescriptor < 0 || fileDescriptor >= N_INODES) {
        perror("writeFile: File descriptor isn't valid\n");
        return -1;    
    }
    if (bitmap_getbit(s_block.inode_map, fileDescriptor) == 0) {
	perror("writeFile: File descriptor doesn't correspond to an existing iNode\n");
    return -1;		
    }
    if (i_nodes[fileDescriptor].type == SYM_LINK) {
        fileDescriptor = i_nodes[fileDescriptor].inode;
    }
    if (inode_x[fileDescriptor].open == 0 && inode_x[fileDescriptor].open_integrity == 0) {
        perror("writeFile: File is not openend\n");
        return -1;
    }
    if (numBytes < 0) {
        perror("writeFile: Size isn't valid\n");
    }
    
    char b[BLOCK_SIZE];
    int block_id, block_offset, bytes_written = 0;

    /* If the data the user wants to write exceeds the size of the file we limit it to the maximum space available */
    if (inode_x[fileDescriptor].f_seek+numBytes > MAX_FILE_SIZE)
        numBytes = MAX_FILE_SIZE - inode_x[fileDescriptor].f_seek;
   
    /* Check if we need to allocate a new indirect block for the file (when the file already has filled the previous block and the pointer is in the last position) */
    if (numBytes > 0 && i_nodes[fileDescriptor].size % BLOCK_SIZE == 0 && i_nodes[fileDescriptor].size > 0 && i_nodes[fileDescriptor].size == inode_x[fileDescriptor].f_seek) {
        if (add_data_block(fileDescriptor) == -1) {
            return bytes_written; /* If we can't add a data block it's because there is no more space in the disk so we will return 0 */
        }
    }  

    while (numBytes > 0) {
        /* Get the block where we have to write and where in the block */
        block_id = bmap(fileDescriptor, inode_x[fileDescriptor].f_seek);  
        block_offset = inode_x[fileDescriptor].f_seek % BLOCK_SIZE;  
      
        /* Limit the bytes to write to the capacity of the block */
        int toWrite;
        if (numBytes <= BLOCK_SIZE-block_offset)
            toWrite = numBytes;
        else
            toWrite = BLOCK_SIZE-block_offset;

        /* Read the block from the disk, add the data and write it again in the disk */
        bread(DEVICE_IMAGE, s_block.first_data_block+block_id, b);
        memmove(b+block_offset, buffer, toWrite);
        bwrite(DEVICE_IMAGE, s_block.first_data_block+block_id, b);

        /* Update the values of the pointers and variables */
        inode_x[fileDescriptor].f_seek += toWrite;
        i_nodes[fileDescriptor].size += toWrite;
        numBytes -= toWrite;
        bytes_written += toWrite;
        
        /* Check if we need to allocate another data block, when we are at the end of the file and there are still bytes to be written */
        if (inode_x[fileDescriptor].f_seek % BLOCK_SIZE == 0 && numBytes > 0 && i_nodes[fileDescriptor].size == inode_x[fileDescriptor].f_seek) {
            if (add_data_block(fileDescriptor) == -1) {
                return bytes_written; /* If we can't add a new data block it means there is no more space in the disk so we return the bytes written */
            }
        }
    }
    
    return bytes_written;
}

/*
 * @brief	Modifies the position of the seek pointer of a file.
 * @return	0 if succes, -1 otherwise.
 */
int lseekFile(int fileDescriptor, long offset, int whence)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("lseekFile: The file system is not mounted\n");
        return -2;
    }
    if (fileDescriptor < 0 || fileDescriptor >= N_INODES) {
        perror("lseekFile: File descriptor is not valid\n");
        return -1;    
    }
    if (bitmap_getbit(s_block.inode_map, fileDescriptor) == 0) {
        perror("lseekFile: File descriptor does not correspond to an existing iNode\n");
        return -1;		
    }
    if (i_nodes[fileDescriptor].type == SYM_LINK) {
        fileDescriptor = i_nodes[fileDescriptor].inode;
    }
    /* When the pointer is set to the current position, update adding the offset */
    if (whence == FS_SEEK_CUR) {
        /* Check that the pointer doesn't go outside of the limits of the file */
        if (inode_x[fileDescriptor].f_seek + offset > i_nodes[fileDescriptor].size || inode_x[fileDescriptor].f_seek + offset < 0) {
            perror("lseekFile: Cannot move pointer outside of the limits of the file\n");
            return -1;
        }
        inode_x[fileDescriptor].f_seek += offset;
    }
    /* Set the pointer to the end of the file */
    else if (whence == FS_SEEK_END) {
        inode_x[fileDescriptor].f_seek = i_nodes[fileDescriptor].size;
    } 
    /* Set te pointer to te beginning of the file */
    else if (whence == FS_SEEK_BEGIN) {
        inode_x[fileDescriptor].f_seek = 0;
    }
    else {
        perror("lseekFile: The value of the argument whence is not valid");
        return -1;
    }
    return 0;
}

/*
 * @brief	Checks the integrity of the file.
 * @return	0 if success, -1 if the file is corrupted, -2 in case of error.
 */

int checkFile (char * fileName)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("checkFile: The file system is not mounted\n");
        return -2;
    }
    int inode_id = namei(fileName);
    if (inode_id < 0) {
        perror("checkFile: File does not exist\n");
        return -2;
    }
    /* Access the actual file in case the argument is a symbolic link */
    if (i_nodes[inode_id].type == SYM_LINK) {
        inode_id = i_nodes[inode_id].inode;
    }
    /* We can only perform this check if the file includes integrity and is closed */
    if (i_nodes[inode_id].includes_integrity == 0) {
        perror("checkFile: File doesn't iclude integrity\n");
        return -2;
    }
    if (inode_x[inode_id].open == 1 || inode_x[inode_id].open_integrity == 1) {
        perror("checkFile: File is openend\n");
        return -2;
    }
    
    /* Get the hash value of the current contents of the file */
    unsigned char buffer[i_nodes[inode_id].size];
    lseekFile(inode_id, 0, FS_SEEK_BEGIN);
    readFile(inode_id, buffer, i_nodes[inode_id].size);
    uint32_t check = CRC32(buffer, i_nodes[inode_id].size);  

    /* Check that the value corresponds to the one already stored in the inode */
    if (check == i_nodes[inode_id].integrity) { /* Not corrupted */
        return 0;
    }
    else {  /* Corrupted */
        return -1;
    }
}

/*
 * @brief	Include integrity on a file.
 * @return	0 if success, -1 if the file does not exists, -2 in case of error.
 */

int includeIntegrity (char * fileName)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("includeIntegrity: The file system isn't mounted\n");
        return -2;
    }
    int inode_id = namei(fileName);
    if (inode_id < 0) {
        perror("includeIntegrity: File doesn't exist\n");
        return -1;
    }
    /* Access the actual file in case it is a symbolic link */
    if (i_nodes[inode_id].type == SYM_LINK) {
        inode_id = i_nodes[inode_id].inode;
    }
    if (i_nodes[inode_id].includes_integrity == 1) {
        perror("includeIntegrity: File already icludes integrity\n");
        return -2;
    }
    /* We can only perform this check if the file is closed */
    if (inode_x[inode_id].open == 1 || inode_x[inode_id].open_integrity == 1) {
        perror("includeIntegrity: File is openend\n");
        return -2;
    }

    /* The inode now includes integrity */
    i_nodes[inode_id].includes_integrity = 1;

    /* Compute the crc value from the contents of the file and store it in the corresponding field */
    unsigned char buffer[i_nodes[inode_id].size];
    lseekFile(inode_id, 0, FS_SEEK_BEGIN);
    readFile(inode_id, buffer, i_nodes[inode_id].size);
    uint32_t integrity = CRC32(buffer, i_nodes[inode_id].size);
    i_nodes[inode_id].integrity = integrity;

    return 0;
}

/*
 * @brief	Opens an existing file and checks its integrity
 * @return	The file descriptor if possible, -1 if file does not exist, -2 if the file is corrupted, -3 in case of error
 */
int openFileIntegrity(char *fileName)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("openFileIntegrity: The file system isn't mounted\n");
        return -3;
    }
    int inode_id = namei(fileName);
    if (inode_id < 0) {
        perror("openFileIntegrity: File doesn't exist\n");
        return -1;
    }
    /* In case it is a symbolic link access the actual file */
    if (i_nodes[inode_id].type == SYM_LINK) {
        inode_id = i_nodes[inode_id].inode;
    }
    if (inode_x[inode_id].open == 1 || inode_x[inode_id].open_integrity == 1) {
        perror("checkFile: File is already open\n");
        return -2;
    }
    if (i_nodes[inode_id].includes_integrity == 0) {
        perror("openFileIntegrity: File does not iclude integrity\n");
        return -3;
    }
    /* Use the function checkFile to know if the file is corrupted */
    if (checkFile(fileName) == -1) {
        perror("openFileIntegrity: File is corrupted\n");
        return -2;
    }
    /* If it isn't set the values of the session and open the file */
    else if (checkFile(fileName) == 0) {
        inode_x[inode_id].f_seek = 0;    
        inode_x[inode_id].open = 0;
        inode_x[inode_id].open_integrity = 1;      
        return inode_id;  
    }
    return -3; /* In case there's an error in checkFile */
}

/*
 * @brief	Closes a file and updates its integrity.
 * @return	0 if success, -1 otherwise.
 */
int closeFileIntegrity(int fileDescriptor)
{
    /* Check for errors */
    if (mounted == 0) {
        perror("closeFileIntegrity: The file system isn't mounted\n");
        return -1;
    }
    if (fileDescriptor < 0 || fileDescriptor >= N_INODES) {
        perror("closeFileIntegrity: File descriptor isn't valid\n");
        return -1;    
    }
    if (bitmap_getbit(s_block.inode_map, fileDescriptor) == 0) {
	perror("closeFileIntegrity: File descriptor doesn't correspond to an existing iNode");
        return -1;		
    }
    /* If the inode is a symbolic link we access the file it points to */
    if (i_nodes[fileDescriptor].type == SYM_LINK) {
        fileDescriptor = i_nodes[fileDescriptor].inode;
    }  
    /* If the file was opened without integrity it can't be closed with it */
    if (inode_x[fileDescriptor].open == 1) {
        perror("closeFileIntegrity: File was opened without integrity\n");
        return -1;    
    }
    /* File has to include integrity and have been opened with it */
    if (i_nodes[fileDescriptor].includes_integrity == 0) {
        perror("closeFileIntegrity: File does not iclude integrity\n");
        return -1;
    } 
    if (inode_x[fileDescriptor].open_integrity == 0) {
        perror("closeFileIntegrity: File is not open\n");
        return -1;
    }
    
    /* Compute the integrity of the file and update its field */
    unsigned char buffer[i_nodes[fileDescriptor].size];
    lseekFile(fileDescriptor, 0, FS_SEEK_BEGIN);
    readFile(fileDescriptor, buffer, i_nodes[fileDescriptor].size);
    uint32_t integrity = CRC32(buffer, i_nodes[fileDescriptor].size);
    i_nodes[fileDescriptor].integrity = integrity;

    /* Close file and reset f_seek */
    inode_x[fileDescriptor].open_integrity = 0;
    inode_x[fileDescriptor].f_seek = 0; 

    return 0;
}

/*
 * @brief	Creates a symbolic link to an existing file in the file system.
 * @return	0 if success, -1 if file does not exist, -2 in case of error.
 */
int createLn(char *fileName, char *linkName)
{
    /* Error checking in case the file system isn't mounted or the link already exists */
    if (mounted == 0) {
        perror("createLn: The file system is not mounted\n");
        return -2;
    }
    if (namei(linkName) >= 0) {
        perror("createLn: Name is already in use\n");
        return -2;
    }
    int file_inode = namei(fileName);
    if (file_inode < 0) {
        perror("createLn: File does not exist\n");
        return -1;
    }
    /* Symbolic links to other symbolic links are not allowed to avoid cycles */
    if (i_nodes[file_inode].type == SYM_LINK) {
        perror("createLn: Cannot create a symbolic link to another symbolic link");
        return -2;
    }
    /* Allocate an inode and a data block for the file */
    int inode_id;
    inode_id = ialloc();
    if (inode_id == -1) {
        return -2;
    }
    /* Initialize inode values, the rest of the fields will not be used for symbolic links */
    i_nodes[inode_id].type = SYM_LINK;
    strcpy(i_nodes[inode_id].name, linkName);
    i_nodes[inode_id].inode = file_inode;
    
    return 0;
}

/*
 * @brief 	Deletes an existing symbolic link
 * @return 	0 if the file is correct, -1 if the symbolic link does not exist, -2 in case of error.
 */
int removeLn(char *linkName)
{
    /* Error checking in case the file system is not mounted or the link already exists */
    if (mounted == 0) {
        perror("removeLn: The file system is not mounted\n");
        return -2;
    }
    int inode_id = namei(linkName);
    if (inode_id < 0) {
        perror("removeLn: Link name does not exist\n");
        return -1;
    }
    /* Cannot use this function to remove a regular file */
    if (i_nodes[inode_id].type != SYM_LINK) {
        perror("removeLn: Name does not correspond to a symbolic link");
        return -2;
    }
    /* Free the inode */
    if (ifree(inode_id) == -1) {
        return -2;
    }
    return 0;
}

/*
* @brief        Writes metadata to disk
* @return       0 if succes, -1 in case of error
*/
int write_metadata() {
    /* Write superblock to disk */
    if (bwrite(DEVICE_IMAGE, 0, (char*) &(s_block)) == -1) {
        perror("write_metadata: Error writing superblock to disk\n");
        return -1;
    }
    char buffer[BLOCK_SIZE];
    /* Write i_nodes to disk */
    /* For each inode block we have to write 16 inodes */
    for (int i = 0; i < s_block.n_blocks_inodes; i++) {
        int buffer_offset = 0;
        /* Fill a buffer with all the inodes that fit in a block and write it to the disk */
        for (int j = 0; j < INODES_BLOCK; j++) {
            memmove(buffer+buffer_offset, &(i_nodes[i*INODES_BLOCK+j]), sizeof(i_nodes[i*INODES_BLOCK+j]));
            buffer_offset += sizeof(i_nodes[i*INODES_BLOCK+j]);
        }
	if (bwrite(DEVICE_IMAGE, 1+i, buffer) == -1) {
	    perror("write_metadata: Error writing inodes to disk\n");
            return -1;
        }
    }
    return 0;
}

/*
* @brief        Reads metadata from disk
* @return       0 if succes, -1 in case of error
*/
int read_metadata() {
    /* Read superblock from disk */
    if (bread(DEVICE_IMAGE, 0, (char*) &(s_block)) == -1) {
        perror("read_metadata: Error reading superblock from disk\n");
        return -1;
    }
    char buffer[BLOCK_SIZE];
    /* Read inodes from disk, using a buffer as we did in the functon write_metadata() */
    for (int i = 0; i < N_INODES/INODES_BLOCK; i++) {
	if (bread(DEVICE_IMAGE, 1+i, buffer) == -1){
            perror("read_metadata: Error reading inode from disk\n");
            return -1;
        }
        int buffer_offset = 0;
        for (int j = 0; j < INODES_BLOCK; j++) {
            memmove(&(i_nodes[i*INODES_BLOCK+j]), buffer+buffer_offset, sizeof(i_nodes[i*INODES_BLOCK+j]));
            buffer_offset += sizeof(i_nodes[i*INODES_BLOCK+j]);
        }
    }
    return 0;
}

/*
* @brief        Allocates new inode
* @return       0 if succes, -1 in case there are no more free inodes
*/
int ialloc() {
    /* Look for a free inode using the map */
    for (int i = 0; i < N_INODES; i++) {
        if (bitmap_getbit(s_block.inode_map, i) == 0) {
            /* Allocate the inode */
            bitmap_setbit(s_block.inode_map, i, 1);
            return i;
        }
    }
    perror("ialloc: There are no free inodes\n");
    return -1;
}


/*
* @brief        Allocates new data block
* @return       0 if succes, -1 in case there are no more free data blocks
*/
int balloc() {
    /* Look for a free block using the map */
    for (int i = 0; i < s_block.n_data_blocks; i++) {
        if (bitmap_getbit(s_block.block_map, i) == 0) {
            /* Allocate te block */
            bitmap_setbit(s_block.block_map, i, 1);
            return i;
        }
    }
    perror("alloc: There are no free blocks\n");
    return -1;
}

/*
* @brief        Frees an inode
* @return       0 if succes, -1 in case of error
*/
int ifree(int inode_id) {
    /* Check validity of argument */
    if (inode_id < 0 || inode_id >= N_INODES) {
        perror("ifree: Node id isn't valid\n");
        return -1;
    }
    /* Set the bit in the map as free */
    bitmap_setbit(s_block.inode_map, inode_id, 0);
    /* Delete its values from the metadata */
    memset(&(i_nodes[inode_id]), 0, sizeof(i_nodes[inode_id]));
    memset(&(inode_x[inode_id]), 0, sizeof(inode_x[inode_id]));
    return 0;
}

/*
* @brief        Frees a data block
* @return       0 if succes, -1 in case there are no more free data blocks
*/
int bfree(int block_id) {
    /* Check the validity of argument */
    if (block_id < 0 || block_id >= s_block.n_data_blocks) {
        perror("bfree: Block id isn't valid\n");
        return -1;
    }
    /* Set the bit in the map as free */
    bitmap_setbit(s_block.block_map, block_id, 0);
    /* Delete contents of te block from the disk */
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, sizeof(buffer));
    if (bwrite(DEVICE_IMAGE, s_block.first_data_block+block_id, buffer) == -1) {
        perror("bfree: Couldn't delete block data\n");
        return -1;
    }
    return 0;
}

/*
* @brief        Looks for a file given its name
* @return       The id of the inode, -1 if the file doesn't exist
*/
int namei(char *fname) {
    /* Look for the name of the file in the metadata */
    for (int i=0; i < N_INODES; i++) {
        if (strcmp(i_nodes[i].name, fname) == 0) {
            return i;
        }
    }
    return -1;
}

/*
* @brief        Translates the offset of an inode to a block address
* @return       The address of the block containing the offset, -1 in case of error
*/
int bmap(int inode_id, int offset) {
    /* Check the validity of the arguments */
    if (inode_id < 0 || inode_id >= N_INODES) {
        perror("bmap: Node id is not valid\n");
        return -1;
    }
    /* Compute the block number of the offset in the file */
    int logic_block;
    logic_block = offset/BLOCK_SIZE;
    
    /* Return its corresponding address in the block map */
    if (logic_block == 0) {
        return i_nodes[inode_id].direct_block;
    }
    else if (logic_block == 1) {
        return i_nodes[inode_id].indirect_block1;
    }
    else if (logic_block == 2) {
        return i_nodes[inode_id].indirect_block2;
    }
    else if (logic_block == 3) {
        return i_nodes[inode_id].indirect_block3;
    }
    else if (logic_block == 4) {
        return i_nodes[inode_id].indirect_block4;
    }
    perror("bmap: Offset is not valid\n");
    return -1;
}

/*
* @brief        Allocates an indirect block to an inode
* @return       The id of the allocated block, -1 in case of error
*/
int add_data_block(int inode_id) {
    /* Check the validity of argument */
    if (inode_id < 0 || inode_id >= N_INODES) {
        perror("add_data_block: Node id isn't valid\n");
        return -1;
    }
    /* Compute which indirect block the new allocation corresponds to and allocate the block */
    int block_id = balloc();
    if (block_id == -1) {
        return -1;
    }
    int logic_block = i_nodes[inode_id].size/BLOCK_SIZE;
    if (logic_block == 1) {
        i_nodes[inode_id].indirect_block1 = block_id;
        return block_id;
    }
    else if (logic_block == 2) {
        i_nodes[inode_id].indirect_block2 = block_id;
        return block_id;
    }
    else if (logic_block == 3) {
        i_nodes[inode_id].indirect_block3 = block_id;
        return block_id;
    }
    else if (logic_block == 4) {
        i_nodes[inode_id].indirect_block4 = block_id;
        return block_id;
    }
    perror("add_data_block: Did not add indirect block");
    return -1;
}

/*
* @brief        Removes all existing links to the file represented by inode_id
* @return       0 if success, -1 in case of error
*/
int remove_links(int inode_id) {
    for (int i=0; i < N_INODES; i++) {
    /* If an allocated inode is a symbolic link and points to the file passed as argument we remove the symbolic link */
        if (bitmap_getbit(s_block.inode_map, i) == 1 && i_nodes[i].type == SYM_LINK && i_nodes[i].inode == inode_id) {
            if (removeLn(i_nodes[i].name) == -1) {
	        return -1;
	    }
        }			
    }
    return 0;
}
