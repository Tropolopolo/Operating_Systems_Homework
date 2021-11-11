//Matthew Irvine

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#define IMG_NAME_MAX 100
#define MAX_LINE_SIZE 1024
#define MAX_CMDS 64

struct __attribute__((__packed__)) DirectoryEntry
{
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t DIR_NTRes;
	uint8_t DIR_CrtTimeTenth;
	uint16_t DIR_CrtTime;
	uint16_t DIR_CrtDate;
	uint16_t DIR_LstAccDate;
	uint16_t DIR_FirstClusterHigh;
	uint16_t DIR_WrtTime;
	uint16_t DIR_WrtDate;
	uint16_t DIR_FirstClusterLow;
	uint32_t DIR_FileSize;
};

typedef struct
{
	//name of the OEM
	char BS_OEMName[8];
	//bytes per sector
	int16_t BPB_BytsPerSec;
	//number of sectors per allocation unit
	int8_t BPB_SecPerClus;
	//Number of reserved sectors in the Reserved region
	int16_t BPB_RsvdSecCnt;
	//The number of FATS on the img
	int8_t BPB_NumFATS;
	//This is always 0
	int16_t BPB_RootEntCnt;
	//... so far unused
	char BS_VolLab[11];
	//This field is the new 32-bit total count of sectors on the volume
	int32_t BPB_TotSec32;
	//This field is the 32-bit count of sectors occupied by ONE FAT.
	int32_t BPB_FATSz32;
	//This field is set to the cluster number of the first cluster of the root directory
	int32_t BPB_RootClus;

	//My calculated variables.
	int32_t RootDirSectors;
	int32_t FirstDataSector;
	int32_t FirstSectorofCluster;
	int32_t DataSec;
	int32_t CountofClusters;
	int32_t FATsize;
	int32_t ClusStart;
	int32_t * FATpos;
	FILE* fatFile;
}FAT32;

typedef struct String
{
	char entry[MAX_LINE_SIZE];
}Str;

typedef struct StringList
{
	char* entry[MAX_CMDS];
	char* head;
	int lineSize;
	int maxStrings;
	void (*set)(char* [], int, int);
	int (*add)(char*, char* [], int, int);
	int (*isEmpty)(char* [], int);
	void (*deleteEntry)(char*);
	void (*clearList)(char* [], int);
	void (*releaseList)(char* [], int);
	int (*insert)(char*, char* [], int, int, int);

}StrList;

/*
INPUT: A string to be deleted
OUTPUT: None
DESCRIPTION: 
	Take a string, frees the string, then resets the string
*/
void deleteEntry(char* entry)
{
	strncpy(entry, "\0", strlen("\0"));
}

/*
INPUT: String entry, List of Strings to add to, Max size of a String, Size of the list
OUTPUT:	0	- found a spot for the String to append to
			-1	- Did not find a spot for the String
DESCRIPTION:
	This inserts a new string to the end of the StringList.
	If it fails then it will return a -1
*/
int append(char* entry, char* list[], int max, int size)
{
	//get the next position that is null
	int counter = 0;
	while (counter < size && strncmp(list[counter], "\0", max) != 0)
	{
		counter++;
	}
	if (counter >= size)
		return -1;
	else
	{
		//printf("size: %d\n", size);
		//printf("counter: %d\n", counter);
		//printf("entry: %s\n", entry);
		strncpy(list[counter], entry, strlen(entry));
		//printf("list[%d]: %s\n", counter, list[counter]);
		return 0;
	}
	return -1;
}

/*
INPUT: The inserting String, List of strings, the Max size of the String, The position to insert it, The Max size of the list 
OUTPUT:	0 - successful, -1 - unsuccessful
DESCRIPTION:
	Inserts String entry into list at position pos, checks that pos is a viable position and ensures String size can fit in the list.
*/
int insert(char* entry, char* list[], int max, int pos, int size)
{
	if (pos > size)
		return -1;
	else
	{
		if (strlen(entry) <= max)
		{
			//deleteEntry(list[pos]);
			strncpy(list[pos], entry, max);
			return 0;
		}
		else
			return -1;
	}
}

/*
INPUT: List of Strings, Size of List, and the Size to malloc each member
OUTPUT: Mallocs each position in entry[], no return
DESCRIPTION: 
	This allocates mal size for each position in entry[]
*/
void setEntry(char* entry[], int size, int mal)
{
	int i = 0;
	for (; i < size; i++)
	{
		entry[i] = (char*)malloc(mal);
		strncpy(entry[i], "\0", strlen("\0"));
	}
}

