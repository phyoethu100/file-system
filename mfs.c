/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  mfs.c
*
* Description: Functions for traversing through the file system, creating
*              directories, modifying files, and displaying them. This
*			   interacts directly with our driver.
*              
**************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
//#include <errno.h>    // for errno error, if use errno

#include "masterBootRecord.h"
#include "mfs.h"
#include "bitmap.h"

fdDir curDir;
// struct fs_stat fs_info;

// The getcwd() function copies an absolute path name of the current working directory
// to the array pointed to by buf, which is of length size.
// Returns a null-terminated string containing an absolute path name that is the current working directory
char *fs_getcwd(char *buf, size_t size)
{
    printf("\n----- fs_getcwd -----\n");

    // pathname to return is stored in buf
    // size is the size of the buf

    //does absolutePath have null terminator?
    //if ( strlen(absolutePath + '\0') > size )
    if ((strlen(curDir.absolutePath) + 1) > size) //include null termminator
    {
        //errno = ERANGE;
        //printf("ERANGE errno %d.   The absolute path name exceeds the size of buf.\n", errno);
        printf("ERANGE ERROR:   the absolute path name exceeds the size of buf.\n");
        printf("Increase the size of buf to at least %ld\n", strlen(curDir.absolutePath) + 1);
        return NULL;

        //errno = ERANGE;
        //return NULL;

        //an application should check for this error, and allocate a larger buffer if necessary.
        //increase the size of buf to at least strlen(absolutePath)+1
    }

    //returns a null-terminated string containing an absolute path name
    curDir.absolutePath[strlen(curDir.absolutePath)] = '\0'; // add null terminator to the path name
    strcpy(buf, curDir.absolutePath);                        // copy path name to buf including null terminator
    return buf;                                              // return the path name allocated in buf
}

// Returns NULL if the directory was not found within the given pathname
// returned fdDir should be freed by caller
fdDir *fs_opendir(const char *pathname)
{
    printf("\n----- Opening directory at pathname [%s] ----\n", pathname);

    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(pathname, absPath);

    if (res == -1)
    {
        return NULL;
    }

    fdDir *dir = (fdDir *)malloc(sizeof(fdDir));
    if (dir == NULL)
    {
        printf("fs_opendir: malloc for dir failed.\n");
        return NULL;
    }

    dir->cwd = (DirEntry *)malloc(mbrBuf->dirByteSize);
    if (dir->cwd == NULL)
    {
        printf("fs_opendir: malloc for dir.cwd failed.\n");
        free(dir);
        return NULL;
    }

    initDir(dir->cwd);
    dir->init = 1;

    DirEntry *parentDE = (DirEntry *)malloc(sizeof(DirEntry));

    if (parentDE == NULL)
    {
        printf("fs_opendir: malloc for parentDE failed\n");
        return NULL;
    }

    DirEntry *temp = getDE(absPath);
    if (temp == NULL)
    {
        printf("fs_opendir: getDE returned NULL\n");
        free(parentDE);
        return NULL;
    }

    memcpy(parentDE, temp, sizeof(DirEntry));

    // printf("PARENT DE IN OPEN\n");
    // displayDE(*parentDE);

    LBAread(dir->cwd, mbrBuf->dirBlockSize, parentDE->LBAindex);

    // printf("NEW CWD DIR IN OPEN\n");
    // displayDirectory(dir->cwd);

    strcpy(dir->name, parentDE->fileName);
    strcpy(dir->absolutePath, absPath);
    dir->dirLen = getDirLen(dir->cwd);
    initChildrenNames(dir->cwd);
    dir->dirEntryPosition = 0; // new dirs begin at 0 for iterating the cwd
    dir->LBAindex = parentDE->LBAindex;

    free(temp);
    free(parentDE);
    // printf("\tOpened directory at pathname [%s] \n", pathname);
    return dir;
}

