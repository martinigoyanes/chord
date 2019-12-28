#include <argp.h>
#include  <stdint.h>
struct chord_arguments {
	char own_ip[16]; /* You can store this as a string, but I probably wouldn't */
	int own_port; /* is there already a structure you can store the address
	           * and port in instead of like this? */
	char join_ip[16];
	int join_port;
	int ts;
	int tff;
	int tcp;
	int succesors_num;
	uint8_t *id;
	
};
error_t chord_parser(int key, char *arg, struct argp_state *state);
struct chord_arguments chord_parseopt(int argc, char *argv[]);