/*
INPUT:	List of strings, the size of the list
OUTPUT:	1- empty,	0- not empty
DESCRIPTION:
	Determines if the list is empty.
*/
int isEmpty(char* list[], int size)
{
	int i = 0;
	for (; i < size; i++)
	{
		if (strncmp(list[i], "\0", strlen(list[i])) > 0)
			break;
	}
	if (i == size)
		return 1;
	return 0;
}

/*
INPUT: List of strings, the size of the list
OUTPUT: resets every postion to "\0", otherwise returns nothing
DESCRIPTION:
	Take the list of strings and resets them to a null terminator.
*/
void clearList(char* list[], int size)
{
	int i = 0;
	for (; i < size; i++)
	{
		strncpy(list[i], "\0", strlen(list[i]));
	}
}

/*
INPUT: The list of strings, size of the list
OUTPUT: Frees up the memory allocated to the list. otherwise nothing
DESCRIPTION:
	Take the list of strings and frees up the memory in every position.
*/
void releaseList(char* list[], int size)
{
	int i = 0;
	for (; i < size; i++)
	{
		free(list[i]);
	}
}

/*
INPUT:	The filename or path
OUTPUT:	FILE pointer
DESCRIPTION:
	returns the FILE pointer associated with the filename or path
*/
FILE* openFile(char* path)
{
	return fopen(path, "r");
}

/*
INPUT:	FILE pointer
OUTPUT:	returns the status of closing the file.
DESCRIPTION:
	Takes a FILE pointer and closes the file.
*/
int closeFile(FILE * file)
{
	//return -1 if there was an error
	return fclose(file);
}

/*
INPUT: FAT32 pointer
OUTPUT: returns 0
DESCRIPTION:
	initializes all the values of a FAT32 data structure
*/
int initFat(FAT32* fat)
{
	//fat->BS_OEMName = malloc(sizeof(char) * 8);
	strncpy(fat->BS_OEMName, "\0", 8);
	fat->BPB_BytsPerSec = 0;
	fat->BPB_SecPerClus = 0;
	fat->BPB_RsvdSecCnt = 0;
	fat->BPB_NumFATS = 0;
	fat->BPB_RootEntCnt = 0;
	//fat->BS_VolLab = malloc(sizeof(char) * 11);
	strncpy(fat->BS_VolLab, "\0", 11);
	fat->BPB_TotSec32 = 0;
	fat->BPB_FATSz32 = 0;
	fat->BPB_RootClus = 0;
	fat->RootDirSectors = 0;
	fat->FirstDataSector = 0;
	fat->FirstSectorofCluster = 0;
	fat->DataSec = 0;
	fat->CountofClusters = 0;
	fat->FATsize = 0;
	fat->ClusStart = 0;
	fat->fatFile = NULL;

	return 0;
}

/*
INPUT:	String command, integer n
OUTPUT:	returns list of strings
DESCRIPTION:
	Takes a String with the full command and splits it into individual parts.
	sets n to the size of the list of strings.
*/
char** parseCmd(char* cmd, int * n)
{
	char** args = (char**)malloc(MAX_LINE_SIZE);
	char* arg = (char*)malloc(MAX_LINE_SIZE);
	char** ptr = args;
	int counter = 0;
	//uses strtok to parse the string into the individual arguments
	arg = strtok(cmd, " ");
	strcat(arg, "\0");
	while (arg != NULL)
	{
		//pointer manipulation if found to be easier than iterative
		//the spot ptr points to is now arg, set arg to the next argument
		//advance ptr to next position, repeat.
		*ptr = arg;
		arg = strtok(NULL, " ");
		strcat(arg, "\0");
		ptr++;
		counter++;
	}

	*n = counter;
	free(arg);
	return args;
}

