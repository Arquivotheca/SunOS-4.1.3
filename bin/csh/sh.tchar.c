/*
 * Copyright (C) 1989 Sun Microsystems Inc.
 */

#ifndef lint
static char *sccsid = "@(#)sh.tchar.c 1.1 92/07/30 SMI.";
#endif
/*
 * This module provides with system/library function substitutes for tchar datatype.
 * This also includes two conversion functions between tchar and char arrays.
 *
 * T. Kurosaka, Palo Alto, California, USA
 * March 1989
 *
 * Implementation Notes:
 *	Many functions defined here use a "char" buffer chbuf[].  In the
 * first attempt, there used to be only one chbuf defined as static 
 *(private) variable and shared by these functions.  csh linked with that
 *version of this file misbehaved in interpreting "eval `tset ....`".
 *(in general, builtin function with back-quoted expression).
 *	This bug seemed to be caused by sharing of chbuf
 *by these functions simultanously (thru vfork() mechanism?).  We could not
 *identify which two functions interfere each other so we decided to
 *have each of these function its private instance of chbuf.
 *The size of chbuf[] might be much bigger than necessary for some functions.
 */
#ifdef DBG
# include <stdio.h>	/* For <assert.h> needs stderr defined. */
#else/*!DBG*/
# define NDEBUG		/* Disable assert(). */
#endif/*!DBG*/
#include <assert.h>
#include "sh.h"
#ifdef MBCHAR
#include <widec.h>	/* For wcsetno() */
#endif
#include <sys/param.h>	/* MAXPATHLEN */


/* strtots(to, from): convert a char string 'from' into a tchar buffer 'to'.
 *	'to' is assumed to have the enough size to hold the conversion result.
 *	When 'to' is NOSTR(=(tchar *)0), strtots() attempts to allocate a space
 *	automatically using xalloc().  It is caller's responsibility to
 *	free the space allocated in this way, by calling XFREE(ptr).
 *	In either case, strtots() returns the pointer to the conversion
 *	result (i.e. 'to', if 'to' wasn't NOSTR, or the allocated space.).
 *	When a conversion or allocateion failed,  NOSTR is returned.
 */
tchar	*
strtots(to, from)
     tchar *to;
     unsigned char *from;
{
	int	i;

	if(to==NOSTR){/* Need to xalloc(). */
		int	i;

		i=mbstotcs(NOSTR, from, 0);
		if(i<0) return NOSTR;
		
		/* Allocate space for the resulting tchar array. */
		to=(tchar *)xalloc(i*sizeof(tchar));
	}
#define	INT_MAX		2147483647 /* Should be in <limits.h> */
	i=mbstotcs(to, from, INT_MAX);
	if(i<0) return NOSTR;
	return to;
}

char	*
tstostr(to, from)
     char *to;
     tchar *from;
{
	tchar	*ptc;
	wchar_t	wc;
	char	*pmb;

	if(to==(char *)NULL){/* Need to xalloc(). */
		int	i;
		char	junk[MB_LEN_MAX]; 

		/* Get sum of byte counts for each char in from. */
		i=0;
		ptc=from; 
		while(wc=(wchar_t)((*ptc++)&TRIM)) i+=wctomb(junk, wc);
		
		/* Allocate that much. */
		to=(char *)xalloc(i+1);
	}
	
	ptc=from;
	pmb=to;
	while(wc=(wchar_t)((*ptc++)&TRIM)) pmb+=wctomb(pmb, wc);
	*pmb=(char)0;
	return to;
}

/* mbstotcs(to, from, tosize) is similar to strtots() except that
 * this returns # of tchars of the resulting tchar string.
 * When NULL is give as the destination, no real conversion is carried out,
 * and the function reports how many tchar characters would be made in
 * the converted result including the terminating 0.
 */
int
mbstotcs(to, from, tosize)
     tchar	*to;	/* Destination buffer, or NULL. */
     unsigned char	*from;	/* Source string. */
     int	tosize; /* Size of to, in terms of # of tchars. */
{
	tchar	*ptc=to;
	unsigned char	*pmb=from;
	wchar_t	wc;
	int	chcnt=0;
	int	j;


	if(to==(tchar *)NULL){/* Just count how many tchar would be in the result.*/
		while(*pmb){
			j=mbtowc(&wc, pmb, MB_CUR_MAX);
			if(j<0) return -1;
			pmb+=j;
			chcnt++;
		}
		chcnt++; /* For terminator. */
		return chcnt; /* # of chars including terminating zero. */
	}else{/* Do the real conversion. */
		while(*pmb){
			j=mbtowc(&wc, pmb, MB_CUR_MAX);
			if(j<0) return -1;
			pmb+=j;
			*(ptc++)=(tchar)wc;
			if(++chcnt>=tosize)break;
		}
		if(chcnt<tosize){/* Terminate with zero only when space is left.*/
			*ptc=(tchar)0;
			++chcnt;
		}
		return chcnt; /* # of chars including terminating zero. */
	}
}

