#include "Practical.h"
#include "inputs.h"
#include "hash.h"
#include "assignment4.h"

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
void lookup(char *key){
	printf("< %s ",key);
	for(size_t i = 0; i < 20; ++i) {
		printf("%02x", hash_key(key)[i]);
	}
	putchar('\n');
}
void print_state(){
	printf("< Self ");
	for(size_t i = 0; i < 20; ++i) {
		printf("%02x", node.id[i]);
	}
	printf(" %s %d\n",node.ip_addr,node.port);
	
}