/*
INPUT:	String, StrList pointer
OUTPUT: returns 0
DESCRIPTION:
	gets the input from the user.
*/
int getInput(char* line, StrList* list)
{
	char* strcmd = (char *)malloc(MAX_LINE_SIZE);
	char** p_cmd =(char**) malloc(MAX_LINE_SIZE);
	memset(p_cmd, 0, MAX_LINE_SIZE);

	printf("mfs>");
	fflush(stdout);
	fgets(strcmd, MAX_LINE_SIZE, stdin);
	//get rid of newline, replace with null terminator
	strcmd[strcspn(strcmd, "\n")] = 0;
	//now I have a version of the line with the null terminator
	//however I need to parse the line to get the individual parts
	strcpy(line, strcmd);
	int n = 0;		//gets the number of entries in p_cmd
	p_cmd = parseCmd(strcmd, &n);

	list->clearList(list->entry, list->maxStrings);
	int i = 0;
	for (; i < n; i++)
	{
		int p = list->add(p_cmd[i], list->entry, list->lineSize, list->maxStrings);
		if (p != 0)
			printf("failed\n");
	}
	free(strcmd);
	return 0;
}

/*
INPUT: StrList pointer
OUTPUT:	no return
DESCRIPTION:
	initializes every variable in data structure StrList
*/
void initList(StrList* list)
{
	list->add = append;
	list->set = setEntry;
	list->clearList = clearList;
	list->deleteEntry = deleteEntry;
	list->isEmpty = isEmpty;
	list->releaseList = releaseList;
	list->insert = insert;
	list->lineSize = MAX_LINE_SIZE;
	list->maxStrings = MAX_CMDS;
	list->set(list->entry, list->maxStrings, list->lineSize);
	list->head = list->entry[0];
}

/*
INPUT:	FAT32 pointer
OUTPUT: no return
DESCRIPTION:
	Gets the fat32 file structure information and stores it in the FAT32 data structure.
*/
void getFileStructure(FAT32* fat)
{
	//BS_OEMName
	fseek(fat->fatFile, 3, SEEK_SET);
	fread(fat->BS_OEMName, strlen(fat->BS_OEMName) + 1, 8, fat->fatFile);

	//BytsPerSec
	fseek(fat->fatFile, 11, SEEK_SET);
	fread(&fat->BPB_BytsPerSec, sizeof(fat->BPB_BytsPerSec), 2, fat->fatFile);

	//SecPerClus
	fseek(fat->fatFile, 13, SEEK_SET);
	fread(&fat->BPB_SecPerClus, sizeof(fat->BPB_SecPerClus), 1, fat->fatFile);

	//RsvdSecCnt
	fseek(fat->fatFile, 14, SEEK_SET);
	fread(&fat->BPB_RsvdSecCnt, sizeof(fat->BPB_RsvdSecCnt), 2, fat->fatFile);

	//NumFATs
	fseek(fat->fatFile, 16, SEEK_SET);
	fread(&fat->BPB_NumFATS, sizeof(fat->BPB_NumFATS), 1, fat->fatFile);

	//RootEntCnt
	fseek(fat->fatFile, 17, SEEK_SET);
	fread(&fat->BPB_RootEntCnt, sizeof(fat->BPB_RootEntCnt), 2, fat->fatFile);

	//TotSec32
	fseek(fat->fatFile, 32, SEEK_SET);
	fread(&fat->BPB_TotSec32, sizeof(fat->BPB_TotSec32), 4, fat->fatFile);

	//FATSz32
	fseek(fat->fatFile, 36, SEEK_SET);
	fread(&fat->BPB_FATSz32, sizeof(fat->BPB_FATSz32), 4, fat->fatFile);

	//RootClus
	fseek(fat->fatFile, 44, SEEK_SET);
	fread(&fat->BPB_RootClus, sizeof(fat->BPB_RootClus), 4, fat->fatFile);

	//others
	fat->RootDirSectors = ((fat->BPB_RootEntCnt * 32) + (fat->BPB_BytsPerSec - 1)) / fat->BPB_BytsPerSec;
	fat->FirstDataSector = fat->BPB_RsvdSecCnt + (fat->BPB_NumFATS * fat->BPB_FATSz32) + fat->RootDirSectors;
	fat->DataSec = fat->BPB_TotSec32 - (fat->BPB_RsvdSecCnt + (fat->BPB_NumFATS * fat->BPB_FATSz32) + fat->RootDirSectors);
	fat->CountofClusters = fat->DataSec / fat->BPB_SecPerClus;
	fat->FATsize = fat->BPB_NumFATS * fat->BPB_FATSz32 * fat->BPB_BytsPerSec;
	fat->ClusStart = (fat->FATsize) + (fat->BPB_RsvdSecCnt * fat->BPB_BytsPerSec);
	fat->FATpos = (int32_t*)malloc(fat->BPB_NumFATS * sizeof(int32_t));
	int i = 0;
	//not necessary
	for (; i < fat->BPB_NumFATS; i++)
	{
		if (i == 0)
			fat->FATpos[i] = fat->BPB_RsvdSecCnt * fat->BPB_BytsPerSec;
		else
			fat->FATpos[i] = fat->FATpos[i-1] + fat->FATsize/fat->BPB_NumFATS;
	}
}

