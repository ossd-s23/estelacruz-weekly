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
    char *startPointer; 
    //unsigned char *encodedResults;
    int startIndex; 
    int bytes; 
    int complete; //0 if not comp, 1 if it is comp
    unsigned char *lastChars;
    int outputSize;
    int lastCharCount;
    int numberofChunks;
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
    //newTask -> encodedResults = (unsigned char*)calloc(size*2, sizeof(unsigned char));
    newTask -> bytes = size; 
    newTask -> complete = 0;
    newTask -> next = NULL;
    newTask->lastCharCount = 0;
    newTask->numberofChunks = chunks;
    

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

int rle(task_t * currentTask){
    
    //Task Info
    int chunkSize = currentTask -> bytes;
    int count = 0;
    int index = currentTask->startIndex;
    int writeIndex = currentTask->startIndex;
    char * fileContent = currentTask->startPointer;
    
    //unsigned char * rleResults = currentTask->encodedResults;
    int storeResults = 0;
    int lastCount = 0;
    unsigned char mychar;
    
    //printf("%s%d\n","starting writing index: ", storeResults); 
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
        //printf("%s%c\n","writing char: ", mychar); 
        encodedData[writeIndex] = mychar;
        writeIndex++;
        //currentTask->encodedResults[storeResults] = mychar;
        storeResults = storeResults + 1;
        //printf("%s%d\n","writing count after incr: ", storeResults); 
        //printf("%s%d\n","writing count: ", currentcharCount); 
        encodedData[writeIndex] = unsignedcount;
        writeIndex++;
        storeResults = storeResults + 1;
        //printf("%s%d\n","writing count after incr: ", storeResults); 
        //currentTask->encodedResults[storeResults] = unsignedcount;  
        //printf("%s\n","------------end----------------");   
        index++;
        count += 1;
      
    } //end while
    //signal that thread is done? Can multiple threads safely do this?
    currentTask->complete = 1; 
    currentTask->lastCharCount = lastCount;
    //printf("%s%d\n","how many times we wrote: ", storeResults);  
    currentTask->outputSize = storeResults;
    // have to lock to signal
    // have to lock to signal
    //printf("%s\n","completed rle!");  
    pthread_mutex_lock(&completeMutex);
    totalOutputBytes += currentTask->outputSize;
    completed += 1;
    if(completed == totalNumTasks){
        pthread_cond_signal(&taskComplete);
    }
    //pthread_cond_signal(&taskComplete);
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
            //it releases the lock, if we didnt then no other thread can grab the lock
            pthread_cond_wait(&taskSignal, &taskMutex);
        }

        //unlock since we required nec info    
        pthread_mutex_unlock(&taskMutex);
    
        if(getTask != NULL){
            //we have to encode file
            //printf("%s\n","sending to rle!");
            int checkingdone = rle(getTask);
            /*
            if(checkingdone == completed){
                break;
            }
            */
            

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
    //printf("%s%d\n", "threads: ", numThreads);
    //printf("%s%d\n", "starting argument Index ", argumentIndex);



    //----------Initialize Thread Pool---------------
    pthread_t threadPool[numThreads]; 
    int t;
    for(t = 0; t<numThreads;t++){
        pthread_create(&threadPool[t], NULL, threadFunction, NULL);
    }
    

    // pthread_t mainThread; 
   // pthread_create(&mainThread, NULL, displayResults, NULL);
   
   // ---Parse Command Line --- getopt rearranges argv

   //----File Info-----
   int fd; //file descriptor
   struct stat sb; //File Size
   //array with all file pointers, each block will hold the ptr that mmap returns
   //char * fileMemoryPointers[numFiles]; 
   int totalChunks = 0;
   //int taskIndex = 0; // for queue 
   //int totalOutputBytes = 0;
   int totalInputBytes = 0;
   //printf("%s%d\n","argument count ARGC ", argc); 
   int numFiles = argc-argumentIndex; 
   //printf("%s%d\n","Number of files: ", numFiles); 
   //----Accessing Filenames---
  
   for (argumentIndex; argumentIndex<argc; argumentIndex++) {
        // ------Files: Virtual Memory Mapping---------------
        //open binary text file, read only, open returns int
        //printf("%s%s\n","filename: ", argv[argumentIndex]);
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
            //printf("%s%d\n","Num bytes sent to task: ", numBytes);     
            queueAddTask(currentFilePtr, numBytes, start, numChunks);
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
    // printf("%s%d\n","total chunks: ",totalChunks); 
    int displayed = 0;  
    int checked = 0;
    //printf("%s%d\n","---completed all tasks--", completed);
    if(completed == totalChunks){
        if(totalNumTasks==1){ 
            //small? should not be a case
            fwrite(encodedData, sizeof(unsigned char), totalOutputBytes, stdout);   
        }else{
        
            unsigned char thisChar;
            unsigned char thisCount;
        
            unsigned char nextChar;
            unsigned char nextCount;
            //printf("%s%c%s\n", "see what is at index 4: '", *(encodedData + 3), "'");
        
            int p = 0;
            bool checkStitch = false;
            int displayed = 0;
            // 6 ... 0 4 6 
            while((checked + 2) < totalOutputBytes){                   
                //dont want to be out of bounds
                //printf("%s%d\n","numbytes = ", totalOutputBytes);        
                //printf("%s%d\n", "start of iteration. p = ", p);
                p = 0;
                thisChar = *(encodedData + p);
            
                //printf("%s%c%s\n", "this char: '", *(encodedData + p), "'");
                //printf("%s%c%s\n", "this char with array index: '",*(encodedData + p), "'");
                //printf("%s%c%s\n", "deferencing array char: '", *encodedData, "'");
                p++; //this count      
                thisCount = *(encodedData + p);
                
                p++; //next char    
                nextChar = *(encodedData + p);
            
                //printf("%s%c%s\n", "next char: '", *(encodedData + p), "'"); 
                //printf("%s%c%s\n", "next char with array index: '", *(encodedData + p), "'");
                //printf("%s%c%s\n", "next char with k: '", nextkChar, "'");      
                
                
                p++;//next count
                nextCount = *(encodedData + p); //p is at count
            
                //next will be char p is current at next char so at 4
                checkStitch = *(encodedData + 0) == *(encodedData + 2);
                if(checkStitch){
                    //
                    //fix out output
                    //printf("%s\n","Stitch! Displaying output:");
                    *(encodedData + 1) = *(encodedData + 1) + *(encodedData + p);
                    //only prints if valid
                    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                    //printf("\n");
                    displayed+=2;
                    
                    //p++; //next char 
                    //printf("%s\n","--incremented memory because we stitched!--");
                    //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                    encodedData+=4;
                    checked+=4; //accounted for 4 bytes             
                    //printf("%s%c%s\n", "after fwrite increment by 2: deferencing array char: '", *encodedData, "'");
                    //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                    //encodedData+=2; //increment memory 
                    //printf("%s%d\n\n", "end of iteration. p = ", p);
                    //encodedData++; //inc memory address
                    
                }else{               
                    //
                    //dont have to stitch
                    //printf("%s\n","NO stitch! Displaying Output:");
                    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                    //printf("\n");
            
                    encodedData+=2; //move to next
                    displayed+=1; //only printed one
                    checked += 2; //checked 2 bytes
                    
                    //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                    //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                    //p++; //next char 
                    //encodedData+=4; 
                    //printf("%s\n","Displayed output");
                    //printf("%s%d\n\n", "end of iteration. p = ", p);
                } //end else
                //

                if((checked+4) == totalOutputBytes){
                    //last chunk
                    //printf("%s\n", "last chunk");
                    //have next char alr
                    p = 0;
                    thisChar = *(encodedData + p);
                    //printf("%s%c%s\n", "this char: '", thisChar, "'");
                    p++; //this count
                    p++; //next char
                    nextChar = *(encodedData + p);
                    p++; //next count
                    //printf("%s%c%s\n", "next char: '", nextChar, "'"); 
                    checkStitch = *(encodedData + 0) == *(encodedData + 2);
                    if(checkStitch){
                        //fix out output
                        //printf("%s\n","Stitch! Displaying output:");
                        *(encodedData + 1) = *(encodedData + 1) + *(encodedData + 3);
                        //only prints if valid
                        fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                        //printf("\n");
                        displayed+=2;
                        p++; //next char 
                        //printf("%s\n","--incremented memory because we stitched!--");
                        //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                        encodedData+=4;
                        checked+=4; //accounted for 4 bytes             
                        //printf("%s%c%s\n", "after fwrite increment by 2: deferencing array char: '", *encodedData, "'");
                        //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                        //encodedData+=2; //increment memory 
                        //printf("%s%d\n\n", "end of iteration. p = ", p);
                        //encodedData++; //inc memory address
                        
                    }else{

                        //dont have to stitch
                        //printf("%s\n","NO stitch! Displaying All Output Anyways:");
                        fwrite(encodedData, sizeof(unsigned char), 4, stdout);
                        //printf("\n");
                        displayed+=2;
                        encodedData+=4; //move to next
                        checked += 4; //checked 4 bytes

                        //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                        //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                        p++; //next char 
                        //encodedData+=4; 
                        //printf("%s\n","Displayed output");
                        //printf("%s%d\n\n", "end of iteration. p = ", p);
                    } //end else
                } //end last chunk
                        
                } //endwhile



        }

        
        
    } //end if comp == totalchunks
    
    

   
    return 0;

}





