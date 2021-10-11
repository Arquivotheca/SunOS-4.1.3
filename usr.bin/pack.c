/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)pack.c 1.1 92/07/30 SMI"; /* from S5R3 1.12 */
#endif

/*
 *	Huffman encoding program 
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */


#include  <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define	END	256
#define NAMELEN 80
#define	SUF0	'.'
#define	SUF1	'z'

struct stat status, ostatus;

/* union for overlaying a long int with a set of four characters */
union FOUR {
	struct { long int lng; } lint;
	struct { char c0, c1, c2, c3; } chars;
};

/* character counters */
long	count [END+1];
union	FOUR insize;
long	outsize;
long	dictsize;
int	diffbytes;

/* i/o stuff */
char	vflag = 0;
int	force = 0;	/* allow forced packing for consistency in directory */
char	filename [NAMELEN];
int	infile;		/* unpacked file */
int	outfile;	/* packed file */
char	inbuff [BUFSIZ];
char	outbuff [BUFSIZ+4];

/* variables associated with the tree */
int	maxlev;
int	levcount [25];
int	lastnode;
int	parent [2*END+1];

/* variables associated with the encoding process */
char	length [END+1];
long	bits [END+1];
union	FOUR mask;
long	inc;
#if defined(vax) || defined(i386)
char	*maskshuff[4]  = {&(mask.chars.c3), &(mask.chars.c2), &(mask.chars.c1), &(mask.chars.c0)};
#elif defined(pdp11)
char	*maskshuff[4]  = {&(mask.chars.c1), &(mask.chars.c0), &(mask.chars.c3), &(mask.chars.c2)};
#else	/* just about everything else */
char	*maskshuff[4]  = {&(mask.chars.c0), &(mask.chars.c1), &(mask.chars.c2), &(mask.chars.c3)};
#endif

/* the heap */
int	n;
struct	heap {
	long int count;
	int node;
} heap [END+2];
#define hmove(a,b) {(b).count = (a).count; (b).node = (a).node;}

/* gather character frequency statistics */
/* return 1 if successful, 0 otherwise */
input ()
{
	register int i;
	for (i=0; i<END; i++)
		count[i] = 0;
	while ((i = read(infile, inbuff, BUFSIZ)) > 0)
		while (i > 0)
			count[inbuff[--i]&0377] += 2;
	if (i == 0)
		return (1);
	printf (": read error");
	return (0);
}

/* encode the current file */
/* return 1 if successful, 0 otherwise */
output ()
{
	extern long lseek();
	int c, i, inleft;
	char *inp;
	register char **q, *outp;
	register int bitsleft;
	long temp;

	/* output ``PACKED'' header */
	outbuff[0] = 037; 	/* ascii US */
	outbuff[1] = 036; 	/* ascii RS */
	/* output the length and the dictionary */
	temp = insize.lint.lng;
	for (i=5; i>=2; i--) {
		outbuff[i] =  (char) (temp & 0377);
		temp >>= 8;
	}
	outp = &outbuff[6];
	*outp++ = maxlev;
	for (i=1; i<maxlev; i++)
		*outp++ = levcount[i];
	*outp++ = levcount[maxlev]-2;
	for (i=1; i<=maxlev; i++)
		for (c=0; c<END; c++)
			if (length[c] == i)
				*outp++ = c;
	dictsize = outp-&outbuff[0];

	/* output the text */
	lseek(infile, 0L, 0);
	outsize = 0;
	bitsleft = 8;
	inleft = 0;
	do {
		if (inleft <= 0) {
			inleft = read(infile, inp = &inbuff[0], BUFSIZ);
			if (inleft < 0) {
				printf (": read error");
				return (0);
			}
		}
		c = (--inleft < 0) ? END : (*inp++ & 0377);
		mask.lint.lng = bits[c]<<bitsleft;
		q = &maskshuff[0];
		if (bitsleft == 8)
			*outp = **q++;
		else
			*outp |= **q++;
		bitsleft -= length[c];
		while (bitsleft < 0) {
			*++outp = **q++;
			bitsleft += 8;
		}
		if (outp >= &outbuff[BUFSIZ]) {
			if (write(outfile, outbuff, BUFSIZ) != BUFSIZ) {
wrerr:				printf (".z: write error");
				return (0);
			}
			((union FOUR *) outbuff)->lint.lng = ((union FOUR *) &outbuff[BUFSIZ])->lint.lng;
			outp -= BUFSIZ;
			outsize += BUFSIZ;
		}
	} while (c != END);
	if (bitsleft < 8)
		outp++;
	c = outp-outbuff;
	if (write(outfile, outbuff, c) != c)
		goto wrerr;
	outsize += c;
	return (1);
}

/* makes a heap out of heap[i],...,heap[n] */
heapify (i)
{
	register int k;
	int lastparent;
	struct heap heapsubi;
	hmove (heap[i], heapsubi);
	lastparent = n/2;
	while (i <= lastparent) {
		k = 2*i;
		if (heap[k].count > heap[k+1].count && k < n)
			k++;
		if (heapsubi.count < heap[k].count)
			break;
		hmove (heap[k], heap[i]);
		i = k;
	}
	hmove (heapsubi, heap[i]);
}