// returns 0 if successful, -1 if not
int fs_setcwd(char *pathname)
{
    printf("\n----- Setting the CWD to [%s] -----\n", pathname);

    fdDir *newCWD = fs_opendir(pathname);

    if (newCWD == NULL)
    {
        return -1;
    }

    memcpy(&curDir, newCWD, sizeof(fdDir));
    memcpy(curDir.cwd, newCWD->cwd, mbrBuf->dirByteSize);

    free(newCWD);

    return 0;
}

// Deletes a file. Success: 0 | Fail: -1
int fs_delete(char *filename) //removes a file based on linux function - unlink or remove
{
    printf("\nDeleting the file...\n");

    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(filename, absPath);

    if (res == -1)
    {
        return 0;
    }

    DirEntry *parentDE = NULL;
    parentDE = getDE(absPath);

    if (parentDE == NULL)
    {
        return 0;
    }
    else if (!parentDE->isFile)
    {
        printf("That path leads to a directory, not a file!\n");
        return 0;
    }

    // mark bitmap space as free for the file
    clearFreeSpace(parentDE->LBAindex, parentDE->totalBlocks);

    // load the parent directory so we can clear the DE inside of it
    DirEntry *parentDir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    LBAread(parentDir, mbrBuf->dirBlockSize, parentDE->parentLBAIndex);
    clearDE(&parentDir[parentDE->dirIndex]);
    // update disk
    LBAwrite(parentDir, mbrBuf->dirBlockSize, parentDir[0].LBAindex);

    // if file removed was a DE in the curDir, refresh it
    if (parentDir[0].LBAindex == curDir.LBAindex)
    {
        // printf("\nRefreshing CWD\n");
        fs_setcwd(curDir.absolutePath);
    }

    free(parentDE);

    return 0;
}
// mkdir @ pathname = /dirA/dirB
// returns -1 if fails, 0 if successful
// creates a directory within the CWD (curDir is the parent)
int fs_mkdir(const char *pathname, mode_t mode)
{
    printf("\n----- Creating directory [%s] -----\n", pathname);
    if (pathname == NULL)
    {
        printf("Invalid pathname\n");
        return -1;
    }
    // find free space
    // printf("mkdir: before findFreeSpace\n");
    int freeIndex = findFreeSpace(mbrBuf->dirBlockSize);
    if (freeIndex == -1)
    {
        printf("not enough freespace for mkdir\n");
        return -1;
    }

    // turn the pathname into an absolute path
    char absPath[MAX_FILEPATH_SIZE];
    // printf("mkdir: before parsePath\n");
    int res = parsePath(pathname, absPath);

    if (res == -1)
    {
        return -1;
    }
    DirEntry *foundDE = getDE(absPath);
    if (foundDE != NULL)
    {
        printf("A file with that name already exists.\n");
        return -1;
    }
    // turn path into an array of pathnames
    char pathArr[MAX_DIRECTORY_DEPTH][MAX_FILENAME_SIZE];
    int arrLen = 0;
    // printf("mkdir: before pathToArray\n");
    pathToArray(absPath, pathArr, &arrLen);

    // printf("Name of directory we want to make: [%s]\n", pathArr[arrLen - 1]);
    // get the parent's path
    char parentPath[MAX_FILEPATH_SIZE];
    strcpy(parentPath, "/");
    for (int i = 0; i < arrLen - 1; i++)
    {
        strcat(parentPath, pathArr[i]);
        strcat(parentPath, "/");
    }
    // printf("Parent path: [%s]\n", parentPath);

    // init the new DE in the parent directory
    DirEntry *newParentDE = (DirEntry *)malloc(sizeof(DirEntry));
    DirEntry *temp = initDE(freeIndex, parentPath, pathArr[arrLen - 1], 0); // our dir's DE
    // printf("mkdir: THE NEW PARENT DE after initDE:\n");
    if (temp == NULL)
    {
        free(newParentDE);
        free(temp);
        free(foundDE);
        return -1;
    }

    memcpy(newParentDE, temp, sizeof(DirEntry));

    if (newParentDE->isFile)
    {
        printf("The directory can only be made inside of another directory, not a file.\n");
        free(newParentDE);
        free(foundDE);
        free(temp);
        return -1;
    }
    // displayDE(*newParentDE);

    // get chunk of memory for directory
    DirEntry *newDir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    if (newDir == NULL)
    {
        printf("newDir malloc failed inside mkdir\n");
        free(newParentDE);
        free(temp);
        free(foundDE);

        return -1;
    }

    // initiate a clean directory
    // printf("Initiating new dir in MKDIR\n");
    initDir(newDir);

    // time_t curtime;
    // curtime = time(NULL);

    // allocate . and ..
    newDir[0].used = 1;
    newDir[0].isFile = 0;
    newDir[0].LBAindex = freeIndex;       // "." points to itself
    newDir[0].parentLBAIndex = freeIndex; // where the DE lives
    newDir[0].dirIndex = 0;
    // strcpy(newDir[0].dateCreated, ctime(&curtime));
    strcpy(newDir[0].fileName, ".");
    newDir[0].totalBytes = mbrBuf->dirByteSize;   // this is actually more like the max dir bytes
    newDir[0].totalBlocks = mbrBuf->dirBlockSize; // same as above but max number of blocks

    newDir[1].used = 1;
    newDir[1].isFile = 0;
    newDir[1].LBAindex = newParentDE->parentLBAIndex; // ".." points to parent dir
    newDir[1].parentLBAIndex = freeIndex;
    newDir[1].dirIndex = 1;
    // strcpy(newDir[1].dateCreated, ctime(&curtime));
    strcpy(newDir[1].fileName, "..");
    newDir[1].totalBytes = mbrBuf->dirByteSize;   // this is actually more like the max dir bytes
    newDir[1].totalBlocks = mbrBuf->dirBlockSize; // same as above but max number of blocks

    // printf("mkdir: newDir after making . and .. in it:\n");
    // displayDirectory(newDir);
    // write new directory to disk
    LBAwrite(newDir, mbrBuf->dirBlockSize, freeIndex);

    // update bitmap in virtual memory and disk memory
    allocateFreeSpace(freeIndex, mbrBuf->dirBlockSize);
    LBAwrite(bitmap, mbrBuf->freeSpaceBlockSize, mbrBuf->freeSpaceLBAIndex);

    printf("\n----- Directory created in [%s] -----\n", parentPath);

    free(newParentDE);
    free(foundDE);
    free(temp);
    free(newDir);
    return 0;
}

