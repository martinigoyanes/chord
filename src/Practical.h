#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>

void DieWithUserMessage(const char *msg, const char *detail);
void DieWithSystemMessage(const char *msg);
void HandleTCPClient(int clntSocket);
void PrintSocketAddress(const struct sockaddr *address, FILE *stream);
bool SockAddrsEqual(const struct sockaddr *addr1, const struct sockaddr *addr2);
int SetupTCPServerSocket(const char *service);
int AcceptTCPConnection(int servSock);
