#include "optparser.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Practical.h"
#include  <stdint.h>
#include  <stdlib.h>
#include <signal.h>
struct Node {
	struct Node *succ;
	struct Node *pred;
	int sock;
	char ip_addr [16];
	uint32_t port;
	uint8_t *id;
	int succ_num;
	//in the future add finger tables	
};
struct Finger_List{
	uint8_t *id;
	uint8_t *node_id;
	char ip_addr [16];
	uint32_t port;
	struct Finger_List *next;
	struct Finger_List *prev;
};
struct ThreadArgs{
	struct chord_arguments args;
	int clntSocket;
}; 
void *handleCommands(void *thread_args);
void *handleRequests(void *thread_sock);

void create_ring(struct chord_arguments args);
void join_ring(struct chord_arguments args);
struct Node *find_successor(char join_addr [16], int join_port, uint8_t id[20]);
struct Node *get_successor_list(char ip_addr[16], int port);
void *stabilize(void *thread_args);
void *check_predecessor(void *thread_args);
void *fix_fingers(void *thread_args);
struct Node *closest_preceding_node(uint8_t *asking_id,uint8_t *my_id);
void init_finger_table();
void init_successors(struct chord_arguments args);
int notify(uint32_t notify_port, char notify_ip [16]);

int setup_mysock(struct chord_arguments args);
int setup_theirsock();
int connect_peer(int port, char ip_addr [16]);

int recv_successor_ret(int socket, struct Node *succ);
int send_successor_args(int socket,uint8_t id[20]);

int recv_ret(int socket,uint8_t **RetData, size_t *RetDataLen);
int recv_call(int socket,uint8_t *call_type, uint8_t **CallData, size_t *CallDataLen);
int send_ret(int socket, size_t SerialDataLen, uint8_t *SerialData);
int send_call(int socket, size_t SerialDataLen, uint8_t *SerialData, char *call_name );

uint8_t *hash_key(char *key);
uint8_t *create_id(char ip_addr[16], uint32_t portno);
uint8_t *add_hash(uint8_t id[20],uint8_t value[20]);

void lookup(char *key, char start_ip [16], uint32_t start_port);
void print_state();
void read_commands(char *action,char *key);