// 0 on success and -1 on error
// Linux rmdir - removes empty directories
int fs_rmdir(const char *pathname)
{
    printf("\n---- Removing directory at pathname [%s] ----\n", pathname);

    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(pathname, absPath);

    if (res == -1)
    {
        printf("Errors with the path \n");
        return -1;
    }

    // Get the absolute path of the given pathname
    DirEntry *parentDE = NULL;
    parentDE = getDE(absPath);

    DirEntry *parentDir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    LBAread(parentDir, mbrBuf->dirBlockSize, parentDE->LBAindex);

    if (parentDE != NULL && parentDE->isFile == 0) // Check if the path is a directory and the path is not null
    {
        // printf("The path is a directory and it is not null \n");

        int usedDE = 0;

        for (int i = 2; i < MAXDE; i++) // i=0 and i=1 are . and ..
        {
            if (parentDir[i].used) // If the directory is used, increment the used count
            {
                usedDE++;
            }
        }

        // printf("Used DE size: %d \n", usedDE); // check how many DEs are used (should be 0 in order to remove)

        if (usedDE == 0) // The directory is empty
        {
            // printf("Found an empty directory \n\n");

            clearFreeSpace(parentDE->LBAindex, parentDE->totalBlocks); // clear free space for the given DE

            LBAread(parentDir, mbrBuf->dirBlockSize, parentDE->parentLBAIndex);
            clearDE(&parentDir[parentDE->dirIndex]);
            LBAwrite(parentDir, mbrBuf->dirBlockSize, parentDir->parentLBAIndex);

            return 0;
        }
        else // The directory is not empty
        {
            printf("The directory is not empty \n\n");
            return -1;
        }
    }

    else
    {
        printf("The path not found \n");
        return -1;
    }

    free(parentDE);
    free(parentDir);

    return 0;
}

