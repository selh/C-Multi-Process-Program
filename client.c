#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //socket
#include <sys/types.h> //socket
#include <netinet/in.h> //for the sockaddr_in struct
#include <netdb.h> // getaddrinfo

#include <netinet/in.h> //inet_addr
#include <arpa/inet.h>
#include <string.h> // memset
#include <unistd.h>//write
#include <sys/wait.h>//waitpid
#include <sys/select.h>
#include "multiproc.h"
#include "decrypt.h"
#include "memwatch.h"

#define NUM_CHILDREN sysconf(_SC_NPROCESSORS_ONLN)-1
//#define NumConnections 20 //number of connections allowed at one time

//messages: "OK" = 1 -finished w/o problem, "BAD" = 2  -could not open file
//messages from server: 0 -telling client to exit, 1- telling it to continue
main( int argc, char **argv){


  int fd[NUM_CHILDREN][2], fdchild[NUM_CHILDREN][2]; //parent writes to fd, child writes to fdchild
  int count=NUM_CHILDREN, status, i; //variables for zombie collection

  char print_time[100];
  int sockfdclient;
  struct sockaddr_in sockinfo;
  //memset(&sockinfo, 0, sizeof(sockinfo));

  int numbytes, readlen=0, writelen=0; //number of bytes read from the socket
  char readfilename[1025];
  char writefilename[1025];
  int toserver =1, fromserver;
  unsigned long process_id = getpid();
  unsigned long child_process[NUM_CHILDREN];
  unsigned long child_id;
  int pid;

  char trackfiles[NUM_CHILDREN][1025];
  //char* decrypt_file;
  //int file_len;
  int childmsg, fromchild, child=0;
  int j=0, flag =0, counter=0;
  int check;

  const int SUCCESS = 1; // for checking the return msg on read_child
  const int FAIL = 2;
  const int sending_file= 400; //for checking the actual msg from the child
  const int sending_fail= 300;
  //const int toserver = 1;
  char get_file_name[1025];
  int file_len=0;
  unsigned long tmp;

  ConnectionSetup(argv[1], argv[2], &sockinfo, &sockfdclient, process_id);
  //DO THE PIPES
  for(i=0; i< NUM_CHILDREN; i++){ //CREATE PIPES AND CHILDREN

    if (pipe(fd[i]) == -1){
      OUTPUT_TIME(print_time);
      printf("[%s] ERROR:Failed to pipe. Process ID #%ld Exiting.\n", print_time,process_id);
      exit(EXIT_FAILURE);
    }
    if (pipe(fdchild[i]) == -1){
      OUTPUT_TIME(print_time);
      printf("[%s] ERROR:Failed to pipe. Process ID #%ld Exiting.\n", print_time,process_id);
      exit(EXIT_FAILURE);
    }
    //FORK THE CHILD PROCESSES
    if( (process_id=fork()) == -1){
      OUTPUT_TIME(print_time);
      printf("[%s] ERROR:Failed to create child processes. Process ID #%ld Exiting.\n", print_time,process_id);
      exit(EXIT_FAILURE);
    }
    else if ( process_id == 0){
      close(fd[i][1]); //close the write end for the parents pipe
      close(fdchild[i][0]);//close the read end of child pipe
      child_id = getpid();

      write(fdchild[i][1], &child_id, sizeof(child_id)); //send the parent a copy of the process id
      RUNCHILD(i, fd, fdchild);
    }
    else{
      close(fd[i][0]); //close read end of parent pipe
      close(fdchild[i][1]); //close write end of child pipe
      read(fdchild[i][0], &child_id, sizeof(child_id)); //read the process id number
      child_process[i] = child_id; //record into tracking table for communication with server later
    }
  } //END OF FOR LOOP

  while(1){

    //toserver=1; // number 1 means client is ready
    send(sockfdclient, &toserver, sizeof(toserver), 0);
    read(sockfdclient, &fromserver, sizeof(fromserver));
    if( fromserver == 0 ){
      break;
    }

    if( (numbytes = recv(sockfdclient, &readlen, sizeof(readlen),0) ) == -1 ){
      printf("error occurred could not recv message\n");
      exit(EXIT_FAILURE);
    }if( (numbytes = recv(sockfdclient, &writelen, sizeof(writelen), 0) ) == -1 ){
      printf("error occurred could not recv message\n");
      exit(EXIT_FAILURE);
    }if( (numbytes = recv(sockfdclient, readfilename, readlen*sizeof(char), 0) ) == -1 ){
      printf("error occurred could not recv message\n");
      exit(EXIT_FAILURE);
    }if( (numbytes = recv(sockfdclient, writefilename, writelen*sizeof(char), 0) ) == -1 ){
      printf("error occurred could not recv message\n");
      exit(EXIT_FAILURE);
    }
    //printf("recieved from server: %s len: %d %s len: %d\n",
    //readfilename , readlen, writefilename, writelen);

    //FCFS(readfilename, writefilename, fd, fdchild, &sockfdclient, flag);
    ROUND_ROBIN(readfilename,writefilename,fd,fdchild,&counter,&child,&sockfdclient,trackfiles,child_process);
    memset(readfilename, 0, sizeof(readfilename));
    memset(writefilename, 0, sizeof(writefilename));
  }//END OF WHILE(1) LOOP


  while( counter > 0){ //READ REMAINING MESSAGES FROM CHILDREN DOING DECRYPTION
    check = READ_CHILD_MSG(fdchild, trackfiles, &counter, &sockfdclient, &pid);
    tmp = child_process[pid];

    if( check == SUCCESS){
      strcpy(get_file_name ,trackfiles[pid]);
      file_len = strlen(get_file_name)+1;
      //printf("sending file with name: %s and len:%d\n", trackfiles[pid], file_len );
      send(sockfdclient, &sending_file, sizeof(sending_file), 0);
      send(sockfdclient, &file_len, sizeof(file_len), 0);
      send(sockfdclient, get_file_name, file_len*sizeof(char), 0);
      send(sockfdclient, &tmp, sizeof(unsigned long),0);
    }
    else if( check == FAIL ){
      strcpy(get_file_name ,trackfiles[pid]);
      file_len = strlen(get_file_name)+1;
      //printf("sending file with name: %s and len:%d\n", trackfiles[pid], file_len );
      send(sockfdclient, &sending_fail, sizeof(sending_fail), 0);
      send(sockfdclient, &file_len, sizeof(file_len), 0);
      send(sockfdclient, get_file_name, file_len*sizeof(char), 0);
      send(sockfdclient, &tmp, sizeof(unsigned long),0);
    }
  }

  for(j=0; j< NUM_CHILDREN; j++){ //CLOSE WRITE PIPES
    close(fd[j][1]);
    //printf("PARENT closed pipe for child %d\n", j);
  }

  child= NUM_CHILDREN-1;
  while( child >= 0 ){ //CHECK FOR CLOSED PIPE
    if( read(fdchild[child][0], &fromchild, sizeof(fromchild)) == 0){
        close(fdchild[child][0]);
        child--;
        //printf("from child: %d\n", fromchild );
    }
  }
  OUTPUT_TIME(print_time); //print to terminal
  printf("[%s] lyrebird.client: PID %ld completed its tasks and is exiting successfully.\n",
          print_time, process_id);
  close(sockfdclient);

}


