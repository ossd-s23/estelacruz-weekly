
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


unsigned char * encodedData; 

int main(int argc, char  **argv){
    encodedData = encodedData = (unsigned char *)calloc(7, sizeof (unsigned char));
    //0 1 2 3 4 5 6
    
    encodedData[0] = 'a';
    encodedData[1] = '5';
    encodedData[2] = 'b';
    encodedData[3] = '3';
    encodedData[4] = 'b';
    encodedData[5] = '2';
    printf("%s\n", "printing out first..skipping second b3'");
    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
    encodedData+=4;
    printf("%s%c%s\n", "encoData deferences to: '", *(encodedData), "'");
    printf("%s%c%s\n", "Previous encoData deferences to: '", *(encodedData-1), "'");
    printf("%s\n", "printing out remaining 2'");
    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
    printf("\n");
    /*
    printf("%s%c%s\n", "char is '", encodedData[3], "'");
    printf("%s%c%s\n", "char is '", encodedData[5], "'");
    encodedData[5] = encodedData[3] + encodedData[5];
    printf("%s%c%s\n", "char is '", encodedData[5], "'");
    */
   /*
    int fileSize = 20480;
    int numChunks = fileSize / 4096;    
    if( (numChunks * 4096) < fileSize){
        //account for one more chunk 
        numChunks += 1;
    }
     printf("%s%d\n", "Total Num Chunks: '", numChunks);

        

    int chunkStartIndex;
    int numBytes = 0;
    for(chunkStartIndex = 0; chunkStartIndex < numChunks; chunkStartIndex++){
        int start = chunkStartIndex * 4096;
        numBytes = 4096;
        printf("%s%d\n", "chunk ID: '", chunkStartIndex);
        printf("%s%d\n", "chunk start index '", start);
        if((chunkStartIndex+1) == numChunks){
            //checking size of last chunk
            if( ((chunkStartIndex + 1)*4096) > fileSize){
                numBytes = fileSize - start;
                }
        }
        printf("%s%d\n", "num bytes: '", numBytes);
    }
   
   */
    /*
    int checked = 0;
    int totalOutputBytes = 6;
    int totalChunks = 3;
    if((checked+2) == totalOutputBytes){ 
        //small? should not be a case
        //printf("%s%d\n","More than one chunk. numbytes = ", totalOutputBytes);
        //printf("%s%d\n","num chunks = ", totalNumTasks);
        fwrite(encodedData, sizeof(unsigned char), totalOutputBytes, stdout);
    }

    unsigned char thisChar;
    unsigned char thisCount;
    
    unsigned char nextChar;
    unsigned char nextCount;
    
    int p = 0;
    bool checkStitch = false;
    int displayed = 0;
    // 6 ... 0 4 6 
    while((checked + 2) < totalOutputBytes){ 
        //dont want to be out of bounds
        //printf("%s%d\n","numbytes = ", totalOutputBytes);

       
        //printf("%s%d\n", "start of iteration. p = ", p);
        thisChar = *(encodedData + p);
       
        printf("%s%c%s\n", "this char: '", thisChar, "'");
        //printf("%s%c%s\n", "this char with array index: '",*(encodedData + p), "'");
        //printf("%s%c%s\n", "deferencing array char: '", *encodedData, "'");
        p++; //this count      
        thisCount = *(encodedData + p);
        
        p++; //next char    
        nextChar = *(encodedData + p);
       
        printf("%s%c%s\n", "next char: '", nextChar, "'"); 
        //printf("%s%c%s\n", "next char with array index: '", *(encodedData + p), "'");
        //printf("%s%c%s\n", "next char with k: '", nextkChar, "'");      
        
        
        p++;//next count
        nextCount = *(encodedData + p); //p is at count
    
        //next will be char p is current at next char so at 4
        checkStitch = thisChar == nextChar;
        if(checkStitch){
            //
            //fix out output
            printf("%s\n","Stitch! Displaying output:");
            *(encodedData + 1) = *(encodedData + 1) + *(encodedData + p);
            //only prints if valid
            fwrite(encodedData, sizeof(unsigned char), 2, stdout);
            printf("\n");
            displayed+=2;
            p = 0;
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
                printf("%s\n","NO stitch! Displaying Output:");
                fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                printf("\n");
        
                encodedData+=2; //move to next
                displayed+=1; //only printed one
                p = 0;
                checked += 2; //checked 2 bytes
                
                //printf("%s%c%s\n", "after fwrite: deferencing array char: '", *encodedData, "'");
                //printf("%s%c%s\n", "after fwrite: deferencing with index array char: '", encodedData[0], "'");
                //p++; //next char 
                //encodedData+=4; 
                //printf("%s\n","Displayed output");
                //printf("%s%d\n\n", "end of iteration. p = ", p);
            } //end else
            if((checked+4) == totalOutputBytes){
                //last chunk
                printf("%s\n", "last chunk");
                //have next char alr
                p = 0;
                thisChar = *(encodedData + p);
                printf("%s%c%s\n", "this char: '", thisChar, "'");
                p++; //this count
                p++; //next char
                nextChar = *(encodedData + p);
                p++; //next count
                printf("%s%c%s\n", "next char: '", nextChar, "'"); 
                checkStitch = thisChar == nextChar;
                if(checkStitch){
                    //fix out output
                    printf("%s\n","Stitch! Displaying output:");
                    *(encodedData + 1) = *(encodedData + 1) + *(encodedData + p);
                    //only prints if valid
                    fwrite(encodedData, sizeof(unsigned char), 2, stdout);
                    printf("\n");
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
                    printf("%s\n","NO stitch! Displaying All Output Anyways:");
                    fwrite(encodedData, sizeof(unsigned char), 4, stdout);
                    printf("\n");
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
        }
                
        } //endwhile
    */

            
        
        

    return 0;
}