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
    unsigned char * sha1; 
    //disk is just a file so we can open using mmap 
    struct BootEntry *disk; //data struct to store the boot entry
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
                    printf("%s%d\n", "option count is not 0 when it should be(cmd i): ", optionCount);
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
                    printf("%s%d\n", "option count is not 0 when it should be(cmd l): ", optionCount);
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
                    printf("%s%d\n", "option count is not 0 when it should be(cmd r): ", optionCount);
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 3;
                //This command requires option argument Filename, store info 
                fileName = (unsigned char *) optarg;
                printf("%s%s\n", "filename: ",fileName);
                optionCount = 1; //inc count
                break;
            case 'R':
                //Command R: Recover a possibly non-contiguous file
                //validiate cmd 
                if(optionCount != 0){ //can only perform one option argument 
                    printf("%s%d\n", "option count is not 0 when it should be(cmd R): ", optionCount);
                    userOption = 5; //print usage info and exit
                    break;
                }
                //Store userOption
                userOption = 4;
                //This command requires option argument Filename, store info 
                fileName = (unsigned char *) optarg;
                printf("%s%s\n", "filename: ", fileName);
                optionCount = 1; //inc count
                break;
            case 's':
                //Command s: recover file with SHA-1
                //when recovered with sha1, 
                //print filename: successfully recovered with SHA-1
                //validiate cmd 
                if(optionCount != 1){
                    //error , s cant be an option argument on its own 
                    printf("%s%d\n", "option count is not 1 when it should be(cmd s): ", optionCount);
                    userOption = 5; //print usage info and exit
                    break;
                }
                //option count is 1, now validate that user 
                //previously entered option arg r or R 
                if(userOption != 3 && userOption != 4){
                    printf("%s%d\n", "user option is not 3 or 4 when it should be(cmd s): ", userOption);
                    userOption = 5; //print usage info and exit
                    break;
                }
                //option count is 1, and useroption is either 3 or 4
                //sha1 is option argument 
                sha1 = (unsigned char *) optarg;
                printf("%s%s\n", "sha: ", sha1);
                givenSha = 1;
                break;
        }
        //end switch
    }
    //end while loop
    printf("%s%s\n", "next argv element: ", argv[optind]);



    return 0;


}