/*CALLED FROM PARENT Round Robin, distribute file to each child like a deck of cards
  WRITES INTO THE fd PIPE
  RUN BY PARENT PROCESS */
int ROUND_ROBIN(char* read_file_name, char* write_file_name, int fd[NUM_CHILDREN][2],
                int fdchild[NUM_CHILDREN][2], int* counter, int* child, int*sockfdclient,
                char trackfiles[NUM_CHILDREN][1025],  unsigned long child_process[NUM_CHILDREN]){

  int SIGFIN = 1; //signal child to exit
  int NOTFIN = 0; //signal child proceed
  const int SUCCESS = 1;
  const int FAIL = 2;
  const int sending_file= 400;
  const int sending_fail= 300;
  int fromchild, j=0;
  char get_file_name[1025];
  int file_len=0;
  unsigned long tmp =0;

  read(fdchild[*child][0], &fromchild, sizeof(int)); //start signal will be 3, run normally
  if( fromchild == SUCCESS ){
    strcpy(get_file_name ,trackfiles[*child]);
    file_len = strlen(get_file_name)+1;
    tmp = child_process[*child];
    send(*sockfdclient, &sending_file, sizeof(sending_file), 0);
    send(*sockfdclient, &file_len, sizeof(file_len), 0);
    send(*sockfdclient, get_file_name, file_len*sizeof(char), 0);
    //printf("sending file with name: %s and len:%d\n", trackfiles[*child], file_len );
    send(*sockfdclient, &tmp, sizeof(unsigned long),0);
    //printf("sending process id of: %ld\n", tmp );
    //printf("SUCCESS FROM RR\n");
    (*counter)--;
  }
  else if ( fromchild == FAIL ){
    strcpy(get_file_name ,trackfiles[*child]);
    file_len = strlen(get_file_name)+1;
    tmp = child_process[*child];
    send(*sockfdclient, &sending_fail, sizeof(sending_fail), 0);
    send(*sockfdclient, &file_len, sizeof(file_len), 0);
    send(*sockfdclient, get_file_name, file_len*sizeof(char), 0);
    //printf("sending file with name: %s and len:%d\n", trackfiles[*child], file_len );
    send(*sockfdclient, &tmp, sizeof(unsigned long),0);
    //printf("FAILURE\n");
    (*counter)--;
  }
  write(fd[*child][1], &NOTFIN, sizeof(NOTFIN));
  WRITE_FILES(*child,fd,read_file_name,write_file_name);
  strcpy(trackfiles[*child], read_file_name);

  (*child)++;
  (*counter)++;

  if( *child == NUM_CHILDREN){ //CHECKS IF DONE FIRST RUN, RESETS CHILD EACH TIME
     *child=0;
  }
  return 0;
}


