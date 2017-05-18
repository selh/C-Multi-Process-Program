#define NUM_CHILDREN sysconf(_SC_NPROCESSORS_ONLN)-1

#ifndef _MULTIPROC_H_
#define _MULTIPROC_H_
//#include "decrypt.h"

int OUTPUT_TIME(char* print_time);
int WRITE_FILES(int child,int fd[NUM_CHILDREN][2],char* read_file_name,char* write_file_name);

#endif
