#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "communication.h"

int setup_mysock(struct chord_arguments args){
	
	in_port_t own_port = args.own_port; // First arg:  local port

  // Create socket for incoming connections
  int mysock; // Socket descriptor for server
  if ((mysock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    DieWithSystemMessage("socket() failed");

  // Construct local address structure
  struct sockaddr_in myAddr;                  // Local address
  memset(&myAddr, 0, sizeof(myAddr));       // Zero out structure
  myAddr.sin_family = AF_INET;                // IPv4 address family
  myAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
  myAddr.sin_port = htons(own_port);          // Local port

  // Bind to the local address
  if (bind(mysock, (struct sockaddr*) &myAddr, sizeof(myAddr)) < 0)
    DieWithSystemMessage("bind() failed");

  // Mark the socket so it will listen for incoming connections
  if (listen(mysock, 5) < 0)
    DieWithSystemMessage("listen() failed");
		
	return mysock;
	
}
int connect_peer(int port, char ip_addr [16])
{
	//server port 
	in_port_t servPort = port;

	// Create a reliable, stream socket using TCP
	 int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	 if (sock < 0)
		DieWithSystemMessage("socket() failed");

	// Construct the server address structure
	 struct sockaddr_in servAddr; // Server address
	 memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	 servAddr.sin_family = AF_INET; // IPv4 address family
	 
	// Convert address
	 //IPv4 TCP Client 
	 int rtnVal = inet_pton(AF_INET, ip_addr, &servAddr.sin_addr.s_addr);
	 if (rtnVal == 0)
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	 else if (rtnVal < 0)
		DieWithSystemMessage("inet_pton() failed");
	 servAddr.sin_port = htons(servPort); // Server port

	 // Establish the connection to the echo server
	 if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
		DieWithSystemMessage("connect() failed");
		
	return sock;
}
