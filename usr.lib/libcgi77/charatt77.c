#ifndef lint
static char	sccsid[] = "@(#)charatt77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Text Attribute functions
 */

/* 
character_set_index
text_font_index
text_precision
character_expansion_factor
text_color
character_up_vector_2
character_path
text_alignment
*/

#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharsetix                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfcharsetix_(index)
int *index;			/* font set */
{
return(character_set_index ((Cint) *index));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftextfontix                                           */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cftextfontix_ (index)
int *index;
{
return (text_font_index ((Cint) *index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftextprec					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cftextprec_ (ttyp)
int *ttyp;		/* text type */
{
	return (text_precision ((Cprectype) *ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharexpfac                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int     cfcharexpfac_ (efac)
float   *efac;			
{
return (character_expansion_factor ((Cfloat) *efac));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharspacing                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int     cfcharspacing_ (efac)
float   *efac;			/* spacing ratio */
{
return (character_spacing (*efac));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftextcolor      	                                    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int     cftextcolor_ (index)
int     *index;			/* color */
{
return (text_color ((Cint) *index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharheight                                          */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfcharheight_ (height)
int    * height;			/* height in VDC */
{
return (character_height ((Cint) *height));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharorient                                     */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfcharorient_ (bx, by, dx, dy)
float   *bx, *by, *dx, *dy;		/* base component and up component of up
				   vector */
{
return (character_orientation (*bx, *by, *dx, *dy));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfcharpath	                                    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int  cfcharpath_ (path)
int *path;		/* text direction */
{
return (character_path ((Cpathtype) *path));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftextalign	                                    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cftextalign_	 (halign, valign, hcalind, vcalind)
int *halign;	/* horizontal alignment type */
int *valign;		/* vertical alignment type */
float   *hcalind, *vcalind;	/* continuous alignment indicators */
{
return (text_alignment ((Chaligntype) *halign, 
	(Cvaligntype) *valign, *hcalind, *vcalind));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cffixedfont				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cffixedfont_	 (index)
int     *index;			/* font set */
{
return (fixed_font ((Cint) *index));
}

