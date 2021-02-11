/**************************************************************************
* Description: To start the file system and format it when
*              necessary, and close the file system. This includes initiating
*              the MBR, Root directory, free space bitmap, and setting the 
*              CWD to start at root.              
**************************************************************************/

#ifndef FSFORMAT_H
#define FSFORMAT_H

#include "fsLow.h"

int startFileSystem(char *filename, uint64_t volumeSize, uint64_t blockSize);
int closeFileSystem();

#endif