/*
THIS IS NOT USED, USE LABToOffset instead

INPUT: Cluster Number n, FAT32 Struct copy
OUTPUT: returns the First Sector of a Cluster
DESCRIPTION:
	This method takes a Cluster number an calculates the first sector of that Cluster
*/
int findFirstSectorofCluster(int32_t n, FAT32 fat)
{
	return ((n - 2) * fat.BPB_SecPerClus) + fat.FirstDataSector;
}

/*
INPUT:	int sector, FAT32 datastructure
OUTPUT:	The offset for the position of the datablock associated with a sector
DESCRIPTION:
	Looks up into the FAT and finds the data associated with a sector.
*/
int LBAToOffset(int32_t sector, FAT32 fat)
{
	return ((sector - 2) * fat.BPB_BytsPerSec) + (fat.BPB_BytsPerSec * fat.BPB_RsvdSecCnt) + (fat.BPB_NumFATS * fat.BPB_FATSz32 * fat.BPB_BytsPerSec);
}

/*
THIS IS NOT USED, USE LBAToOffset instead.

INPUT:	Cluster Number n, Copy of FAT32 structure
OUTPUT:	Where the Entry for this cluster is
DESCRIPTION:
	This finds the Entry of Cluster n for FAT32 file system

	NOTE: This only gets the sector number for the first FAT,
			if you want the second FAT you have to multiply by FATSz32
			or for the third, 2*FATSz32 etcetera
*/
int findEntryofCluster(int n, FAT32 fat)
{
	int fatOffset = n * 4;
	int ThisFATSecNum = fat.BPB_RsvdSecCnt + (fatOffset / fat.BPB_BytsPerSec);
	int ThisFATEntOffset = fatOffset % fat.BPB_BytsPerSec;
	if (ThisFATEntOffset == (fat.BPB_BytsPerSec - 1))
	{
		printf("This cluster access spans a sector boundary in the FAT.");
		/*
		This cluster access spans a sector boundary in the FAT
		Figure out how to handle this later...
		*/
	}
	return ThisFATSecNum;
}

/*
INPUT: int sector, FAT32 data structure, int FAT#
OUTPUT: The address for that block of data
DESCRIPTION:	
	Finds the starting address of a block of data given the sector number
	corresponding to that data block.
*/
int16_t NextLB(uint32_t sector, FAT32 fat)
{
	uint32_t FATAddress = (fat.BPB_BytsPerSec * fat.BPB_RsvdSecCnt) + (sector * 4);
	int16_t val;
	fseek(fat.fatFile, FATAddress, SEEK_SET);
	fread(&val, 2, 1, fat.fatFile);
	return val;
}

/*
INPUT:	DirectoryEntry pointer
OUTPUT: no return
DESCRIPTION: 
	Initializes every member of DirectoryEntry dir
*/
void initDirEnt(struct DirectoryEntry* dir)
{
	strncpy(dir->DIR_Name, "\0", 1);
	dir->DIR_Attr = 0;
	dir->DIR_NTRes = 0;
	dir->DIR_CrtTimeTenth = 0;
	dir->DIR_CrtTime = 0;
	dir->DIR_CrtDate = 0;
	dir->DIR_LstAccDate = 0;
	dir->DIR_FirstClusterHigh = 0;
	dir->DIR_WrtTime = 0;
	dir->DIR_WrtDate = 0;
	dir->DIR_FirstClusterLow = 0;
	dir->DIR_FileSize = 0;
}

/*
INPUT:	DirectoryEntry pointer, FAT32 data structure
OUTPUT:	no return
DESCRIPTION:
	Reads in data to the DirectoryEntry Dir from the fat32.img
*/
void populateDirEnt(struct DirectoryEntry* Dir, FAT32 fat)
{
	fread(Dir, 1, 32, fat.fatFile);
}

