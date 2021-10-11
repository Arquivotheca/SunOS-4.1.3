/*	@(#)nlsenv.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)listen:nlsenv.c	1.5.1.1"

/*
 * nlsenv.c:
 *
 * Utilities for servers to access environment set by listener.
 *
 * nlsgetcall:	Returns pointer to t_call structure listener recieved during
 *		the t_listen.  Gets data from environment and converts
 *		the data to internal address form.
 *
 * nlsprovider:	Returns name of provider from environment.
 *
 */

#include <ctype.h>
#include <tiuser.h>
#include "nlsenv.h"

/*
 * define DEBUGMODE for diagnostic printf's to stderr
 */

/* #define	DEBUGMODE */

#ifdef	DEBUGMODE
#include <stdio.h>
#endif

/*
 * nlsenv: (static)
 *
 * Given an environment variable name, a receiving buffer and the length
 * of the receiving buffer, getenv gets the environment variable, decodes
 * it and places the decoded data in addr.  The return value is the length
 * of "addr" if succesful, or a negative value if unsuccessful.
 */


static int
nlsenv(buf, envname)
register struct netbuf *buf;
char *envname;
{
	register char *charaddr;
	extern char *getenv();
	extern char *calloc();
	extern nlsc2addr();
	int length;

	if (!(charaddr = getenv(envname)))
		return(-11);

#ifdef	DEBUGMODE
	fprintf(stderr, "nlsenv: environ %s = %s len = %d\n", 
		envname, charaddr, strlen(charaddr));
#endif

	if (strlen(charaddr) & 1)
		return(-12);

	length = (strlen(charaddr) + 1) / 2;
	if (!(buf->buf = calloc(1, length)))
		return(-13);
	else
		buf->maxlen = length;
	return(nlsc2addr(buf->buf, buf->maxlen, charaddr));
}


/*
 * nlsgetcall:	Get calling data provided by the client via the listener.
 *
 *		nlsgetcall allows network server processes started by the
 *		network listener process to access the callers 't_call'
 *		structure provided in the client's t_connect primitive.
 *
 *		This routine gets this data from the environment
 *		via putenv(3C), interprets the data and places the data
 *		in a t_call structure allocated via t_alloc.
 *
 *		synopsis:
 *
 *		struct t_call *nlsgetcall(fd);
 *		int fd;		arg now ignored
 *
 *
 *		returns:	Address of an allocated t_call structure
 *				or
 *				NULL for failure. (calloc failed)
 *				If calloc succeeds, non-existant
 *				env. variables or data is indicated
 *				by a negative 'len' field in the approp.
 *				netbuf structure.  A length of zero in the
 *				netbuf structure is valid.
 *
 */

struct t_call *
nlsgetcall(fd)
int fd;
{
	register struct t_call *call;
	extern char *calloc();

	if (!(call = (struct t_call *) calloc(1, sizeof(struct t_call))))
		return((struct t_call *)0);

/*
 * Note: space for buffers gets allocated by nlsenv on the fly
 */

	call->addr.len = nlsenv(&call->addr, NLSADDR);
	call->opt.len = nlsenv(&call->opt, NLSOPT);
	call->udata.len = nlsenv(&call->udata, NLSUDATA);

	return (call);
}


/*
 * nlsprovider:	Return the name of the transport provider
 *		as placed in the environment by the Network listener
 *		process.  If the variable is not defined in the
 *		environment, a NULL pointer is returned.
 *
 *		If the provider is "/dev/starlan", nlsprovider
 *		returns a pointer to the null terminated character string:
 *		"/dev/starlan" if this calling process is a child of the 
 *		network listener process.
 */

char *
nlsprovider()
{
	return(getenv(NLSPROVIDER));
}


/*
 * nlsc2addr:	Convert external address to internal form.
 *	(from nlsaddr.c)
 */

/*
 * asctohex(X):  convert char X to integer value
 *		 assumes isxdigit(X). returns integer value.
 *		 Note that 'a' > 'A'.  See usage in code below.
 */

#define asctohex(X)	\
    ((int)(isdigit(X) ? (int)(X-'0') : (X>='a') ? (X-'a')+10 : (X-'A')+10))

/*
 * nlsc2addr:	Given a buffer containing the hex/ascii representation
 *		of a logical address, the buffer's size and an address
 *		of a receiving buffer, char2addr converts the logical
 *		addr to internal format and returns the size of the logical
 *		address.  A negative value is returned and the receiving
 *		buffers contents are undefined if:
 *
 *		A.  The receiving buffer is not large enough. (rc = -1)
 *		B.  If 'charaddr' does not contain a series of octets 
 *		    (strlen(charaddr) must be even). (rc = -2)
 *		C.  Any character in 'charaddr' is not an ASCII hex digit.
 *		    (rc = -3)
 *
 *	NOTE: that even if the internal representation of an address is
 *	an ASCII string, there is no guarantee that the output will be
 *	null terminated, thus the returned length must be used when
 *	accessing the internal address.
 */


static int
nlsc2addr(addr, maxlen, charaddr)
register char *addr, *charaddr;
register maxlen;
{
	register len;
	register int i;
	register char c;
	register unsigned char val;

	if (strlen(charaddr) & 1)
		return(-1);

	for (len = 0; ((maxlen--) && (*charaddr)); ++len)  {
		for (i = 2,  val = 0;  i--;  )  {
			c = *charaddr++;
			if (!(isxdigit(c)))
				return(-3);
			val = (val << 4) | (unsigned char)asctohex(c);
	    	}

		*addr++ = (char)val;
	}

#ifdef	DEBUGMODE
	fprintf(stderr, "nlsc2addr: returned length = %d\n", len);
#endif

	return(*charaddr ? -2 : len);
}
