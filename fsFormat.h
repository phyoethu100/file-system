/**************************************************************************
* Class:  CSC-415-01 Fall 2020
* Team Name: A++ 
* Student Name-ID: Roberto Herman (918009734), Cindy Fung Chan (920832364)
*                  Phyoe Thu (918656575), Aryanna Brown (920188955)            
* GitHub UserID: mecosteas, cny257, phyoethu100, aryannayazmin   
* Project: Group term assignment - Basic File System 
* 
* File:  fsFormat.h
*
* Description: To start the file system and format it when
*              necessary, and close the file system. This includes initiating
*              the MBR, Root directory, free space bitmap, and setting the 
*              CWD to start at root.
*              
**************************************************************************/

#ifndef FSFORMAT_H
#define FSFORMAT_H

#include "fsLow.h"

int startFileSystem(char *filename, uint64_t volumeSize, uint64_t blockSize);
int closeFileSystem();

#endif