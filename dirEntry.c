/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  dirEntry.c
*
* Description: Directory entries are created with these functions. There
*              are numerous functions to help with initiating, clearing,
*              finding, and displaying directory entries as well as directories.
*              
**************************************************************************/

#include <stdio.h>
#include <sys/types.h>

#include "dirEntry.h"
#include "bitmap.h"
#include "masterBootRecord.h"
#include "fsLow.h"

// INITIALIZES and WRITES a DE in the directory specified by given path
// if given path leads to a file, NULL is returned
// Constraint: the path must lead to a DIRECTORY where the DE will be init
// caller is responsible for freeing returned buf ptr
DirEntry *initDE(int LBAindex, const char *path, const char *filename, int isFile)
{
    printf("\n>>> Creating a DE in [%s]\n", path);

    // convert path to full path to avoid issues
    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(path, absPath);

    if (res == -1)
    {
        return NULL;
    }

    // get the DE pointing to the directory where we will initiate the new DE

    DirEntry *temp = getDE(absPath);
    if (temp == NULL)
    {
        printf("Directory path not found\n");
        return NULL;
    }
    DirEntry *parentDE = (DirEntry *)malloc(sizeof(DirEntry));
    // check if path atually leads to a directory
    if (parentDE == NULL)
    {
        printf("initDE: malloc error for parentDE\n");
        return NULL;
    }
    memcpy(parentDE, temp, sizeof(DirEntry));
    // DirEntry *parentDE = getDE(path);

    if (parentDE->isFile)
    {
        printf("\nThe path is not a directory \n");
        free(temp);
        free(parentDE);
        return NULL;
    }
    // printf("THE PARENT DE IN INIT AFTER GET\n");
    // displayDE(*parentDE);

    // read directory from disk
    DirEntry *parentDir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    // memcpy(dir, getDir(path), mbrBuf->dirByteSize);
    LBAread(parentDir, mbrBuf->dirBlockSize, parentDE->LBAindex);

    // find a free index to init our DE in
    int i = getFreeDirIndex(parentDir);
    if (i == -1)
    {
        free(parentDE);
        free(parentDir);
        free(temp);
        return NULL;
    }

    // DirEntry *dirDE = getDE(path); // parent DE (has been checked that is a dir DE)
    // printf("The DE gotten from the path in initDE used to init new DE's parentLBAIndex:\n");
    // displayDE(*dirDE);

    // init the new DE
    // time_t curtime;
    // curtime = time(NULL);

    parentDir[i].used = 1;
    parentDir[i].isFile = isFile;
    parentDir[i].LBAindex = LBAindex;
    parentDir[i].parentLBAIndex = parentDE->LBAindex;
    parentDir[i].dirIndex = i;
    // strcpy(parentDir[i].dateCreated, ctime(&curtime));
    strcpy(parentDir[i].fileName, filename);

    if (isFile)
    {
        // create a file DE (w/ default values of 0 for size)
        // printf("The DE will be for a file\n");
        parentDir[i].totalBytes = 0;
        parentDir[i].totalBlocks = 0;
    }
    else
    {
        // printf("The DE will be for a directory\n");
        // create a directory DE
        parentDir[i].totalBytes = mbrBuf->dirByteSize;
        parentDir[i].totalBlocks = mbrBuf->dirBlockSize;
    }

    // update changes in disk
    // printf("Rewriting updated PARENT directory at LBA index [%d]. Looks like:\n", parentDir[i].parentLBAIndex);
    // displayDE(*parentDE);

    // displayDirectory(parentDir);
    LBAwrite(parentDir, mbrBuf->dirBlockSize, parentDir[i].parentLBAIndex);

    // if changes were made in CWD, refresh it
    if (parentDE->LBAindex == curDir.LBAindex)
    {
        // printf("\nRefreshing CWD\n");
        fs_setcwd(curDir.absolutePath);
    }

    // printf("DE initiated successfully!\n");
    DirEntry *newDE = (DirEntry *)malloc(sizeof(DirEntry));
    memcpy(newDE, &parentDir[i], sizeof(DirEntry));

    free(temp);
    free(parentDE);
    free(parentDir);
    return newDE;
}

// looks through directory for an unused DE and returns its Dir index
// returns -1 if no position found (dir is full)
int getFreeDirIndex(DirEntry *dir)
{
    // printf("\nFinding an empty entry position in current directory\n");
    for (int i = 0; i < MAXDE; i++)
    {
        if (!dir[i].used)
        {
            // printf("Found a free DE in dir[%d]\n", i);
            return i;
        }
    }
    printf("Empty position not found. The current directory is full.\n");
    return -1;
}

