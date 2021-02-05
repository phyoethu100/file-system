/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  fsFormat.c
*
* Description: To start the file system and format it when
*              necessary, and close the file system. This includes initiating
*              the MBR, Root directory, free space bitmap, and setting the 
*              CWD to start at root.
*              
**************************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fsFormat.h"
#include "bitmap.h"
#include "masterBootRecord.h"
#include "dirEntry.h"

unsigned int *bitmap;
MasterBootRecord *mbrBuf;

// Initiates the formatting as well as the CWD
// returns -1 if not enough space was found for volume -2 if malloc fails -3 if not enough free space for root
int startFileSystem(char *filename, uint64_t volumeSize, uint64_t blockSize)
{
    printf("\n----- Starting file system -----\n");
    //check if it is already formatted. If not proceed to Format it
    //by allocating MBR, freeSpace, and root directory
    startPartitionSystem(filename, &volumeSize, &blockSize);

    printf("Opened %s\t Volume Size: %llu\t  BlockSize: %llu;\n", filename, (ull_t)volumeSize, (ull_t)blockSize);
    printf("Size of MBR structure: %lu\n", sizeof(MasterBootRecord));

    mbrBuf = (MasterBootRecord *)malloc(blockSize);
    if (mbrBuf == NULL)
    {
        printf("MBRbuf malloc failed\n");
        return -2;
    }
    LBAread(mbrBuf, 1, 0);

    // Create and initialize bitmap
    int numIntsToRepresentNumPartitionBlocks = (((volumeSize / blockSize) / 32) + 1); // bitmap array length
    int numFreeSpaceBytes = numIntsToRepresentNumPartitionBlocks * sizeof(unsigned int);
    int numFreeSpaceBytesRoundedUpToNearestBlock = ((numFreeSpaceBytes / blockSize) * blockSize) + blockSize;
    bitmap = malloc(numFreeSpaceBytesRoundedUpToNearestBlock);
    if (bitmap == NULL)
    {
        printf("\nBitmap malloc failed\n");
        return -2;
    }
    initFreeSpace(numIntsToRepresentNumPartitionBlocks); // mark all spaces as 0 (unused)

    //check if filename is same as volume name. If so, this means it's already formatted
    if (!strcmp(mbrBuf->volName, filename))
    {
        LBAread(bitmap, mbrBuf->freeSpaceBlockSize, mbrBuf->freeSpaceLBAIndex);
        printf("File System previously formatted. Opening [%s]\n", mbrBuf->volName);
        printf("\n\tMBR details:\n"
               "numPartitionBlocks: %lu\n"
               "startBlock: %lu\t"
               "\tvolSize: %d\n"
               "blockSize: %d\t"
               "\tfreeSpaceLBAIndex: %d\n"
               "rootDirLBAIndex: %d\t"
               "volName: %s\n"
               "freeSpaceBlockSize: %d\t"
               "dirByteSize: %d\n"
               "dirBlockSize: %d\t"
               "bitmapArrLen: %d\n",
               mbrBuf->numPartitionBlocks, mbrBuf->startBlock,
               mbrBuf->volSize, mbrBuf->blockSize, mbrBuf->freeSpaceLBAIndex,
               mbrBuf->rootDirLBAIndex, mbrBuf->volName, mbrBuf->freeSpaceBlockSize,
               mbrBuf->dirByteSize, mbrBuf->dirBlockSize, mbrBuf->bitmapArrLen);

        fs_setcwd("/");

        printf("----- File system ready -----\n");
        return 0;
    }
    else
    {
        printf("\n----- Formatting [%s] -----\n", filename);
        //not formatted yet
        // init values of mbr w/ given info about partition size and name
        int dirBlockSize = ((sizeof(DirEntry) * MAXDE) / blockSize) + 1;
        int LBAsize = ((volumeSize / blockSize)) + 1; // 10,000,000 / 512 = 19,532
        // printf("LBA size: %d\n", LBAsize);
        blkcnt_t freeSpaceSize = ((volumeSize / blockSize) / 4096) + 1; // (19,531 / 4096) + 1 = 5
        // printf("Free space size: %lli\n", freeSpaceSize);

        mbrBuf->magicNum = 0x0a52424d202b2b41; // A++ MBR
        mbrBuf->volSize = volumeSize;
        mbrBuf->blockSize = blockSize;
        mbrBuf->dirByteSize = dirBlockSize * blockSize;
        mbrBuf->dirBlockSize = dirBlockSize;
        mbrBuf->startBlock = 0;
        mbrBuf->numPartitionBlocks = (volumeSize / blockSize) + 1;
        mbrBuf->freeSpaceBlockSize = freeSpaceSize;
        mbrBuf->bitmapArrLen = numIntsToRepresentNumPartitionBlocks;
        strcpy(mbrBuf->volName, filename);
        mbrBuf->freeSpaceLBAIndex = 1; // update free space LBA location in mbr
        setBit(0);                     // mark mbr LBAindex as used

        // printf("Num of ints to represent num LBA blocks: %d\n", numIntsToRepresentNumPartitionBlocks);
        allocateFreeSpace(1, freeSpaceSize); // alocate bitmap space as occupied

        // Create root directory
        int rootIndex = createRootDir();
        if (rootIndex < 0)
        {
            printf("\nCould not find a free space for the Root Directory\n");
            return -3;
        }

        mbrBuf->rootDirLBAIndex = rootIndex;

        LBAwrite(mbrBuf, 1, 0); // write MBR to LBA[0] count 1 block
        LBAwrite(bitmap, freeSpaceSize, 1);
        printf("\n----- Formatting complete -----\n");
    }
    printf("\n\tMBR details:\n"
           "numPartitionBlocks: %lu\n"
           "startBlock: %lu\t"
           "\tvolSize: %d\n"
           "blockSize: %d\t"
           "\tfreeSpaceLBAIndex: %d\n"
           "rootDirLBAIndex: %d\t"
           "volName: %s\n"
           "freeSpaceBlockSize: %d\t"
           "dirByteSize: %d\n"
           "dirBlockSize: %d\t"
           "bitmapArrLen: %d\n",
           mbrBuf->numPartitionBlocks, mbrBuf->startBlock,
           mbrBuf->volSize, mbrBuf->blockSize, mbrBuf->freeSpaceLBAIndex,
           mbrBuf->rootDirLBAIndex, mbrBuf->volName, mbrBuf->freeSpaceBlockSize,
           mbrBuf->dirByteSize, mbrBuf->dirBlockSize, mbrBuf->bitmapArrLen);
    printf("----- File system ready -----\n");

    fs_setcwd("/");

    return 0;
}

int closeFileSystem()
{
    //check if need to free anything
    free(curDir.cwd);
    free(bitmap);
    free(mbrBuf);
    closePartitionSystem();
    return 0;
}