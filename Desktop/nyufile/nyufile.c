
#include <stdio.h>       
#include <stdlib.h>  
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h> 
#include <sys/types.h>
//#include <openssl/sha.h>

#define SHA_DIGEST_LENGTH 20

unsigned char *SHA1(const unsigned char *d, size_t n, unsigned char *md);


/*
Given: The OpenSSL library provides a function SHA1()
which computes the SHA-1 hash of d[0...n-1] 
and stores the result in md[0...SHA_DIGEST_LENGTH-1]:

Note: You need to add the compiler option -l crypto 
to link with the OpenSSL library.
*/


//Cited Outside Resources

/*
1. Reminded that optarg stores the option argument found by getopt 
and that optind contains the index of the next argument in argv
Cite:  https://www.ibm.com/docs/en/zos/2.1.0?topic=descriptions-getopts-parse-utility-options

2. Described all the fields that struct stat includes and what kind of file information I have access to 
Cite: http://codewiki.wikidot.com/c:struct-stat

*/

//FAT32 data structures provided 

//Boot Sector
#pragma pack(push,1)
typedef struct BootEntry {
  unsigned char  BS_jmpBoot[3];     // Assembly instruction to jump to boot code
  unsigned char  BS_OEMName[8];     // OEM Name in ASCII
  unsigned short BPB_BytsPerSec;    // Bytes per sector. Allowed values include 512, 1024, 2048, and 4096
  unsigned char  BPB_SecPerClus;    // Sectors per cluster (data unit). Allowed values are powers of 2, but the cluster size must be 32KB or smaller
  unsigned short BPB_RsvdSecCnt;    // Size in sectors of the reserved area
  unsigned char  BPB_NumFATs;       // Number of FATs
  unsigned short BPB_RootEntCnt;    // Maximum number of files in the root directory for FAT12 and FAT16. This is 0 for FAT32
  unsigned short BPB_TotSec16;      // 16-bit value of number of sectors in file system
  unsigned char  BPB_Media;         // Media type
  unsigned short BPB_FATSz16;       // 16-bit size in sectors of each FAT for FAT12 and FAT16. For FAT32, this field is 0
  unsigned short BPB_SecPerTrk;     // Sectors per track of storage device
  unsigned short BPB_NumHeads;      // Number of heads in storage device
  unsigned int   BPB_HiddSec;       // Number of sectors before the start of partition
  unsigned int   BPB_TotSec32;      // 32-bit value of number of sectors in file system. Either this value or the 16-bit value above must be 0
  unsigned int   BPB_FATSz32;       // 32-bit size in sectors of one FAT
  unsigned short BPB_ExtFlags;      // A flag for FAT
  unsigned short BPB_FSVer;         // The major and minor version number
  unsigned int   BPB_RootClus;      // Cluster where the root directory can be found
  unsigned short BPB_FSInfo;        // Sector where FSINFO structure can be found
  unsigned short BPB_BkBootSec;     // Sector where backup copy of boot sector is located
  unsigned char  BPB_Reserved[12];  // Reserved
  unsigned char  BS_DrvNum;         // BIOS INT13h drive number
  unsigned char  BS_Reserved1;      // Not used
  unsigned char  BS_BootSig;        // Extended boot signature to identify if the next three values are valid
  unsigned int   BS_VolID;          // Volume serial number
  unsigned char  BS_VolLab[11];     // Volume label in ASCII. User defines when creating the file system
  unsigned char  BS_FilSysType[8];  // File system type label in ASCII
} BootEntry;
#pragma pack(pop)

//Directory Entry
#pragma pack(push,1)
typedef struct DirEntry {
  unsigned char  DIR_Name[11];      // File name
  unsigned char  DIR_Attr;          // File attributes
  unsigned char  DIR_NTRes;         // Reserved
  unsigned char  DIR_CrtTimeTenth;  // Created time (tenths of second)
  unsigned short DIR_CrtTime;       // Created time (hours, minutes, seconds)
  unsigned short DIR_CrtDate;       // Created day
  unsigned short DIR_LstAccDate;    // Accessed day
  unsigned short DIR_FstClusHI;     // High 2 bytes of the first cluster address
  unsigned short DIR_WrtTime;       // Written time (hours, minutes, seconds
  unsigned short DIR_WrtDate;       // Written day
  unsigned short DIR_FstClusLO;     // Low 2 bytes of the first cluster address
  unsigned int   DIR_FileSize;      // File size in bytes. (0 for directories)
} DirEntry;
#pragma pack(pop)


