#ifndef lint
static	char sccsid[] = "@(#)uname.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

#include	<stdio.h>
#include	<sys/utsname.h>

struct utsname	unstr, *un;

main(argc, argv)
char **argv;
int argc;
{
	register i;
	int	sflg=1, nflg=0, rflg=0, vflg=0, mflg=0, errflg=0;
	int	optlet;

	un = &unstr;
	uname(un);

	while((optlet=getopt(argc, argv, "asnrvm")) != EOF) switch(optlet) {
	case 'a':
		sflg++; nflg++; rflg++; vflg++; mflg++;
		break;
	case 's':
		sflg++;
		break;
	case 'n':
		nflg++;
		break;
	case 'r':
		rflg++;
		break;
	case 'v':
		vflg++;
		break;
	case 'm':
		mflg++;
		break;
	case '?':
		errflg++;
	}
	if(errflg) {
		fprintf(stderr, "usage: uname [-snrvma]\n");
		exit(1);
	}
	if(nflg | rflg | vflg | mflg) sflg--;
	if(sflg)
		fprintf(stdout, "%.9s", un->sysname);
	if(nflg) {
		if(sflg) putchar(' ');
		fprintf(stdout, "%.9s", un->nodename);
	}
	if(rflg) {
		if(sflg | nflg) putchar(' ');
		fprintf(stdout, "%.9s", un->release);
	}
	if(vflg) {
		if(sflg | nflg | rflg) putchar(' ');
		fprintf(stdout, "%.9s", un->version);
	}
	if(mflg) {
		if(sflg | nflg | rflg | vflg) putchar(' ');
		fprintf(stdout, "%.9s", un->machine);
	}
	putchar('\n');
	exit(0);
	/* NOTREACHED */
}
