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
#include "hash.h"
#include <pthread.h>
#include "chord.pb-c.h"
pthread_mutex_t node_mutex,finger_mutex,send_mutex,recv_call_mutex,recv_ret_mutex;
static struct Node node;
static struct Finger_List *finger;
void print_notify(struct Node *notifying_node);
void *handleCommands(void *thread_args){
	char *action = malloc(sizeof(char*));
	char *key = malloc(sizeof(char*));
	struct chord_arguments args = ((struct ThreadArgs *) thread_args) -> args;
	            
   
    pthread_detach(pthread_self());
	while(1){
		
		//read input from user          
		read_commands(action,key);
		
		if(strcmp(action,"Lookup") == 0)//Look up key
			lookup(key,args.own_ip,args.own_port);
		else if(strcmp(action,"PrintState\n") == 0)//Print the state
			print_state();
		
	}
}
void *handleRequests(void *thread_sock){
	int clntSocket = ((struct ThreadArgs *) thread_sock) -> clntSocket;
	int error = 0;
	
	pthread_detach(pthread_self());
	
	uint8_t *call_type = (uint8_t*)malloc(sizeof(uint8_t));
	uint8_t **CallData = (uint8_t**)malloc(sizeof(uint8_t*));
	size_t *CallDataLen = (size_t*)malloc(sizeof(size_t));
	
	uint8_t *SerialData;
	size_t *SerialDataLen = (size_t*)malloc(sizeof(size_t));
		
	//recieve message from client and store the type and data from the request to operate with it afterwards
	//pthread_mutex_lock(&recv_call_mutex);
	if ( recv_call(clntSocket, call_type, CallData,CallDataLen) != 0)
		DieWithSystemMessage("recv_call() failed");
	//pthread_mutex_unlock(&recv_call_mutex);
	
	int i = 0;i++;//used so i can instantiate suff inside case	
	//decide which request is it and handle it
	
	switch(*call_type){
		case 0:	//find_successor
			i = 0;
			//find out who is this guy's successor
			struct Node *successor = (struct Node *)malloc(sizeof(struct Node));
			successor->id = malloc(sizeof(uint8_t) * 20);
						
			//get the id of the guy notifying me
			Protocol__FindSuccessorArgs *asking_args = protocol__find_successor_args__unpack(NULL, *CallDataLen, *CallData);
			uint8_t *asking_id = malloc(sizeof(uint8_t) * 20);
			memcpy(asking_id,asking_args->id.data,20);
			
			//if he is in between my_id and my_succ_id 	--->  answer with my successor
			pthread_mutex_lock(&node_mutex);
			//check if the id of the node asking me belongs in (my_id, my_successor_id]
			if( memcmp(asking_id,node.succ->id,20) <= 0  && memcmp(asking_id,node.id,20) > 0)
			    successor = node.succ;
			 //takes care of wrapping around the circle( if the successorY < nodeX(one you are asking)  it means that you wrapped around the cirle
			 //											and therefore your successor would be successorY	)
			else if(memcmp(node.succ->id,node.id,20)<0)
				successor = node.succ;
			else if(memcmp(node.id,node.succ->id,20) == 0)
				successor = node.succ;
				
			//if he is not in between me and my successor --> contact my successor and ask him, and my successor does same thing
			else{
					//struct Node *highest_predecessor = (struct Node *)malloc(sizeof(struct Node));
					//todo , if finger tables not working, highestpred should be my successor
					//highest_predecessor =  closest_preceding_node(asking_id,node.id);
					
					//~ if(highest_predecessor->port == node.port){//if i am the highest predecessor, i compute locally so i dont run into a network loop
						//~ //check if the id of the node asking me belongs in (my_id, my_successor_id]
						//~ if( memcmp(asking_id,node.succ->id,20) <= 0  && memcmp(asking_id,node.id,20) > 0)
							//~ successor = node.succ;
						 //~ //takes care of wrapping around the circle( if the successorY < nodeX(one you are asking)  it means that you wrapped around the cirle
						 //~ //											and therefore your successor would be successorY	)
						//~ else if(memcmp(node.succ->id,node.id,20)<0)
							//~ successor = node.succ;
						//~ else if(memcmp(node.id,node.succ->id,20) == 0)
							//~ successor = node.succ;						
						
					//~ }
					//~ else //contact highest predecessor node on behalf of the guy asking me
						successor = find_successor(node.succ->ip_addr,node.succ->port,asking_id);
			}
			
			
			//pack data	and send recursively
			Protocol__Node succ = PROTOCOL__NODE__INIT;		
			Protocol__FindSuccessorRet 	successor_ret = PROTOCOL__FIND_SUCCESSOR_RET__INIT;
			
			successor_ret.node = &succ;
			successor_ret.node->id.data = malloc(sizeof(uint8_t) * 20);
			successor_ret.node->address = malloc(sizeof(char)* 16);
			
			memcpy(successor_ret.node->id.data,successor->id,20);
			strcpy(successor_ret.node->address,successor->ip_addr);
			successor_ret.node->port = successor->port;
			successor_ret.node->id.len = 20;
			
			pthread_mutex_unlock(&node_mutex);
			
			*SerialDataLen = protocol__find_successor_ret__get_packed_size(&successor_ret);
			SerialData = (uint8_t *)malloc(*SerialDataLen);
			if (SerialData == NULL) {
				error = 1;
				protocol__find_successor_ret__free_unpacked(&successor_ret, NULL);
			}
			
			protocol__find_successor_ret__pack(&successor_ret, SerialData);
			//send data return
			error = send_ret(clntSocket,*SerialDataLen,SerialData);
				
		break;
		case 1:	//notify
			i =0;
			//unpack the "likely to" be my new predecessor
			Protocol__NotifyArgs *new_pred_notify = protocol__notify_args__unpack(NULL, *CallDataLen, *CallData);
			
			//store it into a temporal predecessor to make comparision
			struct Node *new_pred_temp = (struct Node *)malloc(sizeof(struct Node));
			new_pred_temp->id = malloc(sizeof(uint8_t) * 20);
			
			strcpy(new_pred_temp->ip_addr,new_pred_notify->node->address);
			new_pred_temp->id = malloc(sizeof(uint8_t) * 20);
			memcpy(new_pred_temp->id,new_pred_notify->node->id.data,20);
			new_pred_temp->port = new_pred_notify->node->port;
	
	
			// Cleanup
			protocol__notify_args__free_unpacked(new_pred_notify, NULL);
			
			pthread_mutex_lock(&node_mutex);
			if(memcmp(node.pred->id,node.id,20) == 0){
				//If my predecessor is myself the guy who notified me becomes my predecesor
				strcpy(node.pred->ip_addr,new_pred_temp->ip_addr);
				memcpy(node.pred->id,new_pred_temp->id,20);
				node.pred->port = new_pred_temp->port;
			}
			else if( memcmp(new_pred_temp->id,node.id,20) < 0  && memcmp(new_pred_temp->id,node.pred->id,20) > 0){
				//if guy notifying me is in between my "old" predecessor and I , he becomes my predecessor
				strcpy(node.pred->ip_addr,new_pred_temp->ip_addr);
				memcpy(node.pred->id,new_pred_temp->id,20);
				node.pred->port = new_pred_temp->port;
				
			}
			else if((memcmp(new_pred_temp->id,node.id,20) < 0 || memcmp(new_pred_temp->id,node.pred->id,20) > 0)  && memcmp(node.id,node.pred->id,20) < 0){
				//takes of when you wrap around the circle:
				//if ring is: 1 2 3 , and 0 wants to join with the other cases he would NOT be able because he is not >3 however he is <1
				//if ring is: 1 2 3, and 4 wants to join with the other cases he would NOT be ables because he is >3 but not <1
				strcpy(node.pred->ip_addr,new_pred_temp->ip_addr);
				memcpy(node.pred->id,new_pred_temp->id,20);
				node.pred->port = new_pred_temp->port;
				
			}
			if( memcmp(node.succ->id,node.id,20) == 0){ //TODO is this correct? ###boolean could make the algorithm enter here only once at the beggining###
				//this takes care of the edge case at the very begining when you create a ring and some one joins you and you dont have a successor
				//you set the guy that joined you as your successor
				strcpy(node.succ->ip_addr,new_pred_temp->ip_addr);
				memcpy(node.succ->id,new_pred_temp->id,20);
				node.succ->port = new_pred_temp->port;
			}
			pthread_mutex_unlock(&node_mutex);
			
			//send notify_ret back
			Protocol__NotifyRet notify_ret = PROTOCOL__NOTIFY_RET__INIT;
				
			*SerialDataLen = protocol__notify_ret__get_packed_size(&notify_ret);
			SerialData = (uint8_t *)malloc(*SerialDataLen);
			if (SerialData == NULL) {
				error = 1;
				protocol__notify_ret__free_unpacked(&notify_ret, NULL);
			}
			
			protocol__notify_ret__pack(&notify_ret, SerialData);
			
			//send data ret
			error = send_ret(clntSocket,*SerialDataLen,SerialData);
		break;
		case 2:	//get_predecessor
			i=0;
			//1st pack my predecessor into a message
			Protocol__Node proto_pred = PROTOCOL__NODE__INIT;
			Protocol__GetPredecessorRet pred_ret = PROTOCOL__GET_PREDECESSOR_RET__INIT;
			
			pred_ret.node = &proto_pred;
			pred_ret.node->id.data = malloc(sizeof(uint8_t) * 20);
			pred_ret.node->address = malloc(sizeof(char)* 16);
			
			pthread_mutex_lock(&node_mutex);
			memcpy(pred_ret.node->id.data,node.pred->id,20);
			strcpy(pred_ret.node->address,node.pred->ip_addr);
			pred_ret.node->port = node.pred->port;
			pred_ret.node->id.len = 20;
			pthread_mutex_unlock(&node_mutex);
			
			*SerialDataLen = protocol__get_predecessor_ret__get_packed_size(&pred_ret);
			SerialData = (uint8_t *)malloc(*SerialDataLen);
			if (SerialData == NULL) {
				error = 1;
				protocol__get_predecessor_ret__free_unpacked(&pred_ret, NULL);
			}	
					
			protocol__get_predecessor_ret__pack(&pred_ret, SerialData);
			//send data return
			error = send_ret(clntSocket,*SerialDataLen,SerialData);
			
		break;
		case 3:	//check_predecessor
			i=0;
			Protocol__CheckPredecessorRet check_pred_ret = PROTOCOL__CHECK_PREDECESSOR_RET__INIT;
				
			*SerialDataLen = protocol__check_predecessor_ret__get_packed_size(&check_pred_ret);
			SerialData = (uint8_t *)malloc(*SerialDataLen);
			if (SerialData == NULL) {
				error = 1;
				protocol__check_predecessor_ret__free_unpacked(&check_pred_ret, NULL);
			}
			
			protocol__check_predecessor_ret__pack(&check_pred_ret, SerialData);
			
			//send data ret
			error = send_ret(clntSocket,*SerialDataLen,SerialData);
		break;
		case 4:	//get_successor_list
		{
			i=0;
			Protocol__GetSuccessorListRet 	successor_list_ret = PROTOCOL__GET_SUCCESSOR_LIST_RET__INIT;
			Protocol__Node succ [node.succ_num];
			
			pthread_mutex_lock(&node_mutex);
			struct Node *temp_succ = node.succ;
			
			successor_list_ret.n_successors = node.succ_num;
			
			successor_list_ret.successors = (Protocol__Node **)malloc(sizeof(Protocol__Node));
			for(int i = 0 ; i < node.succ_num ; i++){
				protocol__node__init(&succ[i]);
				successor_list_ret.successors[i] = &succ[i];
				successor_list_ret.successors[i]->id.data = malloc(sizeof(uint8_t) * 20);
				successor_list_ret.successors[i]->address = malloc(sizeof(char)* 16);
				
				memcpy(successor_list_ret.successors[i]->id.data,temp_succ->id,20);
				strcpy(successor_list_ret.successors[i]->address,temp_succ->ip_addr);
				successor_list_ret.successors[i]->port = temp_succ->port;
				successor_list_ret.successors[i]->id.len = 20;
				
				if(temp_succ->succ != NULL)
					temp_succ = temp_succ->succ;
			}
			pthread_mutex_unlock(&node_mutex);
			
			*SerialDataLen = protocol__get_successor_list_ret__get_packed_size(&successor_list_ret);
			SerialData = (uint8_t *)malloc(*SerialDataLen);
			if (SerialData == NULL) {
				error = 1;
				protocol__get_successor_list_ret__free_unpacked(&successor_list_ret, NULL);
			}
			
			protocol__get_successor_list_ret__pack(&successor_list_ret, SerialData);
			//send data return
			error = send_ret(clntSocket,*SerialDataLen,SerialData);
		}
		break;
		default:
		break;	
	}
	
	// Cleanup
	error = close(clntSocket);
	
	if(error != 0)
		DieWithSystemMessage("handleRequest() failed");
		
	return NULL;
}
int setup_theirsock()
{	
	int clntSock;
	// Run forever
	struct sockaddr_in clntAddr; // Client address
	// Set length of client address structure (in-out parameter)
	socklen_t clntAddrLen = sizeof(clntAddr);
        
	// Wait for a client to connect
	clntSock = accept(node.sock, (struct sockaddr *) &clntAddr, &clntAddrLen);
	if(clntSock < 0)
		DieWithSystemMessage("accept() failed");
		
	char clntName[INET_ADDRSTRLEN]; // String to contain client address
	if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
		sizeof(clntName)) == NULL)
			puts("Unable to get client address");
	
	return clntSock;
	
}
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
  if (listen(mysock, 1910165408) < 0)
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
	 if( (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0 )
		sock=-1;	    
		
	return sock;
}

