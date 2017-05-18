#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> //socket
#include <sys/types.h> //socket
#include <netinet/in.h> //for the sockaddr_in struct
#include <netdb.h> // getaddrinfo
#include <string.h> // memset
#include <arpa/inet.h> //inet_ntoa
#include <ifaddrs.h> //getifaddrs
#include <unistd.h>//write
#include "memwatch.h"
//FOR ACCEPT - RETURNS POINTER TO NEW FILE DESCRIPTOR TO COMMUNICATE WITH CLIENT
//OLD ONE LISTENS FOR MORE CONNECTIONS;
#define NumConnections 20 //maximum number of allowed connections at any given time --sets array lengths

main( int argc, char **argv){

  char print_time[50];
  int sockfd, returnfd[NumConnections];
  struct sockaddr_in serveraddr;
  struct sockaddr_in clientaddr[NumConnections];
  //struct ifaddrs *ifap, *iter; //for IP address resolution
  socklen_t clientlen = sizeof(struct sockaddr_in);
  int j=0, num_connected = 0, client=0; //loop variables
  int fromclient =0; //message from client 1-done, 2-could not open file
  char readfilename[1024];
  char writefilename[1024];
  int readlen, writelen;
  int SIGFIN=0, NOTFIN =1;
  int check, count=0;
  int returnmsg; //used for checking how socket was closed
  unsigned long process_id = getpid();

  //const int INCOMING_DONE = 400;
  //const int INCOMING_FAIL = 300;

  FILE* configfile = fopen( argv[1], "r");
  FILE* logfile = fopen( argv[2], "w");

  if( configfile == NULL ){
    OUTPUT_TIME(print_time);
    printf("[%s] Failed to open %s. PID %ld Exiting.\n",
          print_time, argv[1], process_id);
    exit(EXIT_FAILURE);
  }
  if( logfile == NULL ){
    OUTPUT_TIME(print_time);
    printf("[%s] Failed to open %s. PID %ld Exiting.\n",
        print_time, argv[2], process_id);
    exit(EXIT_FAILURE);
  }

  GETIP_PORT(&serveraddr, &sockfd );//set up ip port and structs

  if( (returnfd[0] = accept(sockfd, (struct sockaddr *)&clientaddr[0], &clientlen ) ) == -1 ){
      //failed to accept
  }else{
    num_connected++;
    OUTPUT_TIME(print_time);
    fprintf( logfile, "[%s] Successfully connected to lyrebird client %s\n",
      print_time,inet_ntoa(clientaddr[0].sin_addr));
  }
  // if( AcceptClient(&sockfd,returnfd,clientaddr,&num_connected) == 0 ){
  //     OUTPUT_TIME(print_time);
  //     fprintf( logfile, "[%s] Successfully connected to lyrebird client %s\n",
  //         print_time,inet_ntoa(clientaddr[num_connected-1].sin_addr));
  // }

  client =0;
  while(fscanf(configfile, "%s %s", readfilename, writefilename) == 2){

    client = FindReadyClient(num_connected, returnfd, logfile, clientaddr, &count); //find the next available client
    send(returnfd[client], &NOTFIN, sizeof(NOTFIN), 0);

    readlen = strlen(readfilename)+1; //so client will add null byte to the end
    writelen = strlen(writefilename)+1;
    //printf("AT SEVER: readname:%s, writename: %s\n", readfilename, writefilename);
    send(returnfd[client], &readlen, sizeof(readlen),0);
    send(returnfd[client], &writelen, sizeof(writelen),0);
    send(returnfd[client], readfilename, readlen*sizeof(char),0);
    send(returnfd[client], writefilename, writelen*sizeof(char),0);
    count++;

    OUTPUT_TIME(print_time);
    fprintf( logfile, "[%s] The lyrebird client %s has been given the task of decrypting %s.\n",
       print_time,inet_ntoa(clientaddr[client].sin_addr), readfilename);


    if( AcceptClient(&sockfd,returnfd,clientaddr,&num_connected) == 0 ){
      OUTPUT_TIME(print_time);
      fprintf( logfile, "[%s] Successfully connected to lyrebird client %s\n",
          print_time,inet_ntoa(clientaddr[num_connected-1].sin_addr));
    }
    //recv(returnfd[client], &fromclient, sizeof(fromclient), 0);
    //printf("recieved following message from the client: %d\n", fromclient );
  }


  for(j=0; j< num_connected; j++){
    send(returnfd[j], &SIGFIN, sizeof(SIGFIN), 0);
  }

  while( count > 0){
    //printf("test for reading last messages\n");
    FindReadyClient(num_connected, returnfd, logfile, &clientaddr, &count);
  }

  for(j=0; j< num_connected; j++){
    //printf("test in check for close\n");
    returnmsg = recv( returnfd[j], &check, sizeof(check), 0);
    if( returnmsg == 0){
      OUTPUT_TIME(print_time);
        fprintf( logfile, "[%s] lyrebird client %s has disconnected expectedly.\n",
             print_time,inet_ntoa(clientaddr[j].sin_addr));
    }
    else if( returnmsg == -1 ){
      OUTPUT_TIME(print_time);
        fprintf( logfile, "[%s] lyrebird client %s has disconnected unexpectedly.\n",
             print_time,inet_ntoa(clientaddr[j].sin_addr));
    }
  }

  fclose(configfile);
  fclose(logfile);
  close(sockfd);
  OUTPUT_TIME(print_time);
  printf("[%s] lyrebird server: PID %ld completed its task and is exiting successfully.\n",print_time, process_id);
  exit(EXIT_SUCCESS);

}


