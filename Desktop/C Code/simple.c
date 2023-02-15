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
//int totalTasks;
struct node {
    struct node* next;  
    char *startPointer; 
    unsigned char *encodedResults;
    int startIndex; 
    int bytes; 
    int complete; //0 if not comp, 1 if it is comp
    unsigned char *lastChars;
    int outputSize;
    int lastCharCount;
};
typedef struct node task_t;
int completed;
int totalNumTasks;
//gloabl vars for queue linked list
task_t *head = NULL;
task_t *tail= NULL;
task_t *resultsHead = NULL;
task_t *resultsTail = NULL;
task_t *currentNode = NULL;


pthread_mutex_t completeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskComplete = PTHREAD_COND_INITIALIZER;



pthread_mutex_t taskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskSignal = PTHREAD_COND_INITIALIZER;


int queueAddTask (char *content, int size, int starting){
    //printf("%s\n","in queue add task!");
    //printf("%s%s\n","content: ", content);
    //printf("%s%d\n","size: ", size);
    //printf("%s%d\n","starting: ", starting);
    //printf("%s\n","--------------------------");
    //create new node to add to queue
    task_t *newTask = (task_t*)malloc(sizeof(task_t));
   
    newTask -> startPointer = content;
    newTask -> startIndex = starting;
    newTask->lastChars = (unsigned char*)calloc(2, sizeof(unsigned char));
    newTask -> encodedResults = (unsigned char*)calloc(size*2, sizeof(unsigned char));
    newTask -> bytes = size; 
    newTask -> complete = 0;
    newTask -> next = NULL;
    newTask->lastCharCount = 0;
    

    if(tail == NULL){
        head = newTask;
    }else{
        tail->next = newTask;
    }
    tail = newTask;

    if(resultsTail == NULL){
        resultsHead = newTask;
    }else{
        resultsTail->next = newTask;
    }
    resultsTail = newTask;
  
    

    // do the same to results queue
    //printf("%s\n","completely added task!");
    //printf("%s\n","----------");
    return 1;
}


//returns task node if there is one
//FIFO queue
task_t * queueGetTask(){
    if(head == NULL){
        return NULL;
    }else{
        task_t *returnTask = head;
        head = head->next;
        if(head == NULL){
            tail = NULL;
        }
        return returnTask;
    }
}

//returns char * output if there is a task in queue
//FIFO queue
task_t * queueRemoveTask(){
    // Returns null if the queue is empty
    if(resultsHead== NULL){
        return NULL;
    }else{
        //collect task
        task_t *returnTask = resultsHead;
        resultsHead = resultsHead->next;
        if(resultsHead == NULL){
            resultsTail = NULL;
        }
        return returnTask;
    }
}

void rle(task_t * currentTask){
    
    //Task Info
    int chunkSize = currentTask -> bytes;
    int count = 0;
    int index = currentTask->startIndex;
    //int writeIndex = currentTask->startIndex;
    char * fileContent = currentTask->startPointer;
    
    //unsigned char * rleResults = currentTask->encodedResults;
    int storeResults = 0;
    int lastCount = 0;
    unsigned char mychar;

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
        lastCount = currentcharCount;
        unsigned char unsignedcount = currentcharCount;
         //writing to our array, threads will do this at the same time, if indicies are right, there should be no race conditions
        currentTask->encodedResults[storeResults] = mychar;
        storeResults++;
        currentTask->encodedResults[storeResults] = unsignedcount;   
        storeResults++;    
        index++;
        count += 1;
      
    } //end while
    //signal that thread is done? Can multiple threads safely do this?
    currentTask->complete = 1; 
    // have to lock to signal
    // have to lock to signal
    //printf("%s\n","completed rle!");  
    pthread_mutex_lock(&completeMutex);
    completed += 1;
    if(completed == totalNumTasks){
        pthread_cond_signal(&taskComplete);
    }
    //pthread_cond_signal(&taskComplete);
    pthread_mutex_unlock(&completeMutex);
    
    
  
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
        }

        //unlock since we required nec info    
        pthread_mutex_unlock(&taskMutex);
    
        if(getTask != NULL){
            //we have to encode file
            //printf("%s\n","sending to rle!");
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
    completed = 0;
    totalNumTasks = 0;
    //----------Initialize Thread Pool---------------
    pthread_t mainThread; 
    pthread_create(&mainThread, NULL, threadFunction, NULL);
   // pthread_create(&mainThread[1], NULL, displayResults, NULL);
   
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
            //printf("%s%d\n", "bytes: ", numBytes);
            //rle(currentFilePtr, start, numBytes);
            //--Mutual Exclusion-- Adding to task queue
            pthread_mutex_lock(&taskMutex);
            //printf("%s\n","adding task!");      
            queueAddTask(currentFilePtr, numBytes, start);
            totalNumTasks += 1;
            pthread_cond_signal(&taskSignal);
            //can unlock now 
            pthread_mutex_unlock(&taskMutex);
            //printf("%s\n","unlocked from task mutex");
        } // end chunk for loop 
    
   } // end for loop 
    //free(encodedData);
    //fflush(stdout);
    
    pthread_mutex_lock(&completeMutex);
    //there are currently no tasks 
    while (completed != totalChunks){
        //printf("%s%d\n","total num of tasks: ", totalNumTasks);
        //printf("%s%d\n","total chunks: ",totalChunks); 
        //printf("%s%d\n","completed so far: ",completed); 
        pthread_cond_wait(&taskComplete, &completeMutex);
    }
    //unlock since we required nec info 
    pthread_mutex_unlock(&completeMutex);  
    //printf("%s\n","-------out of complete lock------"); 
    //printf("%s%d\n","completed so far: ",completed); 
    //printf("%s%d\n","total chunks: ",totalChunks); 
    int displayed = 0;  
    
    while(displayed < totalChunks) {
        //printf("%s%d\n","displayed: ", displayed);
        unsigned char * lastTwoChars;
        lastTwoChars = (unsigned char *)calloc(2, sizeof(unsigned char));
        int previousCount;
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        task_t * taskResults =  malloc(sizeof(task_t *));
        
        pthread_mutex_lock(&completeMutex);
        taskResults = queueRemoveTask();
        //unlock since we required nec info 
        pthread_mutex_unlock(&completeMutex);  
        //printf("%s\n","unlocked completed task mutex!");        
        if(taskResults != NULL){
            if(displayed == 0){
                //first time
                //printf("%s\n","---FINALLY PRINTING---"); 
                fwrite(taskResults->encodedResults, sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                lastTwoChars[0] = taskResults ->lastChars[0];
                lastTwoChars[1] = taskResults ->lastChars[1];
                previousCount = taskResults ->lastCharCount;
                displayed++;

            }else{

                //check last char
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
                //printf("%s\n","---STITCH PRINTING---"); 
                fwrite(lastTwoChars, sizeof(unsigned char), 2, stdout);
                fwrite(taskResults->encodedResults, sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                //fwrite(&encodedData[taskResults->startIndex], sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                displayed++;   
                //update
                lastTwoChars[0] = taskResults->lastChars[0];
                lastTwoChars[1] = taskResults->lastChars[1];  
             }           
               
        } //end if 
        
    } //end while
    

    //printf("%s\n","DONE WITH PROGRAM?"); 
    //free(encodedData);
    //fflush(stdout);  
    return 0;

}