uint8_t *create_id(char ip_addr[16],uint32_t portno){
	
	uint8_t *checksum = malloc(sizeof(uint8_t) * 20);
	struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
	if (!ctx) {
		DieWithSystemMessage("hash error");
	}
	char *port = malloc(sizeof(char) * 8);
	char *ip = malloc(sizeof(char) * 16);
	uint8_t *colon = (uint8_t *)malloc(sizeof(uint8_t));
	 
	strcpy(ip,ip_addr);
	sprintf(port,"%d",portno);
	*colon = ':';
	
	uint8_t addr_and_port [strlen(ip) + strlen(port) + 1];
	bzero(addr_and_port,strlen(ip) + strlen(port) + 1);
	
	memcpy(&addr_and_port[0],ip,strlen(ip));
	memcpy(&addr_and_port[strlen(ip)],colon,1);
	memcpy(&addr_and_port[strlen(ip) + 1],port,strlen(port));
	
	int error = sha1sum_finish(ctx, (const uint8_t*)addr_and_port, strlen(ip) + strlen(port) + 1, checksum);
	if (error)
		DieWithSystemMessage("hash error");
		
	sha1sum_reset(ctx);
	
	return checksum;
} 
uint8_t *hash_key(char *key){
	
	uint8_t *checksum = malloc(sizeof(uint8_t) * 20);
	struct sha1sum_ctx *ctx = sha1sum_create(NULL, 0);
	if (!ctx) {
		DieWithSystemMessage("hash error");
	}
	
	int error = sha1sum_finish(ctx, (const uint8_t*)key, strlen(key), checksum);
	if (error)
		DieWithSystemMessage("hash error");
	
	sha1sum_reset(ctx);
	
	return checksum;
} 

