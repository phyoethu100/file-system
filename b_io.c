/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  b_io.c
*
* Description: Functions for reading and writing blocks of data. Open file
*              table and file control blocks are also defined here.
*              
**************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "b_io.h"
#include "bitmap.h"
#include "masterBootRecord.h"
#include "fsLow.h"
#include "mfs.h"

int startup = 0;
b_fcb openFileTable[MAXFCBS];

// Initializes all b_fcb's to be marked as free
void initOFT()
{
    printf("Initiating OFT\n");
    //init openFileTableay to all free
    for (int i = 0; i < MAXFCBS; i++)
    {
        openFileTable[i].used = 0; // indicates a free fcb
    }
    startup = 1;
}

// initializes FCB with data from DE and returns a file desc.
int initFCB(DirEntry *de, int flags)
{
    printf("Initiating FCB\n");
    int fd = getFCB();

    if (fd == -1)
    {
        return fd;
    }

    printf("Initializing FCB with fd at OFT[%d]\n", fd);
    openFileTable[fd].used = 1;
    openFileTable[fd].buf = malloc(BUFSIZE);
    openFileTable[fd].bufPos = 0;
    openFileTable[fd].buflen = BUFSIZE;
    openFileTable[fd].filePos = 0;
    openFileTable[fd].bufBytesRemain = BUFSIZE;
    openFileTable[fd].totalBlocks = 1;
    openFileTable[fd].remainingBytes = 0;
    openFileTable[fd].transferBytes = 0;
    //  openFileTable[fd].length = 0;

    // we could add .blockPos = 0;
    openFileTable[fd].flags = flags;
    openFileTable[fd].isFile = de->isFile;
    openFileTable[fd].parentLBAIndex = de->parentLBAIndex;
    openFileTable[fd].fileSize = de->totalBytes;
    openFileTable[fd].fileBlockSize = de->totalBlocks;
    openFileTable[fd].fileLBAIndex = de->LBAindex;
    strcpy(openFileTable[fd].fileName, de->fileName);

    return fd;
}

// returns the index of a free FCB. This index serves as our file descriptor (fd)
// returns -1 if all FCB are being used
int getFCB()
{
    printf("Getting an FCB fd\n");
    for (int i = 0; i < MAXFCBS; i++)
    {
        if (!openFileTable[i].used)
        {
            return i;
        }
    }
    printf("All File Control Blocks are currently being used.\n");
    printf("Close a file to open a new one.\n");
    return -1;
}

void displayFCB(int fd)
{
    printf("\nFile control block at OFT[%d]:\n", fd);
    printf("used: %d\n"
           "buf: %s\n"
           "bufPos: %d\n"
           "buflen: %d\n"
           "fileSize: %d\n"
           "fileLBAIndex: %d\n"
           "filePos: %d\n"
           "flags: %d\n"
           "isFile: %d\n"
           "parentLBAIndex: %d\n"
           "fileName: %s\n",
           openFileTable[fd].used, openFileTable[fd].buf, openFileTable[fd].bufPos,
           openFileTable[fd].buflen, openFileTable[fd].fileSize,
           openFileTable[fd].fileLBAIndex, openFileTable[fd].filePos,
           openFileTable[fd].flags, openFileTable[fd].isFile,
           openFileTable[fd].parentLBAIndex, openFileTable[fd].fileName);
}

