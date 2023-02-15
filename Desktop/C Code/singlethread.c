#include <stdio.h>       
#include <stdlib.h>  
#include <sys/mman.h> 
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h> 
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>

//#define maxbytes 1073741824

//static unsigned char * encodedData;
//unsigned char * encodedData; 

void rle(char * fileContent, int index, int chunkSize){
    //Task Info
    unsigned char mychar;
    int count = 0;
    // a&
    // 0 1 
    while(count < chunkSize){   //count range: 0, filebytesize - 1, so we have according nec indices
        //printf("%s%d%s%c\n", "file char Index ", index, ": ", fileContent[index]);
        mychar = fileContent[index];  
        //check next char 
        bool checkNext = false;
        //
        if((index+1) < (index+chunkSize)){
            //valid index
            checkNext = fileContent[index] == fileContent[index + 1];   
        }
        //keep track how many chars there are in a row
        int currentcharCount = 1; 
        while (checkNext == true) {   
            //account for next char since they are equal
            currentcharCount += 1;
            //update count to next byte
            count += 1;
            //update index 
            index += 1;
            //repeateadly check for next position 

            if((index+1) < (index+chunkSize) ){
                //valid index
                checkNext = fileContent[index] == fileContent[index + 1];   
            }else{
                checkNext = false;
            }
        }

        //want to store count as an unsigned
         unsigned char unsignedcount = currentcharCount;
         //writing to our array, threads will do this at the same time, if indicies are right, there should be no race conditions
         encodedData[resultsIndex] = mychar;
         resultsIndex++;
         encodedData[resultsIndex] = unsignedcount;
         resultsIndex++;
         index++;
         count += 1;
      
    }

   
} //end rle

void * threadFunction(void * arg){
    while(1){
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        task_t * getTask;
        pthread_mutex_lock(&taskMutex);  
        //there are currently no tasks 
        while ((getTask = queueGetTask()) == NULL){
            //we pass mutex bc when a thread is suspended: 
            //it releases the lock, if we didnt then no other thread can grab the lock
            pthread_cond_wait(&taskSignal, &taskMutex);
            //try again
            //getTask = queueGetTask();
        }

        //unlock since we required nec info    
        pthread_mutex_unlock(&taskMutex);
    
        if(getTask != NULL){
            //we have to encode file
            //we have the task node
            //maybe return last char byte
            rle(getTask);

        }
    } //end while

}