void read_commands(char *action,char *key){
	char *input = NULL;
	char *token = NULL;
	int counter = 0;
	ssize_t numbytes = 0;
	size_t n_alloc = 0;
	printf("> ");
	numbytes = getline(&input,&n_alloc,stdin);
	if(numbytes < 0)
			DieWithSystemMessage("getline() failed");
	do{
		token = strtok_r(input," ",&input); //get input delimited by spaces
		if(token == NULL)
			break;
			
		if(counter == 0)
			strcpy(action,token);
		else if(counter == 1)
		{	
			if(token[strlen(token)-1] == '\n')
				token[strlen(token)-1] = '\0'; //getting rid of \n and makin string null terminated
			strcpy(key,token);
		}
		counter++;
	}while(token != NULL);
}
void lookup(char *key, char start_ip [16], uint32_t start_port){
	uint8_t *hashed_key = malloc(sizeof(uint8_t) * 20);
	hashed_key = hash_key(key);
	printf("< %s ",key);
	for(size_t i = 0; i < 20; ++i) {
		printf("%02x", hashed_key[i]);
	}
	putchar('\n');
	
	//find succesor of this key, AKA owner:
	struct Node *successor = (struct Node *)malloc(sizeof(struct Node));
	successor->id = malloc(sizeof(uint8_t) * 20);
	
	successor = find_successor(start_ip,start_port,hashed_key);
	
	pthread_mutex_lock(&node_mutex);
	//Print < owner's hashid ownner's ip owner's port
	printf("< ");
	for(size_t i = 0; i < 20; ++i) {
		printf("%02x", successor->id[i]);
	}
	printf(" %s %d\n",successor->ip_addr,successor->port);
	pthread_mutex_unlock(&node_mutex);
}
void print_state(){  
	//Print myself
	printf("< Self ");
	pthread_mutex_lock(&node_mutex);
	for(size_t i = 0; i < 20; ++i) {
		printf("%02x", node.id[i]);
	}
	printf(" %s %d\n",node.ip_addr,node.port);
	
	//Print successors or successor, if i am the only one in the ring just print me again
	struct Node *temp_succ = node.succ;
	for(int j = 0 ; j<node.succ_num;j++){	
		printf("< Successor [%d] ",j+1);
		for(size_t i = 0; i < 20; ++i) {
			printf("%02x", temp_succ->id[i]);
		}
		printf(" %s %d\n",temp_succ->ip_addr,temp_succ->port);
		
		if(temp_succ->succ != NULL)
			temp_succ = temp_succ->succ;
	}
	//Print finger list
	pthread_mutex_lock(&finger_mutex);
	struct Finger_List *temp_finger = finger;
	for(int k =1; k<161;k++){	
		printf("< Finger [%d] ",k);
		for(size_t i = 0; i < 20; ++i) {
			printf("%02x", temp_finger->node_id[i]);
		}
		printf(" %s %d\n",temp_finger->ip_addr,temp_finger->port);
		
		if(temp_finger->next != NULL)
			temp_finger = temp_finger->next;
	}
	pthread_mutex_unlock(&finger_mutex);
	//TODO get rid of this print predecessor
		//~ printf("< Predecessor [1] ");
	//~ for(size_t i = 0; i < 20; ++i) {
		//~ printf("%02x", node.pred->id[i]);
	//~ }
	//~ printf(" %s %d\n",node.pred->ip_addr,node.pred->port);
	pthread_mutex_unlock(&node_mutex);
	
}
void create_ring (struct chord_arguments args){
	//get a local addres for this node in which other nodes can talk to him
	node.sock = setup_mysock(args);
	strcpy(node.ip_addr,args.own_ip);
	node.port = args.own_port;
	//give node its id formed by ipaddr + port
	node.id = malloc(sizeof(uint8_t) * 20);
	if(args.id == NULL){
		//no id defined in program arguments
		memcpy(node.id,create_id(args.own_ip,args.own_port),20);
	}
	else{
		//id defined in command arguments
		memcpy(node.id,args.id,20);
	}
	//if ring has just been created its predecessor its itself always
	node.pred = (struct Node *)malloc(sizeof(struct Node));
	node.pred->sock = node.sock;
	bzero(node.pred->ip_addr,15);
	strcpy(node.pred->ip_addr,args.own_ip);
	node.pred->port = args.own_port;
	node.pred->id = malloc(sizeof(uint8_t) * 20);
	memcpy(node.pred->id,node.id,20);
	//if ring has just been created its succesor its itself always as many times as r
	init_successors(args);
	node.succ_num =args.succesors_num;
	
	init_finger_table();
}
void join_ring(struct chord_arguments args){
	//get a local addres for this node
	node.sock = setup_mysock(args);
	strcpy(node.ip_addr,args.own_ip);
	node.port = args.own_port;
	//give node its id formed by ipaddr + port
	node.id = malloc(sizeof(uint8_t) * 20);
	if(args.id == NULL){
		//no id defined in program arguments
		memcpy(node.id,create_id(args.own_ip,args.own_port),20);
	}
	else{
		//id defined in command arguments
		memcpy(node.id,args.id,20);
	}
	//when we first join a ring our predecessor is itself
	node.pred = (struct Node *)malloc(sizeof(struct Node));
	node.pred->sock = node.sock;
	bzero(node.pred->ip_addr,15);
	strcpy(node.pred->ip_addr,args.own_ip);
	node.pred->port = args.own_port;
	node.pred->id = malloc(sizeof(uint8_t) * 20);
	memcpy(node.pred->id,node.id,20);
	//find the who should be our successor in the ring we are joining
	node.succ_num =args.succesors_num;
	node.succ = find_successor(args.join_ip,args.join_port,node.id);
	node.succ->succ = get_successor_list(node.succ->ip_addr,node.succ->port);
	
	init_finger_table();
}
struct Node *find_successor(char next_addr [16], int next_port, uint8_t id[20]){
	
