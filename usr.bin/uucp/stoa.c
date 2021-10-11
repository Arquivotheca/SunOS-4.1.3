/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident  "@(#)stoa.c 1.1 92/07/30"	/* from SVR3.2 uucp:stoa.c 1.2 */

#include	"parms.h"

#ifdef TLI

/*
	stoa - convert string to address

	If a string begins in \o or \O, the following address is octal
	"  "   "       "    " \x or \X, the following address is hex

	If ok, return pointer to netbuf structure.
	A  NULL is returned on any error(s).
*/



#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <sys/tiuser.h>
#include <ctype.h>
#include <nsaddr.h>
#define	toupper(c)	(islower(c) ? _toupper(c) : (c))
#define	todigit(c)	((int)((c) - '0'))	/* char to digit */
#define	toxdigit(c)	((isdigit(c))?todigit(c):(toupper(c)-(int)'A'+10))
#define	isodigit(c)	(isdigit(c) && ((c) != '9') && ((c) != '8'))
#define	itoac(i)	(((i) > 9) ? ((char)((i)-10) + 'A'):((char)(i) + '0'))	
#define	MASK(n)		((1 << (n)) - 1)

#define	SBUFSIZE	128

#define	TRUE	1;
#define	FALSE	0;


char	sbuf[SBUFSIZE];

struct netbuf *
stoa(str, addr)			/* Return netbuf ptr if success */
char	*str;			/* Return NULL if error		*/
struct netbuf	*addr;
{
	char	*xfer();

	int	myadr;		/* was netbuf struct allocated here ? */

	myadr = FALSE;

	if (!str)
		return NULL;
	while (*str && isspace(*str))	/* leading whites are OK */
		++str;

	if (!str || !*str) return NULL;		/* Nothing to convert */

	if (!addr) {
		if ((addr = (struct netbuf *)malloc(sizeof(struct netbuf))) == NULL)
			return NULL;
		myadr = TRUE;
		addr->buf = NULL;
		addr->maxlen = 0;
		addr->len = 0;
	}

	/* Now process the address */
	if (*str == '\\') {
		++str;
		switch (*str) {

		case 'X':	/* hex */
		case 'x':
			addr->len = dobase(++str, sbuf, HEX);
			break;

		case 'o':	/* octal */
		case 'O':
			addr->len = dobase(++str, sbuf, OCT);
			break;

		default:	/* error */
			return NULL;
			break;
		}
	}

	if (addr->len == 0) {	/* Error in conversion */
		if (myadr)
			free(addr);
		return NULL;
	}
	if ((addr->buf = xfer(addr->buf, sbuf, addr->len, addr->maxlen)) == NULL)
		return NULL;
	else
		return addr;
}



/*
	dobase :	converts a hex or octal ASCII string
		to a binary address. Only HEX or OCT may be used
		for type.
	return length of binary string (in bytes), 0 if error.
	The binary result is placed at buf.
*/

int
dobase(s, buf, type)	/* read in an address */
char	*s, *buf;	/* source ASCII, result binary string */
int	type;
{
	void	memcp();
	int	bp = SBUFSIZE - 1;
	int	shift = 0;
	char	*end;

	for (end = s; *end && ((type == OCT) ? isodigit(*end) :
		isxdigit(*end)); ++end) ;

	/* any non-white, non-digits cause address to be rejected,
	   other fields are ignored */

	if ((*s == 0) || (end == s) || (!isspace(*end) && *end)) {
		fprintf(stderr, "dobase: Illegal trailer on address string\n");
		buf[0] = '\0';
		return 0;
	}
	--end;

	buf[bp] = '\0';

	while (bp > 0 && end >= s) {
		buf[bp] |= toxdigit(*end) << shift;
		if (type == OCT) {
			if (shift > 5) {
				buf[--bp] = (todigit(*end) >> (8 - shift))
					& MASK(shift-5);
			}
			if ((shift = (shift + 3) % 8) == 0)
				buf[--bp] = 0;
		}
		else	/* hex */
			if ((shift = (shift) ? 0 : 4) == 0)
				buf[--bp] = 0;;
		--end;
	}
	if (bp == 0) {
		fprintf(stderr, "stoa: dobase: number to long\n");
		return 0;
	}

	/* need to catch end case to avoid extra 0's in front	*/
	if (!shift)
		bp++;
	memcp(buf, &buf[bp], (SBUFSIZE - bp));
	return (SBUFSIZE - bp);
}



void
memcp(d, s, n)	/* safe memcpy for overlapping regions */
char	*d, *s;
int	n;
{
	while (n--)
		*d++ = *s++;
}


/* transfer block to a given destination or allocate one of the
    right size 
    if max = 0 : ignore max
*/

char *
xfer(dest, src, len, max)
char	*dest, *src;
unsigned	len, max;
{
	if (max && dest && max < len) {		/* No room */
		fprintf(stderr, "xfer: destination not long enough\n");
		return NULL;
	}
	if (!dest)
		if ((dest = malloc(len)) == NULL) {
			fprintf(stderr, "xfer: malloc failed\n");
			return NULL;
		}

	memcpy(dest, src, (int)len);
	return dest;
}

#endif /* TLI */
