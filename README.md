## Assignment #4 README

### Assignment Overview:
For assignment 4, a client-server architechture was implemented in order to make use of CPU cores on different computers. The goal is to improve overall efficiency for when large numbers of files were being decrypted. Upon execution, a server obtains an IP address and port number, this information is printed to the terminal so that clients may use it to connect. If the server has multiple clients connected, it will run a FCFS scheduling algorithm to assign decryption tasks to them. The sockets that connected the client and server are then used for communication until the processes finish.

### Server Design and Implementation:
The server can be connect to a maximun number of clients, specified by the NUM_CHILDREN variable defintion at the top of server.c. This variable is used to initialize the arrays holding connection information such as IP addresses of the clients. Functions such as getaddrinfo, and getsockname were used to get information about IP addresses and port numbers. In order to find a client ready for files, the server utilizes a function containing select(). The function also handles different messages such as success, failure and unexpected disconnections and writes to the logfile accordingly. After sending a batch of file path names to the client, the server will check if there are any pending connections on the socket again using select(), in order to avoid busy waiting.

When the server concludes that all the file names have been read from the configuration file, it will send a finish signal to the clients. After this it will wait for all the remaining messages from the socket using select, telling it whether a decryption task was completed successfully or unsuccessfully. Finally, it will check that the clients have closed their sockets and exited by waiting for a recv() on their socket to return zero. Before exiting, all remaining sockets and files are closed.


### Client Design and Implementation:
The client sends a signal telling the server that it is ready, then it will wait for incoming messages giving details about the files being sent. If an error occurs when trying to recv() an error will be printed out and the client will exit. This is because the client and server follow a very specific message passing sequence that will be disrupted if any errors occur.

Upon scheduling time, the parent will read any messages from the child, including those detailing the success and failure of its previous task. A success message from the child will be compared to hard-coded const ints, the value for success is 1 and the value for failure is 2. When sending a message to the server, 400 specifies success and 300 specifies failure. The server must use these messages to determine when to prepare to receive three addtional messages detailing the length of the file name, the file name, and the process id from the client. In this case, a round robin scheduling algorithm was used by the client to schedule the child processes.

After receiving the exiting signal the client will wait for remaining messages through the use of select on all the pipes connecting it to the child processes. After this the client it will deallocate resources such as closing pipes and etc.


TO BUILD the project use the commands:
1. make
2. ./lyrebird.server config.txt logfile.txt
3. ./lyrebird.client IP address Port number

### FOR STEP 2
Config.txt will contain the following:
The filepathname to the encrypted file, a space and then the filepathname to the location where you want the decrypted tweet to be located.

(eg. ./encrypted_tweet.txt ./decrypted_tweet.txt
     ./encrypted_tweet2.txt ./decrypted_tweet2.txt)

The logfile is the name of the file where you want the server to write its update messages to.

### FOR STEP 3
Enter the IP address a space followed by the port number printed out by the server.



TO CLEAN up the directory type:
`$ make clean`


### REFERENCES:

1)"Programming IP Sockets on Linux, Part One". gnosis.cx. 3 December 2015.
  <http://gnosis.cx/publish/programming/sockets.html>

2)"Beej's Guide to Network Programming". beejs.us. 3 December 2015. Web.
  <http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo>

3)"Modular Exponentiation". Wikipedia. 9 September 2015. Web. 23 September 2015.
   <https://en.wikipedia.org/wiki/Modular_exponentiation> (Right-to-Left Method)

4)"How to Know IP address for interfaces in C". 3 Decemeber 2015. Web
  <http://stackoverflow.com/questions/4139405/how-to-know-ip-address-for-interfaces-in-c>
