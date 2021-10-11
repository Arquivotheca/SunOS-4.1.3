/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)otic.c 1.1 92/07/30 SMI"; /* from S5R3 1.6 */
#endif

#ifdef pdp11
	/* Has to be this small to fit, even split I/D, with several use='s */
# define CAPSIZ	2048
#else
# define CAPSIZ	8192		/* 4096 */
#endif

#define MAXHOP	32	/* max number of use= indirections */

#define COMMENT		'#'
#define CAPCOMMENT	'.'
#define SEPARATE	','	/* Separates the capabilities. */
#define	NAMESEPARATOR	'|'	/* Separates the names in the list. */
#define NUMBER		'#'
#define STRING		'='
#define CANCEL		'@'

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unctrl.h>

#include "uparm.h"
#ifndef E_TERMINFO
#define E_TERMINFO "terminfo.src"
#endif

#ifndef termpath
#ifdef S5EMUL
#define termpath(name) "/usr/5lib/terminfo/name"
#else
#define termpath(name) "/usr/lib/terminfo/name"
#endif
#endif

/*
 * L_ctermid is only defined on USG.
 * We use it here since we don't include curses.h.
 */
#if defined(L_ctermid) && !defined(sun)
#define index strchr
#endif

/*
 * compile: program to compile a source terminfo file into object files
 */

static char *tbuf;
static int hopcount;	/* detect infinite loops in terminfo, init 0 */
static long starttime;
static int verbose;
static char *progname;
static char *filename;
static int linenumber;
static char *terminfo;

char	*tskip();
char	*tgetstr();
char	*tdecode();

extern char *boolnames[], *numnames[], *strnames[];
extern char *boolcodes[], *numcodes[], *strcodes[];

extern char *malloc();
extern char *getenv();
extern char *index();

main(argc, argv)
char **argv;
{
	int i;

	progname = argv[0];
	initcheck();
	time(&starttime);
	while (argc > 1 && argv[1][0] == '-') {
		switch(argv[1][1]) {
		case 'v':
			if (argv[1][2])
				verbose = argv[1][2] - '0';
			else
				verbose++;
			break;
		default:
			(void) fprintf(stderr, "Usage: %s [-v] [files...]\n",
				progname);
			exit(1);
		}
		argc--; argv++;
	}

	terminfo = getenv("TERMINFO");

	if (argc == 1)
		compfile(stdin, "stdin");
	else for (i=1; i<argc; i++)
		compfile(fopen(argv[i], "r"), argv[i]);
	return 0;
}

/*
 * Compile a file.  This is very similar to the
 * code in tgetstr but it passes through the whole file.
 */
compfile(tf, fname)
	FILE *tf;
	char *fname;
{
	register char *cp;
	char bp[CAPSIZ];
	char ibuf[CAPSIZ];

	if (tf == NULL) {
		perror(fname);
		return;
	}
	ibuf[0] = 0;
	linenumber = 0;
	filename = fname;
	for (;;) {
		tbuf = bp;
		strcpy(bp, ibuf);
		hopcount = 0;
		for (;;) {
			if (fgets(ibuf, sizeof ibuf, tf) == NULL) {
				if (ferror(tf)) {
					perror(fname);
					fclose(tf);
					return;
				}
				fclose(tf);
				checknames(tbuf);
				if (*bp && tnchkuse(fname))
					store(bp);
				return;
			}
			linenumber++;
			/* comment or blank line */
			if (ibuf[0] == COMMENT || ibuf[0] == '\n')
				continue;
			cp = &ibuf[strlen(ibuf)-3];
			/* Allow and ignore old style backslashes */
			if (*cp == SEPARATE && cp[1] == '\\')
				cp[1] = 0;
			cp[2] = 0;	/* get rid of newline */
			/* lines with leading white space are continuation */
			if (!isspace(ibuf[0]) && *bp)
				break;
			if (strlen(bp) + strlen(ibuf) >= CAPSIZ)
				(void) fprintf(stderr,
					"%s: Terminfo entry too long:\n%s",
					progname, bp);
			else {
				cp = ibuf;
				while (isspace(*cp))
					cp++;
				strcat(bp, cp);
			}
		}

		/*
		 * We have it, now do something with it.
		 */
		checknames(tbuf);
		if (tnchkuse(fname))
			store(bp);
	}
}