// fills out a diriteminfo from information gathered from the DE within the passed fdDir
// caller is responsible for freeing buf ptr
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    // if we've iterated through all items in dir once already, return NULL
    // printf("dirp dirEntryPosition: [%d]\t (dirlen: [%d]\n", dirp->dirEntryPosition, dirp->dirLen);
    if (dirp->dirEntryPosition >= dirp->dirLen)
    {
        return NULL;
    }
    // step once through CWD, filling out the item structure
    unsigned char fileType = dirp->cwd[dirp->dirEntryPosition].isFile ? DT_REG : DT_DIR;
    struct fs_diriteminfo *dirItem = (struct fs_diriteminfo *)malloc(sizeof(struct fs_diriteminfo));

    strcpy(dirItem->d_name, dirp->cwd[dirp->dirEntryPosition].fileName);
    dirItem->d_reclen = dirp->dirLen;
    dirItem->fileType = fileType;

    dirp->dirEntryPosition++;

    return dirItem;
}

int fs_closedir(fdDir *dirp)
{
    free(dirp->cwd);
    free(dirp);
    return 0;
}

int fs_isDir(char *path) // Checking if the given path is a directory or not
{
    // printf("\nChecking if the path is a directory.\n");
    if (path == NULL)
    {
        // printf("The directory path not found \n");
        return 0;
    }

    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(path, absPath);

    if (res == -1)
    {
        return 0;
    }

    DirEntry *parentDE = (DirEntry *)malloc(sizeof(DirEntry));
    if (parentDE == NULL)
    {
        printf("fs_isDir: malloc for curDE returned null\n");
        return -1;
    }

    DirEntry *temp = getDE(absPath);
    if (temp == NULL)
    {
        free(parentDE);
        return -1;
    }

    memcpy(parentDE, temp, sizeof(DirEntry));

    if (parentDE->isFile)
    {
        // printf("\nThe path is not a directory \n");
        free(parentDE);
        free(temp);
        return 0;
    }

    // printf("The path is a directory.\n");
    free(parentDE);
    free(temp);
    return 1;
}

// Returns 1 if file and 0 otherwise
int fs_isFile(char *path) // Checking if the given path is a file or not
{
    // printf("\nChecking if the path is a file.\n");
    if (path == NULL)
    {
        // printf("The file path was not found \n");
        return 0;
    }

    char absPath[MAX_FILEPATH_SIZE];
    int res = parsePath(path, absPath);

    if (res == -1)
    {
        return 0;
    }

    DirEntry *parentDE = (DirEntry *)malloc(sizeof(DirEntry));
    if (parentDE == NULL)
    {
        printf("fs_isFile: malloc for curDE returned null\n");
        return -1;
    }

    DirEntry *temp = getDE(absPath);
    if (temp == NULL)
    {
        free(parentDE);
        return -1;
    }

    memcpy(parentDE, temp, sizeof(DirEntry));

    if (!parentDE->isFile)
    {
        // printf("\nThe path is not a file \n");
        free(parentDE);
        free(temp);
        return 0;
    }

    // printf("The path is a file.\n");
    free(parentDE);
    free(temp);
    return 1;
}

