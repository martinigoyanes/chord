#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include "assignment4.h"
#include <pthread.h>
#include <signal.h>
int main(int argc, char **argv)
{	
	struct chord_arguments  args= chord_parseopt(argc, argv);              
	pthread_t tid ;
	if(args.succesors_num == 0)
		args.succesors_num = 3;
		
	struct ThreadArgs *thread_args = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
	thread_args->args = args;
	
	if(strcmp(args.join_ip,"")==0) // not trying to join an existing ring so we CREATE a new one
	{
		create_ring(args);//creates a new ring and gives node a working address to recieve messages on
	}
	else
	{//Join existing ring
		join_ring(args);
	}

	pthread_create(&tid, NULL, &handleCommands, (void*)thread_args); //thread taking care of command inputs
	pthread_create(&tid, NULL, &stabilize, (void*)thread_args);//thread handling stabilize
	pthread_create(&tid, NULL, &check_predecessor, (void*)thread_args);//thread handling check_predecessor
	//pthread_create(&tid, NULL, &fix_fingers, (void*)thread_args);//thread handling updates of my finger table			
	while(1)
	{	
		int clntSocket =  setup_theirsock();
		struct ThreadArgs *thread_sock = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
		thread_sock->clntSocket = clntSocket;
		thread_sock->args = args;
	
		pthread_create(&tid, NULL, &handleRequests, (void*)thread_sock); //thread taking node's requests
	}
	
	return 0;
}
