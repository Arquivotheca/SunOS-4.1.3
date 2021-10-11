#ifndef lint
static char	sccsid[] = "@(#)patsubs77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
pattern_with_fill_color
*/

#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpatfillcolor                                             */
/*                                                                          */
/****************************************************************************/

int cfpatfillcolor_ (flag)
int	*flag;
{
    return( pattern_with_fill_color((Cflag) *flag));
}