// 0: success | -1: fail
int fs_stat(const char *path, struct fs_stat *buf)
{
    // printf("\n ---Inside fs_stat--- \n");

    // off_t st_size;		  /* total size, in bytes */
    // blksize_t st_blksize; /* blocksize for file system I/O */
    // blkcnt_t st_blocks;	  /* number of 512B blocks allocated */
    // time_t st_accesstime; /* time of last access */
    // time_t st_modtime;	  /* time of last modification */
    // time_t st_createtime; /* time of last status change */
    if (path == NULL)
    {
        // printf("The path is not a file \n");
        return -1;
    }

    char absPath[MAX_FILEPATH_SIZE];
    int result = parsePath(path, absPath);

    if (result == -1)
    {
        return 0;
    }

    DirEntry *curDE = (DirEntry *)malloc(sizeof(DirEntry));
    if (curDE == NULL)
    {
        printf("fs_stat: malloc for curDE returned null\n");
        return -1;
    }

    DirEntry *temp = getDE(absPath);
    if (temp == NULL)
    {
        free(curDE);
        return -1;
    }

    memcpy(curDE, temp, sizeof(DirEntry));

    if (curDE->isFile == 1) // If the path is a file
    {
        buf->st_size = curDE->totalBytes; // file size
        // printf("File size: %lli \n", buf->st_size);

        buf->st_blksize = mbrBuf->blockSize; // total blocks
        // printf("Total blocks: %d \n", buf->st_blksize);

        buf->st_blocks = curDE->totalBlocks; // block size
        // printf("Block size: %lli \n", buf->st_blocks);

        // buf->st_accesstime = currentTime(); // accessed date (current date & time)

        // buf->st_createtime = curDE->dateCreated; // created date
        // printf("Created date: %d \n");
    }
    else // If the path is not a file
    {
        buf->st_size = mbrBuf->dirByteSize;
        buf->st_blksize = mbrBuf->blockSize;
        buf->st_blocks = mbrBuf->dirBlockSize;
        // buf->st_createtime = curDE->dateCreated;
        // printf("The path is not a file \n");
        // free(curDE);
        // free(temp);
        // return -1;
    }

    free(curDE);
    free(temp);
    return 0;
}

// Display current time and date (for accessed time)
// int currentTime()
// {

//     char cur_time[128];
//     time_t t;
//     struct tm *ptm;

//     t = time(NULL);
//     ptm = localtime(&t);

//     strftime(cur_time, 128, "%d-%b-%Y %H:%M:%S", ptm);

//     // printf("Accessed date and time: %s\n", cur_time);

//     return 0;
// }

/* helpers */
void initDir(DirEntry *dir)
{
    // printf("\nInitiating directory\n");
    for (int i = 0; i < MAXDE; i++)
    {
        clearDE(&(dir[i]));
        // dir[i].used = 0;
    }
}

void pathToArray(const char *pathname, char arr[][MAX_FILENAME_SIZE], int *pathArrLen)
{
    // printf("\nConverting path [%s] to an array of file names\n", pathname);
    char path[MAX_FILEPATH_SIZE]; // strtok_r needs a mutable object
    strcpy(path, pathname);
    // printf("Path[] in pathToArray: %s\n", path);
    char *saveptr;
    char *token;
    char *delim = "/";
    int i = 0;
    for (token = strtok_r(path, delim, &saveptr);
         token != NULL;
         token = strtok_r(NULL, delim, &saveptr))
    {
        strcpy(arr[i], token);
        i++;
    }

    *pathArrLen = i;
}

void displayCWD()
{
    printf("\n\tCurrent working directory:\n"
           "curDir.init: %d\n"
           "curDir.name: %s\n"
           "curDir.absolutePath: %s\n"
           "curDir.dirLen: %u\n"
           "curDir.dirEntryPosition: %d\n"
           "curDir.LBAindex: %lu\n",
           curDir.init, curDir.name, curDir.absolutePath, curDir.dirLen,
           curDir.dirEntryPosition, curDir.LBAindex);
}

