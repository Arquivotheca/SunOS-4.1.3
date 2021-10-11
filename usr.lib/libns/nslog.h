
/*	@(#)nslog.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:nslog.h	1.8" */
/*
 *	nslog.h contains defines and externs for logging
 */

#ifdef	LOGGING
#define	LOG1(x,a)	if (Loglevel&(x)) fprintf(Logfd,a)
#define	LOG2(x,a,b)	if (Loglevel&(x)) fprintf(Logfd,a,b)
#define	LOG3(x,a,b,c)	if (Loglevel&(x)) fprintf(Logfd,a,b,c)
#define	LOG4(x,a,b,c,d)	if (Loglevel&(x)) fprintf(Logfd,a,b,c,d)
#define	LOG5(x,a,b,c,d,e)	if (Loglevel&(x)) fprintf(Logfd,a,b,c,d,e)
#define	LOG6(x,a,b,c,d,e,f)	if (Loglevel&(x)) fprintf(Logfd,a,b,c,d,e,f)
#define	LOG7(x,a,b,c,d,e,f,g)	if (Loglevel&(x)) fprintf(Logfd,a,b,c,d,e,f,g)

#ifdef	LOGMALLOC
/* defines to track malloc and free	*/
#define	free(p)			xfree(p)
#define	malloc(s)		xmalloc(s)
#define	calloc(n, s)		xcalloc(n, s)
#define	realloc(p, s)		xrealloc(p, s)

/* and declarations	*/
void	xfree();
char	*xmalloc();
char	*xcalloc();
char	*xrealloc();
#endif

#else

#include <malloc.h>

#define	LOG1(x,a)
#define	LOG2(x,a,b)
#define	LOG3(x,a,b,c)
#define	LOG4(x,a,b,c,d)
#define	LOG5(x,a,b,c,d,e)
#define	LOG6(x,a,b,c,d,e,f)
#define	LOG7(x,a,b,c,d,e,f,g)

#endif

#define	L_DB	 0x01	/* data base functions		*/
#define	L_MALLOC 0x02	/* malloc and free tracing	*/
#define	L_COMM	 0x04	/* communications (nsports)	*/
#define	L_BLOCK	 0x08	/* block making, (nsblock)	*/
#define	L_CONV	 0x10	/* data conversion, (ind_data)	*/
#define	L_TRANS	 0x20	/* transactions, (nsfunc)	*/
#define	L_PIPE	 0x40	/* pipe communications, (ptype)	*/
#define	L_STREAM 0x80	/* stream comm., (stype)	*/
#define	L_OVER	 0x100	/* overview.			*/
#define	L_REC	 0x200	/* log recovery stuff		*/
#define	L_ALL	 0xffff	/* log every thing		*/

/* PLOGx defines those things that will be permanently logged	*/
#define	PLOG1(a)	 	fprintf(Logfd,a)
#define	PLOG2(a,b)	 	fprintf(Logfd,a,b)
#define	PLOG3(a,b,c)	 	fprintf(Logfd,a,b,c)
#define	PLOG4(a,b,c,d)		fprintf(Logfd,a,b,c,d)
#define	PLOG5(a,b,c,d,e)	fprintf(Logfd,a,b,c,d,e)
#define	PLOG6(a,b,c,d,e,f)	fprintf(Logfd,a,b,c,d,e,f)
#define	PLOG7(a,b,c,d,e,f,g)	fprintf(Logfd,a,b,c,d,e,f,g)

/* externs for debug */

extern int	Loglevel;
extern FILE	*Logfd;
extern int	Logstamp;
extern char	Logbuf[];
char	*prtype(/*errtype*/); /* returns a string for the error type */
char	*fntype(/*errtype*/); /* returns a string for the request type */
