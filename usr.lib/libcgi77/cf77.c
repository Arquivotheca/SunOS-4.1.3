#ifndef lint
static char	sccsid[] = "@(#)cf77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
 * Permission to use, copy, modify, and distribute this software for any 
 * purpose and without fee is hereby granted, provided that the above 
 * copyright notice appear in all copies and that both that copyright 
 * notice and this permission notice are retained, and that the name 
 * of Sun Microsystems, Inc., not be used in advertising or publicity 
 * pertaining to this software without specific, written prior permission. 
 * Sun Microsystems, Inc., makes no representations about the suitability 
 * of this software or the interface defined in this software for any 
 * purpose. It is provided "as is" without express or implied warranty.
 */

/*
 * CGI Fortran 77 interface support functions.
 *
 * Since these handle common support for different CGI functions,
 * they check most pointers for NULL values, since many CGI routines
 * only fill in parts of structures.  In some cases this is overkill,
 * but it avoids making assumptions that later changes to CGI may
 * invalidate.
 */

/*
_cgi_f77_alloc_coorlist
_cgi_f77_assign_coorlist
_cgi_f77_free_coorlist
_cgi_f77_return_xylist
_cgi_f77_pass_string
_cgi_f77_return_string
_cgi_f77_return_inrep
_cgi_intncpy
_cgi_copy_uc_to_i
_cgi_copy_i_to_uc
_cgi_conditional_free
*/

#include "cgidefs.h"		/* defines types used in this file  */
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_alloc_coorlist                                    */
/*                                                                          */
/****************************************************************************/

