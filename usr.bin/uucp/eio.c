/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)eio.c 1.1 92/07/30"	/* from SVR3.2 uucp:eio.c 2.9 */

#include "uucp.h"

#ifdef ATTSV
#define     MIN(a,b) (((a)<(b))?(a):(b))
#endif

#ifdef BSD4_2
#include <netinet/in.h>
#endif /*BSD4_2*/

#define	EBUFSIZ	1024
#define	EMESGLEN 20

#define TBUFSIZE 1024
#define TPACKSIZE	512

#define EMSGTIME (2*MAXMSGTIME)

static jmp_buf Failbuf;

static char Erdstash[EBUFSIZ];
static int Erdlen;

/*
 * error-free channel protocol
 */
static
ealarm() {
	longjmp(Failbuf, 1);
}
static void (*esig)();

/*
 * turn on protocol timer
 */
eturnon()
{
	esig=signal(SIGALRM, ealarm);
	return(0);
}

eturnoff()
{
	signal(SIGALRM, esig);
	return(0);
}

/*
 * write message across link
 *	type	-> message type
 *	str	-> message body (ascii string)
 *	fn	-> link file descriptor
 * return
 *	FAIL	-> write failed
 *	SUCCESS	-> write succeeded
 */
ewrmsg(type, str, fn)
register char *str;
int fn;
char type;
{
	return(etwrmsg(type, str, fn, 0));
}

/*
 * read message from link
 *	str	-> message buffer
 *	fn	-> file descriptor
 * return
 *	FAIL	-> read timed out
 *	SUCCESS	-> ok message in str
 */
erdmsg(str, fn)
register char *str;
{
	return(etrdmsg(str, fn, 0));
}

/*
 * read data from file fp1 and write
 * on link
 *	fp1	-> file descriptor
 *	fn	-> link descriptor
 * returns:
 *	FAIL	->failure in link
 *	SUCCESS	-> ok
 */
ewrdata(fp1, fn)
register FILE *fp1;
int	fn;
{
	register int ret;
	int	fd1;
	int len;
	char bufr[EBUFSIZ];
	time_t	ticks;
	struct stat	statbuf;
	off_t	msglen;
	char	cmsglen[EMESGLEN];

	if (setjmp(Failbuf)) {
		DEBUG(7, "ewrdata failed\n", 0);
		return(FAIL);
	}
	fd1 = fileno(fp1);
	fstat(fd1, &statbuf);
	msglen = statbuf.st_size;
	(void) millitick();
	sprintf(cmsglen, "%ld", (long) msglen);
	DEBUG(9, "ewrdata writing %d ...", sizeof(cmsglen));
	alarm(EMSGTIME);
	ret = (*Write)(fn, cmsglen, sizeof(cmsglen));
	alarm(0);
	DEBUG(9, "ret %d\n", ret);
	if (ret != sizeof(cmsglen))
		return(FAIL);
	while ((len = read( fd1, bufr, EBUFSIZ )) > 0) {
		DEBUG(7, "ewrdata writing %d ...", len);
		alarm(EMSGTIME);
		ret = (*Write)(fn, bufr, (unsigned) len);
		alarm(0);
		DEBUG(7, "ret %d\n", ret);
		if (ret != len)
			return(FAIL);
		if ((msglen -= len) <= 0)
			break;
	}
	if (len < 0 || (len == 0 && msglen != 0))
		return(FAIL);
	ticks = millitick();
	statlog( "->", statbuf.st_size, ticks );
	return(SUCCESS);
}

/*
 * read data from link and
 * write into file
 *	fp2	-> file descriptor
 *	fn	-> link descriptor
 * returns:
 *	SUCCESS	-> ok
 *	FAIL	-> failure on link
 */