/*Checks for a message on the pipe and then reads the pipe
if the message is a sucess or fail message decrement the counter
returns "SUCCESS =1" on success and "FAIL=2" on failure */
int READ_CHILD_MSG(int fdchild[NUM_CHILDREN][2], char trackfiles[NUM_CHILDREN][1025],
           int* counter, int* sockfdclient, int* pid){

  fd_set readfds;
  struct timeval timeout;
  int max, val, fromchild, j;

  const int SUCCESS = 1;
  const int FAIL = 2;
  //const int sending_file= 400;
  //const int sending_fail= 300;
  char get_file_name[1025];
  int file_len=0;
  //select code///
  FD_ZERO(&readfds);
  max= fdchild[0][0];
  timeout.tv_sec = 0;
  timeout.tv_usec = 10000;

  for(j=0; j< NUM_CHILDREN; j++){
    FD_SET(fdchild[j][0], &readfds);
    if(max < fdchild[j][0]){
        max = fdchild[j][0];
    }
  }//END OF FOR LOOP
  select(max+1, &readfds, NULL, NULL, &timeout);
  for(j=0; j< NUM_CHILDREN; j++){
    if(FD_ISSET(fdchild[j][0], &readfds)){
      val = read(fdchild[j][0], &fromchild, sizeof(fromchild)); //read finish signal from the child
      *pid = j;
      if( fromchild == SUCCESS ){
           (*counter)--;
        return SUCCESS;
        }
        else if ( fromchild == FAIL){
          (*counter)--;
          return FAIL;
       }
    }
  }//END OF FOR LOOP
}

/*  THIS IS RUN ONLY BY CHILD PROCESSES -- Called immediately after each fork()
  Read the file names from pipe and pass to start_decrypt function
  will write 1 to pipe upon successful decryption*/
