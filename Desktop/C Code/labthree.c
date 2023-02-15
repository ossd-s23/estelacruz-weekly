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

#define maxbytes 1073741824

//static unsigned char * encodedData;
unsigned char * encodedData; 
//int totalTasks;
struct node {
    struct node* next;  
    char *startPointer; //to read from our file
    //unsigned char *encodedResults;
    int startIndex; 
    int endIndex; //last time node wrote to our whole encoded data array, 
    int bytes; //number of bytes from file that this task will encode
    int complete; //0 if not comp, 1 if it is comp
    unsigned char *lastChars; //pointer to array which hold previous two chars, the char and the count
    int outputSize; 
    int lastCharCount;
    int numberofChunks;
    int nextStart;
};
typedef struct node task_t;
int completed;
int totalNumTasks;
int totalOutputBytes;
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

int queueAddTask (char *content, int size, int starting, int chunks){
    task_t *newTask = (task_t*)malloc(sizeof(task_t));
   
    newTask -> startPointer = content;
    newTask -> startIndex = starting;
    newTask->lastChars = (unsigned char*)calloc(2, sizeof(unsigned char));
    newTask -> bytes = size; 
    newTask -> complete = 0;
    newTask -> next = NULL;
    newTask->lastCharCount = 0;
    newTask->numberofChunks = chunks;
    newTask->endIndex = 0;
    newTask->nextStart = 0;
    
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
    return 1;
}

