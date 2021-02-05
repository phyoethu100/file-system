/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  bitmap.c
*
* Description: Functions to find free space within our bitmap which
*              represent the available LBA blocks to write to. Functions
*              to mark spaces as occupied are also defined here.
*              
**************************************************************************/

// 0: free
// 1: allocated

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "bitmap.h"
#include "masterBootRecord.h"
// void createBitmap(int numInts)
// {
//     printf("Creating bitmap\n");
//     // unsigned int *bitmap = malloc(numInts * sizeof(unsigned int));
//     bitmap = malloc(numInts * sizeof(unsigned int));
//     if (bitmap == NULL)
//     {
//         printf("Bitmap malloc failed");
//     }
// }
// Find and return the bit in the array
int searchBit(int index)
{
    //printf("Inside searchBit \n");
    // printf("searchBit: before returning. index [%d]\n", index);
    return (bitmap[index / BITS] & (1 << (index % BITS))); // return the index bit, 1 or 0
}

// OR operator is used to set a bit. This will set the remainder or extra free space from bitmap array.
void setBit(int index)
{
    // printf("Inside setBit \n");
    bitmap[index / BITS] |= 1 << (index % BITS);
}

// AND operator is used to clear a bit. This will clear remainder or extra free space from bitmap array.
void clearBit(int index)
{
    // printf("Inside clearBit \n");
    bitmap[index / BITS] &= ~(1 << (index % BITS));
}

// -----Free space functions-----

// Initialize the bitmap array to all zeroes
void *initFreeSpace(int bitmapArrLen)
{
    memset(bitmap, 0, (bitmapArrLen * sizeof(unsigned int)));
}

// Allocate/create free space for the given entries (master boot record, root directory, etc.)
int allocateFreeSpace(int position, int numBlocks)
{
    int index = 0;

    if (numBlocks == 0) // If there are no blocks to allocate
    {
        // printf("\n%d blocks allocated in bitmap from position %d. \n", numBlocks, position);
        return 1;
    }

    for (int i = position; i < mbrBuf->bitmapArrLen; i++)
    {
        setBit(i);
        index++;

        if (index == numBlocks)
        {
            // printf("\n%d blocks allocated in bitmap from position %d. \n", numBlocks, position);
            return 1; // return 1 for successful
        }
    }
    return 0; // return 0 for unsuccessful (not enough space)
}

// Clear free space for the given entries
int clearFreeSpace(int position, int numBlocks)
{
    int index = 0;

    if (numBlocks == 0) // If there are no blocks to clear
    {
        // printf("\n%d blocks cleared in bitmap from position %d. \n", numBlocks, position);
        return 1;
    }

    for (int i = position; i < mbrBuf->bitmapArrLen; i++)
    {
        clearBit(i);
        index++;

        if (index == numBlocks)
        {
            // printf("\n%d blocks cleared in bitmap from position %d. \n", numBlocks, position);
            return 1;
        }
    }

    return 0; // Return 0 for unsuccessful
}

// Count how many allocated blocks and free blocks are taken
void *countFreeSpace(int position, int count)
{

    int allocated_block = 0;
    int free_block = 0;

    for (int i = position; i < count; i++)
    {

        if (searchBit(i))
        {                      // if found in bitmap array, it means allocated blocks
            allocated_block++; // increment the allocated block count
        }

        else
        {                 // if not found in bitmap array, then it means free blocks
            free_block++; // increment the free block count
        }
    }

    // printf("Allocated blocks: %d and free blocks: %d \n", allocated_block, free_block);
}

// Find next available free space with given block counts
int findFreeSpace(int count)
{
    int position;
    int allocated;
    int index = 0;
    // printf("findFreeSpace: before for-loop\n");
    for (int i = 0; i < mbrBuf->bitmapArrLen; i++)
    {
        position = i;
        // printf("Inside i loop\n");

        for (int j = i; j < mbrBuf->bitmapArrLen; j++)
        {
            // printf("Inside j loop\n");

            if (searchBit(i))

            { // if the block already exists in bitmap array, then it's not free space
                // printf("Bit already exists \n");
                break; // break out of j loop and goes back to i loop
            }
            else
            { // else, increment the index
                // printf("Bit not found yet \n");
                index++; // stays in j loop and keeps incrementing the index for requested count
            }

            if (index == count)
            { // if the requested count matches the number of indexes
                // printf("Bit reaches requested count - from j loop \n");
                allocated = count; // allocated blocks = requested blocks
                break;             // breaks out of j loop
            }
        }

        if (index == count)
        {
            // printf("Bit reaches requested count - from i loop \n");
            break; // break out of i loop
        }
    }

    if (allocated == count)
    { // if allocated blocks equal requested blocks
        // printf("\nAvailable position for free space: %d \n", position);
        return position; // return first available position from LBA
    }

    else if (allocated < count)
    {
        printf("Not enough space. Free up some space... \n");
        return -1;
    }

    else // if (allocated > count)
    {
        printf("Allocated blocks shouldn't be more than requested blocks \n");
        return -1;
    }
    // never return
}

// Sample main program to test bitmap implementation
// int main(int argc, char *argv[])
// {

//     int bitmap[50];
//     int i;

//     initFreeSpace(bitmap, 50); // initialize the bitmap array to all zeroes
//     // for (i = 0; i < 10; i++) {
//     //     bitmap[i] = 0; // Clear bitmap array
//     // }

//     printf("\nSet bit positions 0, 1, 2, 10, 20, and 30 \n");
//     setBit(bitmap, 0);
//     setBit(bitmap, 1);
//     setBit(bitmap, 2);
//     setBit(bitmap, 10);
//     setBit(bitmap, 20);
//     setBit(bitmap, 30);

//     for (i = 0; i < 32; i++)
//     {
//         if (searchBit(bitmap, i))
//             printf("Bit %d was set! \n", i);
//     }

//     if (!searchBit(bitmap, 15))
//     {
//         printf("\nBit 15 not found! \n");
//     }

//     if (searchBit(bitmap, 30))
//     {
//         printf("Bit 30 found in bitmap! \n");
//     }

//     printf("\nClear bit positions 20 \n");
//     clearBit(bitmap, 20);

//     for (i = 0; i < 32; i++)
//     {
//         if (searchBit(bitmap, i))
//         {
//             printf("Bit %d was set! \n", i);
//         }
//     }

//     printf("\nAllocate free space for requested block counts \n");
//     allocateFreeSpace(bitmap, 3, 32, 5); // allocate 5 blocks starting from 3rd position in LBA

//     printf("\nFind next available free space \n");
//     findFreeSpace(bitmap, 20, 5); // 0, 1, 2 are already set in bitmap and 3-7 are allocated in previous line, so next available free space should be 8

//     printf("\nCount allocated and free blocks \n");
//     countFreeSpace(bitmap, 0, 100);
// }
