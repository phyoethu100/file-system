/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  bitmap.h
*
* Description: Functions to find free space within our bitmap which
*              represent the available LBA blocks to write to. Functions
*              to mark spaces as occupied are also declared here.
*              
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