void * queueGetTask(){
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

int rle(task_t * currentTask){
    
    //Task Info
    int chunkSize = currentTask -> bytes;
    int count = 0;
    int index = currentTask->startIndex;
    int writeIndex = currentTask->startIndex;
    char * fileContent = currentTask->startPointer;
    
    int storeResults = 0;
    int lastCount = 0;
    unsigned char mychar;

    while(count < chunkSize){   //count range: 0, filebytesize - 1, so we have according nec indices
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
        // a a a b
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
        encodedData[writeIndex] = mychar;
        writeIndex++;
        storeResults = storeResults + 1;
        encodedData[writeIndex] = unsignedcount;
        writeIndex++;
        storeResults = storeResults + 1;
        index++;
        count += 1;
      
    } //end while
    currentTask->complete = 1; 
    currentTask->lastCharCount = lastCount;
    //printf("%s%d\n","how many times we wrote: ", storeResults);  
    currentTask->outputSize = storeResults;
    currentTask->endIndex = writeIndex; 
    pthread_mutex_lock(&completeMutex);
    totalOutputBytes += currentTask->outputSize;
    completed += 1;
    if(completed == totalNumTasks){
        pthread_cond_signal(&taskComplete);
    }
    pthread_mutex_unlock(&completeMutex);
    int done = completed;
    return done;
    
    
  
} //end rle

void * threadFunction(void * arg){
    while(1){
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        task_t * getTask;
        pthread_mutex_lock(&taskMutex);  
        //there are currently no tasks 
        while ((getTask = queueGetTask()) == NULL){
            //we pass mutex bc when a thread is suspended: 
            pthread_cond_wait(&taskSignal, &taskMutex);
        }

        //unlock since we required nec info    
        pthread_mutex_unlock(&taskMutex);
    
        if(getTask != NULL){
            //we have to encode file
            int checkingdone = rle(getTask);          
        }
    } //end while
}

int main(int argc, char  **argv){
   
   //calloc max amount of data that can be encoded 1GB
   encodedData = (unsigned char *)calloc(maxbytes, sizeof (unsigned char));
   if(argc<2){
    exit(0);
   }
   //initialize global vars
    completed = 0;
    totalNumTasks = 0;
    totalOutputBytes = 0;
    
    //Get Number of worker threads / jobs
    int numThreads = 0;
    int argumentIndex = 1; //2 if gdb, 1 if prog
    int checkJobs = getopt(argc, argv, "j");
    
    if(checkJobs == -1){
        //there are no jobs, have to run sequentially
        // r /.nyuenc fileone                                                                                                              
        numThreads = 1;

    }else{
        numThreads =  atoi(argv[optind]);
        // r ./nyuenc -r 3 filename
        argumentIndex = 3; //4 if gdb, 3 if prog
        }

    //----------Initialize Thread Pool---------------
    pthread_t threadPool[numThreads]; 
    int t;
    for(t = 0; t<numThreads;t++){
        pthread_create(&threadPool[t], NULL, threadFunction, NULL);
    }
    

   // ---Parse Command Line --- getopt rearranges argv

    //----File Info-----
    int fd; //file descriptor
    struct stat sb; //File Size
    //array with all file pointers, each block will hold the ptr that mmap returns
    int totalChunks = 0;
    int numFiles = argc-argumentIndex; 
    
    //----Storing File Info in Arrays---
    //int fdArr[argc-argumentIndex];
    char* mappedFiles[argc-argumentIndex];
    int fileChunksArr[argc-argumentIndex];
    int fileSizes[argc-argumentIndex];
    int track = 0;
    for (argumentIndex; argumentIndex<argc; argumentIndex++) {
        // ------Files: Virtual Memory Mapping---------------
        //open binary text file, read only, open returns int
        fd = open(argv[argumentIndex], O_RDONLY);  //fd is -1 if it fails
        if(fd == -1){
            fprintf(stderr, "Error opening file");
            exit(1);
        }
        //File Size
        fstat(fd, &sb);
        //virtually map data, mmap returns char * 
        char * currentFilePtr;
        currentFilePtr = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(currentFilePtr== MAP_FAILED){
            fprintf(stderr, "Error with MMAP");
            exit(1);
        }
        mappedFiles[track]= currentFilePtr;
        // Splitting File by 4KB Chunks
        int fileSize = (int)sb.st_size;
        close(fd);
        fileSizes[track] = fileSize;
        int numChunks = fileSize / 4096;    
        if( (numChunks * 4096) < fileSize){
            //account for one more chunk 
            numChunks += 1;
        }
        // update num of chunks 
        fileChunksArr[track] = numChunks;
        totalChunks += numChunks;
        track++;
        
        // totalInputBytes += fileSize;  
    } // end for loop 


    int u;
    int chunkStartIndex = 0;
    int g;
    for ( u=0; u<numFiles; u++){
        //have mapped content and file size
        int fileChunkSize = fileChunksArr[u];
        int filesize = fileSizes[u];
        g = 0;
        for(g; g < fileChunkSize ; g++){
            //if we do 4096 * chunkStartIndex we have nec start index
            int start = chunkStartIndex * 4096;
            int numBytes = 4096;
            //every chunk will be the same size unless it is smaller thank 4kb
            if((chunkStartIndex+1) == fileChunkSize){
                //checking size of last chunk
                if( ((chunkStartIndex + 1)*4096) > filesize){
                    numBytes = fileSizes[u] - start;
                }
            }
            chunkStartIndex++;     
            //--Mutual Exclusion-- Adding to task queue
            pthread_mutex_lock(&taskMutex);
            queueAddTask(mappedFiles[u], numBytes, start, fileChunksArr[u]);
            totalNumTasks += 1;
            pthread_cond_signal(&taskSignal);
            pthread_mutex_unlock(&taskMutex);
        } // end chunk for loop 

    }
    //free(encodedData);
    //fflush(stdout);
    
    pthread_mutex_lock(&completeMutex);
    //there are currently no tasks 
    while (completed != totalChunks){
        pthread_cond_wait(&taskComplete, &completeMutex);
    }
    //unlock since we required nec info 
    pthread_mutex_unlock(&completeMutex);  
    //printf("%s\n","-------out of complete lock------"); 
    //printf("%s%d\n","completed so far: ",completed); 
    //printf("%s%d\n","total chunks: ",totalChunks); 
    
    int displayed = 0;  
    //int checked = 0;
    unsigned char * lastTwoChars;
    lastTwoChars = (unsigned char *)calloc(2, sizeof(unsigned char));
    //unsigned char firstChar;
    //int previousCount;
    //unt = 0;
    bool checkStitch = false; //default
    //int canCheck = 0;
    int inc = 0;
    //int lastCount = 0;
    task_t * taskResults;
    taskResults = malloc(sizeof(task_t *)); 
    //int taskResults;
    //printf("%s%d\n","---completed all tasks--", completed);
    
    if( (completed == totalChunks) && totalChunks == 1){
        fwrite(encodedData, sizeof(unsigned char),totalOutputBytes, stdout);
        //displayed++;
        return 0;
    }
    
    //printf("%s%d\n","**total num of chunks** ", totalChunks);
    //printf("%s%d\n","**total num of tasks** ", totalNumTasks);
    while( (completed == totalChunks) && resultsHead != NULL) {
      
        pthread_mutex_lock(&completeMutex);
        taskResults = queueRemoveTask();
        //unlock since we required nec info 
        pthread_mutex_unlock(&completeMutex);  
        //printf("%s\n","unlocked completed task mutex!");        
        // we have the thread... just display
        if(taskResults!= NULL){
            //you know theres more than one
            //everything but the count      
            /*  
            printf("%s\n","**Grabbed Task/Chunk** ");
            printf("%s%d\n","starting index: ", taskResults->startIndex);
            printf("%s%d\n","ending index: ", taskResults->endIndex);
            printf("%s%d\n","output: ", taskResults->outputSize);
            printf("%s%d\n","numbytes: ", taskResults->bytes);
            printf("%s\n","**Done w chunk info** ");
            */
            if(displayed == 0){
                //printf("%s\n","Displaying First Chunk");
                lastTwoChars[0] = encodedData[(taskResults->endIndex)]; //count
                lastTwoChars[1] = encodedData[(taskResults->endIndex) - 1]; //char
                fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)-1, stdout);
                encodedData+= ((taskResults->bytes));   //we will be at the start index of the next chunk        
                displayed++;
                continue;
            }else if ((displayed+1 == totalChunks) ){
                //very last chunk
                checkStitch = *(encodedData) == lastTwoChars[1]; // this char vs last char
                if(checkStitch){
                    lastTwoChars[0] = *(encodedData + 1) + lastTwoChars[0]; //char count + prev count
    
                    //printf("%s\n","stitching...displaying count output: ");     
                    fwrite(lastTwoChars, sizeof(unsigned char), 1, stdout);
                    encodedData+=3; //account for this char, and this char count so now we are in next char
                    //printf("%s\n","displaying last chunk output: ");
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)-3, stdout);        
                    displayed++;
                    continue;
                }else{         
                    //dont stitch, print count as is, is last chunk 
                    //printf("%s\n","displaying last chunk count output: ");
                    fwrite(lastTwoChars, sizeof(unsigned char), 1, stdout); //displayed count, no need to increment    
                    //printf("%s\n","displaying all output: ");
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize), stdout);
                    displayed++;
                    continue;
                }
   
            }else{
                //not the first chunk so check, two chars. current vs previous char
                checkStitch = *(encodedData) == lastTwoChars[1];
                if(checkStitch){  
                    lastTwoChars[0] = *(encodedData+1) + lastTwoChars[0]; 
                    //printf("%s\n","stitched! displaying count output: ");
                    fwrite(lastTwoChars, sizeof(unsigned char), 1, stdout);
                    encodedData+=3; //account for this char, this char count, now we are in next char
                    //printf("%s\n","displaying output after stitch: ");
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize) - 4, stdout); //we start off in the next char and want to print out everything but the last count
                    lastTwoChars[0] = encodedData[(taskResults->endIndex)]; //count
                    lastTwoChars[1] = encodedData[(taskResults->endIndex) - 1]; //char
                    encodedData+= ((taskResults->bytes) - 3);  
                    //encodedData+=taskResults->bytes;
                    displayed++;
                    continue;

                }else{
                    //dont stitch
                    //printf("%s\n","no stitch.. displaying output: ");
                    fwrite(lastTwoChars, sizeof(unsigned char), 1, stdout);
                    //printf("%s\n","displaying output: ");
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize) - 1, stdout);
                    lastTwoChars[0] = encodedData[(taskResults->endIndex)]; //count
                    lastTwoChars[1] = encodedData[(taskResults->endIndex) - 1]; //char
                    //9..... but you need to be at 4110 so you need to  increment 4101
                    encodedData+= ((taskResults->bytes));  
                    displayed++;
                    continue;
                }
        }

        } //end if taskResults != NULL                
    } //end while  
    return 0;
}