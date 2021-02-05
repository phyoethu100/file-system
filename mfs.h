/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  mfs.h
*
* Description: Functions for traversing through the file system, creating
*              directories, modifying files, and displaying them. This
*			   interacts directly with our driver.
*              
**************************************************************************/
#ifndef MFS_H
#define MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "fsLow.h"
#include "dirEntry.h"

#include <dirent.h>
#define FT_REGFILE DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK DT_LNK

#define MAX_FILEPATH_SIZE 510 // MAX_FILENAME_SIZE * MAX_DIRECTORY_DEPTH + MAX_DIRECTORY_DEPTH (for the slashes "/")
#define MAX_DIRECTORY_DEPTH 10

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

struct fs_diriteminfo
{
	unsigned short d_reclen; /* length of this record */
	unsigned char fileType;
	char d_name[MAX_FILENAME_SIZE]; /* filename max filename is 255 characters */
};

typedef struct
{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	// this is to know what directory we are in. it can help us iterate through our directory
	int init;									  // to see if the structure has been initialized or not (1 or 0)
	char childrenNames[MAXDE][MAX_FILENAME_SIZE]; // an array that holds names of the children
	char name[MAX_FILENAME_SIZE];				  // holds the file name
	char parentName[MAX_FILENAME_SIZE];
	int parentLBAindex;
	char absolutePath[MAX_FILEPATH_SIZE];
	int dirLen;						 /*length of this record */
	unsigned short dirEntryPosition; /*which directory entry position, like file pos */
	uint64_t LBAindex;				 /*Starting LBA of directory */
	DirEntry *cwd;					 // DirEntry dir[MAXDE];					  // load a directory to memory for easy access/traversing

} fdDir;

extern fdDir curDir;

int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir *fs_opendir(const char *name);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char *fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *pathname);
int fs_isFile(char *path);						//return 1 if file, 0 otherwise
int fs_isDir(char *path);						//return 1 if directory, 0 otherwise
int fs_delete(char *filename);					//removes a file
int fs_moveFile(char *srcFile, char *destFile); //return 0 if success, -1 for failure

// helpers
void initCWD();
int getDirLen(DirEntry *dir);
void initChildrenNames(DirEntry *dir);
void displayCWD();
void pathToArray(const char *path, char arr[][MAX_FILENAME_SIZE], int *pathArrLen);
void initDir(DirEntry *dir);
int parsePath(const char *pathname, char *newAbsolutePath);
// int currentTime();

struct fs_stat
{
	// this is metadata for a file. is a way to get it all from a file
	off_t st_size;		  /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t st_blocks;	  /* number of 512B blocks allocated */
	time_t st_accesstime; /* time of last access */
	time_t st_modtime;	  /* time of last modification */
	time_t st_createtime; /* time of last status change */

	/* add additional attributes here for your file system */
};

int fs_stat(const char *path, struct fs_stat *buf);

#endif