/*Print out the current time*/
int OUTPUT_TIME(char* print_time){

    time_t current_time;
    time(&current_time);
    strftime(print_time, 100, "%a %b %d %T %Y", localtime(&current_time));
    fflush(stdout);
    return 0;
}

/* retrieves the ip address and port number that server is running on
  also does listen */
int GETIP_PORT(struct sockaddr_in* serveraddr, int* sockfd ){

  char print_time[100];
  struct ifaddrs *ifap, *iter; //for IP address resolution
  int val;

  val = SETUP_SOCKET(serveraddr, sockfd);
  if( val == 1 ){
    OUTPUT_TIME(print_time);
    printf("[%s] Failed to get socket. PID %ld Exiting.\n", print_time,(unsigned long)getpid());
    freeifaddrs(ifap);
    return 0;
  }
  /*FIND WHICH PORT YOU ARE CONNECTED TO*/
  socklen_t len = sizeof(struct sockaddr_in);
  getsockname(*sockfd, (struct sockaddr *)serveraddr, &len);

  /*GET AN IP ADDRESS*/
  getifaddrs(&ifap);
  for( iter= ifap; iter != NULL; iter=iter->ifa_next){ //GET THE LAST AVAILABLE IP ADDRESS
    if(iter->ifa_addr->sa_family == AF_INET){
      serveraddr->sin_addr= ((struct sockaddr_in *)iter->ifa_addr)->sin_addr;
    }
  }

  OUTPUT_TIME(print_time);
  printf("[%s] lyrebird.server: PID %ld on host %s, port %d\n",
    print_time, (unsigned long)getpid(), inet_ntoa(serveraddr->sin_addr), htons(serveraddr->sin_port));
  /* LISTEN FOR CONNECTIONS */
  listen(*sockfd, NumConnections);
  freeifaddrs(ifap);
  return 1;
}

/*does getaddrinfo then creates the socket and binds
returns 0 on success, 1 on failure*/
int SETUP_SOCKET(struct sockaddr_in* serveraddr, int* sockfd ){

  struct addrinfo hints; //gets filled out by getaddrinfo later
  struct addrinfo *result, *rp; //for getaddrinfo
  //struct ifaddrs *ifap, *iter; //for IP address resolution
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; //
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  serveraddr->sin_family=AF_INET;
  memset(serveraddr->sin_zero, 0, sizeof(serveraddr->sin_zero));

  /*GETADDRINFO */
  if( getaddrinfo(NULL, "0", &hints, &result) != 0 ){
    //failed
    //fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    printf("exited with error at getaddrinfo\n");
    exit(EXIT_FAILURE);
  }
  for(rp=result; rp != NULL; rp= rp->ai_next){ // set rp as pointer to results and iterate through
    if( (*sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol)) < 0){
      //error
      printf("ERROR: could not create socket\n");
      freeaddrinfo(result);
      return 1;
    }
    if( bind(*sockfd, rp->ai_addr, rp->ai_addrlen) == -1 ){
      //error
      printf("ERROR: could not bind socket\n");
      freeaddrinfo(result);
      close(*sockfd);
      return 1;// free the struct
    }
    break;
  }
  if( rp == NULL ){ //could not make any connections
    printf("ERROR: could not find socket\n");
    close(*sockfd);
    freeaddrinfo(result); // free the struct
    return 1;
  }


  freeaddrinfo(result); // free the struct
  return 0;
}

/* does a non-block accept for a new client, returns 0 when a new client is connected,
returns 1 when no new clients are found. Should pass array to track which clients are online*/
int AcceptClient(int* sockfd, int returnfd[NumConnections],
         struct sockaddr_in clientaddr[NumConnections],int* num_connected){

  fd_set readsocket;
  struct timeval timeout;
  int max, client=0, count=0;
  socklen_t clientlen = sizeof(struct sockaddr_in);
  struct sockaddr* tmp = (struct sockaddr*)clientaddr;

  FD_ZERO(&readsocket);
  max = *sockfd;
  timeout.tv_sec =0;
  timeout.tv_usec = 10000;
  FD_SET(*sockfd, &readsocket);
  select(*sockfd+1, &readsocket, NULL,NULL, &timeout);


  if(FD_ISSET(*sockfd, &readsocket)){
    returnfd[*num_connected]= accept(*sockfd, &tmp[*num_connected],&clientlen);
    (*num_connected)++;
    return 0;
  }


  return 1;
}

