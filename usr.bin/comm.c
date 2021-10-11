#ifndef lint
static	char sccsid[] = "@(#)comm.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*
**	process common lines of two files
*/

#include	<stdio.h>

#define	LB	256

int	one;
int	two;
int	three;

char	*ldr[3];

FILE	*ib1;
FILE	*ib2;
FILE	*openfil();

main(argc,argv)
char **argv;
{
	int	l;
	char	lb1[LB],lb2[LB];

	ldr[0] = "";
	ldr[1] = "\t";
	ldr[2] = "\t\t";
	if(argc > 1)  {
		if(*argv[1] == '-' && argv[1][1] != 0) {
			l = 1;
			while(*++argv[1]) {
				switch(*argv[1]) {
				case '1':
					if(!one) {
						one = 1;
						ldr[1][0] = ldr[2][l--] = '\0';
					}
					break;

				case '2':
					if(!two) {
						two = 1;
						ldr[2][l--] = '\0';
					}
					break;

				case '3':
					three = 1;
					break;

				default:
					usage();
				}
			}
			argv++;
			argc--;
		}
	}

	if(argc < 3)
		usage();
	ib1 = openfil(argv[1]);
	ib2 = openfil(argv[2]);
	if(rd(ib1,lb1) < 0) {
		if(rd(ib2,lb2) < 0)
			exit(0);
		copy(ib2,lb2,2);
	}
	if(rd(ib2,lb2) < 0)
		copy(ib1, lb1, 1);
	while(1) {
		switch(compare(lb1,lb2)) {
			case 0:
				wr(lb1,3);
				if(rd(ib1,lb1) < 0) {
					if(rd(ib2,lb2) < 0)
						exit(0);
					copy(ib2,lb2,2);
				}
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;

			case 1:
				wr(lb1,1);
				if(rd(ib1,lb1) < 0)
					copy(ib2, lb2, 2);
				continue;

			case 2:
				wr(lb2,2);
				if(rd(ib2,lb2) < 0)
					copy(ib1, lb1, 1);
				continue;
		}
	}
}

rd(file,buf)
FILE *file;
char *buf;
{

	register int i, j;
	i = j = 0;
	while((j = getc(file)) != EOF) {
		*buf = j;
		if(j == '\n' || i > LB-2) {
			*buf = '\0';
			return(0);
		}
		i++;
		buf++;
	}
	return(-1);
}

wr(str,n)
char *str;
{
	switch(n) {
		case 1:
			if(one)
				return;
			break;

		case 2:
			if(two)
				return;
			break;

		case 3:
			if(three)
				return;
	}
	printf("%s%s\n",ldr[n-1],str);
}

copy(ibuf,lbuf,n)
FILE *ibuf;
char *lbuf;
{
	do {
		wr(lbuf,n);
	} while(rd(ibuf,lbuf) >= 0);

	exit(0);
}

compare(a,b)
char *a,*b;
{
	register char *ra,*rb;

	ra = --a;
	rb = --b;
	while(*++ra == *++rb)
		if(*ra == '\0')
			return(0);
	if(*ra < *rb)
		return(1);
	return(2);
}
FILE *openfil(s)
char *s;
{
	FILE *b;
	if(s[0]=='-' && s[1]==0)
		b = stdin;
	else if((b=fopen(s,"r")) == NULL) {
		fprintf(stderr,"comm: cannot open %s: ",s);
		perror("");
		exit(2);
	}
	return(b);
}

usage()
{
	fprintf(stderr, "usage: comm [ - [123] ] file1 file2\n");
	exit(2);
}
