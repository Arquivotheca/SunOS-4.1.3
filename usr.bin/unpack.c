/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)unpack.c 1.1 92/07/30 SMI"; /* from S5R3 1.15 */
#endif

/*
 *	Huffman decompressor
 *	Usage:	pcat filename...
 *	or	unpack filename...
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

extern void exit();

#define VOID	(void)

jmp_buf env;
struct	stat status;
char	*argv0, *argvk;
int	rmflg = 0;	/* rmflg, when set it's ok to rm arvk file on caught signals */
short	errorm;

#define NAMELEN 80
#define SUF0	'.'
#define SUF1	'z'
#define US	037
#define RS	036

/* variables associated with i/o */
char	filename[NAMELEN+2];
short	infile;
short	outfile;
short	inleft;
char	*inp;
char	*outp;
char	inbuff[BUFSIZ];
char	outbuff[BUFSIZ];

/* the dictionary */
long	origsize;
short	maxlev;
short	intnodes[25];
char	*tree[25];
char	characters[256];
char	*eof;

/* read in the dictionary portion and build decoding structures */
/* return 1 if successful, 0 otherwise */
getdict ()
{
	register int c, i, nchildren;

	/*
	 * check two-byte header
	 * get size of original file,
	 * get number of levels in maxlev,
	 * get number of leaves on level i in intnodes[i],
	 * set tree[i] to point to leaves for level i
	 */
	eof = &characters[0];

	inbuff[6] = 25;
	inleft = read (infile, &inbuff[0], BUFSIZ);
	if (inleft < 0) {
		eprintf (".z: read error");
		return (0);
	}
	if (inbuff[0] != US)
		goto goof;

	if (inbuff[1] == US) {		/* oldstyle packing */
		if (setjmp (env))
			return (0);
		expand ();
		return (1);
	}
	if (inbuff[1] != RS)
		goto goof;

	inp = &inbuff[2];
	origsize = 0;
	for (i=0; i<4; i++)
		origsize = origsize*256 + ((*inp++) & 0377);
	maxlev = *inp++ & 0377;
	if (maxlev > 24) {
goof:		eprintf (".z: not in packed format");
		return (0);
	}
	for (i=1; i<=maxlev; i++)
		intnodes[i] = *inp++ & 0377;
	for (i=1; i<=maxlev; i++) {
		tree[i] = eof;
		for (c=intnodes[i]; c>0; c--) {
			if (eof >= &characters[255])
				goto goof;
			*eof++ = *inp++;
		}
	}
	*eof++ = *inp++;
	intnodes[maxlev] += 2;
	inleft -= inp - &inbuff[0];
	if (inleft < 0)
		goto goof;

	/*
	 * convert intnodes[i] to be number of
	 * internal nodes possessed by level i
	 */

	nchildren = 0;
	for (i=maxlev; i>=1; i--) {
		c = intnodes[i];
		intnodes[i] = nchildren /= 2;
		nchildren += c;
	}
	return (decode ());
}

/* unpack the file */
/* return 1 if successful, 0 otherwise */
decode ()
{
	register int bitsleft, c, i;
	int j, lev;
	char *p;

	outp = &outbuff[0];
	lev = 1;
	i = 0;
	while (1) {
		if (inleft <= 0) {
			inleft = read (infile, inp = &inbuff[0], BUFSIZ);
			if (inleft < 0) {
				eprintf (".z: read error");
				return (0);
			}
		}
		if (--inleft < 0) {
uggh:			eprintf (".z: unpacking error");
			return (0);
		}
		c = *inp++;
		bitsleft = 8;
		while (--bitsleft >= 0) {
			i *= 2;
			if (c & 0200)
				i++;
			c <<= 1;
			if ((j = i - intnodes[lev]) >= 0) {
				p = &tree[lev][j];
				if (p == eof) {
					c = outp - &outbuff[0];
					if (write (outfile, &outbuff[0], c) != c) {
wrerr:						eprintf (": write error");
						return (0);
					}
					origsize -= c;
					if (origsize != 0)
						goto uggh;
					return (1);
				}
				*outp++ = *p;
				if (outp == &outbuff[BUFSIZ]) {
					if (write (outfile, outp = &outbuff[0], BUFSIZ) != BUFSIZ)
						goto wrerr;
					origsize -= BUFSIZ;
				}
				lev = 1;
				i = 0;
			} else
				lev++;
		}
	}
}