erddata(fn, fp2)
register FILE *fp2;
{
	register int ret;
	int	fd2;
	char bufr[EBUFSIZ];
	time_t	ticks;
	int	len;
	long	msglen, bytes;
	char	cmsglen[EMESGLEN], *cptr, *erdptr = Erdstash;
	int	writefile = TRUE;

	(void) millitick();
	DEBUG(9, "erddata wants %d\n", sizeof(cmsglen));
	if (Erdlen > 0) {
		DEBUG(9, "%d bytes stashed\n", Erdlen);
		if (Erdlen >= sizeof(cmsglen)) {
			memcpy(cmsglen, erdptr, sizeof(cmsglen));
			Erdlen -= sizeof(cmsglen);
			erdptr += sizeof(cmsglen);
			ret = len = 0;
		} else {
			memcpy(cmsglen, Erdstash, Erdlen);
			cptr = cmsglen + Erdlen;
			len = sizeof(cmsglen) - Erdlen;
			ret = erdblk(cptr, len, fn);
			Erdlen = 0;
		}
	} else {
		len = sizeof(cmsglen);
		ret = erdblk(cmsglen, sizeof(cmsglen), fn);
	}
	if (ret != len)
		return(FAIL);
	sscanf(cmsglen, "%ld", &msglen);
	if ( ((msglen-1)/512 +1) > Ulimit ) {
		ret = EFBIG;
		writefile = FALSE;
	}
	bytes = msglen;
	DEBUG(7, "erddata file is %ld bytes\n", msglen);
	fd2 = fileno( fp2 );

	if (Erdlen > 0) {
		DEBUG(9, "%d bytes stashed\n", Erdlen);
		if (write(fileno(fp2), erdptr, Erdlen) != Erdlen)
			return(FAIL);
		msglen -= Erdlen;
		Erdlen = 0;
		DEBUG(7, "erddata remainder is %ld bytes\n", msglen);
	}	

	for (;;) {
		DEBUG(9, "erddata wants %d\n", (int) MIN(msglen, EBUFSIZ));
		ret = erdblk(bufr, (int) MIN(msglen, EBUFSIZ), fn);
		if (ret < 0)
			return(FAIL);
		if ((msglen -= ret) < 0) {
			DEBUG(7, "erdblk read too much\n", 0);
			return(FAIL);
		}
		/* this write is to file -- use write(2), not (*Write) */
		if ( writefile == TRUE && write( fd2, bufr, ret ) != ret ) {
			ret = errno;
			DEBUG(7, "erddata: write to file failed, errno %d\n", ret);
			writefile = FALSE;
		}
		if (msglen == 0)
			break;
	}
	if ( writefile == TRUE ) {
		ticks = millitick();
		statlog( "<-", bytes, ticks );
		return(SUCCESS);
	}
	else
		return(FAIL);
}

/*
 * read block from link
 * reads are timed
 *	blk	-> address of buffer
 *	len	-> size to read
 *	fn	-> link descriptor
 * returns:
 *	FAIL	-> link error timeout on link
 *	i	-> # of bytes read (must not be 0)
 */
erdblk(blk, len,  fn)
register char *blk;
{
	register int i, ret;

	if(setjmp(Failbuf)) {
		DEBUG(7, "timeout (%d sec)\n", EMSGTIME);
		return(FAIL);
	}

	alarm(EMSGTIME);
	for (i = 0; i < len; i += ret) {
		DEBUG(7, "erdblk want %d ...", len - i);
		ret = (*Read)(fn, blk, (unsigned) len - i);
		DEBUG(7, "got %d\n", ret);
		if (ret < 0) {
			alarm(0);
			return(FAIL);
		}
		if (ret == 0)
			break;
		blk += ret;
	}
	alarm(0);
	return(i);
}

struct tbuf {
	long t_nbytes;
	char t_data[TBUFSIZE];
};

/*
 * read message from link
 *	str	-> message buffer
 *	fn	-> file descriptor
 * return
 *	FAIL	-> read timed out
 *	SUCCESS	-> ok message in str
 */
trdmsg(str, fn)
char *str;
{
	return(etrdmsg(str, fn, TPACKSIZE));
}

/*
 * write message across link
 *	type	-> message type
 *	str	-> message body (ascii string)
 *	fn	-> link file descriptor
 * return
 *	FAIL	-> write failed
 *	SUCCESS	-> write succeeded
 */
twrmsg(type, str, fn)
char type;
char *str;
{
	return(etwrmsg(type, str, fn, TPACKSIZE));
}

/*
 * read data from file fp1 and write on link
 *	fp1	-> file descriptor
 *	fn	-> link descriptor
 * returns:
 *	FAIL	->failure in link
 *	SUCCESS	-> ok
 */
twrdata(fp1, fn)
register FILE *fp1;
int	fn;
{
	register int ret;
	int len;
	struct tbuf bufr;
	struct stat statbuf;
	time_t	ticks;

	if (setjmp(Failbuf)) {
		DEBUG(7, "twrdata failed\n", 0);
		return(FAIL);
	}
	fstat(fileno(fp1), &statbuf);
	(void) millitick();
	while ((len = read(fileno(fp1), bufr.t_data, TBUFSIZE)) > 0) {
		bufr.t_nbytes = htonl((long)len);
		DEBUG(7, "twrdata writing %d ...", len);
		len += sizeof(long);
		alarm(EMSGTIME);
		ret = (*Write)(fn, (char *)&bufr, (unsigned) len);
		alarm(0);
		DEBUG(7, "ret %d\n", ret);
		if (ret != len)
			return(FAIL);
		if (len != TBUFSIZE+sizeof(long))
			break;
	}
	bufr.t_nbytes = 0;
	alarm(EMSGTIME);
	ret = write(fn, (char *)&bufr, sizeof(long));
	alarm(0);
	if (ret != sizeof(long))
		return FAIL;
	ticks = millitick();
	statlog( "->", statbuf.st_size, ticks );
	return(SUCCESS);
}

