/*
 * @(#)pctool-listener.c 1.1 7/30/92
 *
 * pctool-listener.c:  This program is invoked by pc.bat, which writes to
 *    LPT1:.  We've set up in ipctest.c for pc.bat to invoke pctool-listener
 *    whenever it writes to LPT1.  This program gets a line of input from
 *    its stdin and then writes that input line to a unix socket connected
 *    to ipctest.
 *
 * usage: pctool-listenerr
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include "ipctest.h"

main(argc, argv)
	int	argc;
	char	*argv[];
{
	char			*strtok();
	int			sock;		/* socket descriptor	*/
	char			ipcnum;		/* ipc board under test */
	char			ipcnumstr[2];	/* ipcnum as a string   */
	struct sockaddr_un	name;		/* name of unix socket	*/
	char			info[INFOSIZ];
	char			msg[INFOSIZ];
	char			*word;
	char			pcstring[INFOSIZ];
	int			i, retval;

	if (gets(info) == NULL) {	/* read a line from stdin */
	    printf("%s: %s was invoked but no input received from pctool",
		   argv[0], argv[0]);
	    strcpy(info, LISTEN_ERRMSG); /* tell ipctest couldn't read */
	}
	strcpy(pcstring, info);
	word = strtok(pcstring, " ");
	for (; word != NULL;) {
	    if (strncmp(word, "ipc", 3) == 0) { /* check for ipc# */
		ipcnum = word[3];
		sprintf(ipcnumstr, "%c", ipcnum);
		break;
	    }
	    word = strtok(NULL, " ");	/* get next word in string */
	}
		/* the socket name is something like '/tmp/ipctestsoc3' */
	sprintf(name.sun_path, "%s%c", SOCKNAME, ipcnum);
	for (i=1; i<5; i++) {
	    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		sleep(atoi(ipcnumstr)+i);
		continue;
	    } else
		break;
	}
	if (sock == -1) {
	    perror("pctool-listener: socket() failed");
	    exit(1);
	}
	name.sun_family = AF_UNIX;
	for (i=1; i<5; i++) {
	    if ((retval = connect(sock, (struct sockaddr *)&name,
		    sizeof(struct sockaddr_un)) == -1)) {
		sleep(atoi(ipcnumstr)+i);
		continue;
	    } else
		break;
	}
	if (retval == -1) {
	    printf("pctool-listener: connect() failed on %s while \
trying to send message '%s'\n", name.sun_path, info);
	    perror("pctool-listener: connect() failed");
	    exit(1);
	}
	write(sock, info, strlen(info));
}
