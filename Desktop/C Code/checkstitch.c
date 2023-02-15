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
    int argumentIndex = 2; //2 if gdb, 1 if prog
    int checkJobs = getopt(argc, argv, "j");
    
    if(checkJobs == -1){
        //there are no jobs, have to run sequentially
        // r /.nyuenc fileone                                                                                                              
        numThreads = 1;

    }else{
        numThreads =  atoi(argv[optind]);
        // r ./nyuenc -r 3 filename
        argumentIndex = 4; //4 if gdb, 3 if prog
        }
    //printf("%s%d\n", "threads: ", numThreads);
    //printf("%s%d\n", "starting argument Index ", argumentIndex);

    //for the file encoder you have to do the following 
    

    //----------Initialize Thread Pool---------------
    pthread_t threadPool[numThreads]; 
    int t;
    for(t = 0; t<numThreads;t++){
        pthread_create(&threadPool[t], NULL, threadFunction, NULL);
    }
    // main p thread of the file 
    // the main thread is at the end of the file 

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
        
        // the total number of chunks is the following 
        //
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
    //the file encoder has to finish and send a signal 
    
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
    //final stitch is to make sure that we stitch correctly 
    // final section 
    printf("%s\n","-------out of complete lock------"); 
    printf("%s%d\n","completed so far: ",completed); 
    //total number of chunks 
    
    printf("%s%d\n","total chunks: ",totalChunks); 
    int displayed = 0;  
    printf("%s%d\n","---completed all tasks--", completed);
    //total number of chunks is
    if(completed == totalChunks){
        bool checkStitch = false; //default
        //iterate through
        // if it is only one just print out the whole thing is that
        // the following was misunderstood
        //the following
        if(totalChunks == 1){
            printf("%s%d\n","one chunk. numbytes = ", totalOutputBytes);
            fwrite(encodedData, sizeof(unsigned char), totalOutputBytes, stdout);
        }
        if(totalChunks != 1){
            // 2 chars at a time...
            // a3 is the file encoder 

            printf("%s%d\n","more than one chunk. numbytes = ", totalOutputBytes);
            printf("%s%d\n","num chunks = ", totalNumTasks);
            //the number of chunks is pre determined the number of chu
            // the total number of chunks at the end can be predetermined
            // that is the thing the number of  FAT 32 disks can be rewritten
            int number_of_total_chunks = 0;
            number_of_total_chunks += 15;
            // the number of total chunks is incremented every time
            // the number of chunks is 
            int p = 0;
            int checked = 0;
            
            //output bytes = 12? 0-11   0 2 4 6 8 10 STOPS AT 10
            
            if((checked+2) == totalOutputBytes){ //small? should not be a case
                //printf("%s%d\n","More than one chunk. numbytes = ", totalOutputBytes);
                //printf("%s%d\n","num chunks = ", totalNumTasks);
                fwrite(encodedData, sizeof(unsigned char), totalOutputBytes, stdout);
            }
            unsigned char thisChar;
            unsigned char thisCount;
            unsigned char thiskChar;
            //we wanna see with this char is that you need to see where is 
            //maybe you can resend where the end byte is 
            // resend where the next char is 
            //maybe you end up where th

            unsigned char nextChar;
            unsigned char nextCount;
            unsigned char nextkChar;
            int k;
            while((checked + 2) < totalOutputBytes){ //dont want to be out of bounds
                //printf("%s%d\n","numbytes = ", totalOutputBytes);
                k = 0;
                //printf("%s%d\n", "start of iteration. p = ", p);
                thisChar = encodedData[p];
                thiskChar = encodedData[k];
                //printf("%s%c%s\n", "this char: '", thisChar, "'");
                //printf("%s%c%s\n", "this char with k: '", thiskChar, "'");
                //printf("%s%c%s\n", "deferencing array char: '", *encodedData, "'");
                p++; //this count
                k++;
                thisCount = encodedData[p];
                
                p++; //next char
                k++;
                nextChar = encodedData[p]; 
                nextkChar = encodedData[k];
                //printf("%s%c%s\n", "next char: '", nextChar, "'"); 
                //printf("%s%c%s\n", "next char with k: '", nextkChar, "'");      
                
                
                p++;//next count
                k++;
                nextCount = encodedData[p]; //p is at count
            
                //next will be char p is current at next char so at 4
                checkStitch = thisChar == nextChar;
                if(checkStitch){
                    //fix out output
                    //printf("%s\n","Stitch! Displaying output:");
                    encodedData[p-2] = encodedData[p-2] + encodedData[p];
                    //only prints if valid
                    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                    checked+=4;
                    p++; //next char 
                    //printf("%s\n","--incremented memory because we stitched!--");
                    //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                    encodedData+2;             
                    //printf("%s%c%s\n", "after fwrite increment by 2: deferencing array char: '", *encodedData, "'");
                    //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                    //encodedData+=2; //increment memory 
                    //printf("%s%d\n\n", "end of iteration. p = ", p);
                    //encodedData++; //inc memory address
                    


                }else{
                    //dont have to stitch
                    //printf("%s\n","NO stitch! Displaying Output:");
                    fwrite(encodedData, sizeof(unsigned char), 4, stdout);
                    checked += 4;
                    encodedData+=4;
                    /*
                    ++encodedData;
                    ++encodedData;
                    ++encodedData;
                    ++encodedData;
                    */
                    //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                    //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                    p++; //next char 
                    //encodedData+=4; 
                    //printf("%s\n","Displayed output");
                    //printf("%s%d\n\n", "end of iteration. p = ", p);
                }
                
            }

            
        } //end if chunks != 1    
        
    } //end if 
    
    
    
    while( (completed == totalChunks) && displayed < totalChunks) {

        //printf("%s%d\n","displayed: ", displayed);
        unsigned char * lastTwoChars;
        lastTwoChars = (unsigned char *)calloc(2, sizeof(unsigned char));
        int previousCount;
        int checkChunk;
        bool checkStitch = false; //default
        int canCheck = 0;
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        task_t * taskResults =  malloc(sizeof(task_t *));
        
        pthread_mutex_lock(&completeMutex);
        taskResults = queueRemoveTask();
        //unlock since we required nec info 
        pthread_mutex_unlock(&completeMutex);  
        //printf("%s\n","unlocked completed task mutex!");        
        // we have the thread... just display
        if(taskResults!= NULL){
            if(totalChunks == 1){
                //print whole
                fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize), stdout);
                displayed++;
            }
            //more than one chunk

            if(displayed = 0){
                //first time, check size
                if ((taskResults->outputSize) <= 2 ){

                }
            }


     
        } //end if taskResults != NULL
                
     
    } //end while
   */
    return 0;

}