Cerror	_cgi_f77_alloc_coorlist (pcoorlist, xcoors, ycoors, n)
Ccoorlist	*pcoorlist;
register int	*xcoors;
register int	*ycoors;
int 		n;
{
    register Ccoor *list;
    register int i;
    register Cerror err;

    pcoorlist->ptlist = list = (Ccoor*) malloc (n * sizeof (Ccoor));
    if (list == NULL)	return(EMEMSPAC);

    /* Now load the points from fortran lists into Ccoors */
    if ((err = _cgi_f77_assign_coorlist (xcoors, ycoors, n, pcoorlist))
    	    != NO_ERROR)
	return (err);

    return(NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_assign_coorlist                                   */
/*                                                                          */
/****************************************************************************/

Cerror	_cgi_f77_assign_coorlist (xcoors, ycoors, n, pcoorlist)
register int	*xcoors;
register int	*ycoors;
int 		 n;
Ccoorlist	*pcoorlist;
{
    register Ccoor *list = pcoorlist->ptlist;
    register int i;

    if (pcoorlist == NULL || pcoorlist->ptlist == NULL)
	return(EF77INTERNAL);
    pcoorlist->n = n;

    /* Now load the points from fortran lists into Ccoorlist */
    for (i = n; --i >= 0; ) {
	(*list).x = *xcoors++;
	(*list++).y = *ycoors++;
    }
    return(NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_free_coorlist                                     */
/*                                                                          */
/****************************************************************************/

Cerror	_cgi_f77_free_coorlist (pcoorlist)
Ccoorlist	*pcoorlist;
{
    if (pcoorlist->ptlist != NULL)	free(pcoorlist->ptlist);
    pcoorlist->n = 0;
    pcoorlist->ptlist = NULL;
    return(NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_return_xylist                                     */
/*                                                                          */
/****************************************************************************/

Cerror	_cgi_f77_return_xylist (xcoors, ycoors, n, pcoorlist)
register int	*xcoors;
register int	*ycoors;
int 		*n;
Ccoorlist	*pcoorlist;
{
    register Ccoor *list = pcoorlist->ptlist;
    register int i;
    register Cerror err;

    if (pcoorlist == NULL || pcoorlist->ptlist == NULL)
	return(EF77INTERNAL);
    if (pcoorlist->n < *n)	*n = pcoorlist->n;

    /* Now load the points from Ccoors into fortran lists */
    for (i = *n; --i >= 0; ) {
	*xcoors++ = (*list).x;
	*ycoors++ = (*list++).y;
    }
    if ((err = _cgi_f77_free_coorlist (pcoorlist)) != NO_ERROR)
	return (err);
    return(NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_pass_string                                       */
/*               This is here so we have an interface point at which        */
/*               to change string handling, if this becomes necessary.      */
/*                                                                          */
/****************************************************************************/

char *	_cgi_f77_pass_string (dest, src, length, array_size)
char	*dest;
char	*src;
int	length, array_size;
{
    extern char	*strcpy(), *strncpy();

    if (!dest)
	return((char*) NULL);
    /*
     * If called as: call foo(string(1:length)), C gets:
     *	foo_(string+length, -length)
     *
     * For all cases, we never strip trailing blanks.
     * If the override length is specified (ie. length < 0), we limit
     * the output string to (-length).
     *
     * We also limit the string length to (array_size - 1).
     */
    if (length < 0) {
	length = -length;
	src -= length;
    }
    if (length >= array_size)	length = array_size - 1;
    *(dest+length) = '\0';	/* null terminate it */

    if (!src)
	return(strcpy(dest,""));
    else
	return(strncpy(dest, src, length));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_return_string                                     */
/*               This is here so we have an interface point at which        */
/*               to change string handling, if this becomes necessary.      */
/*                                                                          */
/*               We return null strings to FORTRAN by space-filling         */
/*               the returned array.                                        */
/****************************************************************************/

char	*_cgi_f77_return_string (dest, src, length)
char	*dest;
char	*src;
int	length;
{
    register char	*cp;
    register int	count, slen;

    if (dest == NULL)	return(NULL);
    if (src != NULL) {

	/* Deal with: call foo(string(1:length)) */
	if (length < 0) {
	    length = -length;
	    src -= length;
	}

	/* Copy the string, and pad with spaces to make fortran happy */
	strncpy(dest, src, length);
	slen = strlen(src);
    }
    else
	slen = 0;
    cp = &dest[slen];
    for (count = length-slen; --count >= 0; )
	*cp++ = ' ';
    return(dest);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_f77_return_inrep                                      */
/*                                                                          */
/****************************************************************************/

Cerror _cgi_f77_return_inrep (devclass, x, y, xlist, ylist, n, val,
	    choice, string, f77strleng, segid, pickid, inrepP)
int	devclass;
int	*x, *y;
int	xlist[], ylist[];
int	*n;
float	*val;
int	*choice;
char	*string;
int	f77strleng;
int	*segid, *pickid;
Cinrep	*inrepP;
{
    if (inrepP == NULL)	return(EF77INTERNAL);

    switch (devclass) {
	case IC_LOCATOR:
	    /* Check in case inrepP->xypt has never been set */
	    if (inrepP->xypt == NULL)	return(EF77INTERNAL);
	    *x = (int) inrepP->xypt->x;
	    *y = (int) inrepP->xypt->y;
	    break;
	case IC_STROKE:
	    /* Check in case inrepP->points has never been set */
	    if (inrepP->points == NULL)	return(EF77INTERNAL);
	    return( RETURN_XYLIST(xlist, ylist, n, inrepP->points) );
	case IC_VALUATOR:
	    *val = (float) inrepP->val;
	    break;
	case IC_CHOICE:
	    *choice = (int) inrepP->choice;
	    break;
	case IC_STRING:
	    RETURN_STRING(string, inrepP->string, f77strleng);
	    break;
	case IC_PICK:
	    /* Check in case inrepP->pick has never been set */
	    if (inrepP->pick == NULL)	return(EF77INTERNAL);
	    *segid = inrepP->pick->segid;
	    *pickid = inrepP->pick->pickid;
	    break;
    }
    return(NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_intncpy                                               */
/*                                                                          */
/*               copy n items from one integer array to another             */
/*                                                                          */
/****************************************************************************/

_cgi_intncpy (dest, src, n)
register int	*dest, *src;
register int	n;
{
    if (dest == NULL || src == NULL)	return;

    while (--n >= 0)
	*dest++ = *src++;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_copy_uc_to_i                                          */
/*                                                                          */
/*               copy n items from an unsigned char array to an int array   */
/*                                                                          */
/****************************************************************************/

_cgi_copy_uc_to_i (dest, src, n)
register int		*dest;
register unsigned char	*src;
register int		n;
{
    if (dest == NULL || src == NULL)	return;

    while (--n >= 0)
	*dest++ = (int) *src++;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_copy_i_to_uc                                          */
/*                                                                          */
/*               copy n items from an int array to an unsigned char array   */
/*                                                                          */
/****************************************************************************/

_cgi_copy_i_to_uc (dest, src, n)
register unsigned char	*dest;
register int		*src;
register int		n;
{
    if (dest == NULL || src == NULL)	return;

    while (--n >= 0)
	*dest++ = (unsigned char) *src++;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_conditional_free                                      */
/*                                                                          */
/*               free a pointer only if it's not NULL                       */
/*                                                                          */
/****************************************************************************/

_cgi_conditional_free (p)
char	*p;
{
    if (p != NULL)	free(p);
}