// returns a pointer to the target DE or NULL if not found in dir
// findDE searches the given directory for a DE with given fileName
// not to be confused with getDE
DirEntry *findDE(DirEntry *dir, char *fileName)
{
    // printf("\nFinding a DE with the name '%s' inside the given directory.\n", fileName);
    for (int i = 0; i < MAXDE; i++) // make sure to test this
    {
        if (strcmp(dir[i].fileName, fileName) == 0)
        {
            // printf("\nDE found. Here are its details:\n");
            // displayDE(dir[i]);
            return &dir[i];
        }
    }
    // printf("\nFile not found in current directory.\n");
    return NULL;
}

// given a pathname, it will return a pointer to the DE, or NULL if not found
// caller is responsible for freeing the buf ptr
DirEntry *getDE(const char *pathname)
{
    // printf("\nGetting DE at pathname [%s]\n", pathname);
    char path[MAX_FILEPATH_SIZE]; // strtok_r needs a mutable object
    strcpy(path, pathname);
    // printf("Path[]: %s\n", path);

    // init tokenizer variables
    char *saveptr;
    char *delim = "/";
    char *token;
    token = strtok_r(path, delim, &saveptr);
    // printf("Token: %s\n", token);

    // If token is NULL ("/" was given) return a "root DE"
    if (token == NULL)
    {
        // time_t curtime;
        // curtime = time(NULL);

        // printf("Returning a 'root DE'\n");
        DirEntry *rootDE = (DirEntry *)malloc(sizeof(DirEntry));
        rootDE->used = 1;
        rootDE->isFile = 0;
        strcpy(rootDE->fileName, "");
        rootDE->LBAindex = mbrBuf->rootDirLBAIndex;
        rootDE->parentLBAIndex = mbrBuf->rootDirLBAIndex;
        rootDE->totalBytes = mbrBuf->dirByteSize;
        rootDE->totalBlocks = mbrBuf->dirBlockSize;
        rootDE->dirIndex = 0;
        // strcpy(rootDE->dateCreated, ctime(&curtime));
        return rootDE;
    }

    int absolute = pathname[0] == '/';
    int current = !strcmp(token, ".");
    int parent = !strcmp(token, "..");

    // break down path into an array of directory names
    int n = MAX_FILEPATH_SIZE / MAX_FILENAME_SIZE;
    char pathArr[n][MAX_FILENAME_SIZE];
    int pathArrLen = 0;
    pathToArray(pathname, pathArr, &pathArrLen);

    DirEntry *curDE = (DirEntry *)malloc(sizeof(DirEntry));
    if (curDE == NULL)
    {
        printf("getDE: malloc for curDE returned null\n");
    }
    DirEntry *curDirectory = (DirEntry *)malloc(mbrBuf->dirByteSize);
    if (curDirectory == NULL)
    {
        printf("getDE: malloc for curDirectory returned null\n");
    }

    // we want it to always be absolute from now on!!!!!! get rid of the rest
    // if is absolute (/)
    if (absolute)
    {
        // printf("Path is absolute.\n");
        LBAread(curDirectory, mbrBuf->dirBlockSize, mbrBuf->rootDirLBAIndex);
    }
    else if (current && (curDir.init))
    {
        // if relative to cwd (.)
        // printf("Path is relative to cwd\n");
        memcpy(curDirectory, curDir.cwd, mbrBuf->dirByteSize);
    }
    else if (parent && (curDir.init))
    {
        // printf("Path is relative to parent\n");
        // if relative to parent (..)
        // set curDirectory to the LBAindex pointed to by ".." (cwd[1])
        LBAread(curDirectory, mbrBuf->dirBlockSize, curDir.cwd[1].LBAindex);
    }
    else if (curDir.init)
    {
        // printf("Path is relative to cwd\n");
        // when path looks like: dirA/ (no . or .. or /)
        // it's the same thing as starting from current working dir
        memcpy(curDirectory, curDir.cwd, mbrBuf->dirByteSize);
    }
    else
    {
        printf("Could not open directory. If the path given is relative to CWD,\n"
               "then the CWD might not have been initialized previously.\n");

        free(curDE);
        free(curDirectory);
        return NULL;
    }

    // iterate until end of the array of directory names
    int i = 0;
    DirEntry *temp = NULL;
    // printf("The path: [%s], length: [%d]\n", pathArr[i], pathArrLen);
    while (i < pathArrLen - 1)
    {
        temp = findDE(curDirectory, pathArr[i]);

        if (temp == NULL) // if DE does not exist in curDirectory
        {
            printf("That directory does not exist.\n");
            printf("The last directory in given path that exists is [%s]\n", curDirectory[0].fileName);

            free(curDirectory);
            free(curDE);
            return NULL;
        }

        memcpy(curDE, temp, sizeof(DirEntry));

        if (curDE->isFile)
        {
            printf("Given path is not a directory!\n");

            free(curDirectory);
            free(curDE);
            return NULL;
        }
        // if found, load that dir up
        // printf("DE for directory [%s] found, loading it up at LBA index %d\n", curDE->fileName, curDE->LBAindex);
        LBAread(curDirectory, mbrBuf->dirBlockSize, curDE->LBAindex);
        // repeat searching for next directory in path
        i++;
    }
    // get the DE from last path, which could be a file or dir DE
    temp = findDE(curDirectory, pathArr[i]);

    if (temp == NULL)
    {
        // printf("DE [%s] not found!\n", pathArr[i]);
        // printf("Either the file does not exist, or the given path is wrong.\n");

        free(curDirectory);
        free(curDE);
        return NULL;
    }

    memcpy(curDE, temp, sizeof(DirEntry));

    // printf("The DE name: [%s]\n", curDE->fileName);
    // displayDE(*curDE);

    free(curDirectory);
    return curDE;
}
// returns -1 if fail, or a freeSpaceIndex if successful
int createRootDir()
{
    printf("\nCreating root directory\n");
    printf("\nSize of a DE: %li\t"
           "Total bytes in %d DEs: %lu\t"
           "Directory byte size: %d.\t"
           "Block size: %d\n",
           sizeof(DirEntry), MAXDE, sizeof(DirEntry) * MAXDE,
           mbrBuf->dirByteSize, mbrBuf->dirBlockSize);

    DirEntry *rootDir = (DirEntry *)malloc(mbrBuf->dirByteSize);

    // init directory
    // printf("Initiating rootDir in createRootDir\n");
    initDir(rootDir);

    int freeIndex = findFreeSpace(mbrBuf->dirBlockSize);
    // check if enough space was found
    if (freeIndex == -1)
    {
        return -1;
    }

    // time_t curtime;
    // curtime = time(NULL);
    // allocate . and ..
    rootDir[0].used = 1;
    rootDir[0].isFile = 0;
    rootDir[0].LBAindex = freeIndex;
    rootDir[0].parentLBAIndex = freeIndex; // for root it is the same as root
    rootDir[0].dirIndex = 0;
    // strcpy(rootDir[0].dateCreated, ctime(&curtime));
    strcpy(rootDir[0].fileName, ".");
    rootDir[0].totalBytes = mbrBuf->dirByteSize;   // this is actually more like the max dir bytes
    rootDir[0].totalBlocks = mbrBuf->dirBlockSize; // same as above but max number of blocks

    // in root, .. points to itself as well
    rootDir[1].used = 1;
    rootDir[1].isFile = 0;
    rootDir[1].LBAindex = freeIndex;
    rootDir[1].parentLBAIndex = freeIndex; // for root it is the same as root
    rootDir[1].dirIndex = 1;
    // strcpy(rootDir[1].dateCreated, ctime(&curtime));
    strcpy(rootDir[1].fileName, "..");
    rootDir[1].totalBytes = mbrBuf->dirByteSize;   // this is actually more like the max dir bytes
    rootDir[1].totalBlocks = mbrBuf->dirBlockSize; // same as above but max number of blocks

    LBAwrite(rootDir, mbrBuf->dirBlockSize, freeIndex);
    // printf("Wrote Root Directory to disk at index %d\n", freeIndex);

    allocateFreeSpace(freeIndex, mbrBuf->dirBlockSize);
    // printf("Allocated free space for Root Directory\n");

    free(rootDir);
    return freeIndex;
}

