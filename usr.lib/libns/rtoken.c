/*	@(#)rtoken.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:rtoken.c	1.3" */
#include <stdio.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include "stdns.h"
#include "nsdb.h"
#include <rfs/nserve.h>

static char	sepstr[2] = { SEPARATOR, '\0' };

/*
 * For left to right syntax of domain names,
 * rtoken is identical to strtok.
 * For right to left syntax, it does the equivalent
 * of strtok, but starts at the right side of the string
 * and goes to the left.
 */
#ifdef RIGHT_TO_LEFT
char	*
rtoken(name)
char	*name;
{
	static	char	*pptr;
	static	char	buffer[BUFSIZ];

	if (name) {	/* initialize */
		strncpy(buffer,name,BUFSIZ);
		pptr = &buffer[strlen(buffer)];
	}
	else if (pptr == NULL) {
		fprintf(stderr,"rtoken: no current name\n");
		return(NULL);
	}

	*pptr = '\0';

	if (buffer == pptr)
		return(NULL);

	while (buffer < pptr && *pptr != SEPARATOR)
		pptr--;

	if (*pptr == SEPARATOR)
		return(pptr+1);

	return(pptr);
}
#else
char	*
rtoken(name)
char	*name;
{
	char	*strtok();
	return(strtok(name,sepstr));
}
#endif
/**************************************************************
 *
 *	dompart returns the domain part of a domain name.
 *
 *	e.g., if RIGHT_TO_LEFT
 *		dompart("a.b.c.d") would return "b.c.d"
 *		dompart("a") would return "" (ptr to null string)
 *	      if LEFT_TO_RIGHT
 *		dompart("a.b.c.d") would return "a.b.c"
 *		dompart("a") would return "" (ptr to null string)
 *
 *	dompart returns a pointer to a static buffer that contains
 *	the result.  An error will cause dompart to return NULL.
 *
 *************************************************************/
char *
dompart(name)
register char	*name;
{
#ifdef RIGHT_TO_LEFT
	static char	sname[MAXDNAME+1];

	if (name == NULL)
		return(NULL);

	while (*name != SEPARATOR && *name != '\0')
		name++;
	if (*name == SEPARATOR)
		name++;	/* skip over separator	*/

	strcpy(sname,name);
	return(sname);
#else
	static char	sname[MAXDNAME+1];
	register char	*pt=sname;
	char	*save=sname;;

	if (name == NULL)
		return(NULL);

	while(*pt++ = *name++)
		if (*name == SEPARATOR)
			save = pt;

	*save = NULL;
	return(sname);
#endif
}
/**************************************************************
 *
 *	namepart returns the name part of a domain name.
 *
 *	e.g., if RIGHT_TO_LEFT
 *		namepart("a.b.c.d") would return "a"
 *		namepart("a") would return "a"
 *		namepart("") would return "" (ptr to null string)
 *	      if LEFT_TO_RIGHT
 *		namepart("a.b.c.d") would return "d"
 *		namepart("a") would return "a"
 *		namepart("") would return "" (ptr to null string)
 *
 *	namepart returns a pointer into a static buffer
 *	so new calls to namepart overwrite previous results.
 *	An error will cause namepart to return NULL.
 *
 *************************************************************/
char *
namepart(name)
register char	*name;
{
#ifdef RIGHT_TO_LEFT
	static char	sname[MAXDNAME+1];
	register char	*p;

	if (name == NULL)
		return(NULL);

	p=sname;

	while (*name !=SEPARATOR && *name != '\0' && p < &sname[NAMSIZ+1])
		*p++ = *name++;

	*p = '\0';
	return(sname);
#else
	static char	sname[MAXDNAME+1];
	register char	*pt=name;
	register char	*npt=sname;

	while (*name++)
		if (*name == SEPARATOR)
			pt = name+1;
	while (*npt++ = *pt++)
		;
	return(sname);
#endif
}
