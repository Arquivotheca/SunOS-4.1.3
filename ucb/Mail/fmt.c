#ifndef lint
static	char sccsid[] = "@(#)fmt.c 1.1 92/07/30 SMI"; /* from UCB 2.1 07/01/81 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <locale.h>

/*
 * fmt -- format the concatenation of input files or standard input
 * onto standard output.  Designed for use with Mail ~|
 *
 * Syntax: fmt [ -width ] [ -c ] [ name ... ]
 * Author: Kurt Shoens (UCB) 12/7/78
 */

#define	NOSTR	((char *) 0)	/* Null string pointer for lint */

char	outbuf[BUFSIZ];			/* Sandbagged output line image */
char	*outp;				/* Pointer in above */
int	filler;				/* Filler amount in outbuf */

int	pfx;			/* Current leading blank count */
int	lineno;			/* Current input line */
int	mark;			/* Last place we saw a head line */
int	width = 72;		/* Width that we will not exceed */
int	nojoin = 0;		/* split lines only, don't join short ones */

enum crown_type	{c_none, c_reset, c_head, c_lead, c_fixup, c_body};
enum crown_type	crown_state;	/* Crown margin state */
int	crown_head;		/* The header offset */
int	crown_body;		/* The body offset */

char	*calloc();		/* for lint . . . */
char	*headnames[] = {"To", "Subject", "Cc", "cc", "Bcc", "bcc", 0};

int	debug;			/* to resolve reference in head.c */

/*
 * Drive the whole formatter by managing input files.  Also,
 * cause initialization of the output stuff and flush it out
 * at the end.
 */

main(argc, argv)
	char **argv;
{
	register FILE *fi;
	register int errs = 0;
	char sobuf[BUFSIZ];
	register char *cp;
	int nofile;

	setout();
	lineno = 1;
	mark = -10;
	setbuf(stdout, sobuf);
	setlocale(LC_CTYPE,"");
	if (argc < 2) {
single:
		fmt(stdin);
		oflush();
		exit(0);
	}
	nofile = 1;
	while (--argc) {
		cp = *++argv;
		if (setopt(cp))
			continue;
		nofile = 0;
		if ((fi = fopen(cp, "r")) == NULL) {
			perror(cp);
			errs++;
			continue;
		}
		fmt(fi);
		fclose(fi);
	}
	if (nofile)
		goto single;
	oflush();
	exit(errs);
	/* NOTREACHED */
}

/*
 * Read up characters from the passed input file, forming lines,
 * doing ^H processing, expanding tabs, stripping trailing blanks,
 * and sending each line down for analysis.
 */

fmt(fi)
	FILE *fi;
{
	char linebuf[BUFSIZ], canonb[BUFSIZ];
	register char *cp, *cp2;
	register int c, col;

	c = getc(fi);
	while (c != EOF) {
		
		/*
		 * Collect a line, doing ^H processing.
		 * Leave tabs for now.
		 */

		cp = linebuf;
		while (c != '\n' && c != EOF && cp-linebuf < BUFSIZ-1) {
			if (c == '\b') {
				if (cp > linebuf)
					cp--;
				c = getc(fi);
				continue;
			}
			if (!(isprint(c)) && c != '\t') {
				c = getc(fi);
				continue;
			}
			*cp++ = c;
			c = getc(fi);
		}
		*cp = '\0';

		/*
		 * Toss anything remaining on the input line.
		 */

		while (c != '\n' && c != EOF)
			c = getc(fi);
		
		/*
		 * Expand tabs on the way to canonb.
		 */

		col = 0;
		cp = linebuf;
		cp2 = canonb;
		while (c = *cp++) {
			if (c != '\t') {
				col++;
				if (cp2-canonb < BUFSIZ-1)
					*cp2++ = c;
				continue;
			}
			do {
				if (cp2-canonb < BUFSIZ-1)
					*cp2++ = ' ';
				col++;
			} while ((col & 07) != 0);
		}

		/*
		 * Swipe trailing blanks from the line.
		 */

		for (cp2--; cp2 >= canonb && *cp2 == ' '; cp2--)
			;
		*++cp2 = '\0';
		prefix(canonb);
		if (c != EOF)
			c = getc(fi);
	}
}

/*
 * Take a line devoid of tabs and other garbage and determine its
 * blank prefix.  If the indent changes, call for a linebreak.
 * If the input line is blank, echo the blank line on the output.
 * Finally, if the line minus the prefix is a mail header, try to keep
 * it on a line by itself.
 */

prefix(line)
	char line[];
{
	register char *cp, **hp;
	register int np, h;
	register int i;

	if (strlen(line) == 0) {
		oflush();
		putchar('\n');
		if (crown_state != c_none)
			crown_state = c_reset;
		return;
	}
	for (cp = line; *cp == ' '; cp++)
		;
	np = cp - line;

	/*
	 * The following horrible expression attempts to avoid linebreaks
	 * when the indent changes due to a paragraph.
	 */

	if (crown_state == c_none && np != pfx && (np > pfx || abs(pfx-np) > 8))
		oflush();
	if (h = ishead(cp))
		oflush(), mark = lineno;
	if (lineno - mark < 7 && lineno - mark > 0)
		/*
		 * make this check for 7 lines because some users like to put
		 * cc's on multiple lines 
		 */
		for (hp = &headnames[0]; *hp != (char *) 0; hp++)
			if (ispref(*hp, cp)) {
				h = 1;
				oflush();
				break;
			}
	if (nojoin) {
		h = 1;
		oflush();
	}
	if (!h && (h = (*cp == '.')))
		oflush();
	pfx = np;
	switch (crown_state) {
	case c_reset:	
		crown_head = pfx;
		crown_state = c_head;
		break;
	case c_lead:
		crown_body = pfx;
		crown_state = c_body;
		break;
	case c_fixup:
		crown_body = pfx;
		crown_state = c_body;
		if (outp) {
			char s[BUFSIZ];

			*outp = '\0';
			strcpy(s, &outbuf[crown_head]);
			outp = NOSTR;
			split(s);
		}
	}
	split(cp);
	if (h)
		oflush();
	lineno++;
}