void displayDE(DirEntry de)
{
    printf("\n\tDisplaying DE:\n");
    printf("Used: %d\n"
           "isFile: %d\n"
           "LBAindex: %d\n"
           "totalBytes: %li\n"
           "totalBlocks: %li\n"
           "fileName: %s\n"

           "parentLBAIndex: %d\n"
           "dirIndex: %d\n",
           de.used, de.isFile, de.LBAindex, de.totalBytes, de.totalBlocks,
           de.fileName, de.parentLBAIndex, de.dirIndex);
}

void displayDirectory(DirEntry *dir)
{
    int n = 5;
    printf("\n----- Displaying first %d DEs in directory -----\n", n);
    for (int i = 0; i < n; ++i)
    {
        displayDE(dir[i]);
    }
}

void clearDE(DirEntry *de)
{
    // printf("\nClearing DE [%s]\n", de->fileName);

    // DirEntry *dir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    // LBAread(dir, mbrBuf->dirBlockSize, de->parentLBAIndex);

    de->used = 0;
    de->isFile = 0;
    de->dirIndex = 0;
    de->LBAindex = 0;
    de->totalBytes = 0;
    de->totalBlocks = 0;
    // strcpy(de->dateCreated, "");
    de->parentLBAIndex = 0;
    strcpy(de->fileName, "");

    // LBAwrite(dir, mbrBuf->dirBlockSize, de->parentLBAIndex);

    // free(dir);
}