void clearFCB(int fd)
{
    printf("\nClearing FCB at oft[%d]\n", fd);
    openFileTable[fd].used = 0;
    openFileTable[fd].isFile = 0;
    openFileTable[fd].bufPos = 0;
    openFileTable[fd].parentLBAIndex = 0;
    openFileTable[fd].buflen = BUFSIZE;
    openFileTable[fd].filePos = 0;
    openFileTable[fd].flags = 0;
    openFileTable[fd].fileSize = 0;
    openFileTable[fd].fileLBAIndex = 0;
    openFileTable[fd].remainingBytes = 0;
    openFileTable[fd].transferBytes = 0;
    //  openFileTable[fd].length = 0;
    openFileTable[fd].totalBlocks = 0;

    strcpy(openFileTable[fd].fileName, "");
    free(openFileTable[fd].buf);
}
//The argument flags must include one of the following access modes:
//    O_RDONLY, O_WRONLY. These request opening the file read-
//    only, write-only, or read/write, respectively.
//
//   O_CREAT:
//   If pathname does not exist, create it as a regular file.
//   O_TRUNC:
//   If the file already exists and is a regular file and the ac‐
//   cess mode allows writing (i.e., is O_RDWR or O_WRONLY) it will
//   be truncated to length 0.  If the file is a FIFO or terminal
//   device file, the O_TRUNC flag is ignored.  Otherwise, the ef‐
//   fect of O_TRUNC is unspecified.
// returns an fd of -1 if we weren't able to open the file (not found, no O_CREAT)
// Constraints: in order for a file to exist, a directory must exist first
int b_open(char *filename, int flags)
{
    printf("\nOpening file %s\n", filename);
    // turn filename into an abs path
    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(filename, absPath);
    if (res == -1)
    {
        return -1;
    }

    DirEntry *de = (DirEntry *)malloc(sizeof(DirEntry));
    if (!de)
    {
        printf("b_open: malloc failed for de.\n");
        return -1;
    }
    // if file exists, init FCB
    DirEntry *temp = getDE(filename);
    if (!temp)
    {
        // This means the file did not exist, check for CREAT flag
        if (O_CREAT == (flags & O_CREAT))
        {
            // if create flag, then init a DE at parent path

            // turn path into an array of pathnames
            char pathArr[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
            int arrLen = 0;
            // printf("mkdir: before pathToArray\n");
            pathToArray(absPath, pathArr, &arrLen);
            char parentPath[MAX_FILEPATH_SIZE];
            strcpy(parentPath, "/");
            for (int i = 0; i < arrLen - 1; i++)
            {
                strcat(parentPath, pathArr[i]);
                strcat(parentPath, "/");
            }
            // init a DE at the parent (pointing to 0 [the mbr] since we don't know where this fill will be at yet)
            temp = initDE(0, parentPath, pathArr[arrLen - 1], 1);
            if (!temp)
            {
                printf("Something went wrong in b_open\n");
                free(de);
                return -1;
            }
            // still need to init an FCB and stuff so we can do that all at once... later? this is why no return here yet
        }
        else
        {
            // if no create flag and file does not exist, then return -1
            printf("Couldn't open file because it does not exist and no O_CREAT flag was given.\n");
            free(de);
            return -1;
        }
    }
    // this means the file exists (if id didn't it now does, otherwise we wouldn't reach this line)
    // we can continue now as if the file existed for sure, because at this point it should!
    memcpy(de, temp, sizeof(DirEntry));
    if (!de->isFile)
    {
        // if DE points to a directory, return -1, this function is for opening files only
        free(temp);
        free(de);
        return -1;
    }
    // initiate the OFT and the FCB using the info from the found DE
    if (!startup)
    {
        initOFT();
    }
    int fd = -1; // declare an fd (not starting at 0)
    fd = initFCB(de, flags);

    if (O_TRUNC == (flags & O_TRUNC)) // if O_TRUNC passed in flags
    {
        openFileTable[fd].fileSize = 0;
        openFileTable[fd].filePos = 0;
    }
    // allocate buffer
    openFileTable[fd].buf = malloc(BUFSIZE);

    free(temp);
    free(de);
    return fd;
}

// We are passing b_io.c functions to fsshell.c
int b_write(int fd, char *buffer, int count)
{
    printf("\n----- Inside b_write ----- \n");

    int transferBytes = 0; // Number of bytes to transfer to LBA block
                           //  int remainingBytes = 0;

    //  int total_blocks = (count / BUFSIZE) + 1; // number of blocks to write
    //  printf("Total blocks to write: %d \n", total_blocks);

    // bitmap = malloc(BUFSIZE * sizeof(unsigned int)); // Allocate bitmap

    if (startup == 0)
    { // Initialize file system
        initOFT();
    }

    if ((fd < 0) || (fd >= MAXFCBS))
    { // Invalid file descriptor
        return -1;
    }

    if (openFileTable[fd].used == -1)
    { // Cannot open the file
        return -1;
    }

    // Find free space in FCB array
    if (openFileTable[fd].fileLBAIndex == 0)
    // if (O_CREAT == (openFileTable[fd].flags & O_CREAT))
    {
        openFileTable[fd].fileLBAIndex = findFreeSpace(BLKSIZE);
        //   allocateFreeSpace(openFileTable[fd].fileLBAIndex, BLKSIZE); // Allocate free block for b_write
    }

    if (openFileTable[fd].totalBlocks == BLKSIZE)
    {
        allocateFreeSpace(openFileTable[fd].fileLBAIndex, BLKSIZE); // Allocate extra free block for b_write
    }

    if (openFileTable[fd].fileLBAIndex == -1)
    {
        printf("No available free space \n");
        return 0;
    }

    else
    {
        printf("Free space for b_write: %d \n", openFileTable[fd].fileLBAIndex);

        //  printf("Before Buffer Position: %d \n", openFileTable[fd].bufPos);
        //  printf("Before Count: %d \n", count);
        //  printf("Before BUFSIZE: %d \n", BUFSIZE);

        // while (total_blocks != 0)
        // {

        // If the count is 100, 300, 600, and 1200
        if ((openFileTable[fd].bufPos + count) < BUFSIZE)
        {                                                                            // If the current position with the count is less than 512 bytes
                                                                                     //   printf("Less than 512 bytes \n");
                                                                                     //      printf("Remaining bytes: %d \n", openFileTable[fd].remainingBytes);
                                                                                     //    printf("Transfer bytes: %d \n", openFileTable[fd].transferBytes);
                                                                                     //    printf("Buf Position before: %d \n", openFileTable[fd].bufPos);
            memcpy(openFileTable[fd].buf + openFileTable[fd].bufPos, buffer, count); // Copy the requested count to our buffer
                                                                                     //   printf("After first memcpy \n");
            openFileTable[fd].bufPos += count;                                       // Keep track of current buffer's position
                                                                                     //  printf("Buf Position: %d \n", openFileTable[fd].bufPos);

            // If total block = 1
            if ((count < openFileTable[fd].bufPos) && (openFileTable[fd].bufPos < BUFSIZE))
            {
                printf("Greater than 200, but less than 512 \n");

                //   openFileTable[fd].fileLBAIndex %= BUFSIZE; // Get the LBA index
                //  LBAwrite(buffer, 1, openFileTable[fd].fileLBAIndex);                       // LBA write 1 block to the file
                // LBAread(buffer, 1, openFileTable[fd].fileLBAIndex);

                openFileTable[fd].fileSize += count; // Keep incrementing the file size after every LBA write by the coun
                                                     //   openFileTable[fd].fileLBAIndex++; // Increment the free block to next LBA index
            }

            else if (count <= openFileTable[fd].bufPos)
            {
                printf("Less than 200 \n");
                //    openFileTable[fd].fileLBAIndex %= BUFSIZE; // Get the LBA index
                //    LBAwrite(buffer, 1, openFileTable[fd].fileLBAIndex);                       // LBA write 1 block to the file
                // LBAread(buffer, 1, openFileTable[fd].fileLBAIndex);

                openFileTable[fd].fileSize += count; // Keep incrementing the file size after every LBA write by the count
                //    openFileTable[fd].fileLBAIndex++; // Increment the free block to next LBA index
            }

            // printf("Buf Position: %d \n", openFileTable[fd].bufPos);
            openFileTable[fd].transferBytes = count;
        }

        else
        { // If the position (with the count) is greater than 512 bytes
            printf("Greater than 512 bytes \n");
            printf("Buffer Pos: %d \n", openFileTable[fd].bufPos);
            openFileTable[fd].transferBytes = BUFSIZE - openFileTable[fd].bufPos;

            printf("Remaining bytes to transfer: %d \n", openFileTable[fd].transferBytes);                     // Number of bytes left to transfer before reaching 512 bytes
            memcpy(openFileTable[fd].buf + openFileTable[fd].bufPos, buffer, openFileTable[fd].transferBytes); // Copy the remaining bytes to the buffer from previous position

            // uint64_t LBAwrite (void * buffer, uint64_t lbaCount, uint64_t lbaPosition);
            openFileTable[fd].fileLBAIndex %= BUFSIZE;           // Get the LBA index
            LBAwrite(buffer, 1, openFileTable[fd].fileLBAIndex); // LBA write 1 block to the file
                                                                 // LBAread(buffer, 1, openFileTable[fd].fileLBAIndex);

            openFileTable[fd].fileSize += count; // Keep incrementing the file size after every LBA write by the coun
            openFileTable[fd].fileLBAIndex++;    // Increment the free block to next LBA index
                                                 // total_blocks--;                   // Decrement the total_blocks until it reaches 0
            openFileTable[fd].totalBlocks++;
            openFileTable[fd].bufPos = 0;
            //  openFileTable[fd].bufPos = BUFSIZE; // Reset buffer position after each LBAwrite

            openFileTable[fd].remainingBytes = count - openFileTable[fd].transferBytes;
            printf("New remaining bytes: %d \n", openFileTable[fd].remainingBytes);                             // Remaining bytes to copy after transferring to LBA block
            memcpy(openFileTable[fd].buf + openFileTable[fd].bufPos, buffer, openFileTable[fd].remainingBytes); // Copy remaining bytes from previous position to our buffer
                                                                                                                //  openFileTable[fd].bufPos = openFileTable[fd].remainingBytes;                             // Set new position
                                                                                                                // openFileTable[fd].length += openFileTable[fd].buflen;
        }
        //  }
    }

    // printf("\nBuffer in b_write: %s \n", openFileTable[fd].buf);
    printf("\nAfter write, file size: %d\n", openFileTable[fd].fileSize);
    printf("After write, fileLBAIndex: %d\n", openFileTable[fd].fileLBAIndex);
    // printf("After write, total blocks: %d \n", total_blocks);
    printf("----- End of b_write -----\n");

    // printf("\nFINAL BUFFER: %s\n", openFileTable[fd].buf);
    // Return the number bytes tranfer to the buffer
    return openFileTable[fd].transferBytes;
}

// return 0 if successful, -1 if not
int b_seek(int fd, off_t offset, int whence)
{
    // 1 = SEEK_SET
    if (whence == 1)
    {
        printf("\n-----Inside SEEK_SET----- \n");
        openFileTable[fd].filePos = offset;
    }

    // 2 = SEEK_CUR
    else if (whence == 2)
    {
        printf("\n-----Inside SEEK_CUR----- \n");
        openFileTable[fd].filePos += offset;
    }

    // 3 = SEEK_END
    else if (whence == 3)
    {
        printf("\n-----Inside SEEK_END----- \n");
        openFileTable[fd].filePos = openFileTable[fd].fileSize + offset;
    }

    else
    {
        printf("SEEK NOT FOUND \n");
        return -1;
    }

    return openFileTable[fd].filePos;
}

void b_close(int fd)
{
    LBAwrite(openFileTable[fd].buf, 1, openFileTable[fd].bufPos);
    clearFCB(fd);
}

// using LBAread() to read number of bytes to b_fcb buffer
// copy the requested bytes to the caller's buffer from b_fcb buffer or directly from the file depending on the size
// return from the function the number of bytes requested or remaining
int b_read(int fd, char *buffer, int count)
{
    printf("\nIn b_read\n");
    int returnBytes;         // hold the number of bytes to return from b_read()
    int userBufferBytes = 0; // hold the number of bytes in the user's buffer

    if (startup == 0)
    {
        initOFT(); // initialize file system
    }

    if ((fd < 0) || (fd >= MAXFCBS))
    {
        return -1; // invalid file descriptor
    }

    if (openFileTable[fd].used == -1)
    {
        return -1; // cannot open the file
    }

    //Remove, for temporary explanation (RTE)

    //RTE- fileSize 4800 - filePOs 4500 = 300<= 512. reached EOF, last LBA block of our file
    // if count is 400, and left 300, enter if(), return the remaining 300
    // if count is 200, and left 300, enter else(), return count bytes.
    // Next time, count is 200, and left is 100, enter if()

    // to check if it has reached the end of file. When bytes are less or equal to blockSize
    if ((openFileTable[fd].fileSize - openFileTable[fd].filePos) <= mbrBuf->blockSize)
    {
        int remainingBytes = openFileTable[fd].fileSize - openFileTable[fd].filePos; // bytes left to read

        // when requested bytes are greater or equal to what is left to read in the file. Return what is left.
        if (count >= remainingBytes)
        {
            if (openFileTable[fd].bufPos == 0)
            {
                LBAread(openFileTable[fd].buf, 1, openFileTable[fd].fileLBAIndex); // read from disk to our buffer
            }

            memcpy(buffer, openFileTable[fd].buf + openFileTable[fd].bufPos, remainingBytes); // copy to user's buffer the remaining bytes
            //RTE- we are reading from disk to our buffer first, because there might be bytes that
            //do not belong to this file. It can be some leftover from previous file since our b_write() overwrite
            //thus, we are giving to user buffer an exact amount of bytes that it's left for read

            //might not be necessary, since already reached the end. for consistency
            openFileTable[fd].bufPos += remainingBytes; // update our buffer position

            openFileTable[fd].filePos += remainingBytes; // update the position in the file. Reached the end

            //userBufferBytes = remainingBytes;
            //returnBytes = userBufferBytes;
            //return returnBytes;

            return remainingBytes; // return the number of bytes left

        } // end of if (count >= remainingBytes)

        //RTE- count 200 < remainingBytes is 300
        // when bytes left to read are enough for requested bytes
        else // (count < remainingBytes)
        {
            if (openFileTable[fd].bufPos == 0)
            {
                LBAread(openFileTable[fd].buf, 1, openFileTable[fd].fileLBAIndex); // read from disk to our buffer
            }

            memcpy(buffer, openFileTable[fd].buf + openFileTable[fd].bufPos, count); // copy to user's buffer the requested bytes
            openFileTable[fd].bufPos += count;                                       // update the position of our buffer
            openFileTable[fd].filePos += count;                                      // update the position by the size of count

            //userBufferBytes += count;
            //returnBytes = userBufferBytes;
            //return returnBytes;

            return count; // number of bytes to return

        } // end of else

    } // end of if ( (openFileTable[fd].fileSize - openFileTable[fd].filePos) <= mbrBuf->blockSize )

    // if count not equal 0, it means there are still bytes to read to complete the user's request
    while (count != 0)
    {

        // when count is less than blockSize or we have enough bytes in our buffer
        if ((openFileTable[fd].bufPos + count) < mbrBuf->blockSize)
        {
            // if bufPos is 0, means we haven't read anything to our buffer yet. Proceed to read
            if (openFileTable[fd].bufPos == 0)
            {
                LBAread(openFileTable[fd].buf, 1, openFileTable[fd].fileLBAIndex); // read from disk to our buffer
                openFileTable[fd].fileLBAIndex++;                                  // move to the next block
                openFileTable[fd].filePos += mbrBuf->blockSize;                    // update to where we have read in the file
            }

            memcpy(buffer + userBufferBytes, openFileTable[fd].buf + openFileTable[fd].bufPos, count); // copy from our buffer to user's buffer
            openFileTable[fd].bufPos += count;                                                         // update the position of our buffer
            userBufferBytes += count;                                                                  // update user's buffer with the requested bytes
            count -= count;                                                                            // since request was fulfilled, count will be 0
            returnBytes = userBufferBytes;                                                             // number of bytes to return

        } // end of if ( (openFileTable[fd].bufPos + count) < mbrBuf->blockSize )

        // when count is greater or equal to blockSize, or we don't have enough bytes in our buffer to fulfill the request
        else // ( (openFileTable[fd].bufPos + count) >= mbrBuf->blockSize )
        {
            // if bufPos no equal 0, it means there are some bytes left to read in our buffer
            if (openFileTable[fd].bufPos != 0)
            {
                memcpy(buffer, openFileTable[fd].buf + openFileTable[fd].bufPos, mbrBuf->blockSize - openFileTable[fd].bufPos); // copy to user the remaining bytes in our buffer
                userBufferBytes += (mbrBuf->blockSize - openFileTable[fd].bufPos);                                              // update the user's buffer bytes
                count -= (mbrBuf->blockSize - openFileTable[fd].bufPos);                                                        // update count with bytes that have already copy to user's buffer
                openFileTable[fd].bufPos += (mbrBuf->blockSize - openFileTable[fd].bufPos);                                     // update our buffer position
                returnBytes = userBufferBytes;
            }

            // reset bufPos to 0, if it has reached the end of the buffer
            if (openFileTable[fd].bufPos == mbrBuf->blockSize)
            {
                openFileTable[fd].bufPos = 0; // set the position of our buffer to the beginning
            }

            // when count is greater than or equal to blockSize, read directly to user buffer
            if (count >= mbrBuf->blockSize)
            {
                LBAread(buffer + userBufferBytes, 1, openFileTable[fd].fileLBAIndex); // read directly from disk to user's buffer
                openFileTable[fd].fileLBAIndex++;                                     // move to the next block
                openFileTable[fd].filePos += mbrBuf->blockSize;                       // update the file position
                userBufferBytes += mbrBuf->blockSize;                                 // number of bytes in the user's buffer
                count -= mbrBuf->blockSize;                                           // count is reduced by the number of bytes already read to user's buffer
                returnBytes = userBufferBytes;
            }

        } // end of ( (openFileTable[fd].bufPos + count) >= mbrBuf->blockSize )

    } // end of while (count != 0)

    return returnBytes; // number of bytes to return from this function

} // end of b_read() method