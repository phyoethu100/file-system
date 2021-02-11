/**************************************************************************
* Description: Functions to find free space within our bitmap which
*              represent the available LBA blocks to write to. Functions
*              to mark spaces as occupied are also declared here.             
**************************************************************************/

#ifndef BITMAP_H
#define BITMAP_H

#define BITS 32

extern unsigned int *bitmap;

//void initBitMap(int[], char*, int, int);
int searchBit(int index);
void setBit(int index);
void clearBit(int index);

void *initFreeSpace(int numInts);
int allocateFreeSpace(int position, int numBlocks);
int clearFreeSpace(int position, int numBlocks);
void *countFreeSpace(int position, int count);
int findFreeSpace(int count);
void createBitmap();

#endif
