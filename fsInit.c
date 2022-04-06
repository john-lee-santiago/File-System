/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fsLow.h"
#include "mfs.h"

#pragma pack(1)
struct VCB {
	uint8_t jmpBoot[3];
	char OEMName[8];
	uint16_t BytesPerSector;
	uint8_t SectorPerCluster;
	uint16_t RsvdSectorCount;
	uint8_t NumOfFATs;
	uint16_t RootEntryCount;
	uint16_t TotalSectors16;
	uint8_t Media;
	uint16_t FATSz16;
	uint16_t SectorsPerTrack;
	uint16_t NumberOfHeads;
	uint32_t HiddenSectors;
	uint32_t TotalSectors32;
	uint32_t FATSz32;
	uint16_t ExtFlags;
	uint16_t FSVer;
	uint32_t RootCluster;
	uint16_t FSInfo;
	uint16_t BkBootSector;
	uint8_t Reserved0[12];
	uint8_t DriverNumber;
	uint8_t Reserved1;
	uint8_t BootSig;
	uint32_t VolumeID;
	char VolumeLabel[11] ;
	char FileSystemType[8];
	uint8_t Reserved2[420];
	uint16_t Signature;
} *VCBptr ;

int initVCB(uint64_t numberOfBlocks, uint64_t blockSize)
	{
	VCBptr->jmpBoot[0] = 235;
	VCBptr->jmpBoot[1] = 0;
	VCBptr->jmpBoot[2] = 144;
	strcpy(VCBptr->OEMName, "MSDOS5.0");
	VCBptr->BytesPerSector = blockSize;
	VCBptr->SectorPerCluster = 1;
	VCBptr->RsvdSectorCount = 2;
	VCBptr->NumOfFATs = 2;
	VCBptr->RootEntryCount = 0;
	VCBptr->TotalSectors16 = 0;
	VCBptr->Media = 248;
	VCBptr->FATSz16 = 0;
	VCBptr->SectorsPerTrack = 0;
	VCBptr->NumberOfHeads = 0;
	VCBptr->HiddenSectors = 0;
	VCBptr->TotalSectors32 = numberOfBlocks;
	VCBptr->FATSz32 = 79;
	VCBptr->ExtFlags = 0;
	VCBptr->FSVer = 0;
	VCBptr->RootCluster = 2;
	VCBptr->FSInfo = 1;
	VCBptr->BkBootSector = 0;
	for (int i = 0 ; i < 12; i++) {
		VCBptr->Reserved0[i] = 0;
	}
	VCBptr->DriverNumber = 128;
	VCBptr->Reserved1 = 0;
	VCBptr->BootSig = 41;
	VCBptr->VolumeID = 1337;
	strcpy(VCBptr->VolumeLabel, "MJ'sFS");
	strcpy(VCBptr->FileSystemType, "FAT32");
	for(int i = 0 ; i < 420 ; i++) {
		VCBptr->Reserved2[420] = 0;
	}
	VCBptr->Signature = 43605;
	LBAwrite(VCBptr, 1, 0);
	return 0;
	}

struct fsFat 
	{
  uint32_t fat[10000];
  // track index of nxt available free space
  uint32_t startingBlock;
	} *FATptr1, *FATptr2;

struct FSInfo
	{
  uint32_t FSI_LeadSig;
  uint32_t FSI_Reserved1[120];
  uint32_t FSI_StrucSig;
  uint32_t FSI_Free_Count;
  uint32_t FSI_Nxt_Free;
  uint32_t FSI_Reserved2[3];
  uint32_t FSI_TrailSig;
	} *FSI_ptr;

int initFAT(uint64_t numberOfBlocks, uint64_t blockSize)
{
    // allocate and initialize FSInfo struct
    FSI_ptr = malloc(blockSize); 

    FSI_ptr->FSI_LeadSig = 0x41615252;
    for (int i = 0; i < 120; i++)
    {
        FSI_ptr->FSI_Reserved1[i] = 0;
    }
    FSI_ptr->FSI_StrucSig = 0x61417272;
    FSI_ptr->FSI_Free_Count = numberOfBlocks - VCBptr->RsvdSectorCount; // -2 bc vcb and FSInfo take up 2 blocks
    FSI_ptr->FSI_Nxt_Free = VCBptr->RootCluster;
    for (int i = 0; i < 3; i++)
    {
        FSI_ptr->FSI_Reserved2[i] = 0;
    }
    FSI_ptr->FSI_TrailSig = 0xAA550000;

		LBAwrite(FSI_ptr, 1, 1);

    // FAT32 filesystem has 2 copies of the FAT
    FATptr1 = malloc(sizeof(struct fsFat));
		// calculate number of blocks the fat will use
    int FATSize = sizeof(struct fsFat) / blockSize;
		if (sizeof(struct fsFat) % blockSize > 0)
    {
        FATSize++;
    }
    FATptr1->fat[0] = 1; // reserved for VCB
    FATptr1->fat[1] = 1; // reserved for FSInfo block
    for (int i = 2; i < 10000; i++)
    {
        FATptr1->fat[i] = 0;
    }
		FATptr1->startingBlock = FSI_ptr->FSI_Nxt_Free;

    FATptr2 = malloc(sizeof(struct fsFat));
    FATptr2->fat[0] = 1; // reserved for VCB
    FATptr2->fat[1] = 1; // reserved for FSInfo block
    for (int i = 2; i < 10000; i++)
    {
        FATptr2->fat[i] = 0;
    }
		FATptr2->startingBlock = FSI_ptr->FSI_Nxt_Free + FATSize;
		

    for(int i = 0; i < FATSize; i++)
    {
        if(i == FATSize - 1)
        {
            FATptr1->fat[i + FATptr1->startingBlock] = 0x0FFFFFFF;
						FATptr2->fat[i + FATptr1->startingBlock] = 0x0FFFFFFF;
        }
        else
        {
            FATptr1->fat[i + FATptr1->startingBlock] = i + FATptr1->startingBlock + 1;
						FATptr2->fat[i + FATptr1->startingBlock] = i + FATptr1->startingBlock + 1;
        }
    }
    for(int i = 0; i < FATSize; i++)
    {
        if(i == FATSize - 1)
        {
            FATptr1->fat[i + FATptr2->startingBlock] = 0x0FFFFFFF;
						FATptr2->fat[i + FATptr2->startingBlock] = 0x0FFFFFFF;
        }
        else
        {
            FATptr1->fat[i + FATptr2->startingBlock] = i + FATptr2->startingBlock + 1;
						FATptr2->fat[i + FATptr2->startingBlock] = i + FATptr2->startingBlock + 1;
        }
    }
    FSI_ptr->FSI_Nxt_Free += (2 * FATSize);
    FSI_ptr->FSI_Free_Count -= (2 * FATSize);

		LBAwrite(FATptr1, FATSize, FATptr1->startingBlock);
		LBAwrite(FATptr2, FATSize, FATptr2->startingBlock);

		LBAwrite(FSI_ptr, 1, 1);

		return 0;
}

