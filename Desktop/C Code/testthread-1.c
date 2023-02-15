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

void * estelasturn(void * arg) {
  printf("%s\n", "---------- start estelas thread-----------");
  int i;
  for(i = 0 ; i < 8; i++){
    printf("\t\t%s\n", "going to sleep for 1 sec");
    sleep(1);
    printf("\t\t%s\n", "waited 1 seconds");
    printf("\t%s%d\n", "Estela's Turn i = ", i);

  }
  printf("%s\n", "--------end estelas thread---------");
  return NULL;

}


void karlasturn(){
  printf("%s\n", "----------- start karlas MAIN thread------------");
  int k;
  for(k = 0 ; k < 3; k++){
    printf("\t\t%s\n", "going to sleep for 2 sec");
    sleep(2);
    printf("\t\t%s\n", "waited 2 seconds");
    printf("%s%d\n", "Turno de Karla! k = ", k);
  }
  printf("%s\n", "----------- end karlas MAIN thread------------");

}

int main(int argc, char  **argv){

  pthread_t newthread;

  pthread_create(&newthread, NULL, estelasturn, NULL); // if i call join right after it will only call estelas turn           
  // karlasturn();                                                                                                            
  //so main is always checking if this is done and not moving on to another function                                          
  // printf("%s\n", "---------right before join-------"); // doesnt print                                                     
  pthread_join(newthread, NULL); // will run estelas again or make sure its finished                                          
  printf("%s\n", "!! done with both threads!! "); //does print                                                                
  return 0;


}



