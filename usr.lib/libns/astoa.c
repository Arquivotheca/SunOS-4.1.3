/*	@(#)astoa.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:astoa.c	1.3" */
/*
	astoa - convert string to address

	string format is

		[n] addr
	where n is an optional string specifying the protocol.
	Default is DEFPROT.

	See stoa() for address rules.

	A  NULL is returned on any error(s).
*/


#include <ctype.h>
#include <stdio.h>
#include <memory.h>
#include <malloc.h>
#include <tiuser.h>
#include "nslog.h"
#include "nsdb.h"
#include <rfs/nsaddr.h>

char	*Net_spec=NULL;

struct address *
astoa(str, addr)		/* Return 0 for success, -1 for error */
char	*str;
struct address	*addr;
{
	char	*xfer(), *prescan(), *nstrcat();
	char	*s;
	int	fields;		/* Number of field in input string */
	int	myadr;		/* was address struct allocated here ? */
	int	quote;		/* quoted string ? */

	myadr = 0;

	if (!str)
		return NULL;
	while (*str && isspace(*str))	/* leading whites are OK */
		++str;

/***	str = prescan(str);		/* Do all \$ ... \$ */

	if (!str) return NULL;		/* Nothing to convert */
	if (!*str) return NULL;

	fields = cfields(str);		/* count # fields */
	if (fields < 1)
		return NULL;

	if (!addr) {
		if ((addr = (struct address *)malloc(sizeof(struct address)))
			== NULL)
			return NULL;
		myadr = 1;
		addr->addbuf.buf = NULL;
	}

	if (fields > 1) {
		s = str;
		while (!isspace(*str))
			++str;
		*str++ = '\0';
		addr->protocol = nstrcat(s, NULL);
		while ( *str && isspace(*str))	/* to next field */
			++str;
	}
	else
		addr->protocol = (char *) copystr(Net_spec);

	/* Now process the address */
	if (stoa(str, &(addr->addbuf)))
		return addr;
	else {
		free(addr->protocol);
		if (myadr)
			free(addr);
		return NULL;
	}
}


/*

	aatos(str, addr, type)

	convert address to ASCII form with address in hex, decimal,
	or character form.
	return pointer to buffer (NULL on failure).
*/


char *
aatos(str, addr, type)
char	*str;
struct address	*addr;
int	type;
{
	char	*xfer(), *atos(), *nstrcat();
	void	memcp();

	int	keepflag;
	int	mystr = 0;
	long	i, j;
	char	*sp;

	if (addr == NULL)
		return NULL;

	keepflag = KEEP & type;
	type &= ~KEEP;

	if (sp = atos(NULL, &(addr->addbuf), type)) {
		if (!str && (str=malloc(strlen(addr->protocol)+2+strlen(sp))) == NULL) {
			free(sp);
			return NULL;
		}
		if (addr->protocol && keepflag) {
			strcpy(str, addr->protocol);
			strcat(str, " ");
			strcat(str, sp);
		}
		else
			strcpy(str, sp);
		free(sp);
		return str;
	}
	else
		return NULL;
}


/*
	nstrcat :	concatenate s1 and s2 onto a newly created string
*/

char *
nstrcat(s1, s2)
char *s1, *s2;
{
	char	*s, *t;

	if (!s1 && !s2)
		return NULL;
	if (!(s = t = malloc((s1 ? strlen(s1) : 0) + (s2 ? strlen(s2) : 0) + 1)))
		return NULL;
	if (s1)
		while (*s1)
			*t++ = *s1++;
	if (s2)
		while (*s2)
			*t++ = *s2++;
	*t++ = '\0';
	return s;
}


/*
	cfields :	Is there a protocol field or not?
		Return number of fields (whitespace separated)
		in the string. 2 means 2 or more.
*/

int
cfields(s)
char	*s;
{
	int	f = 0;

	if (*s == '"')		/* quoted string .. may have blanks */
		return 1;

	while (*s && (f < 2)) {
		++f;
		while(*s && !isspace(*s))
			++s;
		while (isspace(*s))
			++s;
	}
	return f;
}