/*
 * Code to check names in the buffer against list of valid names.
 * This is done against the unexpanded form (use= entries are still there)
 * to avoid redundant messages.
 */

static short *sortedbools, *sortednums, *sortedstrs;
static int numbools, numnums, numstrs;

static int
cmpbools(left, right)
short *left, *right;
{
    return strcmp(boolnames[*left], boolnames[*right]);
}

static int
cmpnums(left, right)
short *left, *right;
{
    return strcmp(numnames[*left], numnames[*right]);
}

static int
cmpstrs(left, right)
short *left, *right;
{
    return strcmp(strnames[*left], strnames[*right]);
}

initcheck()
{
	register int i, j;
	register char **pp;
	for (i = 0, pp = boolnames; *pp; pp++, i++)
		;
	numbools = i;
	sortedbools = (short *) malloc(i * sizeof(short));
	if (!sortedbools) {
		(void) fprintf (stderr, "%s: malloc failed!\n", progname);
		exit(1);
	}
	for (j = 0; j < i; j++)
	    sortedbools[j] = j;
	qsort(sortedbools, numbools, sizeof(short), cmpbools);

	for (i = 0, pp = numnames; *pp; pp++, i++)
		;
	numnums = i;
	sortednums = (short *) malloc(i * sizeof(short));
	if (!sortednums) {
		(void) fprintf (stderr, "%s: malloc failed!\n", progname);
		exit(1);
	}
	for (j = 0; j < i; j++)
	    sortednums[j] = j;
	qsort(sortednums, numnums, sizeof(short), cmpnums);

	for (i = 0, pp = strnames; *pp; pp++, i++)
		;
	numstrs = i;
	sortedstrs = (short *) malloc(i * sizeof(short));
	if (!sortedstrs) {
		(void) fprintf (stderr, "%s: malloc failed!\n", progname);
		exit(1);
	}
	for (j = 0; j < i; j++)
	    sortedstrs[j] = j;
	qsort(sortedstrs, numstrs, sizeof(short), cmpstrs);
}

binsearch(name, sortedtable, size, table)
char *name;
short sortedtable[];
int size;
int table;
{
	int low, mid, high, cmp;
	char *cmpname;

	low = 0;
	high = size - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		switch (table) {
			case 1: cmpname = boolnames[sortedtable[mid]]; break;
			case 2: cmpname = numnames[sortedtable[mid]]; break;
			case 3: cmpname = strnames[sortedtable[mid]]; break;
		}
		cmp = strcmp(cmpname, name);
		if (cmp == 0)
			return 1;
		else if (cmp < 0)
			low = mid + 1;
		else
			high = mid - 1;
	}
	return 0;
}

static int
lookfor(name, table)
char *name;
int table;
{
	switch(table) {
		case 1: return binsearch(name, sortedbools, numbools, table);
		case 2: return binsearch(name, sortednums, numnums, table);
		case 3: return binsearch(name, sortedstrs, numstrs, table);
	}
	return -1;
}

