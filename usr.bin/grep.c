/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)grep.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.13 */
#endif

/*
 * grep -- print lines matching (or not matching) a pattern
 *
 *	status returns:
 *		0 - ok, and some matches
 *		1 - ok, but no matches
 *		2 - some error
 */

#include <stdio.h>
#include <ctype.h>

/*
 * define some macros for rexexp.h
 */

#define INIT	register char *sp = instring;	/* 1st arg pts to RE string */
#define GETC()		((unsigned char)(*sp++))
#define PEEKC()		((unsigned char)(*sp))
#define UNGETC(c)	(--sp)
#define RETURN(c)	return;
#define ERROR(c)	regerr(c)

#include <regexp.h>

#define	errmsg(msg, arg)	fprintf(stderr, msg, arg)
#define ESIZE	256
#define	BLKSIZE	512

char	*strrchr();
char	*malloc();
long	lnum;
char	linebuf[BUFSIZ];
char	prntbuf[BUFSIZ];
char	expbuf[ESIZE];
int	nflag;
int	bflag;
int	lflag;
int	cflag;
int	vflag;
int	hflag	= 1;
int	sflag;
int	iflag;
int	wflag;
int	eflag;
int	errflg;
int	nfile;
long	tln;
int	nsucc;
int	nchars;
int	nlflag;

main(argc, argv)
register argc;
char **argv;
{
	register	c;
	register char	*arg;
	extern char	*optarg;
	extern int	optind;
	char		*argptr;
	unsigned int	wordlen;
	char		*wordbuf;
	register char	*wp;

	while((c=getopt(argc, argv, "blcnsviywhe:")) != EOF)
		switch(c) {
		case 'v':
			vflag++;
			break;
		case 'c':
			cflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'b':
			bflag++;
			break;
		case 's':
			sflag++;
			break;
		case 'l':
			lflag++;
			break;
		case 'i':
		case 'y':
			iflag++;
			break;
		case 'w':
			wflag++;
			break;
		case 'h':
			hflag = 0;
			break;
		case 'e':
			/* next arg is pattern */
			eflag++;
			argptr = optarg;
			break;
		case '?':
			errflg++;
		}

	if(errflg || ((optind >= argc) && !eflag)) {
		errmsg("Usage: grep -blcnsviwh [ -e ] pattern file . . .\n",
			(char *)NULL);
		exit(2);
	}

	argc -= optind;
	if (!eflag) {
		argptr = argv[optind];
		optind++;
		argc--;
	}
	nfile = argc;
	argv = &argv[optind];

	if (strrchr(argptr,'\n'))
		ERROR(41);

	if (iflag) {
		for(arg = argptr; *arg != NULL; ++arg) {
			c = (int)((unsigned char)*arg);
			if (isupper(c))
				*arg = (char)tolower(c);
		}
	}

	if (wflag) {
		wordlen = strlen(argptr) + 4;
		if ((wordbuf = malloc(wordlen)) == NULL) {
			errmsg("grep: Out of memory for word\n", (char *)NULL);
			exit(2);
		}
		wp = wordbuf;
		(void) strcpy(wordbuf, "\\<");
		(void) strcat(wordbuf, argptr);
		(void) strcat(wordbuf, "\\>");
		argptr = wordbuf;
	}

	compile(argptr, expbuf, &expbuf[ESIZE], '\0');

	if (argc == 0)
		execute((char *)NULL);
	else
		while (argc-- > 0)
			execute(*argv++);

	exit(nsucc == 2 ? 2 : nsucc == 0);
	/* NOTREACHED */
}

execute(file)
char *file;
{
	register FILE *temp;
	register char *lbuf;
	register char *p1;
	register int c;

	if (file == NULL)
		temp = stdin;
	else if ( (temp = fopen(file, "r")) == NULL) {
#ifdef S5EMUL
		if (!sflag) {
#endif
			fprintf(stderr, "grep: ");
			perror(file);
#ifdef S5EMUL
		}
#endif
		nsucc = 2;
		return;
	}

	lnum = 0;
	tln = 0;

	/*
	 * This flag will be cleared on the last line if it doesn't contain
	 * a newline.
	 */
	nlflag = 1;
	for (;;) {
		p1 = prntbuf;
		while ((c = getc(temp)) != '\n') {
			if (c == EOF) {
				if (ferror(temp)) {
					fprintf(stderr, "grep: Read error on ");
					perror(file ? file : "standard input");
					nsucc = 2;
					return;
				}
				if (p1 == prntbuf)
					goto out;
				nlflag = 0;
				break;
			}
			if (p1 < &prntbuf[BUFSIZ-1])
				*p1++ = c;
		}
		*p1 = '\0';
		nchars = p1 - prntbuf;

		lnum++;

		if (iflag) {
			lbuf = linebuf;
			p1 = prntbuf;
			while ((c = (int)((unsigned char)*p1++)) != '\0') {
				if (isupper(c))
					*lbuf++ = tolower(c);
				else
					*lbuf++ = c;
			}
			*lbuf = '\0';
			lbuf = linebuf;
		} else
			lbuf = prntbuf;

		if(step(lbuf, expbuf))
			goto found;
		if(vflag) {
			if(succeed(file, temp))
				break;
		}
		continue;

	found:
		if(!vflag) {
			if(succeed(file, temp))
				break;
		}
	}
	if (ferror(temp)) {
		fprintf(stderr, "grep: Read error on ");
		perror(file ? file : "standard input");
		nsucc = 2;
		fclose(temp);
		return;
	}

out:
	fclose(temp);

	if (cflag) {
		if (nfile>1)
			fprintf(stdout, "%s:", file);
		fprintf(stdout, "%ld\n", tln);
	}
	return;
}

succeed(f, iop)
register char *f;
FILE *iop;
{
	nsucc = (nsucc == 2) ? 2 : 1;
#ifndef S5EMUL
	if (sflag)
		return(1);
#endif
	if (cflag) {
		tln++;
		return(0);
	}
	if (lflag) {
		fprintf(stdout, "%s\n", f);
		return(1);
	}

	if (nfile > 1 && hflag)	/* print filename */
		fprintf(stdout, "%s:", f);

	if (bflag)	/* print block number */
		fprintf(stdout, "%ld:", (ftell(iop)-1)/BLKSIZE);

	if (nflag)	/* print line number */
		fprintf(stdout, "%ld:", lnum);

	fwrite(prntbuf, 1, nchars, stdout);
	if (nlflag)
		putc('\n', stdout);
	return(0);
}

regerr(err)
register err;
{
	register char *message;

	switch(err) {
		case 11:
			message = "Range endpoint too large.";
			break;
		case 16:
			message = "Bad number.";
			break;
		case 25:
			message = "``\\digit'' out of range.";
			break;
		case 36:
			message = "Illegal or missing delimiter.";
			break;
		case 41:
			message = "No remembered search string.";
			break;
		case 42:
			message = "\\( \\) imbalance.";
			break;
		case 43:
			message = "Too many \\(.";
			break;
		case 44:
			message = "More than 2 numbers given in \\{ \\}.";
			break;
		case 45:
			message = "} expected after \\.";
			break;
		case 46:
			message = "First number exceeds second in \\{ \\}.";
			break;
		case 49:
			message = "[ ] imbalance.";
			break;
		case 50:
			message = "Regular expression too long.";
			break;
		default:
			message = "Unknown regexp error code!!";
			break;
	}

	errmsg("grep: illegal regular expression: %s\n", message);
	exit(2);
}
