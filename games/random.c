#ifndef lint
static	char sccsid[] = "@(#)random.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include <stdio.h>
#define MAXINT 2147483648.
float fract = 2;
double	atof();
char rflag,eflag,c;
char ibuf[BUFSIZ],obuf[BUFSIZ],line[BUFSIZ];
main(argc,argv) char **argv;
{
	long tvec;
	int i;
	for(i=1;i<argc;i++)
	{	if(*argv[i]!='-')
			fract=atof(argv[i]);
		else if((c=argv[i][1])=='e')
			eflag=1;
		else if(c=='r')
			rflag=1;
	}
	time(&tvec);
	srand((int)tvec);
	if(!rflag && !eflag)
	{	setbuf(stdin,ibuf);
		setbuf(stdout,obuf);
	}
	for(;eflag==0;)
	{	gets(line);
		if(feof(stdin)) break;
		if(rand()/MAXINT*fract<1) puts(line);
	}
	exit((int)(rand()/MAXINT*fract));
	/* NOTREACHED */
}
