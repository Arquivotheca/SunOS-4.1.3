#ifndef lint
static	char sccsid[] = "@(#)prxf.c 1.1 92/07/30 SMI"; /* from UCB 4.11 83/05/19 */
#endif

/*
 * 	filter which reads the output of nroff and converts lines
 *	with ^H's to overwritten lines.  Thus this works like 'ul'
 *	modified by kls to use register references instead of arrays
 *	to try to gain a little speed.
 *	based on lpf but modified for special requirements of
 *	Printronix printer, such as can only overstrike underlines.
 */

#include <stdio.h>
#include <signal.h>

#define MAXWIDTH  132
#define MAXREP    2

char	buf[MAXREP][MAXWIDTH];
int	maxcol[MAXREP] = {-1};
int	lineno;
int	width = 132;	/* default line length */
int	length = 66;	/* page length */
int	indent;		/* indentation length */
int	npages = 1;
int	literal;	/* print control characters */
char	*name;		/* user's login name */
char	*host;		/* user's machine name */
char	*acctfile;	/* accounting information file */

main(argc, argv) 
	int argc;
	char *argv[];
{
	register FILE *p = stdin, *o = stdout;
	register int i, col;
	register char *cp, *cp0;
	int done, linedone, maxrep;
	int big = 0;
	int ch;
	char *limit;

	while (--argc) {
		if (*(cp = *++argv) == '-') {
			switch (cp[1]) {
			case 'n':
				argc--;
				name = *++argv;
				break;

			case 'h':
				argc--;
				host = *++argv;
				break;

			case 'w':
				if ((i = atoi(&cp[2])) > 0 && i <= MAXWIDTH)
					width = i;
				break;

			case 'l':
				length = atoi(&cp[2]);
				break;

			case 'i':
				indent = atoi(&cp[2]);
				break;

			case 'c':	/* Print control chars */
				literal++;
				break;
			}
		} else
			acctfile = cp;
	}

	for (cp = buf[0], limit = buf[MAXREP]; cp < limit; *cp++ = ' ');
	done = 0;
	
	while (!done) {
		col = indent;
		maxrep = -1;
		linedone = 0;
		while (!linedone) {
			switch (ch = getc(p)) {
			case EOF:
				linedone = done = 1;
				ch = '\n';
				break;

			case '\f':
				lineno = length;
			case '\n':
				if (maxrep < 0)
					maxrep = 0;
				linedone = 1;
				break;

			case '\b':
				if (--col < indent)
					col = indent;
				break;

			case '\r':
				col = indent;
				break;

			case '\t':
				col = ((col - indent) | 07) + indent + 1;
				break;

			case '\007':
				/* Do double height characters */
				big = 1;
				break;

			case '\031':
				/*
				 * lpd needs to use a different filter to
				 * print data so stop what we are doing and
				 * wait for lpd to restart us.
				 */
				if ((ch = getchar()) == '\1') {
					fflush(stdout);
					kill(getpid(), SIGSTOP);
					break;
				} else {
					ungetc(ch, stdin);
					ch = '\031';
				}

			default:
				if (col >= width || !literal && ch < ' ') {
					col++;
					break;
				}
				cp = cp0 = &buf[0][col];
				for (i = 0; i < MAXREP; i++) {
					if (i > maxrep)
						maxrep = i;
					if (*cp == ' ') {
						if (*cp0 == '_') {
							*cp = *cp0;
							*cp0 = ch;
						} else
							*cp = ch;
						if (col > maxcol[i])
							maxcol[i] = col;
						break;
					}
					cp += MAXWIDTH;
				}
				col++;
				break;
			}
		}

		/* print out lines */
		if (big) {
			putc('\010', o);
			big = 0;
		}
		for (i = 0; i <= maxrep; i++) {
			for (cp = buf[i], limit = cp+maxcol[i]; cp <= limit;) {
				putc(*cp, o);
				*cp++ = ' ';
			}
			if (i < maxrep)
				putc('\r', o);
			else
				putc(ch, o);
			if (++lineno >= length) {
				npages++;
				lineno = 0;
			}
			maxcol[i] = -1;
		}
	}
	if (lineno) {		/* be sure to end on a page boundary */
		putchar('\f');
		npages++;
	}
	if (name && acctfile && access(acctfile, 02) >= 0 &&
	    freopen(acctfile, "a", stdout) != NULL) {
		printf("%7.2f\t%s:%s\n", (float)npages, host, name);
	}
	exit(0);
	/* NOTREACHED */
}
