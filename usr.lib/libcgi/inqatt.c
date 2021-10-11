#ifndef lint
static char	sccsid[] = "@(#)inqatt.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Changed to return address of statics (or NULL if error) 850927 WDS.
 * (Do not call _cgi_errhand(err) if called via cgipw_...) -- WDS.
 */

/*
inquire_line_attributes
cgipw_inquire_line_attributes
inquire_marker_attributes
cgipw_inquire_marker_attributes
inquire_fill_area_attributes
cgipw_inquire_fill_area_attributes
inquire_pattern_attributes
cgipw_inquire_pattern_attributes
inquire_perimeter_attributes
cgipw_inquire_perimeter_attributes
inquire_text_attributes
cgipw_inquire_text_attributes
inquire_aspect_source_flags
cgipw_inquire_aspect_source_flags
*/

#include "cgipriv.h"

#ifndef	NULL
#define	NULL	0
#endif	NULL

Outatt         *_cgi_att;	/* structure containing current attributes */
View_surface   *_cgi_vws;	/* current view surface */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_line_attributes				    */
/*                                                                          */
/*		inquire_line_attributes reports style, width, and color	    */
/****************************************************************************/
Clinatt        *inquire_line_attributes()
 /* returns pointer to attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&_cgi_att->line);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Clinatt *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_line_attributes				    */
/*                                                                          */
/*		inquire_line_attributes reports style, width, and color	    */
/****************************************************************************/
Clinatt        *cgipw_inquire_line_attributes(desc)
Ccgiwin        *desc;
 /* returns pointer to attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&desc->vws->att->line);
    }
    else
	return ((Clinatt *) NULL);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_marker_attributes				    */
/*                                                                          */
/*		inquire_marker_attributes reports type, width, and color.   */
/****************************************************************************/
Cmarkatt       *inquire_marker_attributes()
 /* returns pointer to attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&_cgi_att->marker);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Cmarkatt *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_marker_attributes				    */
/*                                                                          */
/*		inquire_marker_attributes reports type, width, and color.   */
/****************************************************************************/
Cmarkatt       *cgipw_inquire_marker_attributes(desc)
Ccgiwin        *desc;
 /* returns pointer to attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&desc->vws->att->marker);
    }
    else
	return ((Cmarkatt *) NULL);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_fill_area_attributes				    */
/*                                                                          */
/*		inquire_fill_attributes reports interior style,		    */
/*		perimeter visbility, fill color, hatch index, pattern index */
/****************************************************************************/
Cfillatt       *inquire_fill_area_attributes()
 /* returns pointer to fill area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&_cgi_att->fill);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Cfillatt *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_inquire_fill_area_attributes			    */
/*                                                                          */
/*		inquire_fill_attributes reports interior style,		    */
/*		perimeter visbility, fill color, hatch index, pattern index */
/****************************************************************************/
Cfillatt       *cgipw_inquire_fill_area_attributes(desc)
Ccgiwin        *desc;
 /* returns pointer to fill area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&desc->vws->att->fill);
    }
    else
	return ((Cfillatt *) NULL);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_pattern_attributes				    */
/*                                                                          */
/*		inquire_pattern_attributes reports the current pattern	    */
/*		index, row and column count, color list, pattern ref. pt.   */
/*		and pattern size deltas.				    */
/****************************************************************************/
Cpatternatt    *inquire_pattern_attributes()
 /* returns pointer to pattern area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&_cgi_att->pattern);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Cpatternatt *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_pattern_attributes				    */
/*                                                                          */
/*		inquire_pattern_attributes reports the current pattern	    */
/*		index, row and column count, color list, pattern ref. pt.   */
/*		and pattern size deltas.				    */
/****************************************************************************/
Cpatternatt    *cgipw_inquire_pattern_attributes(desc)
Ccgiwin        *desc;
 /* returns pointer to pattern area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&desc->vws->att->pattern);
    }
    else
	return ((Cpatternatt *) NULL);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_text_attributes				    */
/*                                                                          */
/*		inquire_text_attributes reports font set, font, precision   */
/*		spacing, color, height, base, up, path, and alignment	    */
/****************************************************************************/
Ctextatt       *inquire_text_attributes()
 /* returns pointer to text area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&_cgi_att->text.attr);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Ctextatt *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_text_attributes				    */
/*                                                                          */
/*		inquire_text_attributes reports font set, font, precision   */
/*		spacing, color, height, base, up, path, and alignment	    */
/****************************************************************************/
Ctextatt       *cgipw_inquire_text_attributes(desc)
Ccgiwin        *desc;
 /* returns pointer to text area attribute structure */
{
    int             err = _cgi_check_state_5();
    if (!err)
    {
	return (&desc->vws->att->text.attr);
    }
    else
	return ((Ctextatt *) NULL);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_aspect_source_flags 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cflaglist      *inquire_aspect_source_flags()
 /* returns pointer to text attribute structure */
{
    static Cflaglist flaglist;	/* aspect_source_flags structure */
    static int      hnum[18] =
    {
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
    };
    int             err = _cgi_check_state_5();
    if (!err)
    {
	flaglist.value = _cgi_att->asfs;
	flaglist.n = 18;
	flaglist.num = hnum;
	return (&flaglist);
    }
    else
    {
	(void) _cgi_errhand(err);
	return ((Cflaglist *) NULL);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_aspect_source_flags 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cflaglist      *cgipw_inquire_aspect_source_flags(desc)
Ccgiwin        *desc;
 /* returns pointer to text attribute structure */
{
    static Cflaglist flaglist;	/* aspect_source_flags structure */
    static int      hnum[18] =
    {
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17
    };
    int             err = _cgi_check_state_5();
    if (!err)
    {
	err = special_setup_cgiwin(desc);
	if (!err)
	{
	    flaglist.value = _cgi_att->asfs;
	    flaglist.n = 18;
	    flaglist.num = hnum;
	    return (&flaglist);
	}
    }

    return ((Cflaglist *) NULL);
}

static Cerror   special_setup_cgiwin(desc)
Ccgiwin        *desc;
{
    /*
     * SETUP_CGIWIN will return ECGIWIN if the descriptor is invalid. 
     */
    SETUP_CGIWIN(desc);
    return (NO_ERROR);
}