/* tchar version of STRING functions. */

/*
 * Returns the number of
 * non-NULL tchar elements in tchar string argument.
 */
int
strlen_(s)
	register tchar *s;
{
	register int n;

	n = 0;
	while (*s++)
		n++;
	return (n);
}

/*
 * Concatenate tchar string s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 */
tchar *
strcat_(s1, s2)
	register tchar *s1, *s2;
{
	register tchar *os1;

	os1 = s1;
	while (*s1++)
		;
	--s1;
	while (*s1++ = *s2++)
		;
	return (os1);
}

/*
 * Compare tchar strings:  s1>s2: >0  s1==s2: 0  s1<s2: <0
 * BUGS: Comparison between two characters are done by subtracting two characters
 *	after converting each to an unsigned long int value.  It might not make
 *	a whole lot of sense to do that if the characters are in represented as wide
 *	characters and the two characters belong to different codesets.
 *	Therefore, this function should be used only to test the equallness.
 */
strcmp_(s1, s2)
	register tchar *s1, *s2;
{

	while (*s1 == *s2++)
		if (*s1++==(tchar)0)
			return (0);
	return (((unsigned long)*s1) - ((unsigned long)*--s2));
}

/*
 * Copy tchar string s2 to s1.  s1 must be large enough.
 * return s1
 */
tchar *
strcpy_(s1, s2)
	register tchar *s1, *s2;
{
	register tchar *os1;

	os1 = s1;
	while (*s1++ = *s2++)
		;
	return (os1);
}

/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 */
tchar *
index_(sp, c)
	register tchar *sp, c;
{

	do {
		if (*sp == c)
			return (sp);
	} while (*sp++);
	return (NULL);
}

/*
 * Return the ptr in sp at which the character c last
 * appears; NOSTR if not found
 */

tchar *
rindex_(sp, c)
	register tchar *sp, c;
{
	register tchar *r;

	r = NOSTR;
	do {
		if (*sp == c)
			r = sp;
	} while (*sp++);
	return (r);
}


/* Additional misc functions. */

/* Calculate the display width of a string.
 */
tswidth(ts)
     tchar	*ts;
{
#ifdef MBCHAR
	wchar_t	tc;
	int	w=0;

	while(tc=*ts++) w+=csetcol(wcsetno((wchar_t)tc));
	return w;
#else/*!MBCHAR --- one char always occupies one column. */
	return strlen_(ts);
#endif
}


/* Two getenv() substitute functions.  They differ in the type of arguments.
 * BUGS: Both returns the pointer to an allocated space where the env var's values
 *	is stored.  This space is freed automatically on the successive call of
 *	either function.  Therefore the caller must copy the contents if it
 * 	needs to access two env vars.
 *	There is an arbitary limitation on the number of chars of a env var name.
 */
#define	LONGEST_ENVVARNAME	256		/* Too big? */
tchar *
getenv_(name_)
     tchar	*name_;
{
	char	name[LONGEST_ENVVARNAME*MB_LEN_MAX];
	
	assert(strlen_(name_)<LONGEST_ENVVARNAME);
	return (getenvs_(tstostr(name, name_)));
}
tchar *
getenvs_(name)
     char	*name;
{
	static tchar	*pbuf=(tchar *)NULL;
	char	*val;
	
	if(pbuf){ 
		XFREE((void *)pbuf); 
		pbuf=NOSTR;
	}
	val=getenv(name);
	if(val==(char *)NULL)return NOSTR;
	return(pbuf=strtots(NOSTR, val));
}

/* Followings are the system call interface for tchar strings. */

/* creat() and open() replacement.
 * BUGS: An unusually long file name could be dangerous.
 */
int 
creat_(name_, mode)
     tchar 	* name_;
     int	mode;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */

	tstostr(chbuf, name_);
	return creat(chbuf, mode);
}

/*VARARGS2*/
int 
open_(path_, flags, mode)
     tchar 	*path_;
     int	flags;
     int	mode; /* May be omitted. */
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */

	tstostr(chbuf, path_);
	return open(chbuf, flags, mode);
}

/* read() and write() reaplacement.
 */
