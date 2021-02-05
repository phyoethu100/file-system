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
#ifndef DIRENTRY_H
#define DIRENTRY_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
// #include <time.h>

#define MAXDE 50
#define MAX_FILENAME_SIZE 50
#define DATE_STR_SIZE 26 // DO NOT MAKE < 26

typedef struct DirEntry
{
    int used;   // 0 for not used 1 for used
    int isFile; // 0 for not a file 1 for file
    int LBAindex;
    int parentLBAIndex;
    int dirIndex;
    // char dateCreated[DATE_STR_SIZE]; // change data type to date or something
    off_t totalBytes; // size of file basically
    blkcnt_t totalBlocks;
    char fileName[MAX_FILENAME_SIZE];
} DirEntry;

/* initialize a DE inside of the given pathname. Initalizes using given params */
DirEntry *initDE(int LBAindex, const char *path, const char *filename, int isFile);
DirEntry *findDE(DirEntry *dir, char *fileName); /* get a DE given a directory */
DirEntry *getDE(const char *pathname);           /* get a DE given a pathname */
DirEntry *getDir(const char *path);

void clearDE(DirEntry *de); // resets DE values. returns 1 if successful 0 if not
int createRootDir();
int getFreeDirIndex(DirEntry *dir); /* get index of free location within directory */

void displayDE(DirEntry de);
void displayDirectory(DirEntry *dir);

#endif