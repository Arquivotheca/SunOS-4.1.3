/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)unknown.c 1.1 92/07/30"	/* from SVR3.2 uucp:unknown.c 1.1 */
/*
 *	logs attempts by unknown remote machines to run uucico in FOREIGN
 *	("/usr/spool/uucp/.Admin/Foreign").  if anything goes wrong,
 *	sends mail to login MAILTO ("uucp").  the executable should be 
 *	placed in /usr/lib/uucp/remote.unknown, and should run setuid-uucp.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<time.h>

#define	FOREIGN	"/usr/spool/uucp/.Admin/Foreign"
#define	MAILTO	"uucp"
#define	LOGLEN	256

main(argc, argv)
int	argc;
char	*argv[];
{
	char		buf[LOGLEN], *ctoday;
	FILE		*fp;
	time_t		today;
	extern int	errno;
	extern char	*ctime();
	extern FILE	*fopen();

	if ( argc != 2 ) {
		fputs("USAGE: %s remotename\n", argv[0], stderr);
		exit(101);
	}

	if ( time(&today) != -1 ) {
		ctoday = ctime(&today);
		*(ctoday + strlen(ctoday) - 1) = '\0';	/* no ending \n */
	} else
		ctoday = "NO DATE";

	sprintf(buf, "%s: call from system %s login %s\n",
		ctoday, argv[1], getlogin() );

	errno = 0;
	if ( (fp = fopen(FOREIGN, "a+")) == (FILE *)NULL )
		fall_on_sword("cannot open", buf);
	if ( fputs(buf, fp) == EOF )
		fall_on_sword("cannot write", buf);
	if ( fclose(fp) != 0 )
		fall_on_sword("cannot close", buf);

	exit(0);

	/* NOTREACHED */
}

/* don't return from here */
fall_on_sword(errmsg, logmsg)
char	*errmsg, *logmsg;
{
	char		ebuf[BUFSIZ];
	int		fds[2];
	extern int	errno, sys_nerr;
	extern char	*sys_errlist[];

	sprintf(ebuf, "%s %s:\t%s (%d)\nlog msg:\t%s",
		errmsg, FOREIGN,
		( errno < sys_nerr ? sys_errlist[errno] : "Unknown error " ),
		errno, logmsg);

	/* reset to real uid. get a pipe. put error message on	*/
	/* "write end" of pipe, close it. dup "read end" to	*/
	/* stdin and then execl mail (which will read the error	*/
	/* message we just wrote).				*/

	if ( setuid(getuid()) == -1 || pipe(fds) != 0 
	|| write(fds[1], ebuf, strlen(ebuf)) != strlen(ebuf)
	|| close(fds[1]) != 0 )
		exit(errno);

	if ( fds[0] != 0 ) {
		close(0);
		if ( dup(fds[0]) != 0 )
			exit(errno);
	}

	execl("/bin/mail", "mail", MAILTO, (char *)NULL);
	exit(errno);	/* shouldn't get here */
}
