/*	@(#)rfudaemon.c 1.1 92/07/30 SMI 	*/


/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rfudaemon:rfudaemon.c	1.4.1.1"
/*
 *	User-level daemon for file sharing.
 *	Do syscall to wait for messages.
 *	When we get one, exec admin program
 *	and go back for more.
 */
#include <sys/types.h>
#include <rfs/rfsys.h>
#include <sys/signal.h>
#include <stdio.h>
#include <errno.h>

#define DATASIZE 512
extern int errno;

main(argc,argv)
int argc;
char **argv;
{
	char	cmd[DATASIZE];
	char	buf[DATASIZE];
	int	i;
	char	*tp;

	signal(SIGHUP,  SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT,  SIG_IGN);

	for (;;) {
		/* clear buffer */
		for (tp = buf, i = 0; i < DATASIZE; i++)
			*tp++ = '\0';

		strcpy(cmd, "rfuadmin ");

		switch (rfsys(RF_GETUMSG, buf, DATASIZE)) {
		case RF_GETUMSG:
			break;
		case RF_FUMOUNT:
			strcat(cmd, "fumount ");
			break;
		case RF_DISCONN:
			strcat(cmd, "disconnect ");
			break;
		case RF_LASTUMSG:
			exit(0);
		default:
			strcat(cmd, "error : rfsys in rfudaemon failed");
			break;
		}
		strcat(cmd, buf);
		system(cmd);
	}
}
