/*      @(#)hostrfs.c 1.1 92/07/30 SMI      */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>

/* Default RFS port number */
#define RFS_PORTNUM	"5200"

/*
 * Convert a socket Internet address to RFS format (ascii hex)
 * \x<Family><Port><address><8bytes filler>
 */
main(argc, argv)
int argc; char *argv[];
{
	struct hostent *hp;
	char rfsaddr[100];
	char *portnum;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <hostname> [portnum]\n", argv[0]);
		exit(-1);
	}
	if ((hp = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "%s: unknown hostname '%s'\n", argv[0],argv[1]);
		exit(-1);
	}
	portnum = (argc > 2 ? argv[2] : RFS_PORTNUM);
	printf("\\x%04x%04x%02x%02x%02x%02x0000000000000000\n",
		AF_INET, atoi(portnum), 
		(u_char) hp->h_addr[0], (u_char) hp->h_addr[1], 
		(u_char) hp->h_addr[2], (u_char) hp->h_addr[3]);
}