/*
INPUT:	DirectoryEntry data structure
OUTPUT:	0 - relevant data, -1 - irrelevant data, 1- special, indicates parent directory
DESCRIPTION:	
	Determines if a file has never been used, or was once used and deleted, or is a directory, or is a parent directory
*/
int checkFirstChar(struct DirectoryEntry dir)
{
	uint8_t validate;
	memcpy(&validate, &dir, 1);
	if ((uint32_t)validate != 229)
	{
		if (dir.DIR_Name[0] == '.')
		{
			if (dir.DIR_Name[1] == '.')
			{
				return 1;
			}
			return 0;
		}
		return 0;
	}

	return -1;
}

/*
INPUT:	DirectoryEntry data structure
OUTPUT:	returns 1 upon success, -1 if it failes
DESCRIPTION:
	prints out the directory entry's name, if the filename failes checkFirstChar then the function failes.
*/
int showName(struct DirectoryEntry dir)
{
	int n = 0;
	if (checkFirstChar(dir) == -1)
		return -1;
	for (; n < 11; n++)
	{
		printf("%c", dir.DIR_Name[n]);
	}
	printf("\n");

	return 1;
}

/*
INPUT: string filename, Directory entry dir
OUTPUT:	1 - equal, -1 - not equal
DESCRIPTION:
	determines if filename and dir.DIR_Name are equal
*/
int cmpName(char* filename, struct DirectoryEntry dir)
{
	if (dir.DIR_Attr == 16)
	{
		int n = 0;
		for (; n < 11; n++)
		{
			if (filename[n] == 0)
				break;
			char c = toupper(filename[n]);
			//printf("fn[%d]: %c\tdir.DIR_Name[%d]: %c\n", n, c, n, dir.DIR_Name[n]);

			if (c != dir.DIR_Name[n])
				return -1;
		}
	}
	else
	{
		//check is used to know when the position is after the . in the filename
		uint8_t check = 0;
		int n = 0;
		int i = 0;
		for (; n < 11; n++, i++)
		{
			if (check == 1)
			{
				//if we have gotten to the '.' then leave to after that
				i = 7;
				break;
			}
			else
			{
				if (filename[n] == '.')
				{
					check = 1;
					continue;
				}
				if (filename[n] == 0)
				{
					break;
				}

				char c = toupper(filename[n]);
				//printf("fn[%d]: %c\tdir.DIR_Name[%d]: %c\n", n, c, i, dir.DIR_Name[i]);

				if (c != dir.DIR_Name[i])
					return -1;
			}
		}
		i++;
		for (; n < 11, i < 10; n++, i++)
		{
			char c = toupper(filename[n]);
			//printf("fn[%d]: %c\tdir.DIR_Name[%d]: %c\n", n, c, i, dir.DIR_Name[i]);
			if (c != dir.DIR_Name[i])
				return -1;
		}
		return 1;
	}
	return 1;
}

/*
INPUT:	FAT32 data structure
OUTPUT:	not return
DESCRIPTION:
	displays the information of a fat32 structure
*/
void DisplayFAT(FAT32 fat)
{
	printf("OEMName: %s\n", fat.BS_OEMName);
	printf("BytesPerSec: %d\n", fat.BPB_BytsPerSec);
	printf("FATSz32: %d\n", fat.BPB_FATSz32);
	printf("NumFATS: %d\n", fat.BPB_NumFATS);
	printf("RootClus: %d\n", fat.BPB_RootClus);
	printf("RootEntCnt: %d\n", fat.BPB_RootEntCnt);
	printf("RsvdSecCnt: %d\n", fat.BPB_RsvdSecCnt);
	printf("SecPerClus: %d\n", fat.BPB_SecPerClus);
	printf("TotSec32: %d\n", fat.BPB_TotSec32);
	printf("VolLab: %s\n", fat.BS_VolLab);
	printf("ClusStart: %d\n", fat.ClusStart);
	printf("CountofClusters: %d\n", fat.CountofClusters);
	printf("DataSec: %d\n", fat.DataSec);
	printf("FATsize: %d\n", fat.FATsize);
	printf("FirstDataSector: %d\n", fat.FirstDataSector);
	printf("FirstSectorofCluster: %d\n", fat.FirstSectorofCluster);
	printf("RootDirSectors: %d\n", fat.RootDirSectors);
	int i = 0;
	for(; i<fat.BPB_NumFATS; i++)
	{
		printf("FATpos[%d] = %d	//represents the start of FAT#%d\n",i, fat.FATpos[i],i+1);
	}
}

