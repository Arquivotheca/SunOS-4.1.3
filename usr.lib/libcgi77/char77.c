#ifndef lint
static char	sccsid[] = "@(#)char77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Text functions
 */

/*
text
append_text
vdm_text
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftext                                                     */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cftext_(x,y,string,f77strlen)
int *x;
int *y;
char *string;
int f77strlen;
{
	Ccoor c1;		/* starting point of text (in VDC Space) */
	char   tst[MAXCHAR];	/* text */

	ASSIGN_COOR(&c1, *x, *y);
	return(text (&c1, PASS_STRING(tst, string, f77strlen, MAXCHAR)));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfaptext                                                   */
/*                                                                          */
/*		draws non-closed append_text				    */
/****************************************************************************/
int  cfaptext_(flag,string,f77strlen)
int *flag;
char *string;
int f77strlen;
{
    char   tst[MAXCHAR];		/* text */

    return(append_text ((Ctextfinal) *flag,
		PASS_STRING(tst, string, f77strlen, MAXCHAR)));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqtextext                                                 */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfqtextext_(string,nextchar, conx, cony, llpx, llpy, ulpx, ulpy, urpx, urpy,f77strlen)
char *string;
char *nextchar;
int *conx;
int *cony;
int *llpx;
int *llpy;
int *ulpx;
int *ulpy;
int *urpx;
int *urpy;
int f77strlen;
{
Ccoor concat;		/* concatentation point */
Ccoor lleft, uleft, uright; /* coordinates of text bounding box */
char   tst[MAXCHAR];		/* text */
int  err;

    err = inquire_text_extent (PASS_STRING(tst, string, f77strlen, MAXCHAR),
		*nextchar, &concat, &lleft, &uleft, &uright);
    ASSIGN_XY(*conx, *cony, &concat);
    ASSIGN_XY(*llpx, *llpy, &lleft);
    ASSIGN_XY(*ulpx, *ulpy, &uleft);
    ASSIGN_XY(*urpx, *urpy, &uright);
     return(err);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfvdmtext 					    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfvdmtext_(x,y,flag,string,f77strlen)
int *x;
int *y;
int *flag;
char *string;
int f77strlen;
{
Ccoor c1;
char   tst[MAXCHAR];		/* text */

    ASSIGN_COOR(&c1, *x, *y);
    return(vdm_text (&c1, (Ctextfinal)*flag,
		PASS_STRING(tst, string, f77strlen, MAXCHAR)));
}
