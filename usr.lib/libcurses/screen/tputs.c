/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tputs.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.10 */
#endif

/* Copyright (c) 1979 Regents of the University of California */
#include	<ctype.h>
#include	"curses_inc.h"

/*
 * Put the character string cp out, with padding.
 * The number of affected lines is affcnt, and the routine
 * used to output one character is outc.
 */
static	char	*ocp;

static	char	*
_tpad(cp, affcnt, outc)
register	char	*cp;
int		affcnt;
int		(*outc)();
{
    register	int	delay = 0;
    register	char	*icp = cp;
    register	int	ignorexon = 0, doaffcnt = 0;

#ifdef	_VR2_COMPAT_CODE
    /*
     * Why is this here?
     * Because mandatory padding must be used for flash_screen
     * and bell. We cannot force users to code mandatory padding 
     * in their terminfo entries, as that would break compatibility.
     * We therefore, do it here.
     *
     * When compatibility is to be broken, his will go away
     * and users will be informed that they MUST use mandatory
     * padding for flash and bell.
    */
    if (ocp == bell || ocp == flash_screen)
	    ignorexon = TRUE;
#endif	/* _VR2_COMPAT_CODE */

    /* Eat initial $< */
    cp += 2;

    /* Convert the number representing the delay. */
    if (isdigit(*cp))
    {
	do
	    delay = delay * 10 + *cp++ - '0';
	while (isdigit(*cp));
    }
    delay *= 10;
    if (*cp == '.')
    {
	cp++;
	if (isdigit(*cp))
	    delay += *cp - '0';
	/* Only one digit to the right of the decimal point. */
	while (isdigit(*cp))
	    cp++;
    }

    /*
     * If the delay is followed by a `*', then
     * multiply by the affected lines count.
     * If the delay is followed by a '/', then
     * the delay is done irregardless of xon/xoff.
     */
    while (TRUE)
    {
	if (*cp == '/')
	    ignorexon = TRUE;
	else
	    if (*cp == '*')
		doaffcnt = TRUE;
	    else
		break;
	cp++;
    }
    if (doaffcnt)
	delay *= affcnt;
    if (*cp == '>')
	cp++;	/* Eat trailing '>' */
    else
    {
	/*
	 * We got a "$<" with no ">".  This is usually caused by
	 * a cursor addressing sequence that happened to generate
	 * $<.  To avoid an infinite loop, we output the $ here
	 * and pass back the rest.
	 */
	(*outc)(*icp++);
	return (icp);
    }

    /*
     * If no delay needed, or output speed is
     * not comprehensible, then don't try to delay.
     */
    if (delay == 0)
	return (cp);
    /*
     * Let handshaking take care of it - no extra cpu load from pads.
     * Also, this will be more optimal since the pad info is usually
     * worst case.  We only use padding info for such terminals to
     * estimate the cost of a capability in choosing the cheapest one.
     * Some capabilities, such as flash_screen, really want the
     * padding irregardless.
     */
    if (xon_xoff && !ignorexon)
	return (cp);
    (void) _delay(delay, outc);
    return (cp);
}

tputs(cp, affcnt, outc)
register	char	*cp;
int			affcnt;
int			(*outc)();
{
    /* static (11 cc gripes) */
    char	*_tpad();

    if (cp != 0)
    {
	ocp = cp;

	/* The guts of the string. */
	while (*cp)
	    if (*cp == '$' && cp[1] == '<')
		cp = _tpad(cp, affcnt, outc);
	    else
		(*outc)(*cp++);
    }
    return (OK);
}