int RUNCHILD(int child, int fd[NUM_CHILDREN][2], int fdchild[NUM_CHILDREN][2]){

  char print_time[100];
  char read_file_name[1025];
  char write_file_name[1025];
  int lenread=0,lenwrite=0; //tracks the length of the file to be read
  int finish=0, done=1, fail=2;//message passing variables
  int wait =0, check=1;//tracks the state
  int start = 3;
  //printf("              I AM CHILD:%ld #%d\n", , child );

  while (1){
    write(fdchild[child][1], &start, sizeof(start));

    if( wait == 0 ){
      //printf("CHILD %d IS WAITING.....\n", child);
      if( read(fd[child][0], &finish, sizeof(finish)) == 0){
        //printf("  CHILD %d recieved closed pipe signal\n", child);
        close(fd[child][0]);
        close(fdchild[child][1]);
        exit(EXIT_SUCCESS);
      }
    }
    read(fd[child][0], &lenread, sizeof(lenread));
    read(fd[child][0], &lenwrite, sizeof(lenwrite));
    read(fd[child][0], read_file_name, sizeof(char)*lenread);
    read(fd[child][0], write_file_name, sizeof(char)*lenwrite);
    check = START_DECRYPT(read_file_name, write_file_name);

    if( check == 1){
      write(fdchild[child][1], &fail, sizeof(fail));
    }
    else { //otherwise tell parent you are done with no errors!
      write(fdchild[child][1], &done, sizeof(done));
    }
    //reset all variables
    memset(read_file_name,0, 1024);
    memset(write_file_name,0, 1024);
    lenread=0;
    lenwrite=0; //wait for reading length
    wait=0; //wait for signal

  } //END OF WHILE TRUE LOOP
  return 0;
}//END OF FUNCTION


//return 1 on failure, 0 on success, this is used by the child process to start decryption
int START_DECRYPT(char* read_file_name, char* write_file_name){

  FILE* encrypt_file = NULL;
  FILE* decrypt_file = NULL;
  char print_time[100];
  char* decryptedtweet;
  char tweet[300]; //max length an arbitrary 300

//printf("FROM CHILD readfilename: %s\nwritefilename: %s\n", read_file_name, write_file_name);
//IF NULL POINTER AT FILE LOCATION DO NOTHING
  if( (encrypt_file = fopen(read_file_name, "r")) == NULL ||
      (decrypt_file = fopen(write_file_name, "w")) == NULL ){
    return 1;
  }
  else{

   while( fgets(tweet, sizeof(tweet) , encrypt_file) != NULL ){
      decryptedtweet = decrypt(tweet);
      fputs(decryptedtweet, decrypt_file);
      fputs("\n", decrypt_file);
      free(decryptedtweet);
    }
    fputs("\0", decrypt_file);
    //ONLY CLOSE FILES IF THEY WERE OPENED
    fclose(decrypt_file);
    fclose(encrypt_file);
  }

  return 0;
}


int ConnectionSetup(char* IP, char* Port, struct sockaddr_in* sockinfo,
          int* sockfdclient, unsigned long process_id){

  char print_time[100];
  sockinfo->sin_family= AF_INET;
  sockinfo->sin_addr.s_addr = inet_addr(IP);
  sockinfo->sin_port = htons(atoi(Port));
  printf("IP addr:%s, PORT:%s\n", IP, Port);

  *sockfdclient = socket(sockinfo->sin_family, SOCK_STREAM, 0);
  if( *sockfdclient == -1 ){
    OUTPUT_TIME(print_time);
    printf("[%s] lyrebird.client: PID %ld failed to create socket to %s, port %d\n", print_time,
           process_id,inet_ntoa(sockinfo->sin_addr), htons(sockinfo->sin_port));
    exit(EXIT_FAILURE); //failed to create socket
  }

  if( connect(*sockfdclient, (struct sockaddr*)sockinfo, sizeof(struct sockaddr_in)) != 0){
    OUTPUT_TIME(print_time);
    printf("[%s] lyrebird.client: PID %ld failed to connect to %s, port %d\n", print_time,
    process_id,inet_ntoa(sockinfo->sin_addr), htons(sockinfo->sin_port));
    close(*sockfdclient);
    exit(EXIT_FAILURE); //failed to connect
  }

  OUTPUT_TIME(print_time); //print to terminal
  printf("[%s] lyrebird.client: PID %ld connected to server %s, port %d\n",
  print_time, process_id,
  inet_ntoa(sockinfo->sin_addr), htons(sockinfo->sin_port));
  return 0;
}

/*


struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};
struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

*/