	int error = 0;
	int next_sock = 0;
	struct Node *successor = (struct Node *)malloc(sizeof(struct Node));
	
	//first i connect to the join(the guy who was previously on the ring)
	if ((next_sock = connect_peer(next_port,next_addr)) < 0){
		printf("Address : %s\n",next_addr);
		DieWithSystemMessage("connect_peer() in find_successor failed");
	}
	//ask for succesor
	if((error = send_successor_args(next_sock,id)) != 0)
		DieWithSystemMessage("send_successor_args() failed");
	
	//recieve the succesor
	if((error = recv_successor_ret(next_sock, successor)) != 0)
		DieWithSystemMessage("recv_succesor_ret() failed");
	
	close(next_sock);
	return successor;
}

int notify(uint32_t notify_port, char notify_ip [16]){
	int error = 0;
	char *call_name = "notify";
	uint8_t *SerialData;
	size_t *SerialDataLen = (size_t*)malloc(sizeof(size_t));
	int socket = 0;
	if((socket = connect_peer(notify_port,notify_ip)) < 0)
		DieWithSystemMessage("connect_peer() in notify failed");
	
	//Pack myself into a message to tell my successor that i MIGHT be his predeccessor
	Protocol__Node myself = PROTOCOL__NODE__INIT;		
	Protocol__NotifyArgs notify_myself = PROTOCOL__NOTIFY_ARGS__INIT;
		
	notify_myself.node = &myself;
	notify_myself.node->id.data = malloc(sizeof(uint8_t) * 20);
	notify_myself.node->address = malloc(sizeof(char)* 16);
	
	pthread_mutex_lock(&node_mutex);
	memcpy(notify_myself.node->id.data,node.id,20);
	strcpy(notify_myself.node->address,node.ip_addr);
	notify_myself.node->port = node.port;
	notify_myself.node->id.len = 20;
	pthread_mutex_unlock(&node_mutex);
	
	*SerialDataLen = protocol__notify_args__get_packed_size(&notify_myself);
	SerialData = (uint8_t *)malloc(*SerialDataLen);
	if (SerialData == NULL) {
		error = 1;
		protocol__notify_args__free_unpacked(&notify_myself, NULL);
	}
	
	protocol__notify_args__pack(&notify_myself, SerialData);
	
	//send data call
	//~ pthread_mutex_lock(&send_mutex);
	error = send_call(socket,*SerialDataLen,SerialData,call_name);
	//~ pthread_mutex_unlock(&send_mutex);
	
	close(socket);
	return error;
}
void *check_predecessor(void *thread_args){
	struct chord_arguments args = ((struct ThreadArgs *) thread_args) -> args;
	
	pthread_detach(pthread_self());
	
	while(args.tcp != 0){
		//sleep till is time to run
		usleep(args.tcp*1000);//because its ms
		
		pthread_mutex_lock(&node_mutex);
		int pred_sock = connect_peer(node.pred->port,node.pred->ip_addr);
		pthread_mutex_unlock(&node_mutex);
			
		if(pred_sock < 0){
				//predecessor failed --> set it back to myself/null
				pthread_mutex_lock(&node_mutex);
				node.pred->sock = node.sock;
				bzero(node.pred->ip_addr,15);
				strcpy(node.pred->ip_addr,node.ip_addr);
				node.pred->port = node.port;
				memcpy(node.pred->id,node.id,20);
				pthread_mutex_unlock(&node_mutex);
		}
		else{//send message so the guy i connected to just doesnt hang at recv_Call forever and eats all my RAM
				char *call_name = "check_predecessor";
				uint8_t *CallData;
				size_t *CallDataLen = (size_t*)malloc(sizeof(size_t));
				Protocol__CheckPredecessorArgs check_pred_args = PROTOCOL__CHECK_PREDECESSOR_ARGS__INIT;
				
				*CallDataLen = protocol__check_predecessor_args__get_packed_size(&check_pred_args);
				CallData = (uint8_t *)malloc(*CallDataLen);
				if (CallData == NULL) {
					protocol__check_predecessor_args__free_unpacked(&check_pred_args, NULL);
				}
				
				protocol__check_predecessor_args__pack(&check_pred_args, CallData);
				
				//send data call
				//~ pthread_mutex_lock(&send_mutex);
				send_call(pred_sock,*CallDataLen,CallData,call_name);
				//~ pthread_mutex_unlock(&send_mutex);
				
				//recieve ret msg from my predecessor, i dont need to do anything with it
				uint8_t **RetData = (uint8_t**)malloc(sizeof(uint8_t*));
				size_t *RetDataLen = (size_t*)malloc(sizeof(size_t));
				
				//pthread_mutex_lock(&recv_ret_mutex);
				recv_ret(pred_sock, RetData, RetDataLen);
				//pthread_mutex_unlock(&recv_ret_mutex);
		}
			
		close(pred_sock);
		
	}
	return NULL;
}
void* stabilize(void *thread_args){
	struct chord_arguments args = ((struct ThreadArgs *) thread_args) -> args;
	
	pthread_detach(pthread_self());
	
	while(args.ts != 0){
		//sleep till is time to execute
		usleep(args.ts*1000);//because its ms
		
		//1st get my successor's predecessor
		int error = 0;
		char *call_name = "get_predecessor";
		uint8_t *CallData;
		size_t *CallDataLen = (size_t*)malloc(sizeof(size_t));
		
		//Connect to my successor
		pthread_mutex_lock(&node_mutex);
		int socket = 0;
		while(socket <= 0){
			socket = connect_peer(node.succ->port,node.succ->ip_addr);
			if (socket < 0){ 
				if(node.succ->succ == NULL || memcmp(node.succ->succ->id,node.id,20)==0 ){
					//if all of my successor fails , i set myself as successor or if my in a 2 node ring, then my succ->succ is myself 
					//then dont need to go ask myself since i would do crazy shit with the mutex_locks 
					//---> i just set all my successors as my self
					init_successors(args);
				}
				else //if my successor fails , i go to the next one and get his successors list																	
					node.succ = get_successor_list(node.succ->succ->ip_addr,node.succ->succ->port);
			}				
		}
		pthread_mutex_unlock(&node_mutex);
		
		//Pack message to ask for his predecessor
		Protocol__GetPredecessorArgs pred_args = PROTOCOL__GET_PREDECESSOR_ARGS__INIT;
			
		*CallDataLen = protocol__get_predecessor_args__get_packed_size(&pred_args);
		CallData = (uint8_t *)malloc(*CallDataLen);
		if (CallData == NULL) {
			error = 1;
			protocol__get_predecessor_args__free_unpacked(&pred_args, NULL);
		}
		
		protocol__get_predecessor_args__pack(&pred_args, CallData);
		
		//send data call
		error = send_call(socket,*CallDataLen,CallData,call_name);
		
		//recieve ret msg from my successor
		uint8_t **RetData = (uint8_t**)malloc(sizeof(uint8_t*));
		size_t *RetDataLen = (size_t*)malloc(sizeof(size_t));
	
		error = recv_ret(socket, RetData, RetDataLen);
		
		//unpack my succesor's predecessor

		Protocol__GetPredecessorRet *succ_pred_ret = protocol__get_predecessor_ret__unpack(NULL, *RetDataLen, *RetData);
		
		struct Node *new_succ_temp = (struct Node *)malloc(sizeof(struct Node));
		
		strcpy(new_succ_temp->ip_addr,succ_pred_ret->node->address);
		new_succ_temp->id = malloc(sizeof(uint8_t) * 20);
		memcpy(new_succ_temp->id,succ_pred_ret->node->id.data,20);
		new_succ_temp->port = succ_pred_ret->node->port;
		
		
		// Cleanup
		protocol__get_predecessor_ret__free_unpacked(succ_pred_ret, NULL);
		
		pthread_mutex_lock(&node_mutex);
		//2nd Check if my successor's predecessor is in between me and my current successor
		if( memcmp(new_succ_temp->id,node.id,20) > 0  && memcmp(new_succ_temp->id,node.succ->id,20) <0){
			//if he is , he becomes my successor
			strcpy(node.succ->ip_addr,new_succ_temp->ip_addr);
			memcpy(node.succ->id,new_succ_temp->id,20);
			node.succ->port = new_succ_temp->port;
		}
		else if((memcmp(new_succ_temp->id,node.id,20) > 0  ||  memcmp(new_succ_temp->id,node.succ->id,20) <=0) && memcmp(node.succ->id,node.id,20) <0){
			//takes care of wrapping around the ring
			// if ring is : 1 2 3, and 0 joins when they stabilze , 0 would have never been added to the ring with the other conditions
			//since he is <1 but not >3
			// if ring is : 1 2 3, and 4 joins when they stabilze , 4 would have never been added to the ring with the other conditions
			//since he is >3 but not <1
			strcpy(node.succ->ip_addr,new_succ_temp->ip_addr);
			memcpy(node.succ->id,new_succ_temp->id,20);
			node.succ->port = new_succ_temp->port;
		}
		pthread_mutex_unlock(&node_mutex);

		pthread_mutex_lock(&node_mutex);
		//update my successor table, and to not overload, only do it when my successor is not my self
		if(memcmp(node.succ->id,node.id,20) != 0)
			node.succ->succ = get_successor_list(node.succ->ip_addr,node.succ->port);
		pthread_mutex_unlock(&node_mutex);
		
		pthread_mutex_lock(&node_mutex);    
		//3rd notify my new_successor of this change or if nothing has changed
		char notify_ip[16];
		uint32_t notify_port = node.succ->port;
		strcpy(notify_ip,node.succ->ip_addr);
		pthread_mutex_unlock(&node_mutex);
		
		//close connection to my old successor
		close(socket);
		//connect and notify to my new successor
		notify(notify_port,notify_ip);
			
		if(error != 0)
			DieWithSystemMessage("stabilize() failed");
	}
	return NULL;
}
int send_successor_args(int socket, uint8_t id [20]) 
{	
   char *call_name = "find_successor";
   int error = 0;
  // Serializing/Packing the arguments
  
  Protocol__FindSuccessorArgs successor_args = PROTOCOL__FIND_SUCCESSOR_ARGS__INIT;
  successor_args.id.len = 20;
  successor_args.id.data = id;   
  
  
  size_t argsSerialLen = protocol__find_successor_args__get_packed_size(&successor_args);
  uint8_t *argsSerial = (uint8_t *)malloc(argsSerialLen);
  if (argsSerial == NULL) {
    return 1;
  }

  protocol__find_successor_args__pack(&successor_args, argsSerial);
  
  //~ pthread_mutex_lock(&send_mutex);
  error = send_call(socket,argsSerialLen,argsSerial,call_name);
  //~ pthread_mutex_unlock(&send_mutex);
  
  return error;
 }

