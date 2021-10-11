#ifndef lint
static	char sccsid[] = "@(#)time.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*
**	Time a command
*/

#include	<stdio.h>
#include	<signal.h>
#include	<errno.h>
#include	<sys/param.h>		/* HZ defined here */

main(argc, argv)
char **argv;
{
	struct {
		long user;
		long sys;
		long childuser;
		long childsys;
	} buffer;

	register p;
	extern	errno;
	extern	char	*sys_errlist[];
	int	status;
	long	before, after;
	extern long times();

	before = times(&buffer);
	if(argc<=1)
		exit(0);
	p = fork();
	if(p == -1) {
		fprintf(stderr,"time: cannot fork -- try again.\n");
		exit(2);
	}
	if(p == 0) {
/*		close(1);	lem commented this out	*/
		execvp(argv[1], &argv[1]);
	        fprintf(stderr, "%s: %s\n", sys_errlist[errno], argv[1]);
		exit(2);
	}
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while(wait(&status) != p);
	if((status & 0377) != '\0')
		fprintf(stderr,"time: command terminated abnormally.\n");
	after = times(&buffer);
	fprintf(stderr,"\n");
	printt("real", (after-before));
	printt("user", buffer.childuser);
	printt("sys ", buffer.childsys);
	exit(status >> 8);
	/* NOTREACHED */
}

/*
The following use of HZ/10 will work correctly only if HZ is a multiple
of 10.  However the only values for HZ now in use are 100 for the 3B
and 60 for other machines.
*/
char quant[] = { HZ/10, 10, 10, 6, 10, 6, 10, 10, 10 };
char *pad  = "000      ";
char *sep  = "\0\0.\0:\0:\0\0";
char *nsep = "\0\0.\0 \0 \0\0";

printt(s, a)
char *s;
long a;
{
	register i;
	char	digit[9];
	char	c;
	int	nonzero;

	for(i=0; i<9; i++) {
		digit[i] = a % quant[i];
		a /= quant[i];
	}
	fprintf(stderr,s);
	nonzero = 0;
	while(--i>0) {
		c = digit[i]!=0 ? digit[i]+'0':
		    nonzero ? '0':
		    pad[i];
		if (c != '\0')
			putc (c, stderr);
		nonzero |= digit[i];
		c = nonzero?sep[i]:nsep[i];
		if (c != '\0')
			putc (c, stderr);
	}
	fprintf(stderr,"\n");
}