/*
INPUT:	DirectoryEntry data structure 
OUTPUT:	no return
DESCRIPTION:
	Displays a DirectoryEntry's information
	for debugging
*/
void DisplayDir(struct DirectoryEntry dir)
{
	int check = showName(dir);
	if (check == -1)
		return;
	printf("DIR_Attr: %x\n", dir.DIR_Attr);
	//printf("DIR_NTRes: %x\n", dir.DIR_NTRes);
	//printf("DIR_CrtTimeTenth: %x\n", dir.DIR_CrtTimeTenth);
	//interpretTime(dir.DIR_CrtTimeTenth);
	//printf("DIR_CrtTime: %x\n", dir.DIR_CrtTime);
	//printf("DIR_CrtDate: %x\n", dir.DIR_CrtDate);
	//printf("DIR_LstAccDate: %x\n", dir.DIR_LstAccDate);
	printf("DIR_FirstClusterHigh: %d\n", dir.DIR_FirstClusterHigh);
	//printf("DIR_WrtTime: %x\n", dir.DIR_WrtTime);
	//printf("DIR_WrtDate: %x\n", dir.DIR_WrtDate);
	printf("DIR_FirstClusterLow: %d\n", dir.DIR_FirstClusterLow);
	printf("DIR_FileSize: %d\n\n", dir.DIR_FileSize);
}

/*
INPUT:	StrList data struct, list of DirectoryEntry data structs, size of the DirectoryEntry list, int output
OUTPUT:	The position in the list of DirectoryEntry that matches the filename in StrList
DESCRIPTION:
	finds the DirectoryEntry postion with the same filename as in list.entry[1]
*/
int findFile(StrList list, struct DirectoryEntry dir[], int size, int output)
{
	//printf("file: %s\n", list.entry[1]);
	int i = 0;
	for (; i < size; i++)
	{
		if (dir[i].DIR_Attr == 32 || dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16)
		{
			//printf("i: %d\n", i);
			if (cmpName(list.entry[1], dir[i]) == 1)
				break;
		}
	}
	if (i == size)
	{
		if(output == 1)
			printf("Error: File not found\n");
		return -1;
	}

	return i;
}

/*
INPUT:	char filename, list of DirectoryEntry data structs, size of the DirectoryEntry list, int output
OUTPUT:	The position in the list of DirectoryEntry that matches path
DESCRIPTION:
	finds the DirectoryEntry postion with the same filename as path
*/
int findFileWithStr(char* path, struct DirectoryEntry dir[], int size, int output)
{
	int i = 0;
	for (; i < size; i++)
	{
		if (dir[i].DIR_Attr == 32 || dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16)
		{
			//printf("i: %d\n", i);
			if (cmpName(path, dir[i]) == 1)
				break;
		}
	}
	if (i == size)
	{
		if(output == 1)
			printf("Error: File not found\n");
		return -1;
	}

	return i;
}

/*
INPUT:	A String
OUTPUT:	No return
DESCRIPTION:
	Translates every postion in a string into the capital version.
*/
void toUpperCase(char* entry)
{
	int n = 0;
	int c = strcspn(entry, "\0");
	while (n < c)
	{
		entry[n] = toupper(entry[n]);
		n++;
	}
}

/*
INPUT:	String path
OUTPUT: n - position in string, -1 -Does not exist
DESCRIPTION:
	Finds any back slashes in a pathname and returns the position of the first one, otherwise returns -1
*/
int findBackSlash(char* path)
{
	int n = strcspn(path, "/");
	return n;
}

