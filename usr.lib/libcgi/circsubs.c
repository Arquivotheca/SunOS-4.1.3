#ifndef lint
static char	sccsid[] = "@(#)circsubs.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI circle accessory functions
 */

/*
_cgi_oct_circ_radius
__cgi_text_inc
*/

short           _cgi_texture[8];
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_oct_circ_radius                                       */
/*                                                                          */
/****************************************************************************/
int             _cgi_oct_circ_radius(rad)
int             rad;
{
    int             x, y, d;
    int             inc;
    x = 0;
    y = rad;
    d = 3 - 2 * rad;
    inc = 0;			/* count number of points in an interior octant */
    while (x < y)
    {
	inc++;
	if (d < 0)
	    d = d + 4 * x + 6;
	else
	{
	    d = d + 4 * (x - y) + 10;
	    y = y - 1;
	}
	x += 1;
    }
/*     if (x = y) inc++; */
    if (inc)
	inc = (45 << 10) / inc;
    return (inc);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_text_inc	 					    */
/*                                                                          */
/*		 _cgi_text_inc						    */
/****************************************************************************/

int             _cgi_text_inc(ptca, style, ia, angsum, angsum1)
int            *ptca, *style, *ia, *angsum, *angsum1;
{
    int             ptc, i, k;
    static int      pattern2[6][9] =
    {
     {
      9000, 46, 0, 0, 0, 0, 0, 0, 0
      },			/* solid  */
     {
      2, 2, 2, 2, 2, 2, 2, 2, 16
      },			/* dotted */
     {
      6, 2, 6, 2, 6, 2, 6, 2, 32
      },			/* dashed */
     {
      8, 2, 2, 2, 8, 2, 2, 2, 24
      },			/* dash dot */
     {
      8, 2, 2, 2, 2, 2, 2, 2, 18
      },			/* dash dot dotted */
     {
      10, 10, 10, 10, 10, 10, 10, 10, 80
      }
    };				/* long dashed */

    i = *ia;
    ptc = *ptca;
    if (ptc >= pattern2[*style][i])
    {
	i++;
	if ((i % 2) == 0)
	    for (k = 0; k < 8; k++)
		_cgi_texture[k] = 1;
	else
	    for (k = 0; k < 8; k++)
		_cgi_texture[k] = 0;
	ptc = 0;
	*angsum1 -= *angsum;
	*angsum = 0;
	i &= 7;
    }
    *ia = i;
    *ptca = ptc;
}
