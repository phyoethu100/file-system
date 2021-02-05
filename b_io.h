/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  b_io.h
*
* Description: Functions for reading and writing blocks of data. Open file
*              table and file control blocks are also declared here.
*              
**************************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>

#include "dirEntry.h"

#define MAXFCBS 20
#define BUFSIZE 512
#define BLKSIZE 50

typedef struct b_fcb
{
  int used;   // 0 means unused, 1 means used
  char *buf;  // open file buffer
  int bufPos; // used to keep track of position within buf
  int buflen; // tells us how many bytes are in the buffer
  int bufBytesRemain;
  int remainingBytes;
  int transferBytes;
  int fileSize; // taken from DE. Copying it into this structure for convenience
  int fileBlockSize;
  int fileLBAIndex; // gotten from DE. Copying into this structure for convenience
  int filePos;      // used to keep track of position in the file
  int flags;
  int totalBlocks;
  //  int length;
  char fileName[MAX_FILENAME_SIZE]; // name of file the DE points to
  int isFile;                       // 0 for not a file 1 for file
  int parentLBAIndex;

} b_fcb;

extern b_fcb openFileTable[MAXFCBS];

int getFCB();                         // returns a file descriptor
void initOFT();                       // initializes FCBs to default (unused) values
int initFCB(DirEntry *de, int flags); // initializes FCB with data from DE
void displayFCB(int fd);
void clearFCB(int fd);

int b_open(char *filename, int flags);
int b_read(int fd, char *buffer, int count);
int b_write(int fd, char *buffer, int count);
int b_seek(int fd, off_t offset, int whence);
void b_close(int fd);

#endif