/* return 1 after successful packing, 0 otherwise */
int packfile ()
{
	register int c, i, p;
	long bitsout;

	/* gather frequency statistics */
	if (input() == 0)
		return (0);

	/* put occurring chars in heap with their counts */
	diffbytes = -1;
	count[END] = 1;
	insize.lint.lng = n = 0;
	for (i=END; i>=0; i--) {
		parent[i] = 0;
		if (count[i] > 0) {
			diffbytes++;
			insize.lint.lng += count[i];
			heap[++n].count = count[i];
			heap[n].node = i;
		}
	}
	if (diffbytes == 1) {
		printf (": trivial file");
		return (0);
	}
	insize.lint.lng >>= 1;
	for (i=n/2; i>=1; i--)
		heapify(i);

	/* build Huffman tree */
	lastnode = END;
	while (n > 1) {
		parent[heap[1].node] = ++lastnode;
		inc = heap[1].count;
		hmove (heap[n], heap[1]);
		n--;
		heapify(1);
		parent[heap[1].node] = lastnode;
		heap[1].node = lastnode;
		heap[1].count += inc;
		heapify(1);
	}
	parent[lastnode] = 0;

	/* assign lengths to encoding for each character */
	bitsout = maxlev = 0;
	for (i=1; i<=24; i++)
		levcount[i] = 0;
	for (i=0; i<=END; i++) {
		c = 0;
		for (p=parent[i]; p!=0; p=parent[p])
			c++;
		levcount[c]++;
		length[i] = c;
		if (c > maxlev)
			maxlev = c;
		bitsout += c*(count[i]>>1);
	}
	if (maxlev > 24) {
		/* can't occur unless insize.lint.lng >= 2**24 */
		printf (": Huffman tree has too many levels");
		return(0);
	}

	/* don't bother if no compression results */
	outsize = ((bitsout+7)>>3)+6+maxlev+diffbytes;
	if ((insize.lint.lng+BUFSIZ-1)/BUFSIZ <= (outsize+BUFSIZ-1)/BUFSIZ
	    && !force) {
		printf (": no saving");
		return(0);
	}

	/* compute bit patterns for each character */
	inc = 1L << 24;
	inc >>= maxlev;
	mask.lint.lng = 0;
	for (i=maxlev; i>0; i--) {
		for (c=0; c<=END; c++)
			if (length[c] == i) {
				bits[c] = mask.lint.lng;
				mask.lint.lng += inc;
			}
		mask.lint.lng &= ~inc;
		inc <<= 1;
	}

	return (output());
}

main(argc, argv)
int argc; char *argv[];
{
	register int i;
	register char *cp;
	int k, sep;
	int fcount =0; /* count failures */
	struct utimbuf {
		time_t	actime;
		time_t	modtime;
	} utb;

	for (k=1; k<argc; k++) {
		if (argv[k][0] == '-' && argv[k][1] == '\0') {
			vflag = 1 - vflag;
			continue;
		}
		if (argv[k][0] == '-' && argv[k][1] == 'f') {
			force++;
			continue;
		}
		fcount++; /* increase failure count - expect the worst */
		printf ("%s: %s", argv[0], argv[k]);
		sep = -1;  cp = filename;
		for (i=0; i < (NAMELEN-3) && (*cp = argv[k][i]); i++)
			if (*cp++ == '/') sep = i;
		if (cp[-1]==SUF1 && cp[-2]==SUF0) {
			printf (": already packed\n");
			continue;
		}
		if (i >= (NAMELEN-3) || (i-sep) > 13) {
			printf (": file name too long\n");
			continue;
		}
		if ((infile = open (filename, 0)) < 0) {
			printf (": cannot open\n");
			continue;
		}
		fstat(infile,&status);
		if ((status.st_mode&S_IFMT) == S_IFDIR) {
			printf (": cannot pack a directory\n");
			goto closein;
		}
		if (status.st_size == 0) {
			printf(": cannot pack a zero length file\n");
			goto closein;
		}
		if( status.st_nlink != 1 ) {
			printf(": has links\n");
			goto closein;
		}
		*cp++ = SUF0;  *cp++ = SUF1;  *cp = '\0';
		if( stat(filename, &ostatus) != -1) {
			printf(".z: already exists\n");
			goto closein;
		}
		if ((outfile = creat (filename, status.st_mode&07777)) < 0) {
			printf (".z: cannot create\n");
			goto closein;
		}
		if (chmod (filename, status.st_mode) != 0)
			printf("can't change mode to %o\n", status.st_mode);
		chown (filename, status.st_uid, status.st_gid);

		if (packfile()) {
			if (unlink(argv[k]) != 0)
				fprintf(stderr, "%s: can't unlink %s\n",
					argv[0], argv[k]);
			fcount--;  /* success after all */
			printf (": %.1f%% Compression\n",
				((double)(-outsize+(insize.lint.lng))/(double)insize.lint.lng)*100);
			/* output statistics */
			if (vflag) {
				printf("	from %ld to %ld bytes\n",  insize.lint.lng, outsize);
				printf("	Huffman tree has %d levels below root\n", maxlev);
				printf("	%d distinct bytes in input\n", diffbytes);
				printf("	dictionary overhead = %ld bytes\n", dictsize);
				printf("	effective  entropy  = %.2f bits/byte\n", 
					((double) outsize / (double) insize.lint.lng) * 8 );
				printf("	asymptotic entropy  = %.2f bits/byte\n", 
					((double) (outsize-dictsize) / (double) insize.lint.lng) * 8 );
			}
		}
		else
		{       printf (" - file unchanged\n");
			unlink(filename);
		}

      closein:	close (outfile);
		close (infile);
		utb.actime = status.st_atime;
		utb.modtime = status.st_mtime;
		(void)utime(filename, &utb);	/* preserve acc & mod times */
	}
	return (fcount);
}
