
#ifndef _ARGMANIP_H_
#define _ARGMANIP_H_
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h> 

// citations have been referenced at the end 

char **manipulate_args(int argc, const char *const *argv, int (*const manip)(int)){
// first, allocate memory to whole array                                                                              
// each block is a char ptr                                                                                           

  int totalBlocks = argc + 1; //include null ptr                                                                      
  char **copyargv = (char **)calloc(totalBlocks, totalBlocks * sizeof(char*));

// calloc initializes value by default to 0, we have to initialize it                                                 
//go and allocate memory to each cell                                                                                 


// null pointer is set to zero by calloc, initalize to null after                                                     
  for(int i = 0; i<argc; i++){

    // allocate this amount of memory to this char ptr. Note, argv[i] = *(argv + i)                                   
    int totalChars = strlen(*(argv + i))+ 1; // include null char                                                     
    copyargv[i] = (char *)calloc(totalChars, totalChars * sizeof(char));

    // After initializing this memory allocation,  we can copy string char by char, including null char               
    for(int j = 0; j<totalChars; j++){

      // send jth char  to function, manip, to get upper or lower                                                     
      copyargv[i][j] = manip(argv[i][j]);
    } //copied whole string                                                                                           


   ;
}
  //initialize null ptr                                                                                               
  copyargv[totalBlocks] = NULL;                                                                     
  return copyargv;
  
}


void free_copied_args(char **args, ...){
  

  //The free_copied_args() function takes a variable number of arguments, 
  //each of which is a “copied” argument list returned by manipulate_args()

  //define list of arguments that were sent as argument to this function
  //holds the information needed by va_start, va_arg, va_end, and va_copy
  va_list args_sent;

  //begin going through arguments sent, va start (list of args, last argument)
  //last arg  is the last known fixed argument being passed to our func
  va_start(args_sent, args);
  // we will stop until we reach last argument NULL
  while(args != NULL){
    // we know that args first argument is our array of char ptrs 
    // iterate through this array of character ptrs and free them until we reach last block null
    for(char *const *charptr = args; *charptr != NULL; ++charptr){
      free(*charptr);
    }
    // free the whole array
    free(args);


    //va_arg accesses the next variadic function argument, so memory allocated for upper and lower case 
    args = va_arg(args_sent, char **);

  }

  // we reached the end
  va_end(args_sent);
}

#endif


/*
CITATIONS 


1.) https://www.geeksforgeeks.org/command-line-arguments-in-c-cpp/

Since I have limited experience with C, I wanted to make sure I understood  what was going on in the main program 
This website clarified how the main prog takes in the parameters argc and argv and how argv is an array of char ptrs in our program


2.) https://opensource.com/article/19/5/how-write-good-c-main-function

Gave me further understanding of the main program, argv, argc, to solidfy again, what I did not know about the argument counter and argument vector

3.) https://en.cppreference.com/w/c/memory/malloc
Gave me a better understanding of what malloc does and how it allocates memory but does not initialize it

4.) https://blog.akshayraichur.com/how-malloc-works-in-c

5.) https://www.geeksforgeeks.org/difference-between-malloc-and-calloc-with-examples/
Informed me on the difference between malloc and calloc; malloc does not initialize allocated memory where calloc initializes it to 0, 
and both functions take different parameters

6.) https://www.log2base2.com/C/pointer/malloc-vs-calloc.html

7.) https://www.geeksforgeeks.org/dynamic-memory-allocation-in-c-using-malloc-calloc-free-and-realloc/#:~:text=The%20%E2%80%9Cmalloc%E2%80%9D%20or%20%E2%80%9Cmemory,a%20pointer%20of%20any%20form.

8.)  https://byjus.com/gate/difference-between-static-and-dynamic-memory-allocation-in-c/#:~:text=In%20static%20memory%20allocation%2C%20while,the%20memory%20can%20be%20changed.&text=Static%20memory%20allocation%20is%20preferred,preferred%20in%20the%20linked%20list.

Cites 3-8 gave me further explainations on malloc and pushed me to come to question what is the difference between static and dynamic memory allocation
and essentially why c needs a function like malloc at all. Since I had not previously really coded in C, I had all of these questions. 
This website along other ones cited helped me understand that C allows us to dynamically allocate memory during runtime, whereas static memory
allocation is done during compilation, so static memory allocation cannot be changed and should be for variables that are essential to my program. Also 
It also informed me how imperative it is that I free dynamically allocated memory. I also learned dynamic memory allocation can be changed and
how it is stored in the heap whereas static memory is stored in the stack 


9.) https://www.cs.nmsu.edu/~rth/cs/cs271/notes/Pointers.html
Provided me with examples on how pointers work and how to dereference. These examples help me understand what even is an array of character pointers 
and how I can access not only the whole string, but how to access it character by character

10.) https://twiserandom.com/c/sizeof-operator-in-c/#:~:text=The%20sizeof%20operator%2C%20returns%20the,and%20its%20unit%20is%20byte.

Explained the Size of operator so that I could use it to give me the length of the string I was trying to allocate memory for in manipulate_args function

11.) https://www.geeksforgeeks.org/conditional-or-ternary-operator-in-c-c/
Helped me understand how a conditional statement can work as an if-else statement and how I can use it to my advantage (first encountered in textbook example)

12.) https://www.geeksforgeeks.org/variadic-functions-in-c/
Helped me understand what a variadic function was, how a functions parameters can vary and how we
can access all of these arguments with the use of va_list, va_start, va_arg,va_end

This website also provided examples with how each method worked  like how va_list initializes a list with all arguments
and how the second parameter represents the last KNOWN fixed argument that was passed to our function

*/