// returns 0 if successful, -1 if not. It parses path and constructs an absolute one
int parsePath(const char *pathname, char *newAbsolutePath)
{
    // printf("\nParsing path [%s]\n", pathname);
    if (strlen(pathname) + strlen(curDir.absolutePath) > MAX_FILEPATH_SIZE)
    {
        printf("Filepath length too long.\n");
        return -1;
    }

    char path[MAX_FILEPATH_SIZE]; // strtok_r needs a mutable object
    strcpy(path, pathname);
    // printf("Path[]: %s\n", path);

    strcpy(newAbsolutePath, "/");

    // init tokenizer variables
    char *saveptr;
    char *delim = "/";
    char *token;
    token = strtok_r(path, delim, &saveptr);
    // printf("Token: %s\n", token);

    if (token == NULL)
    {
        // This happens when only "/" is passed
        // printf("Absolute path set to root\n");
        return 0;
    }

    int absolute = pathname[0] == '/';
    int current = !strcmp(token, ".");
    int parent = !strcmp(token, "..");

    // break down path into an array of directory names
    int n = MAX_FILEPATH_SIZE / MAX_FILENAME_SIZE;
    char pathArr[n][MAX_FILENAME_SIZE];
    int pathArrLen = 0;
    pathToArray(pathname, pathArr, &pathArrLen);

    // if is absolute (/)
    if (absolute)
    {
        // printf("Path is absolute.\n");
        // load up root as starting directory to search from
        strcpy(newAbsolutePath, pathname);
        // printf("The updated absolute path for the curDir: %s\n", curDir.absolutePath);
    }
    else if (current && curDir.init)
    {
        if (curDir.absolutePath[strlen(curDir.absolutePath) - 1] != '/')
        {
            // printf("adding / to the end of [%s]\n", curDir.absolutePath);
            strcat(curDir.absolutePath, "/");
        }
        // if relative to cwd (.)
        // printf("Path is relative to cwd\n");
        // first copy the current absolute path, then concatinate it
        strcpy(newAbsolutePath, curDir.absolutePath);
        // start at 1 to not include "."
        for (int pathIndex = 1; pathIndex < pathArrLen; pathIndex++)
        {
            strcat(newAbsolutePath, pathArr[pathIndex]);
            strcat(newAbsolutePath, "/");
        }
    }
    else if (parent && curDir.init)
    {
        // printf("Path is relative to parent\n");
        // if relative to parent (..)
        // set curDirectory to the LBAindex pointed to by ".." (cwd[1])

        // char *newAbsolutePath = "/";
        for (int pathIndex = 2; pathIndex < curDir.dirLen - 1; pathIndex++)
        {
            strcat(newAbsolutePath, curDir.childrenNames[pathIndex]);
            strcat(newAbsolutePath, "/");
        }
        // start at 1 to not include ".."
        for (int pathIndex = 1; pathIndex < pathArrLen; pathIndex++)
        {
            strcat(newAbsolutePath, pathArr[pathIndex]);
            strcat(newAbsolutePath, "/");
        }
    }
    else
    {
        if (curDir.absolutePath[strlen(curDir.absolutePath) - 1] != '/')
        {
            // printf("adding / to the end of [%s]\n", curDir.absolutePath);
            strcat(curDir.absolutePath, "/");
        }
        // printf("Path is relative to cwd\n");
        // when path looks like: dirA/ (no . or .. or /)
        // first copy the current absolute path, then concatinate it
        strcpy(newAbsolutePath, curDir.absolutePath);
        // it's the same thing as starting from current working dir
        for (int pathIndex = 0; pathIndex < pathArrLen; pathIndex++)
        {
            strcat(newAbsolutePath, pathArr[pathIndex]);
            strcat(newAbsolutePath, "/");
        }
    }

    // printf("Path parsed: [%s]\n", newAbsolutePath);
    return 0;
}

int getDirLen(DirEntry *dir)
{
    int length = 0;
    // printf("\nGetting the number of used DEs in directory (the length)\n");

    for (int i = 0; i < MAXDE; i++)
    {
        // printf("File name: %s\n", dir[i].fileName);
        if (dir[i].used)
        {
            // displayDE(dir[i]);
            length++;
        }
    }
    // printf("Directory's length is %u.\n", length);
    return length;
}

