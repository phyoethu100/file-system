/**************************************************************************
* Description: Header for the master boot record structure which will keep
*              a record of the details of our volume.             
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
