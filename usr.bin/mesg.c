#ifndef lint
static	char sccsid[] = "@(#)mesg.c 1.1 92/07/30 SMI"; /* from UCB 4.2 10/18/80 */
#endif
/*
 * mesg -- set current tty to accept or
 *	forbid write permission.
 *
 *	mesg [y] [n]
 *		y allow messages
 *		n forbid messages
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

struct stat sbuf;

char *tty;
char *ttyname();

main(argc, argv)
char *argv[];
{
	int i, r=0;

	for (i = 0; i <= 2; i++) {
		if ((tty = ttyname(i)) != NULL)
			break;
	}

	if (stat(tty, &sbuf) < 0)
		error("cannot stat");

	if(argc < 2) {
		if (sbuf.st_mode & 020)
			printf("y\n");
		else {	
			r=1;
			printf("is n\n");
		}
	} else	switch(*argv[1]) {
		case 'y':
			newmode(sbuf.st_mode|020); break;

		case 'n':
			newmode(sbuf.st_mode&~020); r=1; break;

		default:
			error("usage: mesg [y] [n]");
		}
	exit(r);

	/* NOTREACHED */
}

error(s)
char *s;
{
	fprintf(stderr,"mesg: %s\n",s);
	exit(2);
}

newmode(m)
{
	if(chmod(tty,m)<0)
		error("cannot change mode");
}
