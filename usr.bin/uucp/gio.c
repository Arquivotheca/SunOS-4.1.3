/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)gio.c 1.1 92/07/30"	/* from SVR3.2 uucp:gio.c 2.5 */

#include "uucp.h"

#include "pk.h"

jmp_buf Gfailbuf;

pkfail()
{
	longjmp(Gfailbuf, 1);
}

gturnon()
{
	struct pack *pkopen();
	if (setjmp(Gfailbuf))
		return(FAIL);
	if (Debug > 4)
		pkdebug = 1;
	Pk = pkopen(Ifn, Ofn);
	if (Pk == NULL)
		return(FAIL);
	return(0);
}


gturnoff()
{
	if(setjmp(Gfailbuf))
		return(FAIL);
	pkclose();
	return(0);
}


/*ARGSUSED*/
gwrmsg(type, str, fn)
char type, *str;
{
	char bufr[BUFSIZ], *s;
	int len, i;

	if(setjmp(Gfailbuf))
		return(FAIL);
	bufr[0] = type;
	s = &bufr[1];
	while (*str)
		*s++ = *str++;
	*s = '\0';
	if (*(--s) == '\n')
		*s = '\0';
	len = strlen(bufr) + 1;
	if ((i = len % PACKSIZE)) {
		len = len + PACKSIZE - i;
		bufr[len - 1] = '\0';
	}
	gwrblk(bufr, len);
	return(0);
}


/*ARGSUSED*/
grdmsg(str, fn)
char *str;
{
	unsigned len;

	if(setjmp(Gfailbuf))
		return(FAIL);
	for (;;) {
		len = pkread(str, PACKSIZE);
		if (len == 0)
			continue;
		str += len;
		if (*(str - 1) == '\0')
			break;
	}
	return(0);
}


/*ARGSUSED*/
gwrdata(fp1, fn)
FILE *fp1;
{
	char bufr[BUFSIZ];
	int fd1;
	int len;
	int ret;
	time_t ticks;
	long bytes;

	if(setjmp(Gfailbuf))
		return(FAIL);
	bytes = 0L;
	fd1 = fileno( fp1 );
	(void) millitick();	/* set msec timer */
	while ((len = read( fd1, bufr, BUFSIZ )) > 0) {
		bytes += len;
		ret = gwrblk(bufr, len);
		if (ret != len) {
			return(FAIL);
		}
		if (len != BUFSIZ)
			break;
	}
	ret = gwrblk(bufr, 0);
	ticks = millitick();
	statlog( "->", bytes, ticks );
	return(0);
}

/*ARGSUSED*/
grddata(fn, fp2)
FILE *fp2;
{
	int fd2;
	int len;
	char bufr[BUFSIZ];
	time_t ticks;
	long bytes;

	if(setjmp(Gfailbuf))
		return(FAIL);
	bytes = 0L;
	fd2 = fileno( fp2 );
	(void) millitick();	/* set msec timer */
	for (;;) {
		len = grdblk(bufr, BUFSIZ);
		if (len < 0) {
			return(FAIL);
		}
		bytes += len;
		if (write( fd2, bufr, len ) != len) {
			DEBUG(7, "grddata: write to file failed, errno %d\n", errno);
			return(FAIL);
		}
		if (len < BUFSIZ)
			break;
	}
	ticks = millitick();
	statlog( "<-", bytes, ticks );
	return(0);
}


static
grdblk(blk, len)
char *blk;
{
	int i, ret;

	for (i = 0; i < len; i += ret) {
		ret = pkread(blk, len - i);
		if (ret < 0)
			return(FAIL);
		blk += ret;
		if (ret == 0)
			return(i);
	}
	return(i);
}


static int
gwrblk(blk, len)
char *blk;
{
	return(pkwrite(blk, len));
}