int main()
{
	//Get the data structures needed to work ready 

	StrList list;
	initList(&list);

	FAT32 fat;
	initFat(&fat);
	
	int size = 16;
	struct DirectoryEntry dir[size];
	int i = 0;
	for (; i < size; i++)
	{
		initDirEnt(&dir[i]);
	}

	Str command;
	int cmd;
	do
	{
		cmd = 0;
		getInput(command.entry, &list);

		//ignore empty entries
		if (strncmp(list.head, "\n", strlen(list.head)) == 0)
			continue;
		//flush it just in case it does something stupid.
		fflush(stdin);

		//Opens the fat32.img
		if (strncmp(list.head, "open", strlen(list.head)) == 0)
		{
			/*
			I have this program designed to for fat.fatFile to be initialized to NULL
				futhermore, when the file is closed fat.fatFile is reset to NULL
				thus, if fat.fatFile is not NULL then it already has an image.
			*/
			if (fat.fatFile != NULL)
			{
				printf("Error: File system image already open\n");
				cmd = 1;
				continue;
			}

			fat.fatFile = fopen(list.entry[1], "r");
			if (!fat.fatFile)
			{
				printf("Error: File system image not found\n");
				cmd = 1;
				continue;
			}

			//receive the fat32 file structure information
			getFileStructure(&fat);
			//get to the root directory
			fseek(fat.fatFile, fat.ClusStart, SEEK_SET);
			//extract data into directoryentry structure.
			i = 0;
			for (; i < size; i++)
			{
				populateDirEnt(&dir[i], fat);
			}
			cmd = 1;
			continue;
		}
		
		//Close the fat32.img
		if (strncmp(list.head, "close", strlen(list.head)) == 0)
		{
			if (!fat.fatFile)
			{
				printf("Error: File system not open.\n");
				cmd = 1;
				continue;
			}
			//close file
			fclose(fat.fatFile);
			//set file to null so we can check if it currently has a file or not.
			fat.fatFile = NULL;
			cmd = 1;
			continue;
		}
		
		//Displays info about the FAT32 structure
		if (strncmp(list.head, "info", strlen(list.head)) == 0)
		{	
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}
			//print the information in FAT32
			printf("BPB_BytesPerSec:\tDEC:%d\t\tHEX:%x\n", fat.BPB_BytsPerSec, fat.BPB_BytsPerSec);
			printf("BPB_SecPerClus:\t\tDEC:%d\t\tHEX:%x\n", fat.BPB_SecPerClus, fat.BPB_SecPerClus);
			printf("BPB_RsvdSecCnt:\t\tDEC:%d\t\tHEX:%x\n", fat.BPB_RsvdSecCnt, fat.BPB_RsvdSecCnt);
			printf("BPB_NumFATS:\t\tDEC:%d\t\tHEX:%x\n", fat.BPB_NumFATS, fat.BPB_NumFATS);
			printf("BPB_FATSz32:\t\tDEC:%d\t\tHEX:%x\n", fat.BPB_FATSz32, fat.BPB_FATSz32);
			cmd = 1;
			continue;
		}
		
		//Displays info about a DirectoryEntry
		if (strncmp(list.head, "stat", strlen(list.head)) == 0)
		{
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}
			//I will have the position in dir that matches the filename or directory name
			int i = findFile(list, dir, size, 1);
			//if it don't exist leave
			if (i == -1)
			{
				cmd = 1;
				continue;
			}

			//print out the attributes
			printf("Attribute: ");
			if (dir[i].DIR_Attr == 32)
				printf("ATTR_ARCHIVE\n");
			else if(dir[i].DIR_Attr == 1)
				printf("ATTR_READ_ONLY\n");
			else
				printf("ATTR_DIRECTORY\n");

			//print out the starting cluster number.
			printf("Starting Cluster Number: ");
			if (dir[i].DIR_Attr != 32 && dir[i].DIR_Attr != 1)
				printf("0\n");
			else
				printf("%d\n", dir[i].DIR_FirstClusterLow);
			cmd = 1;
			continue;
		}
		
		//CD to change directory
		if (strncmp(list.head, "cd", strlen(list.head)) == 0)
		{
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}
			//for clarity
			char* path = list.entry[1];
			if (strncmp(path, ".", strlen(path)) == 0)
			{
				//if the command has '.' then we aren't changing directories, so don't do anything.
				continue;
			}
			//used to find a backslash ex: foldera/folderc
			int bckSlsh = findBackSlash(path);

			//if a backslash is present in the string
			if (bckSlsh < strlen(path) && bckSlsh > 0)
			{
				//split the string up into each part delimited by /
				char* split = strtok(path, "/");
				//if split isn't null
				while (split != NULL)
				{
					//find the directory associated with split
					int i = findFileWithStr(split, dir, size, 1);
					//out data block offset
					int subDir = 0;
					//if it doesn't exist leave
					if (i == -1)
					{
						break;
					}
					//otherwise find the offset
					subDir = LBAToOffset(dir[i].DIR_FirstClusterLow, fat);
					//if the directory name is '..' then we need to check for something specific
					if (checkFirstChar(dir[i]) == 1 && dir[i].DIR_FirstClusterLow == 0)
					{
						//this is for the '..' that goes to root directory, not for the others
						subDir = fat.ClusStart;
					}
					//seek out the data block
					fseek(fat.fatFile, subDir, SEEK_SET);

					//extract data into the DirectoryEntries structure.
					i = 0;
					for (; i < size; i++)
					{
						populateDirEnt(&dir[i], fat);
					}
					//check for the next directory
					split = strtok(NULL, "/");
				}
				//get rid of split
				free(split);
				continue;
			}
			else
			{
				//if no backslash found then do the same as above but only once.
				int i = findFile(list, dir, size, 1);
				int subDir = 0;
				if (i == -1)
				{
					cmd = 1;
					continue;
				}

				subDir = LBAToOffset(dir[i].DIR_FirstClusterLow, fat);
				//if the directory name is '..' then we need to check for something specific
				if (checkFirstChar(dir[i]) == 1 && dir[i].DIR_FirstClusterLow == 0)
				{
					subDir = fat.ClusStart;
				}

				fseek(fat.fatFile, subDir, SEEK_SET);

				i = 0;
				for (; i < size; i++)
				{
					populateDirEnt(&dir[i], fat);
				}
			}
			
			cmd = 1;
			continue;
		}
		
		//GET to retreive data from the image.
		if (strncmp(list.head, "get", strlen(list.head)) == 0)
		{
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}
			//open up the new file using the given filename
			FILE* myfile = fopen(list.entry[1], "w");
			//find the associated file in the image
			int i = findFile(list, dir, size, 1);
			//if not found then leave
			if (i == -1)
			{
				break;
			}
			//find the file data block offset
			int file = LBAToOffset(dir[i].DIR_FirstClusterLow, fat);
			//counter
			int n = 0;
			//recieves the data
			uint8_t val = 0;
			//start the data extraction
			for (; n < dir[i].DIR_FileSize; n++)
			{
				//offset by n every iteration
				fseek(fat.fatFile, file + n, SEEK_SET);
				//read one byte into val
				fread(&val, 1, 1, fat.fatFile);
				//write val into myfile
				fwrite(&val, 1, 1, myfile);
			}
			//go back to the beginning
			fseek(fat.fatFile, 0, SEEK_SET);
			//close file.
			fclose(myfile);
			cmd = 1;
			continue;
		}
		
		//lS to list the directory
		if (strncmp(list.head, "ls", strlen(list.head)) == 0)
		{
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}

			//have this present, but it is just for appearances
			//other parts of the code handle how this works.
			printf(".\n");

			int i = 0;
			for (; i < size; i++)
			{
				//if a directoryentry has one of these attributes
				if (dir[i].DIR_Attr == 32 || dir[i].DIR_Attr == 1 || dir[i].DIR_Attr == 16)
				{
					//and it isn't a deleted file or something then print it, otherwise skip.
					if (showName(dir[i]) == -1)
					{
						continue;
					}
				}
			}

			cmd = 1;
			continue;
		}
		
		//READ some data from a file in the image
		if (strncmp(list.head, "read", strlen(list.head)) == 0)
		{
			if (fat.fatFile == NULL)
			{
				printf("Error: File system image must be opened first.\n");
				cmd = 1;
				continue;
			}
			
			//this will hold our value
			int val = 0;
			//first find the file
			int i = findFile(list, dir, size, 1);
			//be sure to check that the file was found
			if (i == -1)
			{
				break;
			}
			//then find the offset to the data
			int file = LBAToOffset(dir[i].DIR_FirstClusterLow, fat);
			//then apply the offset in the command to the offset of the data block
			int initOff = file + atoi(list.entry[2]);
			//then find the size of the file in question
			int fsize = atoi(list.entry[3]);
			//then start the data extraction
			int n = 0;
			for (; n < fsize; n++)
			{
				//seek to the next place in memory using n
				fseek(fat.fatFile, initOff + n, SEEK_SET);
				//read one byte into val
				fread(&val, 1, 1, fat.fatFile);
				printf("%x ", val);
			}

			printf("\n");
			cmd = 1;
			continue;
		}
		
		//QUIT is used to safely exit the program
		if (strncmp(list.head, "quit", strlen(list.head)) == 0)
		{
			//free up malloced space
			list.releaseList(list.entry, list.maxStrings);
			//if the fat.fatFile is not null close it
			if (fat.fatFile)
			{
				//close the file in use
				fclose(fat.fatFile);
				fat.fatFile = NULL;
			}
			cmd = -1;
			continue;
		}
		
		if (cmd == 0)
			printf("That is not a command\n");

	} while (cmd >= 0);
}
