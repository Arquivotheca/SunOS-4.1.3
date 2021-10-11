#ifndef lint
static char	sccsid[] = "@(#)polyatt77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI line and solid object attribute functions
 */

/*
line_endstyle
line_type
line_width 
line_color 
interior_style
fill_color
hatch_index
pattern_index
pattern_table
pattern_reference_point
pattern_size
perimeter_type
perimeter_width 
perimeter_color 
*/

#include "cgidefs.h"
#include "cf77.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cflnendstyle 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cflnendstyle_ (ttyp)
int  *ttyp;		/* style of line */

{
	return (line_endstyle ((Cendstyle) *ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cflntype 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int    cflntype_ (ttyp)
int  *ttyp;		/* style of line */

{
    return (line_type ((Clintype) *ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cflnwidth 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cflnwidth_ (index)
float   *index;			/* line width */

{
return (line_width (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cflncolor 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cflncolor_ (index)
int     *index;			/* line color */
{
return (line_color (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfintstyle 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfintstyle_ (istyle, perimvis)
int *istyle;	/* fill style */
int *perimvis;		/* perimeter visibility */
{
    return (interior_style ((Cintertype) *istyle, (Cflag)( *perimvis)));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfflcolor 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfflcolor_ (color)
int     *color;			/* color for polygon fill */
{
return (fill_color (*color));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfhatchix 					    */
/*                                                                          */
/*                                                                          */
/*              is bound to the HATCH constant	                            */
/****************************************************************************/

int cfhatchix_ (index)
int     *index;			/* index in the pattern table to be bound
				   to "HATCH" */
{
return (hatch_index (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpatix 					    */
/*                                                                          */
/*                                                                          */
/*              is bound to the PATTERN constant                            */
/****************************************************************************/

int cfpatix_ (index)
int     *index;			/* index in the pattern table to be bound
				   to "PATTERN" */
{
return (pattern_index (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpattable 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfpattable_ (index, m, n, colorind)
int     *index;			/* entry in table */
int   *m, *n;			/* number of rows and columns */
int 	colorind[];		/* array containing pattern */
{
    return (pattern_table (*index, *m, *n, colorind));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpatrefpt			    */
/*                                                                          */
/*                                                                          */
/*		pattern	box begins.					    */
/****************************************************************************/

int     cfpatrefpt_ (x,y)
int *x,*y;
{
Ccoor begin;

    ASSIGN_COOR(&begin, *x, *y);
return (pattern_reference_point (&begin));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpatsize				    */
/*                                                                          */
/*                                                                          */
/*		in screen coordinates.					    */
/****************************************************************************/

int  cfpatsize_ (dx, dy)
int  *dx, *dy;			/* size of pattern in VDC space */
{
return (pattern_size (*dx, *dy));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfperimtype 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfperimtype_ (ttyp)
int  *ttyp;		/* style of perimeter */

{
    return (perimeter_type ((Clintype) *ttyp));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfperimwidth 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int    cfperimwidth_ (index)
float   *index;			/* perimeter width */
{
return (perimeter_width (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfperimcolor 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfperimcolor_ (index)
int     *index;			/* perimeter color */
{
return (perimeter_color (*index));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cflnspecmode 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cflnspecmode_ (mode)
int *mode;		/* pixels or percent */
{
    return(line_width_specification_mode ((Cspecmode) *mode));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfperimspecmode 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfperimspecmode_ (mode)
int *mode;		/* pixels or percent */
{
    return(perimeter_width_specification_mode ((Cspecmode) *mode));
}
