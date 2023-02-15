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



struct node {
    struct node* next;  
    unsigned char *startPointer; 
    unsigned char *encodedResults;
    int startIndex; 
    int bytes; 
    int complete; //0 if not comp, 1 if it is comp
    unsigned char *lastChars;
    int outputSize;
    int lastCharCount;
};
typedef struct node task_t;


//gloabl vars for queue linked list
static task_t *head = NULL;
static task_t *tail = NULL;


static task_t *resultsHead = NULL;
static task_t *resultsTail = NULL;


static int taskIndex = 0;
//static unsigned char * encodedData;



pthread_mutex_t completeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskComplete = PTHREAD_COND_INITIALIZER;



pthread_mutex_t taskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t taskSignal = PTHREAD_COND_INITIALIZER;





void queueAddTask (unsigned char *content, int size, int starting){
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
    
    if(head == NULL){
        //add to beg
        head = newTask;
    }else{
        //traverse
        task_t * temp = head;
        while(temp->next != NULL){temp = temp->next;}
        temp->next = newTask;
    }
    // do the same to results queue
    if(resultsHead == NULL){
        //add to beg
        resultsHead = newTask;
    }else{
        //traverse
        task_t * temp2 = resultsHead;
        while(temp2->next != NULL){temp2 = temp2->next;}
        temp2->next = newTask;
    }


}


//returns task node if there is one
//FIFO queue
task_t * queueGetTask(){
    //also removes task from task queue
    // Returns null if the queue is empty
    if(head == NULL){
        //No more tasks
        return NULL;
    }else{
        //collect char ptr
        task_t *returnTask = head;
        //have to update our queue and remove node
        //task_t *temp = head;
        //move to next node
        head = head->next;
        //free previously malloced memory
        //free(temp);
        return returnTask;
    }
}

//returns char * output if there is a task in queue
//FIFO queue
task_t * queueRemoveTask(){
    // Returns null if the queue is empty
    if(resultsHead== NULL){
        //No more tasks
        return NULL;
    }else{
        //collect task
        task_t *returnTask = resultsHead;
        //have to update our queue and remove node
        //task_t *temp = resultsHead;
        //move to next node
        resultsHead = resultsHead->next;
        //free previously malloced memory
        //free(temp);
        return returnTask;
    }
}


void rle(task_t * currentTask){
    //Task Info

    int fileByteSize = currentTask -> bytes;
    int count = 0;
    int index = currentTask->startIndex;
    int writeIndex = currentTask->startIndex;
    unsigned char * fileContent = currentTask->startPointer;
    
    //unsigned char * rleResults = currentTask->encodedResults;
    int storeResults = 0;
    int lastCount = 0;
    //index <  start+size
    // 5 + 10, 14
    while(count < fileByteSize){   //count range: 0, filebytesize - 1, so we have according nec indices
        // each byte as data will be stored as an unsigned char
        unsigned char mychar = fileContent[index];  
        //check next char 
        bool checkNext = false;
        if((index+1) < ((currentTask->startIndex)+fileByteSize)){
            //valid index
            checkNext = fileContent[index] == fileContent[index + 1];   
        }
        //keep track how many chars there are in a row
        count += 1;
        int currentcharCount = 1; 
        while (checkNext == true) {   
            //account for next char since they are equal
            currentcharCount += 1;
            //update count to next byte
            count += 1;
            //update index 
            index += 1;
            //repeateadly check for next position 

            if((index+1) < ((currentTask->startIndex)+fileByteSize)){
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
         //encodedData[writeIndex] = mychar;
         currentTask->encodedResults[storeResults] = mychar;
         //rleResults[storeResults] = mychar;
         storeResults++;
         //writeIndex++; // using encoded arr
         //add unsigned count to encoded array
         currentTask->encodedResults[storeResults] = unsignedcount;
         //encodedData[writeIndex] = unsignedcount;
         //rleResults[storeResults] = unsignedcount;
         //increment index
         //writeIndex++;      
         storeResults++;    
        //count this byte, move onto next
         //count += 1;   
         index++;
    }

    // use results to track
    //store last char
    //result outputsize 
    currentTask->outputSize = storeResults;
    //currentTask->lastChar[0] = encodedData[storeResults-1];
    //currentTask->lastChar[1] = encodedData[storeResults];
    currentTask->lastChars[1] = currentTask->encodedResults[storeResults-2];
    currentTask->lastChars[1] = currentTask->encodedResults[storeResults-1];
    //currentTask->lastChar[0] = rleResults[storeResults-2];
    //currentTask->lastChar[1] = rleResults[storeResults-1];
    currentTask->lastCharCount = lastCount;

    //signal that thread is done? Can multiple threads safely do this?
    currentTask->complete = 1; 
    // have to lock to signal
    pthread_mutex_lock(&completeMutex);
    pthread_cond_signal(&taskComplete);
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
   if(argc<2){
    exit(0);
   }
   //encodedData = ( unsigned char*)calloc(1073741824, sizeof (unsigned char));
   
   // ---Parse Command Line --- getopt rearranges argv
  
   int argumentIndex; //argv index, will be used to access filenames
   //getopt vars
   extern char *optarg; 


    //----------Initialize Thread Pool---------------
    pthread_t mainThread; 
    mainThread = (pthread_t) malloc (sizeof(pthread_t));
    pthread_create(&mainThread, NULL, threadFunction, NULL);
   


   //----File Info-----
   int fd; //file descriptor
   struct stat sb; //File Size
   //array with all file pointers, each block will hold the ptr that mmap returns
   //char * fileMemoryPointers[numFiles]; 
   int totalChunks = 0;
   //int taskIndex = 0; // for queue 
   int totalInputBytes = 0;
   //int fileInd = 0;

   //----Accessing Filenames---
   //Want my algorithm to open a file, read it, split it into 4KB Chunks, create tasks, push to task queue
   
   for (argumentIndex = 1; argumentIndex<argc; argumentIndex++) {

    // ------Files: Virtual Memory Mapping---------------
    //open binary text file, read only, open returns int
    fd = open(argv[argumentIndex], O_RDONLY);  //fd is -1 if it fails
    //File Size
    fstat(fd, &sb);
    //virtually map data, mmap returns char * 
    unsigned char * currentFilePtr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    // Splitting File by 4KB Chunks
    int fileSize = (int)sb.st_size;
    close(fd);


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
     
    int displayed = 0; 
    
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
                fwrite(&(taskResults->encodedResults[taskResults->startIndex]), sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
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
                fwrite(&(taskResults->encodedResults[taskResults->startIndex]), sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                //fwrite(&encodedData[taskResults->startIndex], sizeof(unsigned char),(taskResults->outputSize)-2, stdout);
                displayed++;   
                //update
                lastTwoChars[0] = taskResults->lastChars[0];
                lastTwoChars[1] = taskResults->lastChars[1];  
             }           
               
        } //end if 
        
    } //end while
    //free(encodedData);
    //fflush(stdout);
    return 0;

}

