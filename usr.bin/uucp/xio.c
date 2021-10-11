/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)xio.c 1.1 92/07/30"	/* from SVR3.2 uucp:xio.c 2.5 */

#include "uucp.h"

#ifdef X25

#define XBUFSIZ 512
static jmp_buf Xfailbuf;

/*
 * x.25 protocol
 */
static xalarm()
{
	longjmp(Xfailbuf, 1);
}

static void (*xsig)();

/*
 * turn on protocol timer
 */
xturnon()
{
	xsig=signal(SIGALRM, xalarm);
	return(0);
}
xturnoff()
{
	(void) signal(SIGALRM, xsig);
	return(0);
}

/*
 * write message across x.25 link
 *	type	-> message type
 *	str	-> message body (ascii string)
 *	fn	-> x.25 file descriptor
 * return
 *	0
 */
xwrmsg(type, str, fn)
register char *str;
int fn;
char type;
{
	register char *s;
	char bufr[XBUFSIZ];

	bufr[0] = type;
	s = &bufr[1];
	while (*str)
		*s++ = *str++;
	*s = '\0';
	if (*(--s) == '\n')
		*s = '\0';
	(void) (*Write)(fn, bufr, strlen(bufr) + 1);
	return(0);
}

/*
 * read message from x.25 link
 *	str	-> message buffer
 *	fn	-> x.25 file descriptor
 * return
 *	FAIL	-> send timed out
 *	0	-> ok message in str
 */
xrdmsg(str, fn)
register char *str;
{
	register unsigned len;

	if(setjmp(Xfailbuf))
		return(FAIL);

	for (;;) {
		(void) alarm(60);
		if( (len = (*Read)(fn, str, XBUFSIZ)) == 0)
			continue;
		str += len;
		if (*(str - 1) == '\0')
			break;
	}
	(void) alarm(0);
	return(0);
}

/*
 * read data from file fp1 and write
 * on x.25 link
 *	fp1	-> file descriptor
 *	fn	-> x.25 descriptor
 * returns:
 *	FAIL	->failure in x.25 link
 *	0	-> ok
 */
xwrdata(fp1, fn)
FILE *fp1;
{
	register fd1;
	register int len, ret;
	long bytes;
	char bufr[XBUFSIZ];
	char text[XBUFSIZ];
	time_t ticks;

	bytes = 0L;
	fd1 = fileno( fp1 );
	(void) millitick();	/* set msec timer */
	while ((len = read( fd1, bufr, XBUFSIZ )) > 0) {
		bytes += len;
		ret = (*Write)(fn, bufr, len);
		if (ret != len) {
			return(FAIL);
		}
		if (len != XBUFSIZ)
			break;
	}
	ret = (*Write)(fn, bufr, 0);
	ticks = millitick();
	statlog( "->", bytes, ticks );
	return(0);
}

/*
 * read data from x.25 link and
 * write into file
 *	fp2	-> file descriptor
 *	fn	-> x.25 descriptor
 * returns:
 *	0	-> ok
 *	FAIL	-> failure on x.25 link
 */
xrddata(fn, fp2)
FILE *fp2;
{
	register int fd2;
	register int len;
	long bytes;
	char text[XBUFSIZ];
	char bufr[XBUFSIZ];
	time_t ticks;

	bytes = 0L;
	fd2 = fileno( fp2 );
	(void) millitick();	/* set msec timer */
	for (;;) {
		len = xrdblk(bufr, XBUFSIZ, fn);
		if (len < 0) {
			return(FAIL);
		}
		bytes += len;
		if( write( fd2, bufr, len ) != len )
			return(FAIL);
		if (len < XBUFSIZ)
			break;
	}
	ticks = millitick();
	statlog( "<-", bytes, ticks );
	return(0);
}

/*
 * read blank from x.25 link
 * reads are timed
 *	blk	-> address of buffer
 *	len	-> size to read
 *	fn	-> x.25 descriptor
 * returns:
 *	FAIL	-> link error timeout on link
 *	i	-> # of bytes read
 */
xrdblk(blk, len,  fn)
register char *blk;
{
	register int i, ret;

	if(setjmp(Xfailbuf))
		return(FAIL);

	for (i = 0; i < len; i += ret) {
		(void) alarm(60);
		if ((ret = (*Read)(fn, blk, len - i)) < 0) {
			(void) alarm(0);
			return(FAIL);
		}
		blk += ret;
		if (ret == 0)
			break;
	}
	(void) alarm(0);
	return(i);
}
#endif /* X25 */