/*
 * read data from link and write into file
 *	fp2	-> file descriptor
 *	fn	-> link descriptor
 * returns:
 *	SUCCESS	-> ok
 *	FAIL	-> failure on link
 */
trddata(fn, fp2)
register FILE *fp2;
{
	register int len, nread;
	long bytes, Nbytes;
	char bufr[TBUFSIZE];

	bytes = 0L;
	(void) millitick();
	for (;;) {
		len = erdblk((char *)&Nbytes, sizeof(Nbytes), fn);
		DEBUG(7, "trddata ret %d\n", len);
		if (len != sizeof(Nbytes))
			return(FAIL);
		Nbytes = ntohl(Nbytes);
		DEBUG(7,"trddata expecting %ld bytes\n", Nbytes);
		nread = Nbytes;
		if (nread == 0)
			break;
		len = erdblk(bufr, nread, fn);
		if (len != Nbytes)
			return(FAIL);
		if (write(fileno(fp2), bufr, len) != len)
			return(FAIL);
		bytes += len;
	}
	statlog( "<-", bytes, millitick() );
	return(SUCCESS);
}

/*
 * read message from link
 *	str	-> message buffer
 *	fn	-> file descriptor
 *	i	-> if non-zero, amount to read; o.w., read up to '\0'
 * return
 *	FAIL	-> read timed out
 *	SUCCESS	-> ok message in str
 *
 * 'e' is fatally flawed -- in a byte stream world, rdmsg can pick up
 * the cmsglen on a R request.  if this happens, we stash the excess
 * where rddata can pick it up.
 */

etrdmsg(str, fn, i)
register char *str;
register int i;
{
	register long len;
	int nullterm = 0;
	char *null, *argstr;


	if (i == 0) {
		DEBUG(9, "etrdmsg looking for null terminator\n", 0);
		nullterm++;
		i = EBUFSIZ;
		argstr = str;
	}

	if(setjmp(Failbuf)) {
		DEBUG(7, "timeout (%d sec)\n", EMSGTIME);
		return(FAIL);
	}

	alarm(EMSGTIME);
	for (;;) {
		DEBUG(9, "etrdmsg want %d ...", i);
		len = (*Read)(fn, str, i);
		DEBUG(9, "got %d\n", len);
		if (len == 0)
			continue;	/* timeout will get this */
		if (len < 0) {
			alarm(0);
			return(FAIL);
		}
		str += len;
		i -= len;
		if (nullterm) {
			/* no way can a msg be as long as EBUFSIZ-1 ... */
			*str = 0;
			null = strchr(argstr, '\0');
			if (null != str) {
				null++;	/* start of stash */
				memcpy(Erdstash + Erdlen, null, str - null);
				Erdlen += str - null;
				break;
			} else
				argstr = str;
		} else {
			if (i == 0)
				break;
		}
	}
	alarm(0);
	return(SUCCESS);
}

/*
 * write message across link
 *	type	-> message type
 *	str	-> message body (ascii string)
 *	fn	-> link file descriptor
 *	len	-> if non-zero, amount to write;
		   o.w., write up to '\0' (inclusive)
 * return
 *	FAIL	-> write failed
 *	SUCCESS	-> write succeeded
 */
etwrmsg(type, str, fn, len)
char type;
register char *str;
int fn, len;
{
	char bufr[EBUFSIZ], *endstr;
	int ret;

	bufr[0] = type;

	/* point endstr to last character to be sent */
	if ((endstr = strchr(str, '\n')) != 0)
		*endstr = 0;
	else
		endstr = str + strlen(str);

	memcpy(bufr+1, str, (endstr - str) + 1);	/* include '\0' */
	if (len == 0)
		len = (endstr - str) + 2;	/* include bufr[0] and '\0' */
	else
		bufr[len-1] = 0;		/* 't' needs this terminator */


	if (setjmp(Failbuf)) {
		DEBUG(7, "etwrmsg write failed\n", 0);
		return(FAIL);
	}
	DEBUG(9, "etwrmsg want %d ... ", len);
	alarm(EMSGTIME);
	ret = (*Write)(fn, bufr, (unsigned) len);
	alarm(0);
	DEBUG(9, "sent %d\n", ret);
	if (ret != len)
		return(FAIL);
	return(SUCCESS);
}