static
lookup(buf, termname)
register char *buf;
char *termname;
{
#define NAMESIZE 10
	char name[NAMESIZE];
	char *nptr = name;

	if (*buf == CAPCOMMENT)
		return 1;

	/* a name is followed by a SEPARATE, @, # or =. */
	while (*buf && *buf != SEPARATE && *buf != CANCEL &&
	       *buf != STRING && *buf != NUMBER && !isspace(*buf) &&
	       nptr < &name[NAMESIZE-1])
		*nptr++ = *buf++;
	*nptr = '\0';
	switch(*buf) {
		case STRING:		/* string capability */
			if (!lookfor(name, 3) && strcmp(name, "use") != 0)
				(void) fprintf (stderr,
					"%s: file %s, line %d, terminal '%s' has unknown string capability '%s'.\n",
					progname, filename, linenumber, termname, name);
			break;
		case NUMBER:		/* numeric capability */
			if (!lookfor(name, 2))
				(void) fprintf (stderr,
					"%s: file %s, line %d, terminal '%s' has unknown numeric capability '%s'.\n",
					progname, filename, linenumber, termname, name);
			break;
		case SEPARATE:		/* boolean capability */
		case ' ':
		case '\t':
			if (!lookfor(name, 1))
				(void) fprintf (stderr,
					"%s: file %s, line %d, terminal '%s' has unknown boolean capability '%s'.\n",
					progname, filename, linenumber, termname, name);
			break;
		case CANCEL:		/* may be boolean, number or string */
			if (!lookfor(name, 1) && !lookfor(name, 2) &&
			    !lookfor(name, 3))
				(void) fprintf (stderr,
					"%s: file %s, line %d, terminal '%s' has unknown capability '%s'.\n",
					progname, filename, linenumber, termname, name);
			break;
		default:
			(void) fprintf (stderr,
				"%s: file %s, line %d, terminal '%s' has capability name which is too long: '%s...'.\n",
				progname, filename, linenumber, termname, name);
		}
}

static
checknames(buf)
register char *buf;
{
	char *bufptr = buf;
	char termname[20];
	char *termptr = termname;

	while (*bufptr && *bufptr != SEPARATE && *bufptr != NAMESEPARATOR &&
	       termptr < &termname[20])
		*termptr++ = *bufptr++;
	if (termptr < &termname[20])
		*termptr = '\0';
	else
		termname[19] = '\0';

	for (buf = tskip(buf); *buf; buf = tskip(buf))
		lookup(buf, termname);
}

/*
 * Get an entry for terminal name in buffer bp,
 * from the terminfo file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
tgetent(bp, name, fname)
	char *bp, *name, *fname;
{
	register char *cp;
	char ibuf[CAPSIZ];
	FILE *tf;

	ibuf[0] = 0;
	tf = fopen(fname, "r");
	if (tf == NULL)
		return (-1);
	tbuf = bp;
	for (;;) {
		strcpy(bp, ibuf);
		for (;;) {
			if (fgets(ibuf, sizeof ibuf, tf) == NULL) {
				if (ferror(tf)) {
					perror(fname);
					fclose(tf);
					return (-1);
				}
				fclose(tf);
				if (tnamatch(name))
					return(tnchkuse(fname));
				return 0;
			}
			if (ibuf[0] == COMMENT || ibuf[0] == '\n') /* comment */
				continue;
			cp = &ibuf[strlen(ibuf)-3];
			/* Allow and ignore old style backslashes */
			if (*cp == SEPARATE && cp[1] == '\\')
				cp[1] = 0;
			cp[2] = 0;	/* get rid of newline */
			/* lines with leading white space are continuation */
			if (!isspace(ibuf[0]) && *bp)
				break;
			if (strlen(bp) + strlen(ibuf) >= CAPSIZ)
				(void) fprintf(stderr,
					"%s: file %s, line %d: Terminfo entry too long:\n%s",
					progname, filename, linenumber, bp);
			else {
				cp = ibuf;
				while (isspace(*cp))
					cp++;
				strcat(bp, cp);
			}
		}

		/*
		 * The real work for the match.
		 */
		if (tnamatch(name)) {
			fclose(tf);
			return(tnchkuse(fname));
		}
	}
}

/*
 * tnchkuse: look for use=xxx. When found, recursively
 * find xxx and append that entry (minus the names)
 * to take the place of the use=xxx entry. This allows terminfo
 * entries to say "like an HP2621 but doesn't turn on the labels".
 * Note that this works because of the left to right scan.
 * Multiple use= entries are allowed.
 */
