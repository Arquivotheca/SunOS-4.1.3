#ifndef lint
static	char sccsid[] = "@(#)tee.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

/*
 * tee-- pipe fitting
 */

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
int *openf;
char **namef;
int n = 0;
int t = 0;
int errflg;
int aflag;

#define	TEEBUFSIZ	8192

char in[TEEBUFSIZ];

extern errno;
long	lseek();
char	*malloc();

main(argc,argv)
char **argv;
{
	int dtablesize;
	int register w;
	extern int optind;
	int c;
	struct stat buf;
	while ((c = getopt(argc, argv, "ai")) != EOF)
		switch(c) {
			case 'a':
				aflag++;
				break;
			case 'i':
				signal(SIGINT, SIG_IGN);
				break;
			case '?':
				errflg++;
		}
	if (errflg) {
		fprintf(stderr, "usage: tee [ -i ] [ -a ] [file ] ...\n");
		exit(2);
	}
	argc -= optind;
	argv = &argv[optind];
	if (argc > 0 && strcmp(argv[0], "-") == 0) {
		/*
		 * An undocumented feature of the old version was that
		 * "tee -" disabled interrupts like "tee -i".
		 */
		signal(SIGINT, SIG_IGN);
		argc--;
		argv++;
	}
	dtablesize = getdtablesize();
	if ((openf = (int *)malloc(dtablesize * sizeof (int))) == NULL
	    || (namef = (char **)malloc(dtablesize * sizeof (char *))) == NULL) {
		perror("tee");
		exit(1);
	}
	openf[n] = 1;
	namef[n] = "standard output";
	n++;
	fstat(1,&buf);
	t = (buf.st_mode&S_IFMT)==S_IFCHR;
	if(lseek(1,0L,1)==-1&&errno==ESPIPE)
		t++;
	while(argc-->0) {
		openf[n] = open(argv[0],(aflag?O_WRONLY|O_CREAT|O_APPEND:
			O_WRONLY|O_CREAT|O_TRUNC), 0666);
		namef[n] = argv[0];
		if(openf[n] < 0) {
			fprintf(stderr, "tee: ");
			perror(argv[0]);
		} else {
			if(fstat(openf[n],&buf) < 0) {
				fprintf(stderr, "tee: cannot stat ");
				perror(argv[0]);
			} else {
				n++;
				if((buf.st_mode&S_IFMT)==S_IFCHR)
					t++;
			}
		}
		argv++;
	}
	while ((w = read(0, in, TEEBUFSIZ)) > 0)
		stash(w);
	if(w < 0) {
		perror("tee: read error on input");
		exit(1);
	}
	exit(0);
	/* NOTREACHED */
}

stash(p)
{
	int k;
	int i;
	int d;
	d = t ? 16 : p;
	for(i=0; i<p; i+=d)
		for(k=0;k<n;k++)
			if(write(openf[k], in+i, d<p-i?d:p-i) < 0) {
				fprintf(stderr, "tee: ");
				perror(namef[k]);
				exit(1);
			}
}
