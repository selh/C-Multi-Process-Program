#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>
#include "multiproc.h"
#include "decrypt.h"


/*Print out the current time*/
int OUTPUT_TIME(char* print_time){

    time_t current_time;
    time(&current_time);
    strftime(print_time, 100, "%a %b %d %T %Y",localtime(&current_time));
    return 0;
}

//writes a series of messages to the child
int WRITE_FILES(int child,int fd[NUM_CHILDREN][2],char* read_file_name,char* write_file_name){
  int lenread=0, lenwrite =0;

  lenread= (int)strlen(read_file_name)+1; //write the length of each filename first
  lenwrite= (int)strlen(write_file_name)+1;

  write(fd[child][1],&lenread, sizeof(lenread));
  write(fd[child][1],&lenwrite, sizeof(lenwrite));

  write(fd[child][1],read_file_name, strlen(read_file_name)+1); //send file names to child
  write(fd[child][1],write_file_name, strlen(write_file_name)+1);

}