struct DirectoryEntry {
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FstClusHI;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FstClusLO;
	uint32_t DIR_FileSize;
};

struct Directory {
	struct DirectoryEntry Directory[16];
} *ROOTptr;

int initRootDirectory( uint64_t blockSize) {
	int rootStartingBlock = FSI_ptr->FSI_Nxt_Free;
	ROOTptr = malloc(sizeof(struct Directory));
	for(int i = 0; i < 2 ; i++) {
		if(i == 0) {
			strcpy(ROOTptr->Directory[i].DIR_Name, ".");
		}
		else {
			strcpy(ROOTptr->Directory[i].DIR_Name, "..");
		}
		ROOTptr->Directory[i].DIR_Attr = 16;
		ROOTptr->Directory[i].DIR_NTRes = 0;
		ROOTptr->Directory[i].DIR_CrtTimeTenth = 0;
		ROOTptr->Directory[i].DIR_CrtTime = 0;
		ROOTptr->Directory[i].DIR_CrtDate = 0;
		ROOTptr->Directory[i].DIR_LstAccDate = 0;
		ROOTptr->Directory[i].DIR_FstClusHI = rootStartingBlock >> 8;
		ROOTptr->Directory[i].DIR_FstClusLO = rootStartingBlock % 256;
		ROOTptr->Directory[i].DIR_FileSize = blockSize;
		time_t currTime;
  	time(&currTime);
  	char* token;
  	char* timeTok = ctime(&currTime);
  	int timeArray[6];
  	token = strtok(timeTok, " :");
  	timeTok = NULL;
  	for(int i = 0; i < 6; i++) {
    	token = strtok(timeTok, " :");
    	if(i == 0) {
      	if(strncmp(token, "Jan", 3) == 0){
        	timeArray[i] = 1;
      	}
      	else if (strncmp(token, "Feb", 3) == 0){
        	timeArray[i] = 2;
      	}
	      else if (strncmp(token, "Mar", 3) == 0){
  	      timeArray[i] = 3;
    	  }
      	else if (strncmp(token, "Apr", 3) == 0){
        	timeArray[i] = 4;
	      }
  	    else if (strncmp(token, "May", 3) == 0){
    	    timeArray[i] = 5;
      	}
      	else if (strncmp(token, "Jun", 3) == 0){
        	timeArray[i] = 6;
    	  }
    	  else if (strncmp(token, "Jul", 3) == 0){
    	    timeArray[i] = 7;
    	  }
    	  else if (strncmp(token, "Aug", 3) == 0){
    	    timeArray[i] = 8;
    	  }
    	  else if (strncmp(token, "Sep", 3) == 0){
    	    timeArray[i] = 9;
    	  }
    	  else if (strncmp(token, "Oct", 3) == 0){
    	    timeArray[i] = 10;
    	  }
    	  else if (strncmp(token, "Nov", 3) == 0){
    	    timeArray[i] = 11;
    	  }
    	  else {
    	    timeArray[i] = 12;
    	  }
    	}
    	else {
      	timeArray[i] = atoi(token);
    	}
  	}
		ROOTptr->Directory[i].DIR_WrtDate = timeArray[1] +
																				(timeArray[0] << 5) +
																				((timeArray[5]-1980) << 9);
		ROOTptr->Directory[i].DIR_WrtTime = (timeArray[4]/2) +
																				(timeArray[0] << 5) +
																				((timeArray[5]-1980) << 11);
	}

	LBAwrite(ROOTptr, 1, rootStartingBlock);
	FATptr1->fat[rootStartingBlock] = 0x0FFFFFFF;
	FATptr2->fat[rootStartingBlock] = 0x0FFFFFFF;
	FSI_ptr->FSI_Nxt_Free += 1;
  FSI_ptr->FSI_Free_Count -= 1;
	LBAwrite(FATptr1, VCBptr->FATSz32, FATptr1->startingBlock);
	LBAwrite(FATptr2, VCBptr->FATSz32, FATptr2->startingBlock);

	LBAwrite(FSI_ptr, 1, 1);


	return 0;
}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	VCBptr = malloc ( blockSize );
	LBAread( VCBptr, 1, 0 );
	if ( VCBptr->Signature != 43605 ) {
		initVCB(numberOfBlocks, blockSize);
		initFAT(numberOfBlocks, blockSize);
		initRootDirectory(blockSize);
	}
	printf("this is a test.\n");

	return 0;
	}
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}