int recv_successor_ret(int socket, struct Node *successor)
{	
  int error = 0;
  //reciver return from node
  uint8_t **RetData = (uint8_t**)malloc(sizeof(uint8_t*));
  size_t *RetDataLen = (size_t*)malloc(sizeof(size_t));
  error = recv_ret(socket,RetData, RetDataLen);
  
  // Deserialize/Unpack the return value of the invert call
  Protocol__FindSuccessorRet *value = protocol__find_successor_ret__unpack(NULL, *RetDataLen, *RetData);

  strcpy(successor->ip_addr,value->node->address);
  successor->id = malloc(sizeof(uint8_t) * 20);
  memcpy(successor->id,value->node->id.data,20);
  successor->port = value->node->port;

  // Cleanup
  protocol__find_successor_ret__free_unpacked(value, NULL);

	return error;
}
struct Node *get_successor_list(char ip_addr[16], int port)
{	
   char *call_name = "get_successor_list";
   int error = 0;
   int socket = 0;
   //first connect to the successor
   if((socket= connect_peer(port,ip_addr)) < 0)
			DieWithSystemMessage("connect_peer() in get_succ_list failed");
   
   
  // Serializing/Packing the arguments
  Protocol__GetSuccessorListArgs successor_list_args = PROTOCOL__GET_SUCCESSOR_LIST_ARGS__INIT;
  