/*
 * Split up the passed line into output "words" which are
 * maximal strings of non-blanks with the blank separation
 * attached at the end.  Pass these words along to the output
 * line packer.
 */

split(line)
	char line[];
{
	register char *cp, *cp2;
	char word[BUFSIZ];

	cp = line;
	while (*cp) {
		cp2 = word;

		/*
		 * Collect a 'word,' allowing it to contain escaped
		 * white space.
		 */

		while (*cp && *cp != ' ') {
			if (*cp == '\\' && isspace(cp[1]))
				*cp2++ = *cp++;
			*cp2++ = *cp++;
		}

		/*
		 * Guarantee a space at end of line.
		 * Two spaces after end of sentence punctuation.
		 */

		if (*cp == '\0') {
			*cp2++ = ' ';
			if (any(cp[-1], ".:!?"))
				*cp2++ = ' ';
		}
		while (*cp == ' ')
			*cp2++ = *cp++;
		*cp2 = '\0';
		pack(word);
	}
}

/*
 * Output section.
 * Build up line images from the words passed in.  Prefix
 * each line with correct number of blanks.  The buffer "outbuf"
 * contains the current partial line image, including prefixed blanks.
 * "outp" points to the next available space therein.  When outp is NOSTR,
 * there ain't nothing in there yet.  At the bottom of this whole mess,
 * leading tabs are reinserted.
 */

/*
 * Initialize the output section.
 */

setout()
{
	outp = NOSTR;
}

/*
 * Pack a word onto the output line.  If this is the beginning of
 * the line, push on the appropriately-sized string of blanks first.
 * If the word won't fit on the current line, flush and begin a new
 * line.  If the word is too long to fit all by itself on a line,
 * just give it its own and hope for the best.
 */

pack(word)
	char word[];
{
	register char *cp;
	register int s, t;

	if (outp == NOSTR)
		leadin();
	t = strlen(word);
	s = outp-outbuf;
	if (t+s <= width) {
		
		/*
		 * In like flint!
		 */

		for (cp = word; *cp; *outp++ = *cp++)
			;
		return;
	}
	if (s > filler) {
		oflush();
		leadin();
	}
	for (cp = word; *cp; *outp++ = *cp++)
		;
}

/*
 * If there is anything on the current output line, send it on
 * its way.  Set outp to NOSTR to indicate the absence of the current
 * line prefix.
 */

oflush()
{
	if (outp == NOSTR)
		return;
	*outp = '\0';
	tabulate(outbuf);
	outp = NOSTR;
}

/*
 * Take the passed line buffer, insert leading tabs where possible, and
 * output on standard output (finally).
 */

tabulate(line)
	char line[];
{
	register char *cp, *cp2;
	register int b, t;

	/*
	 * Toss trailing blanks in the output line.
	 */

	cp = line + strlen(line) - 1;
	while (cp >= line && *cp == ' ')
		cp--;
	*++cp = '\0';
	
	/*
	 * Count the leading blank space and tabulate.
	 */

	for (cp = line; *cp == ' '; cp++)
		;
	b = cp-line;
	t = b >> 3;
	b &= 07;
	if (t > 0)
		do
			putc('\t', stdout);
		while (--t);
	if (b > 0)
		do
			putc(' ', stdout);
		while (--b);
	while (*cp)
		putc(*cp++, stdout);
	putc('\n', stdout);
}

/*
 * Initialize the output line with the appropriate number of
 * leading blanks.
 */

leadin()
{
	register int b;
	register char *cp;
	register int l;

	switch (crown_state) {
	case c_head:
		l = crown_head;
		crown_state = c_lead;
		break;

	case c_lead:
	case c_fixup:
		l = crown_head;
		crown_state = c_fixup;
		break;

	case c_body:
		l = crown_body;
		break;

	default:
		l = pfx;
		break;
	}
	filler = l;
	for (b = 0, cp = outbuf; b < l; b++)
		*cp++ = ' ';
	outp = cp;
}

/*
 * Save a string in dynamic space.
 * This little goodie is needed for
 * a headline detector in head.c
 */

char *
savestr(str)
	char str[];
{
	register char *top;

	top = calloc(strlen(str) + 1, 1);
	if (top == NOSTR) {
		fprintf(stderr, "fmt:  Ran out of memory\n");
		exit(1);
	}
	copy(str, top);
	return(top);
}

/*
 * Is s1 a prefix of s2??
 */

ispref(s1, s2)
	register char *s1, *s2;
{

	while (*s1 != '\0' && *s2 != '\0')
		if (*s1++ != *s2++)
			return (0);
	return (1);
}

/*
 * Set an input option
 */

setopt(cp)
	register char *cp;
{
	if (*cp != '-')
		return (0);
	if (cp[1] == 'c' && cp[2] == '\0') {
		crown_state = c_reset;
		return (1);
	}
	if (cp[1] == 's' && cp[2] == '\0') {
		nojoin = 1;
		return (1);
	}
	width = atoi(cp+1);
	if (width <= 0 || width >= BUFSIZ-2) {
		fprintf(stderr, "fmt:  bad width: %d\n", width);
		exit(1);
	}
	return(1);
}