int main(int argc, char  **argv){
   
   //calloc max amount of data that can be encoded 1GB
   //encodedData = (unsigned char *)calloc(maxbytes, sizeof (unsigned char));
   if(argc<2){
    exit(0);
   }
    
    //----------Initialize Thread Pool---------------
    pthread_t *mainThread[2]; 
    mainThread = (pthread_t* ) malloc ( 2 * sizeof(pthread_t));
    pthread_create(&mainThread[0], NULL, threadFunction, NULL);
   

    pthread_create(&mainThread[1], NULL, threadFunction, NULL);
 


   // ---Parse Command Line --- getopt rearranges argv
  
   int argumentIndex; //argv index, will be used to access filenames

   //----File Info-----
   int fd; //file descriptor
   struct stat sb; //File Size
   //array with all file pointers, each block will hold the ptr that mmap returns
   //char * fileMemoryPointers[numFiles]; 
   int totalChunks = 0;
   //int taskIndex = 0; // for queue 
   int totalInputBytes = 0;
   //int fileInd = 0;
   //char* file_c[argc-1];

   //----Accessing Filenames---
  
   for (argumentIndex = 1; argumentIndex<argc; argumentIndex++) {
        // ------Files: Virtual Memory Mapping---------------
        //open binary text file, read only, open returns int
        //printf("%s%s\n","filename: ", argv[argumentIndex]);
        fd = open(argv[argumentIndex], O_RDONLY);  //fd is -1 if it fails
        if(fd == -1){
            fprintf(stderr, "Error: Issue with opening provided file");
            exit(1);
        }
        //File Size
        fstat(fd, &sb);
        //virtually map data, mmap returns char * 
        char * currentFilePtr;
        currentFilePtr = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(currentFilePtr== MAP_FAILED){
            fprintf(stderr, "Error: Issue with MMAP");
            exit(1);
        }
        // Splitting File by 4KB Chunks
        int fileSize = (int)sb.st_size;
        close(fd);
        //printf("%s%d\n", "filesize: ", fileSize);

        int numChunks = fileSize / 4096;    
        if( (numChunks * 4096) < fileSize){
            //account for one more chunk 
            numChunks += 1;
        }
        // update num of chunks 
        totalChunks += numChunks;
        totalInputBytes += fileSize;
        

        int chunkStartIndex;

        for(chunkStartIndex = 0; chunkStartIndex < numChunks; chunkStartIndex++){
            //if we do 4096 * chunkStartIndex we have nec start index
            int start = chunkStartIndex * 4096;
            int numBytes = 4096;
            //every chunk will be the same size unless it is smaller thank 4kb
            if((chunkStartIndex+1) == numChunks){
                //checking size of last chunk
                if( ((chunkStartIndex + 1)*4096) > fileSize){
                    numBytes = fileSize - start;
                }
            }
    
            //--Mutual Exclusion-- Adding to task queue
            pthread_mutex_lock(&taskMutex);
            queueAddTask(currentFilePtr, numBytes, start);
            //signal any waiting threads that a new task has been added
            pthread_cond_signal(&taskSignal);
            //can unlock now 
            pthread_mutex_unlock(&taskMutex);
        } // end chunk for loop 
    
   } // end for loop 
    //printf("%s\n", "made it out while...");

   displayResults(totalChunks);

    int display = 0;
    while(displayed <= totalChunks) {
        unsigned char * lastTwoChars;
        lastTwoChars = (unsigned char *)calloc(2, sizeof(unsigned char));
        int previousCount;
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        task_t * taskResults =  malloc(sizeof(task_t *));    
        pthread_mutex_lock(&completeMutex);
        //there are currently no tasks 
        while ((taskResults = queueRemoveTask()) == NULL){
            //we pass mutex bc when a thread is suspended: 
            //it releases the lock, if we didnt then no other thread can grab the lock
            //queue remove tasks removes the head so it is in order
            pthread_cond_wait(&taskComplete, &taskMutex);
        }
        //unlock since we required nec info 
        pthread_mutex_unlock(&completeMutex);          
        if(taskResults != NULL){
            if(displayed == 0){
                //first time
                fwrite(taskResults->encodedResults, sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                lastTwoChars[0] = taskResults ->lastChars[0];
                lastTwoChars[1] = taskResults ->lastChars[1];
                previousCount = taskResults ->lastCharCount;
                displayed++;

            }else{

                //chech last char
                bool checkStitch; 

                unsigned char lastCharOne = taskResults->lastChars[0];
                checkStitch = lastCharOne == lastTwoChars[0];  
                //if you have to stitch
                if(checkStitch){
                    //update 
                    int newcount = previousCount + taskResults->lastCharCount;
                    unsigned char updatedC = newcount;
                    lastTwoChars[1] = updatedC;

                }
                fwrite(lastTwoChars, sizeof(unsigned char), 2, stdout);
                fwrite(taskResults->encodedResults, sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                //fwrite(&encodedData[taskResults->startIndex], sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                displayed++;   
                //update
                previousCount = taskResults ->lastCharCount;
                lastTwoChars[0] = taskResults->lastChars[0];
                lastTwoChars[1] = taskResults->lastChars[1];  
             }           
               
        } //end if 
        
    } //end while
    

    //fwrite(encodedData, sizeof(unsigned char), resultsIndex, stdout);
    //free(encodedData);
    //fflush(stdout);
    return 0;

}