  size_t argsSerialLen = protocol__get_successor_list_args__get_packed_size(&successor_list_args);
  uint8_t *argsSerial = (uint8_t *)malloc(argsSerialLen);
  if (argsSerial == NULL) {
    error = 1;
  }

  protocol__get_successor_list_args__pack(&successor_list_args, argsSerial);
  //~ pthread_mutex_lock(&send_mutex);
  error = send_call(socket,argsSerialLen,argsSerial,call_name);
  //~ pthread_mutex_unlock(&send_mutex);
  
  //RECIEVE LIST from node
  uint8_t **RetData = (uint8_t**)malloc(sizeof(uint8_t*));
  size_t *RetDataLen = (size_t*)malloc(sizeof(size_t));
  //~ pthread_mutex_lock(&recv_ret_mutex);
  error = recv_ret(socket,RetData, RetDataLen);
  //~ pthread_mutex_unlock(&recv_ret_mutex);
  
  // Deserialize/Unpack the return value of the invert call
  Protocol__GetSuccessorListRet *successor_list_ret = protocol__get_successor_list_ret__unpack(NULL, *RetDataLen, *RetData);

  struct Node *temp_successors_list = NULL;
  
  struct Node **successor_list = (struct Node**) malloc(sizeof(struct Node*));
  *successor_list = (struct Node*) malloc(successor_list_ret->n_successors);
  
  for(int i = 0 ; i<successor_list_ret->n_successors;i++){
	  
		struct Node new_succ;
		new_succ.pred = (struct Node*) malloc(sizeof(struct Node));
		new_succ.id = malloc(sizeof(uint8_t) * 20);
		strcpy(new_succ.ip_addr,successor_list_ret->successors[i]->address);
		memcpy(new_succ.id,successor_list_ret->successors[i]->id.data,20);
		new_succ.port = successor_list_ret->successors[i]->port;
		
		struct Node *last_succ = temp_successors_list; 
		 
		new_succ.succ = NULL; 
	  
		if (temp_successors_list == NULL){ 
			temp_successors_list =  (struct Node*) malloc(sizeof(struct Node));
		   *temp_successors_list = new_succ; 
		   continue; 
		}   
		   
		while (last_succ->succ != NULL) 
			last_succ = last_succ->succ; 
			
		last_succ->succ =  (struct Node*) malloc(sizeof(struct Node));
		*last_succ->succ = new_succ; 
  }
  
  *successor_list = temp_successors_list;
  
  // Cleanup
  protocol__get_successor_list_ret__free_unpacked(successor_list_ret, NULL);
  close(socket);
  if(error != 0)
	DieWithSystemMessage("get_successor_list() failed");
	
  return *successor_list;
 }

