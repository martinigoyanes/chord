/* This is an example program that parses the options provided on the
 * command line that are needed for assignment 0. You may copy all or
 * parts of this code in the assignment */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <argp.h>
#include "optparser.h"

error_t chord_parser(int key, char *arg, struct argp_state *state) {
	struct chord_arguments *args = state->input;
	char *temp_id;
	error_t ret = 0;
	switch(key) {
	case 'a':
		/* validate that address parameter makes sense */
		strncpy(args->own_ip, arg, 16);
		if (0 /* ip address is goofytown */) {
			argp_error(state, "Invalid address");
		}
		break;
	case 'p':
		/* Validate that port is correct and a number, etc!! */
		args->own_port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		break;
	case 'r':
		/* validate argument makes sense */
		args->succesors_num = atoi(arg);
		break;
	case 'i':
		/* validate file */
		temp_id = malloc(sizeof(char)*41);
		args->id = malloc(sizeof(uint8_t)*20);
		strncpy(temp_id, arg,40);
		//if id given is not 40 char-->abort
		printf("%ld\n",strlen(temp_id));
		if(strlen(temp_id) != 40){
			puts("Id not long enough");
			exit(errno);
		}
		temp_id[40] = '\0';
		char *buffer;
		char this_byte[3];
		for(int i = 0 ; i<20 ; i++){
			this_byte[0] = temp_id[i*2];
			this_byte[1] = temp_id[i*2 + 1];
			this_byte[2] = '\0';
			uint8_t value = (uint8_t)strtol(this_byte,&buffer,16);
			
			args->id[i] = value;
		}
		free(temp_id);
		break;
	case 300:
		/* validate that address parameter makes sense */
		strncpy(args->join_ip, arg, 16);
		if (0 /* ip address is goofytown */) {
			argp_error(state, "Invalid address");
		}
		break;
	case 301:
		//* Validate that port is correct and a number, etc!! */
		args->join_port = atoi(arg);
		if (0 /* port is invalid */) {
			argp_error(state, "Invalid option for a port, must be a number");
		}
		break;
	case 302:
		/* validate arg */
		args->ts = atoi(arg);
		break;
	case 303:
		/* validate arg */
		args->tff = atoi(arg);
		break;
	case 304:
		/* validate arg */
		args->tcp = atoi(arg);
		break;
		
	
	default:
		ret = ARGP_ERR_UNKNOWN;
		break;
	}
	return ret;
}


struct chord_arguments chord_parseopt(int argc, char *argv[]) {
	struct argp_option options[] = {
		{ "own_addr", 'a', "own_addr", 0, "The IP address the server is listening at", 0},
		{ "own_port", 'p', "own_port", 0, "The port that is being used at the server", 0},
		{ "succesors_num", 'r', "num_succesors", 0, "The number of hash requests to send to the server", 0},
		{ "id", 'i', "id", 0, "ID provided to chord client will override SHA1 sum", 0},
		{ "ja", 300, "join_addr", 0, "The minimum size for the data payload in each hash request", 0},
		{ "jp", 301, "join_port", 0, "The maximum size for the data payload in each hash request", 0},
		{ "ts", 302, "t_stabilize", 0, "The maximum size for the data payload in each hash request", 0},
		{ "tff", 303, "t_fixfinger", 0, "The maximum size for the data payload in each hash request", 0},
		{ "tcp", 304, "t_checkpredecessor", 0, "The maximum size for the data payload in each hash request", 0},
		{0}
	};

	struct argp argp_settings = { options, chord_parser, 0, 0, 0, 0, 0};

	struct chord_arguments args;
	bzero(&args, sizeof(args));

	if (argp_parse(&argp_settings, argc, argv, 0, NULL, &args) != 0) {
		printf("Got error in parse\n");
	}

	/* If they don't pass in all required settings, you should detect
	 * this and return a non-zero value from main */
	//printf("Got %s on port %d with n=%d smin=%d smax=%d filename=%s\n",
	       //args.ip_address, args.port, args.hashnum, args.smin, args.smax, args.filename);
	
	
	return args;
}