int read_(d, buf, nchreq)
     int	d;
     tchar 	*buf;	/* where the result be stored.  Not NULL terminated.*/
     int	nchreq;	/* # of tchars requrested. */
{
	unsigned char chbuf[BUFSIZ*MB_LEN_MAX]; /* General use buffer. */
#ifdef MBCHAR
	/* We would have to read more than tchar bytes
	 * when there are multibyte characters in the file.
	 */
	register int	i,j;
	register unsigned char	*s;	/* Byte being scanned for a multibyte char.*/
	register unsigned char	*p;	/* Points to the pos where next read() to
				   read the data into. */
	register tchar	*t;
	wchar_t		wc;
	int		nchread=0;/* Count how many bytes has been read.*/
	int		nbytread=0;/* Total # of bytes read. */
	int		delta;/* # of bytes needed to complete the last char
				 just read. */
#ifdef DBG
	tprintf("Entering read_(d=%d, buf=0x%x, nchreq=%d);\n", 
	       d, buf, nchreq);
#endif/*DBG*/
	/* Step 1: We collect the exact number of bytes that make
	 *	nchreq characters into chbuf.
	 *	We must be careful not to read too many bytes as we
	 *	cannot push back such over-read bytes.
	 *	The idea we use here is that n multibyte characters are stored
	 *	in no less than n but less than n*MB_CUR_MAX bytes.
	 */
	assert(nchreq<=BUFSIZ);
	delta=0;
	p=s=chbuf;
	while(nchread<nchreq){
		int		m;	/* # of bytes to try to read this time.*/
		int		k;	/* # of bytes successfully read. */
		unsigned char	*q;	/* q points to the first invalid byte. */

		/* Let's say the (N+1)'th byte bN is actually the first
		 * byte of a three-byte character c.
		 * In that case, p, s, q look like this:
		 *
		 *          /-- already read--\ /-- not yet read --\
		 * chbuf[]: b0 b1 ..... bN bN+1 bN+2 bN+2 ... 
		 *	    ^           ^       ^
		 *	    |		|       |
		 *	    p		s       q
		 *			\----------/
		 *			 c hasn't been completed
		 *
		 * Just after the next read(), p and q will be adavanced to:
		 *
		 *          /-- already read-------------------\ /-- not yet -
		 * chbuf[]: b0 b1 ..... bN bN+1 bN+2 bN+2 ... bX bX+1 bX+2...
		 *	                ^       ^		 ^
		 *	  		|       |		 |
		 *	  		s       p		 q
		 *			\----------/
		 *			 c has been completed 
		 *			 but hasn't been scanned
		 */
		m=nchreq-nchread+delta-(delta?1:0);
		assert(p+m<chbuf+sizeof(chbuf));
		k=read(d, p, m);
		if(k==-1) return -1; /* Read error!*/
		nbytread+=k;
		q=p+k; 
		delta=0;

		/* Try scaning characters in s..q-1 */
		while(s<q){
			j=(*s)?euclen(s):1; /* NUL is treated as a normal char here.*/
			if(s+j>q){/* Needs more byte to complete this char.*/
				delta=(s+j)-q;
				break; /* In order to read() more than delta 
					* bytes.
					*/
			}
			s+=j;
			++nchread;
		}

		if(k<m){/* We've read as many bytes as possible.*/
			break;
		}
		
		p=q;
	}

	/* Step 2: Convert the collected bytes into tchar array.
	 *	Can't use mbstotcs() because NUL byte must be handled
	 *	correctly.
	 */
	i=nbytread;
	s=chbuf;
	t=buf;
	while(i>0){
		if(*s==0){
			wc=(wchar_t)0;
		}else{
			j=mbtowc(&wc, s, i);
			if(j==-1){
				return -1;
			}
		}
		*t++=wc;
		i-=j;
		s+=j;	/* Advance s to the next multibyte char. */
	}
	return nchread;
#else/*!MBCHAR*/
	/* One byte always represents one tchar.  Easy! */
	register int	i;
	register unsigned char	*s;
	register tchar	*t;
	int		nchread;

#ifdef DBG
	tprintf("Entering read_(d=%d, buf=0x%x, nchreq=%d);\n", 
	       d, buf, nchreq);
#endif/*DBG*/
	assert(nchreq<=BUFSIZ);
	nchread=read(d, (char *)chbuf, nchreq);
	if( nchread>0 ){
		for(i=0, t=buf, s=chbuf; i<nchread; ++i)
		    *t++=((tchar)*s++);
	}
	return nchread;
#endif
}
int write_(d, buf, nch)
     int	d;
     tchar 	*buf;
     int	nch;/* # of tchars to write. */