// initiates the fdDir's childrenNames by copying over all the children names
// from the given directory.
void initChildrenNames(DirEntry *dir)
{
    // printf("\nInitiating directory's children names.\n");
    printf("Directory files:\n");
    for (int i = 0; i < MAXDE; i++)
    {
        if (dir[i].used)
        {
            strcpy(curDir.childrenNames[i], dir[i].fileName);
            printf("\t%s\n", curDir.childrenNames[i]);
        }
        // printing the first 5 copied children only for test purposes
        // end of test
    }
}

// move a file
// return 0 upon success, -1 for failure
int fs_moveFile(char *srcFile, char *destFile)
{
    // printf("\nLeaving---------------> @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@    fs_moveFile\n");
    printf("\n----- fs_moveFile -----\n");

    // printf("srcFile:    %s\n", srcFile);
    // printf("destFile:   %s\n", destFile);

    // get absolute path of source and destination file
    char srcAbsPath[MAX_FILEPATH_SIZE];
    int res = parsePath(srcFile, srcAbsPath);
    // failure finding absolute path
    if (res == -1)
    {
        return (-1);
    }

    char destAbsPath[MAX_FILEPATH_SIZE];
    res = parsePath(destFile, destAbsPath);
    // failure finding absolute path
    if (res == -1)
    {
        return (-1);
    }

    DirEntry *srcDE = (DirEntry *)malloc(sizeof(DirEntry));
    DirEntry *srcParentDir = (DirEntry *)malloc(mbrBuf->dirByteSize);
    DirEntry *destDE = (DirEntry *)malloc(sizeof(DirEntry));
    DirEntry *destDir = (DirEntry *)malloc(mbrBuf->dirByteSize);

    if (!srcDE || !srcParentDir || !destDE || !destDir)
    {
        printf("fs_moveFile: one or more mallocs failed\n");
        free(srcDE);
        free(srcParentDir);
        free(destDE);
        free(destDir);
        return -1;
    }

    initDir(srcParentDir);
    initDir(destDir);

    // get src and dest DEs
    DirEntry *temp = getDE(srcAbsPath);
    if (!temp)
    {
        return -1;
    }
    memcpy(srcDE, temp, sizeof(DirEntry));

    // temp = NULL;
    DirEntry *temp2 = getDE(destAbsPath);
    if (!temp2)
    {
        return -1;
    }
    memcpy(destDE, temp2, sizeof(DirEntry));

    // load src and dest directories
    LBAread(srcParentDir, mbrBuf->dirBlockSize, srcDE->parentLBAIndex);
    LBAread(destDir, mbrBuf->dirBlockSize, destDE->LBAindex);

    int i = getFreeDirIndex(destDir); // find a place to copy the srcDE over to in the destDir
    if (i == -1)
    {
        return -1;
    }
    // save some values we'll need for after clearing srcDE
    int srcDEdirIndex = srcDE->dirIndex;
    int srcDEparentLBAindex = srcDE->parentLBAIndex;
    // update srcDE
    srcDE->dirIndex = i;
    srcDE->parentLBAIndex = destDE->LBAindex;

    memcpy(&destDir[i], srcDE, sizeof(DirEntry)); // copy the srcDE over to the destDir

    LBAwrite(destDir, mbrBuf->dirBlockSize, destDE->LBAindex); // update destDir

    clearDE(&srcParentDir[srcDEdirIndex]); // clear ("delete") the srcDE from srcParentDir

    //  issue with destDE->dirIndex
    LBAwrite(srcParentDir, mbrBuf->dirBlockSize, srcDEparentLBAindex); // update srcParentDir after clearDE

    free(temp);
    free(temp2);
    free(srcDE);
    free(srcParentDir);
    free(destDE);
    free(destDir);

    return 0;
}
