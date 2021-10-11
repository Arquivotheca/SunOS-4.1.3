#ifndef lint
static	char *sccsid = "@(#)mknod.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

main(argc, argv)
int argc;
char **argv;
{
	register  int  m, a, b;

	if(argc == 3 && !strcmp(argv[2], "p")) { /* fifo */
		if (mknod(argv[1], S_IFIFO|0666, 0)) {
			perror("mknod");
			exit(2);
		}
		exit(0);
	}
	if(argc != 5) {
		fprintf(stderr,"mknod: arg count\n");
		usage();
	}
	if(geteuid()) {
		fprintf(stderr, "mknod: must be super-user\n");
		exit(2);
	}

	if(*argv[2] == 'b')
		m = S_IFBLK|0666;
	else if(*argv[2] == 'c')
		m = S_IFCHR|0666;
	else
		usage();
	a = number(argv[3]);
	if(a < 0)
		usage();
	b = number(argv[4]);
	if(b < 0)
		usage();
	if(mknod(argv[1], m, (a<<8)|b) < 0)
		perror("mknod");
	exit(0);
	/* NOTREACHED */
}

number(s)
register  char  *s;
{
	register  int  n, c;

	n = 0;
	if(*s == '0') {
		while(c = *s++) {
			if(c < '0' || c > '7')
				return(-1);
			n = n * 8 + c - '0';
		}
	} else {
		while(c = *s++) {
			if(c < '0' || c > '9')
				return(-1);
			n = n * 10 + c - '0';
		}
	}
	return(n);
}
usage()
{
	fprintf(stderr,"usage: mknod name b/c major minor\n");
	fprintf(stderr,"   OR: mknod name p\n");
	exit(2);
}
