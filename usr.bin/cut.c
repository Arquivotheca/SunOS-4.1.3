/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)cut.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.11 */
#endif

/* cut : cut and paste columns of a table (projection of a relation) */
/* Release 1.5; handles single backspaces as produced by nroff    */
# include <stdio.h>	/* make: cc cut.c */
# include <ctype.h>
# define NFIELDS 1024	/* max no of fields or resulting line length */
# define BACKSPACE '\b'

int strcmp(), atoi();
void exit();


main(argc, argv)
int	argc;
char	**argv;
{
	extern int	optind;
	extern char	*optarg;
	static char	usage[] = "Usage: cut [-s] [-d<char>] {-c<list> | -f<list>} file ...";
	static char	cflist[] = "bad list for c/f option";
	register int	c;
	register char	*p1, *rbuf;
	register char	*p, *list;
	int	del = '\t';
	int	num, j, count, poscnt, r, s;
	int	endflag, supflag, cflag, fflag, backflag, filenr;
	static  int	sel[NFIELDS];
	char 	buf[NFIELDS];
	char	*p2, outbuf[NFIELDS];
	FILE	*inptr;
	supflag = cflag = fflag = r = num = s = 0;


	while((c = getopt(argc, argv, "c:d:f:s")) != EOF)
		switch(c) {
			case 'c':
				if (fflag)
					diag(usage);
				cflag++;
				list = optarg;
				break;
			case 'd':
				if (strlen(optarg) > 1)
					diag("no delimiter");
				else
					del = (int)*optarg;
				break;
			case 'f':
				if (cflag)
					diag(usage);
				fflag++;
				list = optarg;
				break;
			case 's':
				supflag++;
				break;
			case '?':
				diag(usage);
		}

	argv = &argv[optind];
	argc -= optind;

	if (!(cflag || fflag))
		diag(cflist);

	do {
		p = list;
		switch(*p) {
			case '-':
				if (r)
					diag(cflist);
				r = 1;
				if (num == 0)
					s = 1;
				else {
					s = num;
					num = 0;
				}
				break;
			case '\0' :
			case ','  :
				if (num >= NFIELDS)
					diag(cflist);
				if (r) {
					if (num == 0)
						num = NFIELDS - 1;
					if (num < s)
						diag(cflist);
					for (j = s; j <= num; j++)
						sel[j] = 1;
				} else
					sel[num] = (num > 0 ? 1 : 0);
				s = num = r = 0;
				if (*p == '\0')
					continue;
				break;
			default:
				if (!isdigit(*p))
					diag(cflist);
				num = atoi(p);
				while (isdigit(*list))
					list++;
				continue;
		}
		list++;
	}while (*p != '\0');
	for (j=0; j < NFIELDS && !sel[j]; j++);
	if (j >= NFIELDS)
		diag("no fields");

	filenr = 0;
	do {	/* for all input files */
		if ( argc == 0 || strcmp(argv[filenr],"-") == 0 )
			inptr = stdin;
		else
			if ((inptr = fopen(argv[filenr], "r")) == NULL) {
				fprintf(stderr, "cut: WARNING: cannot open ");
				perror(argv[filenr]);
				continue;
			}
		endflag = 0;
		do {	/* for all lines of a file */
			count = poscnt = backflag = 0;
			p1 = &outbuf[0] - 1 ;
			p2 = p1;
			rbuf = buf;
			if ((fgets(buf, NFIELDS, inptr)) == NULL) {
				if (ferror(inptr)) {
					fprintf(stderr, "cut: WARNING: I/O error reading ");
					if (inptr == stdin)
						perror("standard input");
					else
						perror(argv[filenr]);
				}
				endflag = 1;
				continue;
			}
			do { 	/* for all char of the line */
				if (rbuf >= &buf[NFIELDS])
					diag("line too long");
				if (*rbuf != '\n')
					*++p1 = *rbuf;
				if (cflag && (*rbuf == BACKSPACE))
					backflag++;
				else if (!backflag)
					poscnt += 1;
				else
					backflag--;
				if ( backflag > 1 )
					diag("cannot handle multiple adjacent backspaces\n");
				if (*rbuf == '\n' && count > 0  || *rbuf == del || cflag) {
					count += 1;
					if (fflag)
						poscnt = count;
					if (sel[poscnt])
						p2 = p1;
					else
						p1 = p2;
				}
			} while (*rbuf++ != '\n');
			if ( !endflag && (count > 0 || !supflag)) {
				if (*p1 == del && !sel[count])
					*p1 = '\0'; /*suppress trailing delimiter*/
				else
					*++p1 = '\0';
				puts(outbuf);
			}
		} while (!endflag);
		fclose(inptr);
	} while (++filenr < argc);
	exit(0);

	/* NOTREACHED */
}


diag(s)
char	*s;
{
	fprintf(stderr, "cut: ERROR: %s\n", s);
	exit(2);
}