tnchkuse(fname)
char *fname;
{
	register char *p, *q;
	char tcname[128 /* 16 */];	/* name of similar terminal */
	char tcbuf[CAPSIZ];
	char restbuf[CAPSIZ];
	char *holdtbuf = tbuf;
	char *beg_use, *beg_next;

	p = tbuf;
	if (++hopcount > MAXHOP) {
		(void) fprintf(stderr, "%s: file %s, line %d: Infinite use= loop '%s'\n",
			progname, filename, linenumber, tbuf);
		return (0);
	}
	for (;;) {
		p = index(p, 'u');
		if (p == NULL) {
			tbuf = holdtbuf;
			return 1;
		}
		beg_use = p;
		if (*++p != 's' || *++p != 'e' || *++p != '=')
			continue;
		strncpy(tcname, ++p, sizeof tcname);
		q = index(tcname, SEPARATE);
		if (q)
			*q = 0;
		/* try local file ... */
		if (tgetent(tcbuf, tcname, fname) != 1)	{
			/* ... and master */
			if (tgetent(tcbuf, tcname, E_TERMINFO) != 1) {
				printf("Cannot find term %s\n", tcname);
				return(0);
			}
		}

		/* Find the end of the use= spec */
		for(beg_next=beg_use;
		    *beg_next && *beg_next!=SEPARATE;
		    beg_next++)
			;
		beg_next++;
		while (isspace(*beg_next++))
			;
		beg_next--;

		/* Now shuffle string around. */
		strcpy(restbuf, beg_next);
		p = index(tcbuf, SEPARATE);
		if (p == NULL)
			p = tcbuf;
		else
			p++;
		strcpy(beg_use, p);
		p = index(beg_use, '\0');
		strcpy(p, restbuf);
	}
}

/*
 * Tnamatch deals with name matching.  The first field of the terminfo
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal SEPARATE terminator after the last
 * name (before the first field) stops us.
 */