int main(int argc, char **argv){
    
    //------Section: Parsing Command Line----------
    int userOption; //will store what the user option was, initialize data with provided info, and then exec command option accordingly 
    int commandLine; //getopt int
    int optionCount; //to make sure command line if valid
    
    //Getopt vars, Note: getopt rearranges argv
    extern char *optarg; //stores the value of the option argument found by getopts
    extern int optind; //optind contains the index of the next argument in argv

    //File and Disk Info
    int fd; //file descriptor for when opening file
    int givenSha; //see if sha1 was an option
    unsigned char * fileName; //filename will be stored in optarg
    unsigned char * sha1Hash; 
    //disk is just a file so we can open using mmap 
    struct BootEntry *disk; //data struct to store the boot sector
    struct stat fileInfo; //to have access to file info like file size

    //initialize sha option as 0
    givenSha = 0;
    //option count will start at 0
    optionCount = 0;
    
    /*----Command Option Requirements-----
    user options can be i,l,r,R,s 
    Important: r,R,s must require option arguments   
        -r requires filename (required option argument) input and can have s as an option 
        -R requires filename (required option argument) as well, as well as s (required option argument) 
        -s sha1 helps identify which deleted directory entry should be the target file
        sha1 is represented as 40 hexadecimal digitis
        we can assume identical files have the same sha1 hash 
        
    
    Validity: You need to check if the command-line arguments are valid.
    If not, your program should print the above usage information and exit.
    
    */
 
    while ((commandLine = getopt(argc, argv, "ilr:R:s:")) != -1){
        switch (commandLine) {
            case 'i':
                //Command i: Print the file system information
                //validiate cmd
                if(optionCount != 0){ //only can perform one option argument
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 1;
                optionCount = 1; //inc count
                break;
            case 'l':
                //Command l: List the root directory
                //validiate cmd 
                if(optionCount != 0){ //can only perform one option argument
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 2;
                 optionCount = 1; //inc count
                break;
            case 'r':
                //Command r: Recover a contiguous file, might have s as an option
                //validiate cmd 
                if(optionCount != 0){ //can only perform one option argument
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 3;
                //This command requires option argument Filename, store info 
                fileName = (unsigned char *) optarg;
                optionCount = 1; //inc count
                break;
            case 'R':
                //Command R: Recover a possibly non-contiguous file
                //validiate cmd 
                if(optionCount != 0){ //can only perform one option argument 
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 4;
                //This command requires option argument Filename, store info 
                fileName = (unsigned char *) optarg;
                optionCount = 1; //inc count
                break;
            case 's':
                //Command s: recover file with SHA-1
                //when recovered with sha1, 
                //print filename: successfully recovered with SHA-1
                //validiate cmd 
                if(optionCount != 1){
                    //error , s cant be an option argument on its own 
                    userOption = 5; //print usage info and exit
                    break;
                }
                //option count is 1, now validate that user 
                //previously entered option arg r or R 
                if(userOption != 3 && userOption != 4){
                    userOption = 5; //print usage info and exit
                    break;
                }
                //option count is 1, and useroption is either 3 or 4
                //sha1 is option argument 
                sha1Hash = (unsigned char *) optarg;
                givenSha = 1;
                break;
            default:
                userOption = 5;
                break;
        }
        //end switch
    }
    //end while loop

    //---------Section: Executing Error Command-------------
    //user option 5
    if(userOption == 5){
        //print usage and exit
        fprintf(stderr, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
        return 0;
    }

    //-----------Section: Boot Entry/Disk----------------
   
    struct DirEntry *rootDirectory;
    int firstByte;
  
    //The first argument is the filename of the disk image 
    //getopt rearranged argv so next argument is disk name & optind contains the index of the next argument in argv
    fd = open(argv[optind], O_RDWR); //file descriptor
    if(fd == -1){ //fd failure, cant open file/disk
        fprintf(stderr, "Usage: ./nyufile disk <options>\n  -i                     Print the file system information.\n  -l                     List the root directory.\n  -r filename [-s sha1]  Recover a contiguous file.\n  -R filename -s sha1    Recover a possibly non-contiguous file.\n");
        return 0;
    }
    fstat(fd, &fileInfo); //file size
    //Declare data struct (boot entry) and read FAT 32 disk image into the data struct
    //virtually map data, mmap returns bootentry *: contains info about file system as well as boot code

    disk = (BootEntry *) mmap(NULL, fileInfo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED){
        //error with mmap
        fprintf(stderr, "Error with MMAP");
        return 0;
    }
    //In order to locate where each FAT is, we can just read the entire FAT table into an array
    //virtually map data, mmap returns unsigned char *
    unsigned char * diskArr;
    diskArr = (unsigned char *) mmap(NULL, fileInfo.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd); //close file descriptor 


    // Access first data sector: (seccount + (fatsize * numFats)) * bytes per sec
    firstByte =  (disk ->BPB_RsvdSecCnt + (disk->BPB_NumFATs * disk->BPB_FATSz32)) * disk->BPB_BytsPerSec; //first byte of root directory
    
    // any valid data cluster number N, 
    //the sector number of the first sector of that cluster (again relative to sector 0 of the FAT volume) is computed as follows:
    //FirstSectorofCluster = ((N – 2) * BPB_SecPerClus) + FirstDataSector;
    /*
    0x5000 happens to be the start of the data area, 
    and is where the root directory table is located. 
    This is where you start iterating 
    to print out entries of the root directory.
    */
    
  
    /*
    Once you have maped your disk image,
     you can cast any address to the FAT32 data structure type,
     such as (DirEntry *)(mapped_address + 0x5000).
    */

   //---- Section: Executing User Command----------
   // cluster size = bytes per sec * sectors per cluster
   // next cluster byte start = (reserved sector count * bytes per sec) + cluster 
   if(userOption == 1){
    //command i: print the file system info
    printf("%s%d\n", "Number of FATs = ", disk->BPB_NumFATs);
    printf("%s%d\n", "Number of bytes per sector = ", disk->BPB_BytsPerSec);
    printf("%s%d\n", "Number of sectors per cluster = ", disk->BPB_SecPerClus);
    printf("%s%d\n", "Number of reserved sectors = ", disk->BPB_RsvdSecCnt);
    return 0;
   }else if (userOption == 2){
    //command l: list the root directory
    //access FAT right after res sectors
    unsigned int * FAT = (unsigned int *) &diskArr[disk->BPB_BytsPerSec * disk->BPB_RsvdSecCnt];
    int i;
    int clustNum = disk->BPB_RootClus;
    int numEntries = 0; //some may be empty   
    unsigned int entriesPerCluster = (disk->BPB_BytsPerSec * disk->BPB_SecPerClus)/sizeof(DirEntry); //clus size/dir entry size
    // printf("entries per clus = %d\n", entriesPerCluster);
    //printf("first cluster num = %d\n", disk->BPB_RootClus);
    
    int chByte = firstByte;
    unsigned char ch = diskArr[chByte];
    //first entry
    struct DirEntry *rdir = (DirEntry *) &diskArr[chByte];
    //printf("%s%d\n", "Number of sectors per cluster = ", disk->BPB_SecPerClus);
    // printf("bytes per sec = %d\n", disk->BPB_BytsPerSec);
    // printf("Number of bytes for %d sector in each clus: %d\n = ", disk->BPB_SecPerClus, disk->BPB_SecPerClus*disk->BPB_BytsPerSec);

    //we know how many clusters

    //printf("fat value = %d\n", FAT[clustNum]);
    
   
    int countEntries = 0;
    if(rdir->DIR_Name[0]== (unsigned char)0xE5 ){

        //Del file, move on
        printf("Total number of entries = %d\n", numEntries);
        return 0;
    }

    //iterate through entries
    //    //next cluster , documentation: ((N – 2) * BPB_SecPerClus) + FirstDataSector;
    
    struct DirEntry *nextCluster = (DirEntry *) &diskArr[FAT[clustNum]];

    // printf("first byte = %d\n",  firstByte);
    //printf("next cluster value = %d\n",  FAT[clustNum]);
    //((FAT[clustNum] - disk->BPB_RootClus) * disk->BPB_SecPerClus) + firstByte
    //printf("entries per first clus = %d\n", disk->BPB_RootEntCnt);
    //printf("first cluster num = %d\n", disk->BPB_RootClus);
   // printf("next cluster name = %d\n",  nextCluster->DIR_Name);


  
 

    //print all entires of cluster
    while(rdir->DIR_Name[0] != 0){ 

        if(rdir->DIR_Name[0] == (unsigned char)0xE5 ){

            //del entry, move on
            chByte += 32;
            rdir = (DirEntry *) &diskArr[chByte];
            countEntries +=1;
            continue;
        }
        //valid to print
        for(i = 0; i<8; i++){ 

            //filename 
            //space
            if(rdir->DIR_Name[i] == ' '){ //space
                continue;
            }else{
                printf("%c", rdir->DIR_Name[i]);
            }
            
        }
        // . txtA
        //Check if dir
        // 01234567 8. 9 10 11
        //H E L L O H H H . TXT
        if(rdir->DIR_FileSize == 0 && rdir->DIR_FstClusLO){

            //is a dir
            printf("/");
        }else if (rdir->DIR_Name[8] != ' ') //There is an extension
        {
            printf(".");
            
            for(i = 8; i<11; i++){ 

                //filename 
                //space
                if(rdir->DIR_Name[i] != ' '){ //space
                    printf("%c", rdir->DIR_Name[i]);
                }

            
            }
        }
        
        //check if file or dir
        //we have: unsigned char DIR_NTRes;
        if(rdir->DIR_FstClusHI == 0){
        printf(" (size = %d, starting cluster = %d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusLO);
    
        numEntries +=1; 
        }else{
        printf(" (size = %d, starting cluster = %d%d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusHI, rdir->DIR_FstClusLO);
        numEntries +=1; 
        
        }  
        
        countEntries +=1;
        chByte += 32;
        rdir = (DirEntry *) &diskArr[chByte];

} //end while


    //upate cluster , next clus    
    //clustNum = FAT[clustNum];
    //rdir = (DirEntry *) &diskArr[clustNum];



    /*
    while(rdir->DIR_Name[0] != 0){ 

        if(rdir->DIR_Name[0] == (unsigned char)0xE5 ){
            //del entry, move on
            chByte += 32;
            rdir = (DirEntry *) &diskArr[chByte];
            countEntries +=1;
            continue;
        }
        //valid to print
        for(i = 0; i<8; i++){ 

            //filename 
            //space
            if(rdir->DIR_Name[i] == ' '){ //space
                continue;
            }else{
                printf("%c", rdir->DIR_Name[i]);
            }
            
        }
        // . txtA
        //Check if dir
        // 01234567 8. 9 10 11
        //H E L L O H H H . TXT
        if(rdir->DIR_FileSize == 0 && rdir->DIR_FstClusLO){

            //is a dir
            printf("/");
        }else if (rdir->DIR_Name[8] != ' ') //There is an extension
        {
            printf(".");
            
            for(i = 8; i<11; i++){ 

                //filename 
                //space
                if(rdir->DIR_Name[i] != ' '){ //space
                    printf("%c", rdir->DIR_Name[i]);
                }

            
            }
        }
        
        //check if file or dir
        //we have: unsigned char DIR_NTRes;
        if(rdir->DIR_FstClusHI == 0){
        printf(" (size = %d, starting cluster = %d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusLO);
       
        numEntries +=1; 
        }else{
        printf(" (size = %d, starting cluster = %d%d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusHI, rdir->DIR_FstClusLO);
        numEntries +=1; 
        
        }  
        
        countEntries +=1;
        chByte += 32;
        rdir = (DirEntry *) &diskArr[chByte];

    } //end while
    */

    //no more entries in this cluster
   // printf("fat value = %d\n",  FAT[clustNum]);
    //printf("next cluster byte: %d\n", firstByte + (FAT[clustNum] * 32) );
    //((FAT[clustNum] - disk->BPB_RootClus) * disk->BPB_SecPerClus) + firstByte
   // printf("next cluster byte formula: %d\n", ((FAT[clustNum] - disk->BPB_RootClus) * disk->BPB_SecPerClus) + firstByte);
    //rdir = (DirEntry *) &diskArr[firstByte + (FAT[clustNum] * 32) ];
    //printf("next cluster name = %s\n",  rdir->DIR_Name);
   // clustNum = FAT[clustNum];
    //printf("next cluster value from fat = %d\n",  FAT[clustNum]);
    

    //next cluster address
    


    /*
    printf("next cluster value = %d\n",  FAT[clustNum]);
    printf("next cluster byte: %d\n", firstByte + (FAT[clustNum] * 32) );
    rdir = (DirEntry *) &diskArr[firstByte + (FAT[clustNum] * 32) ];
    printf("next cluster name = %s\n",  rdir->DIR_Name);
    clustNum = FAT[clustNum];
    printf("next cluster value from fat = %d\n",  FAT[clustNum]);
    */
   //((FAT[clustNum] - disk->BPB_RootClus) * disk->BPB_SecPerClus) + firstByte
   //printf("entries per first clus = %d\n", disk->BPB_RootEntCnt);
   //printf("first cluster num = %d\n", disk->BPB_RootClus);
   // printf("next cluster name = %d\n",  nextCluster->DIR_Name);

 
 

    // printf("diskarray[chbyte]:  = %c\n", ch);
    //printf("filename:  = ");

        
    //printf("\n");
    //printf(" displayed filename \n");

  
   //printf("%s%s\n", " next cluster name:  ", nextCluster->DIR_Name);

    printf("Total number of entries = %d\n", numEntries);
    //printf("diskarray[1]:  = %c\n", diskArr[1]);
   } //end else if
    return 0;


}
/*
    if(FAT[clustNum] != EOF){
        //another cluster
        rdir = (DirEntry *) &diskArr[FAT[clustNum]];
        chByte = FAT[clustNum];
        while(rdir->DIR_Name[0] != 0){

            //valid to print
            if(rdir->DIR_Name[0] == (unsigned char)0xE5 ){

                //del entry, move on
                chByte += 32;
                rdir = (DirEntry *) &diskArr[chByte];
                countEntries +=1;
                continue;
            }
    
            for(i = 0; i<8; i++){ 
                //filename 
                //space
                if(rdir->DIR_Name[i] == ' '){ //space
                    continue;
                }else{
                    printf("%c", rdir->DIR_Name[i]);
                }
                
            }
            // . txtA
            //Check if dir
            // 01234567 8. 9 10 11
            //H E L L O H H H . TXT
            if(rdir->DIR_FileSize == 0 && rdir->DIR_FstClusLO){

                //is a dir
                printf("/");
            }else if (rdir->DIR_Name[8] != ' ') //There is an extension
            {
                printf(".");
                
                for(i = 8; i<11; i++){ 

                    //filename 
                    //space
                    if(rdir->DIR_Name[i] != ' '){ //space
                        printf("%c", rdir->DIR_Name[i]);
                    }

                
                }
            }
            
            //check if file or dir
            //we have: unsigned char DIR_NTRes;
            if(rdir->DIR_FstClusHI == 0){
            printf(" (size = %d, starting cluster = %d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusLO);
            
            }else{
            printf(" (size = %d, starting cluster = %d%d)\n",  rdir->DIR_FileSize, rdir->DIR_FstClusHI, rdir->DIR_FstClusLO);
            
            }  
            
            countEntries +=1;
            chByte += 32;
            rdir = (DirEntry *) &diskArr[chByte];

        } //end while

    } //end if
*/