/* returns the number associated with the first available client
Uses select() to implement a FCFS algorithm for fetching the client
will not return until atleast one client is found
Will ready for success and failure messages from the client over the socket
and will write to the log file accordingly*/
int FindReadyClient(int num_connected, int returnfd[NumConnections], FILE* logfile,
          struct sockaddr_in* clientaddr[NumConnections], int* counter){

  char print_time[100];
  fd_set readsocket;
  struct timeval timeout;
  int max, client=0, count=0, j=0, msg=0;

  FD_ZERO(&readsocket);
  max = returnfd[num_connected];
  timeout.tv_sec =0;
  timeout.tv_usec = 10000;

  const int INCOMING_DONE = 400;
  const int INCOMING_FAIL = 300;
  int file_len =0;
  char getfilename[1025]={0};
  unsigned long process_id;

  int len = 20;
  char IP_str[len];
  struct sockaddr_in* tmp = (struct sockaddr_in*)clientaddr;

  int check;

  while ( count == 0){ //waits until atleast one client is ready
    for(j=0; j< num_connected; j++){
      FD_SET(returnfd[j], &readsocket);
      if( max < returnfd[j]){
        max = returnfd[j];
      }
    }
    select(max+1, &readsocket, NULL,NULL, &timeout);

    for(j=0; j< num_connected; j++){
      if(FD_ISSET(returnfd[j], &readsocket)){
        check = recv(returnfd[j], &msg, sizeof(msg),0);
        inet_ntop(AF_INET, &(tmp[j].sin_addr), IP_str, len );
        //if( check > 0  ){

          if( msg == INCOMING_DONE ){

            recv(returnfd[j], &file_len, sizeof(file_len),0);
            recv(returnfd[j], &getfilename, file_len*sizeof(char), 0);
            recv(returnfd[j], &process_id, sizeof(process_id),0);
            OUTPUT_TIME(print_time);
                fprintf( logfile, "[%s] lyrebird client %s has successfully decrypted %s in process %ld.\n",
                 print_time, IP_str, getfilename, process_id);
            (*counter)--;

          }
          else if( msg == INCOMING_FAIL ){

            recv(returnfd[j], &file_len, sizeof(file_len),0);
            recv(returnfd[j], &getfilename, file_len*sizeof(char), 0);
            recv(returnfd[j], &process_id, sizeof(process_id),0);
            OUTPUT_TIME(print_time);
                fprintf( logfile, "[%s] lyrebird client %s has encountered an error: Unable to open %s in process %ld.\n",
                 print_time, IP_str, getfilename, process_id);
            (*counter)--;
          }

          client = j;
          count++;
          break;
        //}else if(check == -1){
          // OUTPUT_TIME(print_time);
         //    fprintf( logfile, "[%s] lyrebird client %s has disconnected unexpectedly.\n",
          //        print_time, IP_str);

        //}
      } //END OF IF_ISSET
    }//END OF FOR LOOP
  }//END OF WHILE LOOP
  return client;
}


/*
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
    int              ai_protocol;  // use 0 for "any"
    size_t           ai_addrlen;   // size of ai_addr in bytes
    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
    char            *ai_canonname; // full canonical hostname

    struct addrinfo *ai_next;      // linked list, next node
};
struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};
struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};
struct in_addr {
    unsigned long s_addr;  // load with inet_aton()
};

int getaddrinfo(const char *node,     // e.g. "www.example.com" or IP
                const char *service,  // e.g. "http" or port number
                const struct addrinfo *hints,
                struct addrinfo **res);
struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };

struct ifaddrs {
           //     struct ifaddrs  *ifa_next;    Next item in list
           //     char            *ifa_name;    Name of interface
           //     unsigned int     ifa_flags;   Flags from SIOCGIFFLAGS
           //     struct sockaddr *ifa_addr;    Address of interface
           //     struct sockaddr *ifa_netmask; Netmask of interface
           //     union {
           //         struct sockaddr *ifu_broadaddr;
           //                          Broadcast address of interface
           //         struct sockaddr *ifu_dstaddr;
           //                          Point-to-point destination address
           //     } ifa_ifu;
           // //#define              ifa_broadaddr ifa_ifu.ifu_broadaddr
           // //#define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
           //     void            *ifa_data;    Address-specific data
           // };

*/