/* BUG: write_() returns -1 on failure, or # of BYTEs it has written.
 *	For consistency and symmetry, it should return the number of
 *	characters it has actually written, but that is technically
 *	difficult although not impossible.  Anyway, the return
 *	value of write() has never been used by the original csh,
 *	so this bug should be OK.
 */
{
	unsigned char chbuf[BUFSIZ*MB_LEN_MAX]; /* General use buffer. */
#ifdef MBCHAR
	tchar		*pt;
	unsigned char	*pc;
	wchar_t		wc;
	int		i, j;

#ifdef	DBG
	tprintf("Entering write_(d=%d, buf=0x%x, nch=%d);\n",
	       d, buf, nch); /* Hope printf() doesn't call write_() itself!*/
#endif/*DBG*/
	assert(nch*MB_CUR_MAX<sizeof(chbuf));
	i=nch;
	pt=buf;
	pc=chbuf;
	while(i--){
		/* Convert to tchar string.  NUL is treated as normal char here.*/
		wc=(wchar_t)((*pt++)&TRIM);
		if(wc==(wchar_t)0){
			*pc++=0;
		}else{
			j=wctomb(pc, wc);
			if(j==-1) return -1;
			pc+=j;
		}
	}
	return write(d, chbuf, pc-chbuf);
#else/*!MBCHAR*/
	/* One byte always represents one tchar.  Easy! */
	register int	i;
	register unsigned char	*s;
	register tchar	*t;

#ifdef	DBG
	tprintf("Entering write_(d=%d, buf=0x%x, nch=%d);\n",
	       d, buf, nch); /* Hope printf() doesn't call write_() itself!*/
#endif/*DBG*/
	assert(nch<=sizeof(chbuf));
	for(i=0, t=buf, s=chbuf; i<nch; ++i)
	    *s++=(char)((*t++)&0xff);
	return write(d, (char *)chbuf, nch);
#endif
}
#undef chbuf

#include <sys/types.h>
#include <sys/stat.h>	/* satruct stat */
#include <dirent.h>	/* DIR */

int
stat_(path, buf)
     tchar		*path;
     struct stat	*buf;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, path);
	return stat(chbuf, buf);
}

int
lstat_(path, buf)
     tchar		*path;
     struct stat	*buf;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, path);
	return lstat(chbuf, buf);
}

int
access_(path, mode)
     tchar		*path;
     int		mode;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, path);
	return access(chbuf, mode);
}

int
chdir_(path)
     tchar		*path;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, path);
	return chdir(chbuf);
}

tchar *
getwd_(path)
     tchar		*path;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	int	rc;
	
	rc=(int) getwd(chbuf);
	if(rc==0) {
		strtots(path, chbuf);
		return 0;
	}
	else 
		return strtots(path, chbuf);
}

int
unlink_(path)
     tchar 		*path;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, path);
	return unlink(chbuf);
}

DIR *
opendir_(dirname)
     tchar		* dirname;
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	extern DIR *opendir();

	return opendir(tstostr(chbuf, dirname));
}

int 
gethostname_(name, namelen)
     tchar *		name;
     int		namelen; 
{
	unsigned char chbuf[BUFSIZ*MB_LEN_MAX]; /* General use buffer. */
	assert(namelen<BUFSIZ);
	if(gethostname(chbuf, sizeof(chbuf))!=0) return -1;
	if(mbstotcs(name, chbuf, namelen)<0) return -1;
	return 0; /* Succeeded. */
}

int 
readlink_(path, buf, bufsiz)
     tchar	*path;
     tchar	*buf;
     int	bufsiz; /* Size of buf in terms of # of tchars. */
{
	unsigned char chbuf[MAXPATHLEN*MB_LEN_MAX]; /* General use buffer. */
	char	chpath[MAXPATHLEN+1];
	int	i;

	tstostr(chpath, path);
	i=readlink(chpath, chbuf, sizeof(chbuf));
	if(i<0) return -1;
	chbuf[i]=(char)0;	/* readlink() doesn't put NULL. */
	i=mbstotcs(buf, chbuf, bufsiz);
	if(i<0) return -1;
	return i-1; /* Return # of tchars EXCLUDING the terminating NULL. */
}

double
atof_(str)
     tchar	       *str;
{
	unsigned char chbuf[BUFSIZ*MB_LEN_MAX]; /* General use buffer. */
	extern double atof();

	tstostr(chbuf, str);
	return atof(chbuf);
}

int 
atoi_(str)
     tchar		*str;
{
	unsigned char chbuf[BUFSIZ*MB_LEN_MAX]; /* General use buffer. */
	tstostr(chbuf, str);
	return atoi(chbuf);
}
