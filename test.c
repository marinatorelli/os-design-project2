
/*
 *
 * Operating System Design / Diseño de Sistemas Operativos
 * (c) ARCOS.INF.UC3M.ES
 *
 * @file 	test.c
 * @brief 	Implementation of the client test routines.
 * @date	01/03/2017
 *
 */


#include <stdio.h>
#include <string.h>
#include "filesystem/filesystem.h"


// Color definitions for asserts
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"

#define N_BLOCKS 300					  // Number of blocks in the device
#define DEV_SIZE N_BLOCKS *BLOCK_SIZE // Device size, in bytes
#define N_INODES 48
#define NAME_LENGTH 32

int main()
{
	int ret;
	
	///////  Make file system with a size smaller than the minimum size
	ret = mkFS(5*1024);
        if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-01 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-01 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	
	/////// Make file system with a size larger than the maximum size
	ret = mkFS(1000*1024);
        if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-02 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-02 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);
	
	/////// Correct functionality of mkFS
	ret = mkFS(DEV_SIZE);
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-03 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mkFS TP-03 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Correct functionality of mountFS
	ret = mountFS();
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS TP-04 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS TP-04 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Check that you cannot mount a system when it is already mounted
	ret = mountFS();
	if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS TP-05 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST mountFS TP-05 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Correct functionality of createFile
	ret = createFile("/test.txt");
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-06 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-06 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Check that we cannot create a file with the name of a file that already exists
	ret = createFile("/test.txt");
	if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-07 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
	}  
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-07 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Check that we cannot create more than 48 files
	char fileName[NAME_LENGTH];
	for (int i=0; i<N_INODES; i++) {
	    sprintf(fileName, "/file%d.txt", i);
            ret = createFile(fileName);
            /* As we already created 1 file, the last file of this loop would be the 49th file created */
	    if (i == N_INODES-1 && ret != -2)
	    {
		    fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-08 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		    return -1;
	    }		
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createFile TP-08 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Correct functionality of openFile
	ret = openFile("/test.txt");
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile TP-09 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}  
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile TP-09 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Check that we cannot open a file that does not exist
	ret = openFile("/notafile.txt");
	if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile TP-10 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFile TP-10 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Correct functionality of writeFile
        char buffer_write[BLOCK_SIZE];
        memset(buffer_write, 1, BLOCK_SIZE);
        /* We will write into the file "/test.txt" a block full of 1s */
	ret = writeFile(0, buffer_write, sizeof(buffer_write));
	if (ret != BLOCK_SIZE)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-11 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-11 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);    

	/////// Correct functionality of lseekFile
	ret = lseekFile(0, 0, FS_SEEK_BEGIN); // Move the pointer of the file we just wrote to the beginning of the file
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST lseekFile TP-12 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST lseekFile TP-12 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET); 

	/////// Correct functionality of readFile
        char buffer_read[BLOCK_SIZE];
	ret = readFile(0, buffer_read, sizeof(buffer_read)); // Read a block from the file "/test.txt", which we wrote in the previous tests
	if (ret != BLOCK_SIZE)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST readFile TP-13 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST readFile TP-13 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);  

	/////// Check that reading an empty file returns 0 bytes
	ret = readFile(1, buffer_read, sizeof(buffer_read)); // Except the first file, the rest of the files are empty
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST readFile TP-14 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST readFile TP-14 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);   

	/////// Check that lseekFile works correctly with an offset
        ret = lseekFile(0, -1024, FS_SEEK_CUR); // The file "/test.txt" has 2048 bytes, so we will move the pointer to the half point and try to read 2048 bytes, the readFile function should only read 1024 bytes
	ret = readFile(0, buffer_read, sizeof(buffer_read));
	if (ret != 1024)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST lseekFile TP-15 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST lseekFile TP-15 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

	/////// Check that we cannot write more bytes than the maximum size of a file
        char buffer_maximum[MAX_FILE_SIZE + 1];
        memset(buffer_maximum, 2, sizeof(buffer_maximum));
        ret = openFile("/file0.txt");
        ret = writeFile(ret, buffer_maximum, sizeof(buffer_maximum));
        if (ret != MAX_FILE_SIZE)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-16 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-16 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);  

	/////// Correct functionality of removeFile
	ret = removeFile("/test.txt");
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST removeFile TP-17 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST removeFile TP-17 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET); 

        
	/////// Check that we cannot write to a file that does not exist
        ret = writeFile(0, buffer_write, sizeof(buffer_write)); // As the file we previously removed corresponded to inode 0 we will use that fd for this check
        if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-18 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}       
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST writeFile TP-18 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);  
        
	/////// Check that we cannot open with integrity a file that does not include it
        ret = openFileIntegrity("/file2.txt");
        if (ret != -3)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-19 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}  
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-19 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);  

	/////// Check that we cannot close with integrity a file that was opened without it
        ret = openFile("/file2.txt");
        ret = closeFileIntegrity(ret);
        if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFileIntegrity TP-20 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}  
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFileIntegrity TP-20 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);  
	
        /////// Correct functionality of closeFile
	ret = closeFile(1);
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFile TP-21 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFile TP-21 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET); 

        /////// Correct functionality of includeIntegrity
	ret = includeIntegrity("/file0.txt"); // We use this function with the file we just closed
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST includeIntegrity TP-22 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST includeIntegrity TP-22 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);      

        /////// Correct functionality of openFileIntegrity
	ret = openFileIntegrity("/file0.txt"); // Open the file to which we just added integrity
	if (ret != 1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-23 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-23 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);    

        /////// Correct functionality of closeFileIntegrity
	ret = closeFileIntegrity(1); // Close the file we just opened
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFileIntegrity TP-23 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST closeFileIntegrity TP-23 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Correct functionality of openFileIntegrity when file is corrupted
        char buffer_integrity[1024];
        memset(buffer_integrity, 3, sizeof(buffer_integrity));
        /* Open a file, write something into it and close it */
        ret = openFile("/file1.txt");
        writeFile(ret, buffer_integrity, sizeof(buffer_integrity));
        ret = closeFile(ret);
        /* Include integrity and open file again without integrity and write something, corrupting the file */
	ret = includeIntegrity("/file1.txt");
        ret = openFile("/file1.txt");
        writeFile(ret, buffer_integrity, sizeof(buffer_integrity));
        ret = closeFile(ret);
        /* When we open the file with integrity it will be corrupted */
        ret = openFileIntegrity("/file1.txt");
	if (ret != -2)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-24 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
                return -1;
	}   
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST openFileIntegrity TP-24 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);


        /////// Correct functionality of createLn
	ret = createLn("/file0.txt", "/link0"); // As we previously deleted one file we will have space for one link
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createLn TP-25 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST createLn TP-25 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// When we remove a file its symbolic links are also deleted
        ret = removeFile("/file0.txt"); 
        ret = removeLn("/link0"); // As the link was already deleted with removeFile this function should return that the link does no exist      
	if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST removeLn TP-26 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST removeLn TP-26 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Correct functionality of unmount
	ret = unmountFS();
	if (ret != 0)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS TP-27 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS TP-27 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Check that we cannot unmount a system that is already unmounted
	ret = unmountFS();
	if (ret != -1)
	{
		fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS TP-28 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		return -1;
	}
	fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST unmountFS TP-28 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        /////// Check that if we reach the limit of the device we cannot write more blocks
        ret = mkFS(243*BLOCK_SIZE); // As the maximum size of a file is 5 blocks and we can have maximum 48 files they can occupy maximum 240 blocks, so we will create a device where we will be able to fill all files with 5 blocks except 1, which will only have 4 blocks
        ret = mountFS();
        
        // Create 48 files and fill them up to their maximum size
	char newfileName[NAME_LENGTH];
        char newBuffer[MAX_FILE_SIZE];
	for (int i=0; i<N_INODES; i++) {
	    sprintf(newfileName, "/file%d.txt", i);
            ret = createFile(newfileName);
            memset(newBuffer, i+1, sizeof(newBuffer));
            ret = openFile(newfileName);
            ret = writeFile(ret, newBuffer,sizeof(newBuffer));
            // The last file we create will only have space for four blocks
	    if (i == N_INODES-1 && ret != MAX_FILE_SIZE-BLOCK_SIZE)
	    {
		    fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST TP-29 ", ANSI_COLOR_RED, "FAILED\n", ANSI_COLOR_RESET);
		    return -1;
	    }		
	}
        fprintf(stdout, "%s%s%s%s%s", ANSI_COLOR_BLUE, "TEST TP-29 ", ANSI_COLOR_GREEN, "SUCCESS\n", ANSI_COLOR_RESET);

        ret = unmountFS();

	///////

	return 0;
}
