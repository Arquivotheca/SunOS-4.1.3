/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)dio.c 1.1 92/07/30"	/* from SVR3.2 uucp:dio.c 2.6 */

#include "uucp.h"

#ifdef DATAKIT
#include "dk.h"

#define XBUFSIZ 1024
time_t time();
static jmp_buf Dfailbuf;

/*
 * Datakit protocol
 */
static dalarm() {longjmp(Dfailbuf);}
static void (*dsig)();
#ifndef V8
static short dkrmode[3] = { DKR_BLOCK, 0, 0 };
static short dkeof[3] = { 106, 0, 0 };	/* End of File signal */
#endif

/*
 * turn on protocol
 */
dturnon()
{
#ifdef V8
	extern int dkp_ld;
#endif

	dsig=signal(SIGALRM, dalarm);
#ifdef V8
	if (dkproto(Ofn, dkp_ld) < 0)
	   {
		DEBUG(3, "%s\n", "No dkp_ld");
		return(-1);
	   }
#else
	if((*Ioctl)(Ofn, DIOCRMODE, dkrmode) < 0) {
	    int ret; extern int errno;
	    ret=(*Ioctl)(Ofn, DIOCRMODE, dkrmode);
	    DEBUG(4, "dturnon: ret=%d, ", ret);
	    DEBUG(4, "Ofn=%d, ", Ofn);
	    DEBUG(4, "errno=%d\n", errno);
	    return(-1);
	}
#endif /* V8 */
	return(0);
}
dturnoff()
{
	(void) signal(SIGALRM, dsig);
	return(0);
}

/*
 * write message across Datakit link
 *	type	-> message type
 *	str	-> message body (ascii string)
 *	fn	-> Datakit file descriptor
 * return
 *	SUCCESS	-> message sent
 *	FAIL	-> write failed
 */
dwrmsg(type, str, fn)
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
	return((*Write)(fn, bufr, (unsigned) strlen(bufr) + 1) < 0 ? FAIL : SUCCESS);
}

/*
 * read message from Datakit link
 *	str	-> message buffer
 *	fn	-> Datakit file descriptor
 * return
 *	FAIL	-> send timed out
 *	SUCCESS	-> ok message in str
 */
drdmsg(str, fn)
register char *str;
{

	register int len;

	if(setjmp(Dfailbuf))
		return(FAIL);

	(void) alarm(60);
	for (;;) {
		if( (len = (*Read)(fn, str, XBUFSIZ)) <= 0) {
			(void) alarm(0);
			return(FAIL);
		}
		str += len;
		if (*(str - 1) == '\0')
			break;
	}
	(void) alarm(0);
	return(SUCCESS);
}

/*
 * read data from file fp1 and write
 * on Datakit link
 *	fp1	-> file descriptor
 *	fn	-> Datakit descriptor
 * returns:
 *	FAIL	->failure in Datakit link
 *	SUCCESS	-> ok
 */
dwrdata(fp1, fn)
FILE *fp1;
{
	register int fd1;
	register int len, ret;
	long bytes;
	char bufr[XBUFSIZ];
	char text[128];
	time_t	ticks;

	bytes = 0L;
	fd1 = fileno( fp1 );
	(void) millitick();	/* set msec timer */
	while ((len = read( fd1, bufr, XBUFSIZ )) > 0) {
		bytes += len;
		ret = (*Write)(fn, bufr, (unsigned) len);
		if (ret != len) {
			return(FAIL);
		}
		if (len != XBUFSIZ)
			break;
	}
#ifndef V8
	(*Ioctl)(fn, DIOCXCTL, dkeof);
#endif
	ret = (*Write)(fn, bufr, (unsigned) 0);
	ticks = millitick();
	statlog( "->", bytes, ticks );
	return(SUCCESS);
}

/*
 * read data from Datakit link and
 * write into file
 *	fp2	-> file descriptor
 *	fn	-> Datakit descriptor
 * returns:
 *	SUCCESS	-> ok
 *	FAIL	-> failure on Datakit link
 */
drddata(fn, fp2)
FILE *fp2;
{
	register int fd2;
	register int len;
	long bytes;
	char text[128];
	char bufr[XBUFSIZ];
	time_t ticks;

	bytes = 0L;
	fd2 = fileno( fp2 );
	(void) millitick();	/* set msec timer */
	for (;;) {
		len = drdblk(bufr, XBUFSIZ, fn);
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
	return(SUCCESS);
}

/*
 * read block from Datakit link
 * reads are timed
 *	blk	-> address of buffer
 *	len	-> size to read
 *	fn	-> Datakit descriptor
 * returns:
 *	FAIL	-> link error timeout on link
 *	i	-> # of bytes read
 */
drdblk(blk, len,  fn)
register char *blk;
{
	register int i, ret;
	struct dkqqabo	why;

	if(setjmp(Dfailbuf))
		return(FAIL);

	for (i = 0; i < len; i += ret) {
		(void) alarm(60);
		if ((ret = (*Read)(fn, blk, (unsigned) len - i)) < 0) {
			(void) alarm(0);
			return(FAIL);
		}
		blk += ret;
		if (ret == 0) {	/* zero length block contains only EOF signal */
			ioctl(fn, DIOCQQABO, &why);
			if (why.rcv_ctlchar != dkeof[0])
				i = FAIL;
			break;
		}
	}
	(void) alarm(0);
	return(i);
}
#endif /* D_PROTOCOL */
