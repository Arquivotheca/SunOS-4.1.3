#ifndef lint
static char	sccsid[] = "@(#)lookup77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Color lookup table functions
 */

/*
 * color_table
 */

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcotable 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfcotable_ (istart, ra,ga,ba,n)
int     *istart;			/* starting address */
int ra[],ga[],ba[];
int *n;
{
    Ccentry  clist;		/* color triples and number of entries */
    int		err;
    extern char	*malloc();

    clist.ra = (unsigned char *)malloc(*n * sizeof(*clist.ra));
    clist.ga = (unsigned char *)malloc(*n * sizeof(*clist.ga));
    clist.ba = (unsigned char *)malloc(*n * sizeof(*clist.ba));
    if (clist.ra == NULL || clist.ga == NULL || clist.ba == NULL) {
	CONDITIONAL_FREE(clist.ra);
	CONDITIONAL_FREE(clist.ga);
	CONDITIONAL_FREE(clist.ba);
	return(EMEMSPAC);
    }

    clist.n = *n;
    _cgi_copy_i_to_uc(clist.ra, ra, clist.n);
    _cgi_copy_i_to_uc(clist.ga, ga, clist.n);
    _cgi_copy_i_to_uc(clist.ba, ba, clist.n);
    err = color_table (*istart, &clist);
    free(clist.ra);
    free(clist.ga);
    free(clist.ba);
    return(err);
}