tnamatch(np)
	char *np;
{
	register char *Np, *Bp;

/* printf("tnamatch, np '%s', tbuf '%s'\n", np, tbuf); */
	Bp = tbuf;
	if (*Bp == COMMENT)
		return(0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			;
		if (*Np == 0 &&
		    (*Bp == NAMESEPARATOR || *Bp == SEPARATE || *Bp == 0))
			return (1);
		while (*Bp && *Bp != SEPARATE && *Bp != NAMESEPARATOR)
			Bp++;
		if (*Bp == 0 || *Bp == SEPARATE)
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next SEPARATE delimited field. Allow \SEPARATE escape.
 */
static char *
tskip(bp)
	register char *bp;
{

	while (*bp && *bp != SEPARATE)
		if ((*bp++ == '\\') && *bp)
			bp++;
	if (*bp == 0)
		return bp;
	bp++;
	while (isspace(*bp) || *bp == SEPARATE)
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
tgetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;
	int idl = strlen(id);
	int sign = 1;

	for (;;) {
		bp = tskip(bp);
		if (*bp == 0)
			return (-1);
		if (strncmp(id, bp, idl))
			continue;
		bp += idl;
		if (*bp == CANCEL)
			return(-1);
		if (*bp != NUMBER)
			continue;
		bp++;
		if (*bp == '-') {
			sign = -1;
			bp++;
		}
		base = 10;
		if (*bp == '0') {
			base = 8;
			bp++;
		}
		if (*bp == 'x' || *bp == 'X') {
			base = 16;
			bp++;
		}
		i = 0;
		switch (base) {
			case 8:	while (*bp >= '0' && *bp <= '7') {
					i *= 8;
					i += *bp++ - '0';
				}
				break;
			case 10:while (isdigit(*bp)) {
					i *= 10;
					i += *bp++ - '0';
				}
				break;
			case 16:while (isdigit(*bp) ||
				       (*bp >= 'a' && *bp <= 'f') ||
				       (*bp >= 'A' && *bp <= 'F')) {
					i *= 16;
					if (isdigit(*bp))
						i += *bp++ - '0';
					else if (*bp >= 'a' && *bp <= 'f')
						i += *bp++ - 'a' + 10;
					else
						i += *bp++ - 'A' + 10;
				}
				break;
		}
		i *= sign;
		return (i);
	}
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a SEPARATE or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given or is cancelled.
 */
tgetflag(id)
	char *id;
{
	register char *bp = tbuf;
	int idl = strlen(id);

	for (;;) {
		bp = tskip(bp);
		if (!*bp)
			return (0);
		if (strncmp(bp, id, idl) == 0) {
			bp += idl;
			if (!*bp || *bp == SEPARATE)
				return (1);
			else if (*bp == CANCEL)
				return(0);
		}
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
tgetstr(id, area)
	char *id, **area;
{
	register char *bp = tbuf;
	int idl = strlen(id);

	for (;;) {
		bp = tskip(bp);
		if (!*bp)
			return (0);
		if (strncmp(id, bp, idl))
			continue;
		bp += idl;
		if (*bp == CANCEL)
			return(0);
		if (*bp != STRING)
			continue;
		bp++;
		return (tdecode(bp, area));
	}
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
tdecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != SEPARATE) {
		switch (c) {

		case '^':
			c = *str++ & 037;
			if (c == 0)
				c = 0200;
			break;

		case '\\':
			/*
			 * \x escapes understood:
			 *	\e	escape
			 *	\E	escape
			 *	\^	^
			 *	\\	\
			 *	\,	,
			 *	\:	:
			 *	\l	linefeed
			 *	\n	newline (=linefeed)
			 *	\r	return
			 *	\t	tab
			 *	\b	backspace
			 *	\f	formfeed
			 *	\s	space
			 *	\0	null
			 *	\###	octal ###
			 */
			dp = "e\033E\033^^\\\\,,::l\012n\nr\rt\tb\bf\fs ";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (c >= '0' && c <= '7') {
				c -= '0';
				i = 2;
				do {
					if (*str < '0' || *str > '7')
						break;
					c <<= 3;
					c |= *str++ - '0';
				} while (--i);
				if (c == 0)
					c = 0200;	/* don't term. str. */
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}

#define TIMAGNUM 0432

unstr(cp)
char *cp;
{
    if (cp)
	for ( ; *cp ; cp++)
	    if ((*cp & 0377) < 0200)
		printf("%s", unctrl(*cp));
	    else
		printf("M-%s", unctrl(*cp));
}

store(cap)
char *cap;
{
	register char *cp;
	register int i;
	register char **pp, **np;
	char tcpbuf[2048 /* 1024 */];
	char *tcp = tcpbuf;
	char *tname = cap;
	char *tnp;
	char tnbuf[256], names[256];
	char fnbuf[128 /* 64 */], lnbuf[128 /* 64 */];
	char strtab[8192 /* 4096 */];
	register char *strtabptr;
	FILE *fd;
	int sname, sbool;

	while (*cap != SEPARATE)	/* skip over names */
		cap++;
	*cap = 0;
	strcpy(tnbuf, tname);
	strcpy(names, tname);
	*cap = SEPARATE;

	for (tnp=tnbuf;
	     *tnp && *tnp != NAMESEPARATOR && *tnp != SEPARATE;
	     tnp++)
		;
	if (*tnp)
		*tnp++ = 0;
	if (terminfo) {
		strcpy(fnbuf, terminfo);
		strcat(fnbuf, "/");
	} else {
		strcpy(fnbuf, termpath(/));
	}
	strcat(fnbuf, tnbuf);
	checkon(fnbuf);
	if (verbose)
		printf("create '%s'\n", fnbuf);
	fd = fopen(fnbuf, "w");
	if (fd == NULL) {
		perror(fnbuf);
		return;
	}

	putsh(TIMAGNUM, fd);
	sname = strlen(names)+1;
	putsh(sname, fd);
	sbool = listlen(boolcodes);
	putsh(sbool, fd);
	putsh(listlen(numcodes), fd);
	putsh(listlen(strcodes), fd);
	putsh(0, fd);			/* length of string table */

	/* Write out various terminal names to file, null terminated. */
	for (cp=names; *cp; cp++)
		putc(*cp, fd);
	putc(0, fd);

	/* Write out the booleans: flag */
	for (pp=boolnames, np=boolcodes; *np; pp++,np++) {
		i = tgetflag(*pp);
		putc(i, fd);
		if (verbose > 2)
			printf("bool cap %s code %s val %d\n", *pp, *np, i);
	}
	if ((sname + sbool) & 1)
		putc(0, fd);

	/* Numbers: highbyte, lowbyte.  0377,0377 means -1 (missing) */
	for (pp=numnames, np=numcodes;   *np; pp++,np++) {
		i = tgetnum(*pp);
		putsh(i, fd);
		if (verbose > 1)
			printf("num cap %s code %s val %d\n", *pp, *np, i);
	}

	/* Strings: offset into string table.  If cap is missing, -1 is used */
	strtabptr = strtab;
	for (pp=strnames, np=strcodes;   *np; pp++,np++) {
		cp = tgetstr(*pp, &tcp);
		if (verbose > 3)
			if (cp) {
				printf("str %s code %s val ", *pp, *np);
				unstr(cp);
				printf("\n");
			} else
				printf("str %s code %s val NULL\n", *pp, *np);
		if (cp) {
			putsh(strtabptr-strtab, fd);
			while (*strtabptr++ = *cp++)
				;
		} else {
			putsh(-1, fd);
		}
	}
	fwrite(strtab, 1, strtabptr-strtab, fd);
	fseek(fd, 10L, 0);	/* Back to string table size in header */
	putsh(strtabptr-strtab, fd);
	fclose(fd);

	while (*tnp) {
		i = 0;
		for (tname=tnp;
		     *tnp && *tnp != NAMESEPARATOR && *tnp != SEPARATE;
		     tnp++)
			if (isspace(*tnp))
				i = 1;
		if (*tnp)
			*tnp++ = 0;
		if (i)
			continue;
		if (terminfo) {
			strcpy(lnbuf, terminfo);
			strcat(lnbuf, "/");
		} else {
			strcpy(lnbuf, termpath(/));
		}
		strcat(lnbuf, tname);
		checkon(lnbuf);
		if (verbose)
			printf("link '%s' '%s'\n", fnbuf, lnbuf);
		if (link(fnbuf, lnbuf) < 0) {
			fprintf(stderr, "Can't link %s to %s: ", fnbuf, lnbuf);
			perror("");
		}
	}
}

/*
 * Write a short out to the file in machine-independent format.
 */
putsh(val, fd)
register val;
FILE *fd;
{
	if (val != -1) {
		putc(val&0377, fd);
		putc((val>>8)&0377, fd);
	} else {
		/* Write -1 as two 0377's. */
		putc(0377, fd);
		putc(0377, fd);
	}
}

listlen(list)
register char **list;
{
	register int rv = 0;

	while (*list) {
		list++;
		rv++;
	}
	return rv;
}

/*
 * Do various processing on a file name we are about to create.
 * If it already exists, and it's older than we started, unlink it.
 * Also insert a / after the 2nd char of the tail, and make sure
 * that directory exists.
 */
checkon(fn)
char *fn;
{
	struct stat stbuf;
	char *fp, *cp;
	char nbuf[64];

	/* Find last / */
	for (cp=fn; *cp; cp++)
		if (*cp == '/')
			fp = cp;
	if (cp-fp > 2) {
		cp = fp+2;
		strcpy(nbuf, fp+1);
		*cp = 0;
		if (stat(fn, &stbuf) < 0) {
			if (verbose)
				printf("mkdir %s\n", fn);
			if (mkdir(fn, 0777) < 0) {
				perror(fn);
				return;
			}
		}
		*cp++ = '/';
		strcpy(cp, nbuf);
	}
	else
		printf("%s: terminal name too short\n", fp+1);
	if (stat(fn, &stbuf) < 0)
		return;
	if (stbuf.st_mtime < starttime) {
		if (verbose > 1)
			printf("unlink %s\n", fn);
		unlink(fn);
	}
}