main (argc, argv)
	char *argv[];
{
	register i, k;
	int sep, pcat = 0;
	register char *p1, *cp;
	int fcount = 0;		/* failure count */
	int (*onsig)();
	struct utimbuf {
		time_t	actime;
		time_t	modtime;
	} utb;


	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, onsig);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, onsig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, onsig);

	p1 = *argv;
	while(*p1++);		/* Point p1 to end of argv[0] string */
	while(--p1 >= *argv)
		if(*p1 == '/')break;
	*argv = p1 + 1;
	argv0 = argv[0];
	if(**argv == 'p')pcat++;	/* User entered pcat (or /xx/xx/pcat) */
	for (k=1; k<argc; k++) {
		errorm = 0;
		sep = -1;
		cp = filename;
		argvk = argv[k];
		for (i=0; i < (NAMELEN-3) && (*cp = argvk[i]); i++)
			if (*cp++ == '/')
				sep = i;
		if (cp[-1] == SUF1 && cp[-2] == SUF0) {
			argvk[i-2] = '\0'; /* Remove suffix and try again */
			k--;
			continue;
		}

		fcount++;	/* expect the worst */
		if (i >= (NAMELEN-3) || (i - sep) > 13) {
			eprintf (": file name too long");
			goto done;
		}
		*cp++ = SUF0;
		*cp++ = SUF1;
		*cp = '\0';
		if ((infile = open (filename, 0)) == -1) {
			eprintf (".z: cannot open");
			goto done;
		}

		if (pcat)
			outfile = 1;	/* standard output */
		else {
			if (stat (argvk, &status) != -1) {
				eprintf (": already exists");
				goto done;
			}
			VOID fstat (infile, &status);
			if (status.st_nlink != 1)
				eprintf (".z: Warning: file has links");
			if ((outfile = creat (argvk, status.st_mode&07777)) == -1) {
				eprintf (": cannot create");
				goto done;
			}

			rmflg = 1;
			if (chmod (argvk, status.st_mode) != 0)
			    printf("can't change mode to %o\n", status.st_mode);
			VOID chown (argvk, status.st_uid, status.st_gid);
		}

		if (getdict ()) {	/* unpack */
			fcount--; 	/* success after all */
			if (!pcat) {
				/*
				 * preserve acc & mod dates
				 */
				utb.actime = status.st_atime;
				utb.modtime = status.st_mtime;
				VOID utime (argvk, &utb);
				rmflg = 0;
				eprintf (": unpacked");
				VOID unlink (filename);

			}
		}
		else
			if (!pcat)
				VOID unlink (argvk);
done:		if (errorm)
			VOID fprintf (stderr, "\n");
		VOID close (infile);
		if (!pcat)
			VOID close (outfile);
	}
	return (fcount);
}

eprintf (s)
	char *s;
{
	if (!errorm) {
		errorm = 1;
		VOID fprintf (stderr, "%s: %s", argv0, argvk);
	}
	VOID fprintf (stderr, s);
}

/*
 * This code is for unpacking files that
 * were packed using the previous algorithm.
 */

int	Tree[1024];

expand ()
{
	register tp, bit;
	short word;
	int keysize, i, *t;

	outp = outbuff;
	inp = &inbuff[2];
	inleft -= 2;
	origsize = ((long) (unsigned) getwd ())*256*256;
	origsize += (unsigned) getwd ();
	t = Tree;
	for (keysize = getwd (); keysize--; ) {
		if ((i = getch ()) == 0377)
			*t++ = getwd ();
		else
			*t++ = i & 0377;
	}

	bit = tp = 0;
	for (;;) {
		if (bit <= 0) {
			word = getwd ();
			bit = 16;
		}
		tp += Tree[tp + (word<0)];
		word <<= 1;
		bit--;
		if (Tree[tp] == 0) {
			putch (Tree[tp+1]);
			tp = 0;
			if ((origsize -= 1) == 0) {
				write (outfile, outbuff, outp - outbuff);
				return;
			}
		}
	}
}

getch ()
{
	if (inleft <= 0) {
		inleft = read (infile, inp = inbuff, BUFSIZ);
		if (inleft < 0) {
			eprintf (".z: read error");
			longjmp (env, 1);
		}
	}
	inleft--;
	return (*inp++ & 0377);
}

getwd ()
{
	register char c;
	register d;
	c = getch ();
	d = getch ();
	d <<= 8;
	d |= c & 0377;
	return (d);
}

void
onsig()
{
				/* could be running as unpack or pcat		*/
				/* but rmflg is set only when running	*/
				/* as unpack and only when file is	*/
				/* created by unpack and not yet done	*/
	if (rmflg == 1)
		VOID unlink(argvk);
	exit(1);
}

putch (c)
	char c;
{
	register n;

	*outp++ = c;
	if (outp == &outbuff[BUFSIZ]) {
		n = write (outfile, outp = outbuff, BUFSIZ);
		if (n < BUFSIZ) {
			eprintf (": write error");
			longjmp (env, 2);
		}
	}
}
