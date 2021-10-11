/*	@(#)lssmb.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:lssmb.c	1.5"

/*
 * lssmb.c:	Contains all code specific to the  MS-NET file server.
 *		Undef SMBSERVER to remove SMB support.
 */


#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <tiuser.h>

#include "lsparam.h"
#include "lssmbmsg.h"
#include "lsdbf.h"

#define lobyte(X)	(((unsigned char *)&X)[1])
#define hibyte(X)	(((unsigned char *)&X)[0])

#ifdef	SMBSERVER


/*
 * Dlevel	- Debug level for DEBUG((level, ... ) type calls
 * Msnet	- Who is logging this message (the SMB code is)
 */

#define Dlevel	3
#define Msnet	"SMB parser:"

extern char *malloc();

/*
 * In the event of an error, it may be necessary to send a response to
 * the remote node before closing the virtual circuit.  The following
 * is the return message that should be sent.  (Initially, I am not
 * bothering to send the response message; I am assuming that the
 * MS-NET client will be able to figure out that things went wrong, but
 * we may find that is not the case.
 */

static char errbuf[] = {
/* NegProt Return	*/	0xff, 'S', 'M', 'B', 0x72,
/* ERRSRV		*/	0x2,
				0,
/* SMBerror		*/	0x1, 0,
				0,
				0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				0, 0,
				0, 0,
				0, 0, 0, 0,
/* wcnt == 1		*/	1,
/* no dialects		*/	0xff, 0xff,
				0, 0
};


/*
 * s m b s e r v i c e
 *
 * Function called by listener process when it receives a connect
 * request from a node that wants to talk Microsoft's MS-NET Core
 * Protocol...the functions gets called after the listener forks.
 */

smbservice(fd, bp, bufsize, call, argv)
int fd;			/* new virtual circuit (should be zero) */
register char *bp;	/* pointer to message buffer */
int bufsize;		/* size of message */
struct t_call	*call;	/* pointer to call structure from t_accept */
char **argv;		/* server arguments */
{
	char *server = *argv;	/* path of server 		*/
	char logbuf[256];
	register char **args;
	register int i, m_size;
	register int twos, nulls;
	register char *p, *q;
	short size;

	/*
	 * Is this really a correct negotiate protocol message?
	 */

	if (*(bp+FSP_COM) != FSPnegprot){
		sprintf(logbuf, "%s: Bad Command Code, 0x%x", 
			Msnet, *(bp+FSP_COM));
		goto badexit;
	}

	/*
	 * Are there exactly 0 argument words in the message?
	 */

	if (*(bp+FSP_WCNT) != 0){
		sprintf(logbuf, "%s: Incorrect # of Parameter Words, 0x%x",
			Msnet, *(bp+FSP_WCNT));
		goto badexit;
	}

	/*
	 * get the size of the data in the message
	 */

	p = bp + FSP_PARMS;
	getword(p, &size);

	/*
	 * make sure the data is valid; it should have a series of
	 * "dialect" strings, which are of the form [02 string 00].
	 * if(twos == nulls) then the data is well formed, else something
	 * is wrong.
	 */

	twos = nulls = 0;
	p += 2;
	for(q = p; q < p + size; ++q){
		if(*q == '\0')
			nulls++;
		else if(*q == 02)
			twos++;
	}

	if(twos != nulls){
		sprintf(logbuf, "%s: Bad Data Format, twos=%d, nulls=%d",
			Msnet, twos, nulls);
		goto badexit;
	}

	/*
	 * Count the number of arguments that were passed
	 * to me by the listener...
	 */

	for(i=0, args=argv; *args; ++args, ++i)
		;

	/*
	 * There are a few kinds of arguments that I will pass to the server:
	 *
	 * -D<string>	- means "the client speaks this dialect . . ."
	 * 		  there me be more than one of these, if the client
	 * 		  is able to speak multiple dialects.
	 *
	 * Any arguments passed to me by the listener will be passed along
	 * as is . . .
	 *
	 * Allocate an array of "char *"s that will let me point to all
	 * of the following:
	 * 1.	As many -D options as are needed (the exact number is
	 *  	contained in the variable "twos"),
	 *  2.	One -A option for the single logical name
	 *  	of the client,
	 *  3.	As many positions as are needed to pass along the arguments
	 *  	passed to me by the listener (variable "i"),
	 *  4.	The name of the Server executable file (always arg[0]), and
	 *  5.	A NULL terminator.
	 */

	m_size = sizeof(char *) * (twos + i + 3);
	if((args = (char **)malloc((unsigned)m_size)) == 0){
		sprintf(logbuf, "%s: Can't malloc arg space, %d bytes", 
			Msnet, m_size);
		goto badexit;
	}

	/*
	 * put together the first argument to exec(2) which should be
	 * the full pathname of the executable server file.
	 */

	args[0] = server;

	/*
	 * Send dialect strings down, in order of preference
	 */

	for(i=1, q=p; q < p + size; ++i, ++q){
		q = strchr(q, 02);		/* find start of string */

		m_size = strlen(++q) + 1 + 2;
		if((args[i] = malloc((unsigned)m_size)) == 0){
			sprintf(logbuf, 
				"%s: Can't malloc Server Path buf, %d bytes",
				Msnet, m_size);
			goto badexit;
		}

		strcpy(args[i], "-D");
		strcat(args[i], q);		/* put -Ddialect\0 in arglist */
		q = strchr(q, '\0');		/* find end of string */
	}

	/*
	 * Add in arguments that were passed to me by the listener
	 * first arg is server path, so we ignore that.
	 */

	for( ++argv; *argv; ++argv, ++i)
		args[i] = *argv;

	/*
	 * NULL terminate the list
	 */

	args[i] = NULL;

	start_server(fd, (dbf_t *)0, args, call);
	return(-1);			/* error logged in start_server */

badexit:
	logmessage(logbuf);
	return(-1);
}


/*
 * g e t w o r d
 *
 * move a word from an arbitrary position in a character buffer, into
 * a short, and flip the bytes.
 * (NOTE that word is a 16-bit iapx-286 word).
 */

getword(addr, w)
register char *addr;
short *w;
{
	*addr++ = lobyte(*w);
	*addr = hibyte(*w);
}

/*	End of @(#)lssmb.c	1.5 line 233	*/



#else


smbservice(fd, bp, size, call, argv)
int fd;			/* new virtual circuit (should be zero) */
char *bp;		/* pointer to message buffer */
int size;		/* size of message */
struct t_call	*call;	/* pointer to call structure from t_accept */
char **argv;		/* server arguments */
{
	logmessage("SMB service NOT supported");
	return(-1);
}

#endif	/* SMBSERVICE */
