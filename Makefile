all:
	gcc	-Wall	-g	-o	chord	src/chord.c	src/optparser.c src/diewithmessage.c	src/assignment4.c	src/hash.c	-lprotobuf-c	src/chord.pb-c.c	-lcrypto	-lpthread