int recv_ret(int socket,uint8_t **RetData, size_t *RetDataLen){
  int error = 0;
  int remaining = 8;
  uint64_t *reply_len = (uint64_t *)malloc(sizeof(uint64_t));
  
  do{//first read 8 bytes to see size of payload
	  int numbytes = recv(socket,reply_len,remaining,0);
	  if (numbytes < 0)
			DieWithSystemMessage("recv_reply_len_ret() failed");
	  remaining = remaining - numbytes;
  }while(remaining > 0);
  
  //then read actual payload
  size_t retSerialLen = be64toh(*reply_len) - 8;
  uint8_t *retSerial=(uint8_t *)malloc(retSerialLen);
  
  remaining = retSerialLen;
  do{
	  int numbytes = recv(socket,retSerial,remaining,0);
	  if (numbytes < 0)
			DieWithSystemMessage("recv_payload_ret() failed");
		remaining = remaining - numbytes;
  }while(remaining > 0);
  
  
  // Deserialize/Unpack the return message
  Protocol__Return *ret = protocol__return__unpack(NULL, retSerialLen, retSerial);
  if (ret == NULL) {
    error = 1;
    free(retSerial);
  }
  //get data from return
  *RetData = (uint8_t *)malloc(ret->value.len);
  memcpy(*RetData,ret->value.data,ret->value.len);
  //get lenght of data from return
  *RetDataLen = ret->value.len;
  
  protocol__return__free_unpacked(ret,NULL);
  return error;

}
int recv_call(int socket, uint8_t *call_type, uint8_t **CallData, size_t *CallDataLen){
  int error = 0;
  int remaining = 8;
  uint64_t *call_len = (uint64_t *)malloc(sizeof(uint64_t));
  
  do{//first read 8 bytes to see size of payload
	  int numbytes = recv(socket,call_len,remaining,0);
	  if (numbytes < 0){
			//todo
			printf("thread: %ld  socket:%d\n",pthread_self(),socket);
			DieWithSystemMessage("recv_call_len() failed");
		}
		remaining = remaining - numbytes;
  }while(remaining > 0);
  
  //then read actual payload
  size_t callSerialLen = be64toh(*call_len) - 8;
  uint8_t *callSerial=(uint8_t *)malloc(callSerialLen);
  
  remaining = callSerialLen;
  do{
	  int numbytes = recv(socket,callSerial,remaining,0);
	  if (numbytes < 0)
			DieWithSystemMessage("recv_call() failed");
		remaining = remaining - numbytes;
  }while(remaining > 0);
  
  
  // Deserialize/Unpack the Call message
  Protocol__Call *call = protocol__call__unpack(NULL, callSerialLen, callSerial);
  
  if (call == NULL) {
    error = 1;
    free(callSerial);
  }		
  //get data from request
  *CallData = (uint8_t *)malloc(call->args.len);
  memcpy(*CallData,call->args.data,call->args.len);
  //get lenght of data from request
  *CallDataLen = call->args.len;
  
  //get the type of request	
  if(strcmp(call->name,"find_successor") == 0)
	*call_type = 0;
  else if(strcmp(call->name,"notify") == 0)
	*call_type = 1;
  else if(strcmp(call->name,"get_predecessor") == 0)
	*call_type = 2;
  else if(strcmp(call->name,"check_predecessor") == 0)
	*call_type = 3;
  else if(strcmp(call->name,"get_successor_list") == 0)
	*call_type = 4;
  else
	protocol__call__free_unpacked(call, NULL);
  
  // Cleanup
  protocol__call__free_unpacked(call,NULL);
  return error;
}
int send_call(int socket, size_t SerialDataLen, uint8_t *SerialData, char *call_name ){
  int error = 0;
  int numbytes = 0;
  // Serializing/Packing the call, which also placing the serialized/packed
  // arguments inside
  Protocol__Call call = PROTOCOL__CALL__INIT;
  call.name = (char*)malloc(sizeof(char));
  strcpy(call.name,call_name);
  call.args.len = SerialDataLen;
  call.args.data = SerialData;

  size_t callSerialLen = protocol__call__get_packed_size(&call);
  uint8_t *callSerial = (uint8_t *)malloc(callSerialLen);
  
  uint64_t *length = (uint64_t *)malloc(sizeof(uint64_t));
  uint8_t message[callSerialLen + 8];
  if (callSerial == NULL) {
    error = 1;
    free(SerialData);
  }

  protocol__call__pack(&call, callSerial);
  
  *length = htobe64(callSerialLen + 8);
  
  memcpy(&message[0],length,8);
  memcpy(&message[8],callSerial,callSerialLen);
  
  int remaining = callSerialLen + 8;
								
  do{
		numbytes = send(socket,&message,remaining,MSG_NOSIGNAL);
		remaining = remaining - numbytes;
  }while(remaining > 0);
  
  if(numbytes<0){
	error=1;
  }
    
  return error;

}
int send_ret(int socket, size_t SerialDataLen, uint8_t *SerialData){
	int error = 0;
	int numbytes = 0;
	Protocol__Return ret = PROTOCOL__RETURN__INIT;
	ret.success = 1;
	if(SerialData != NULL)
		ret.has_value = 1;
	uint8_t *retSerial;
	size_t retSerialLen;
	//pack return message
	if (ret.success) {
		ret.value.data = SerialData;
		ret.value.len =  SerialDataLen;
	}

	retSerialLen = protocol__return__get_packed_size(&ret);
	retSerial = (uint8_t *)malloc(retSerialLen);
	
	uint64_t *length = (uint64_t *)malloc(sizeof(uint64_t));
	uint8_t message[retSerialLen + 8];
	
	if (retSerial == NULL) {
		error = 1;
		goto errRetMalloc;
	}
		  
	protocol__return__pack(&ret, retSerial);
	
	 
	*length = htobe64(retSerialLen + 8);
		  
	memcpy(&message[0],length,8);
	memcpy(&message[8],retSerial,retSerialLen);
	  
	int remaining = retSerialLen + 8;
										
	do{
		numbytes = send(socket,&message,remaining,MSG_NOSIGNAL);
		remaining = remaining - numbytes;
	}while(remaining > 0);
		  
	if(numbytes<0){
		error=1;
	}	
	
	errRetMalloc:
		free(SerialData);
	return error;
}
void init_successors(struct chord_arguments args){

	struct Node* temp_successors = NULL;
  
	for(int i = 0 ; i < args.succesors_num ; i++){
		struct Node new_succ;
		new_succ.pred = (struct Node*) malloc(sizeof(struct Node));
		memcpy(new_succ.pred,node.pred,sizeof(struct Node));
		memcpy(new_succ.ip_addr,node.ip_addr,16);
		new_succ.port = node.port;
		new_succ.sock = node.sock;
		new_succ.id = malloc(sizeof(uint8_t) * 20);
		memcpy(new_succ.id,node.id,20);
		
		struct Node *last_succ = temp_successors; 
		 
		new_succ.succ = NULL; 
	  
		if (temp_successors == NULL){ 
			temp_successors =  (struct Node*) malloc(sizeof(struct Node));
		   *temp_successors = new_succ; 
		   continue; 
		}   
		   
		while (last_succ->succ != NULL) 
			last_succ = last_succ->succ; 
			
		last_succ->succ =  (struct Node*) malloc(sizeof(struct Node));
		*last_succ->succ = new_succ; 
	}
	
	node.succ = temp_successors;
}
void *fix_fingers(void *thread_args){
	struct chord_arguments args = ((struct ThreadArgs *) thread_args) -> args;
	pthread_detach(pthread_self());
	
	while(args.tff != 0){
		//sleep till is time to execute
		usleep(args.tff*1000);//because its ms

		struct Finger_List* temp_fingers = NULL;
		uint8_t value[20];
		pthread_mutex_lock(&node_mutex);
		
		uint8_t *my_id= malloc(sizeof(uint8_t) * 20);
		char my_addr [16];
		uint32_t my_port = node.port;
		memcpy(my_id,node.id,20);
		strcpy(my_addr,node.ip_addr);
		
		pthread_mutex_unlock(&node_mutex);

		for(int i = 0 ; i < 160 ; i++){
			struct Finger_List new_finger;
			struct Node *temp;
			new_finger.id = malloc(sizeof(uint8_t) * 20);
			bzero(new_finger.id,20);
			bzero(value,20);
			//2^n ---> we put the 1 in the right position out of all the bytes
			value[19 - i/8] = 1;//MSB is 0
			for(int k = 0; k < i%8 ; k++){//take care of power of 2 w/o using "double datatype"
				value[19 - i/8] = 2 * value[19 - i/8];
			}
			//finger entry = my_id + 2^n
			new_finger.id = add_hash(my_id, value);

			temp = find_successor(my_addr,my_port,new_finger.id);
			memcpy(new_finger.ip_addr,temp->ip_addr,16);
			new_finger.node_id = malloc(sizeof(uint8_t) * 20);
			memcpy(new_finger.node_id,temp->id,20);
			new_finger.port = temp->port;
			
			
			
			struct Finger_List *last_finger = temp_fingers; 
			 
			new_finger.next = NULL; 
		  
			if (temp_fingers == NULL){ 
				temp_fingers =  (struct Finger_List*) malloc(sizeof(struct Finger_List));
			   *temp_fingers = new_finger; 
			   continue; 
			}   
			   
			while (last_finger->next != NULL) 
				last_finger = last_finger->next; 
				
			last_finger->next =  (struct Finger_List*) malloc(sizeof(struct Finger_List));
			*last_finger->next = new_finger; 
		}
		
		pthread_mutex_lock(&finger_mutex);
		finger = temp_fingers;
		pthread_mutex_unlock(&finger_mutex);
		
	}
	
	return NULL;
}
struct Node *closest_preceding_node(uint8_t *asking_id,uint8_t *my_id){
	struct Node *highest_predecessor = (struct Node *)malloc(sizeof(struct Node));
	struct Finger_List *temp_fingers;
	pthread_mutex_lock(&finger_mutex);
	temp_fingers = finger;
	
