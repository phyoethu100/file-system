/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  masterBootRecord.h
*
* Description: Header for the master boot record structure which will keep
*              a record of the details of our volume.
*              
**************************************************************************/
#ifndef MASTERBOOTRECORD_H
#define MASTERBOOTRECORD_H

#include "mfs.h"

typedef struct
{
    uint64_t magicNum;
    uint64_t numPartitionBlocks;
    uint64_t startBlock;
    int volSize;   // byte size of volume
    int blockSize; // byte size of each block
    int dirByteSize;
    int dirBlockSize;
    int freeSpaceLBAIndex;
    int freeSpaceBlockSize;
    int rootDirLBAIndex;
    int bitmapArrLen;
    char volName[256];
} MasterBootRecord;

extern MasterBootRecord *mbrBuf;

MasterBootRecord *getMBR();

#endif