/* 
    int displayed = 0;  
    printf("%s%d\n","---completed all tasks--", completed);

    while( (completed == totalChunks) && displayed < totalChunks) {

        //printf("%s%d\n","displayed: ", displayed);
        unsigned char * lastTwoChars;
        lastTwoChars = (unsigned char *)calloc(2, sizeof(unsigned char));
        int previousCount;
        int checkChunk;
        bool checkStitch = false; //default
        int canCheck = 0;
        //thread will try to grab a task: have to lock bc what if multiple threads try to grab a task?        
        //quiero saber porque me lo haces
        task_t * taskResults =  malloc(sizeof(task_t *));
        // the one in the file sustem is the one at the end
        pthread_mutex_lock(&completeMutex);
        taskResults = queueRemoveTask();
        //unlock since we required nec info 
        pthread_mutex_unlock(&completeMutex);  
        //printf("%s\n","unlocked completed task mutex!");        
        // we have the thread... just display
        if(taskResults!= NULL){
            //printf("%s%d\n","---we are going to print--", displayed);
            if(totalChunks == 1){
                //print the whole thing bc we only have one
                fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize), stdout);
                displayed++;
            }else if ((taskResults->outputSize) <= 2 ){
                //
                if(displayed + 1 == totalChunks){
                    //in the last chunk
                    if(canCheck == 1){

                        //prev stored data
                        unsigned char lastCharOne = taskResults->lastChars[0];
                        checkStitch = lastCharOne == lastTwoChars[0];  
                        //if you have to stitch
                        if(checkStitch){
                    
                            //update 
                            int newcount = previousCount + taskResults->lastCharCount;
                            unsigned char updatedC = newcount;
                            encodedData[(taskResults->startIndex)-1] = updatedC;
                            //encodedData[(taskResults->startIndex)+1] = updatedC;
                            fwrite(encodedData, sizeof(unsigned char),1, stdout);
                            //increment address address encoded data points to? check if this works
                            encodedData++;
                            lastTwoChars[1] = updatedC;

                        }else{
                            fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)+1, stdout);
                            encodedData++;
                        }
                        //dont have to stitch, have to account for the count
                        fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize), stdout);
                        displayed++;
     
                    } //end can check
                } // not the last chunk
                // 4 for gdb, 2 for our prog bc we cant do output - 2 

                fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)-1, stdout); //only print char
                //account for next char
                //save last char info if there is another chunk 
                previousCount = taskResults ->lastCharCount;
                lastTwoChars[0] = taskResults->lastChars[0];
                lastTwoChars[1] = taskResults->lastChars[1];
                displayed++;
                canCheck = 1;           
            }else{ //we can check for stitching bc output size allows us to
                //make sure this isnt last chunk
                if(canCheck == 1){

                    //check if this is last one
                    if(displayed + 1 == totalChunks){
                        //in the last chunk, can check 
                        //prev stored data
                        unsigned char lastCharOne = taskResults->lastChars[0];
                        checkStitch = lastCharOne == lastTwoChars[0];  
                        //if you have to stitch
                        if(checkStitch){                    
                            //print out count
                            int newcount = previousCount + taskResults->lastCharCount;
                            unsigned char updatedC = newcount;
                            encodedData[(taskResults->startIndex)-1] = updatedC;
                            //encodedData[(taskResults->startIndex)+1] = updatedC;
                            fwrite(encodedData, sizeof(unsigned char),1, stdout);
                            //increment address address encoded data points to? check if this works
                            encodedData++; //for count
                            encodedData++; //for char
                            encodedData++; //for char count
                            fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize), stdout);
                            lastTwoChars[1] = updatedC;

                        }else{
                            //dont have to stitch
                            fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)+1, stdout);
                            displayed++;
                        }
                        //last one, dont have to stitch
                        
                                                    
                        

        
                        
                    } // not the last chunk, big enough, we can check

                    //prev stored data            
                    unsigned char lastCharOne = taskResults->lastChars[0];
                    checkStitch = lastCharOne == lastTwoChars[0];  
                    //if you have to stitch
                    if(checkStitch){
                        //update 
                        int newcount = previousCount + taskResults->lastCharCount;
                        unsigned char updatedC = newcount;
                        encodedData[(taskResults->startIndex)-1] = updatedC;
                        //encodedData[(taskResults->startIndex)+1] = updatedC;
                        fwrite(encodedData, sizeof(unsigned char),1, stdout);
                        //increment address address encoded data points to? check if this works
                        encodedData++;
                        lastTwoChars[1] = updatedC;
                    }

                    //fwrite(lastTwoChars, sizeof(unsigned char), 2, stdout);
                    //increment address address encoded data points to? check if this works
                    //encodedData++;
                    //encodedData++;
                    //now partiall print out

                    //not stitching, only -2 bc we arent printing last count
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)-1, stdout);
                    displayed++;
                    //fwrite(&encodedData[taskResults->startIndex], sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
    
                }else{
                    //havent prev checked... so print partial, update info 
                    //big enough to print
                    fwrite(encodedData, sizeof(unsigned char),(taskResults->outputSize)-1, stdout); //stops at count
                    previousCount = taskResults ->lastCharCount;
                    lastTwoChars[0] = taskResults->lastChars[0];
                    lastTwoChars[1] = taskResults->lastChars[1];
                    displayed++; // will exit if this was last chunk
                    //but what if this is last chunk?
                    canCheck = 1;
                }
 
            }
                        
            //printf("%s%d\n","displayed = ", displayed);
        } //end if taskResults != NULL
                
     
    } //end while
*/

//the number 