	//wrapping around ring
	//example : ring is 1 2 3 4 5, if 0 joins 2, 0<2 ---> he is successor is 2's predecessor ---> i can directly send him to my last finger
	if(memcmp(asking_id,my_id,20)<=0){
		for(int i = 0; i < 159 ;i++){
			if(temp_fingers->next != NULL)
				temp_fingers = temp_fingers->next;
		}	
	}
	else{
		//1st --- go to the end of the list
		for(int i = 0 ; i <159;i++){
			if(temp_fingers->next != NULL)
				temp_fingers = temp_fingers->next;
		}
		for(int i = 0 ; i <159;i++){
			
			//finger_id is in between myid and the askingid(id from node that started the find successor)
			if(memcmp(temp_fingers->id,asking_id,20)<=0  && memcmp(temp_fingers->id,my_id,20)>0)
				break;
			
			if(temp_fingers->prev != NULL)
				temp_fingers = temp_fingers->prev;
		}
	}
	pthread_mutex_unlock(&finger_mutex);
	
	highest_predecessor->port = temp_fingers->port;
	strcpy(highest_predecessor->ip_addr,temp_fingers->ip_addr);
	
	return highest_predecessor;
}
void init_finger_table(){
		struct Finger_List* temp_fingers = NULL;
		uint8_t value[20];
		pthread_mutex_lock(&node_mutex);
		
		uint8_t *my_id= malloc(sizeof(uint8_t) * 20);
		uint8_t *succ_id= malloc(sizeof(uint8_t) * 20);
		char succ_addr [16];
		uint32_t succ_port = node.succ->port;
		memcpy(my_id,node.id,20);
		memcpy(succ_id,node.succ->id,20);
		strcpy(succ_addr,node.succ->ip_addr);
		
		pthread_mutex_unlock(&node_mutex);

		for(int i = 0 ; i < 160 ; i++){
			struct Finger_List new_finger;
			struct Finger_List last_finger_next_prev;
			
			new_finger.id = malloc(sizeof(uint8_t) * 20);
			bzero(new_finger.id,20);
			bzero(value,20);
			//2^n ---> we put the 1 in the right position out of all the bytes
			value[19 - i/8] = 1;//MSB is 0
			for(int k = 0; k < i%8 ; k++){//take care of power of 2 w/o using "double datatype"
				value[19 - i/8] = 2 * value[19 - i/8];
			}
			//finger entry = my_id + 2^n
			new_finger.id = add_hash(my_id, value);
			
			memcpy(new_finger.ip_addr,succ_addr,16);
			new_finger.node_id = malloc(sizeof(uint8_t) * 20);
			memcpy(new_finger.node_id,succ_id,20);
			new_finger.port = succ_port;
			
			struct Finger_List *last_finger = temp_fingers; 
			 
			new_finger.next = NULL; 
		  
			if (temp_fingers == NULL){ 
				temp_fingers =  (struct Finger_List*) malloc(sizeof(struct Finger_List));
			   *temp_fingers = new_finger; 
			   continue; 
			}   
			   
			while (last_finger->next != NULL){
				last_finger_next_prev = *last_finger;
				last_finger = last_finger->next; 
			}
				
			last_finger->next =  (struct Finger_List*) malloc(sizeof(struct Finger_List));
			last_finger->prev =  (struct Finger_List*) malloc(sizeof(struct Finger_List));
			*last_finger->next = new_finger; 
			*last_finger->prev = last_finger_next_prev;
			
		}
		
		pthread_mutex_lock(&finger_mutex);
		finger = temp_fingers;
		pthread_mutex_unlock(&finger_mutex);

}
uint8_t *add_hash(uint8_t id[20],uint8_t value[20]){
	uint8_t *addition = malloc(sizeof(uint8_t) * 20);
	uint8_t carry = 0; //no carry at the beggining
	for(int i = 19; i != -1; i--){
		if(carry)
			addition[i] = id[i] + value[i] + 1;
		else
			addition[i] = id[i] + value[i];
		
		if(addition[i] < id[i] )//if your result obatined is lower than any input it means overflow/carry
			carry = 1;
		else
			carry = 0;
			
	}
	
	return addition;
}
