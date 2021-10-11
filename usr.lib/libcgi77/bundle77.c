#ifndef lint
static char	sccsid[] = "@(#)bundle77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Attribute control functions
 */


#include "cgidefs.h"

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsaspsouflags_	 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfsaspsouflags_ (fval,fnum,n)
int *fval,*fnum,*n;
{
    Cflaglist flags;	/* value of attribute (INDICIDUAL or BUNDLED) */
    flags.n = *n;
    flags.num = fnum;
    flags.value = (Casptype*)fval;
    return (set_aspect_source_flags (&flags));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdefbundix    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfdefbundix_ (index, line_type, line_width, line_color, marker_type,
		marker_size, marker_color, interior_style, hatch_index,
		pattern_index, fill_color, perimeter_type, perimeter_width,
		perimeter_color, text_font, text_precision,
		character_expansion, character_spacing, text_color)
Cint     	*index;			/* entry in AES table */
Clintype	*line_type;		/* members of Cbunatt structure */
Cfloat		*line_width;
Cint		*line_color;
Cmartype	*marker_type;
Cfloat		*marker_size;
Cint		*marker_color;
Cintertype	*interior_style;
Cint 		*hatch_index;
Cint 		*pattern_index;
Cint		*fill_color;
Clintype	*perimeter_type;
Cfloat		*perimeter_width;
Cint		*perimeter_color;
Cint		*text_font;
Cprectype	*text_precision;
Cfloat		*character_expansion;
Cfloat		*character_spacing;
Cint		*text_color;
{
    Cbunatt	attributes;

    attributes.line_type = *line_type;
    attributes.line_width = *line_width;
    attributes.line_color = *line_color;
    attributes.marker_type = *marker_type;
    attributes.marker_size = *marker_size;
    attributes.marker_color = *marker_color;
    attributes.interior_style = *interior_style;
    attributes.hatch_index = *hatch_index;
    attributes.pattern_index = *pattern_index;
    attributes.fill_color = *fill_color;
    attributes.perimeter_type = *perimeter_type;
    attributes.perimeter_width = *perimeter_width;
    attributes.perimeter_color = *perimeter_color;
    attributes.text_font = *text_font;
    attributes.text_precision = *text_precision;
    attributes.character_expansion = *character_expansion;
    attributes.character_spacing = *character_spacing;
    attributes.text_color = *text_color;
    return (define_bundle_index (*index, &attributes));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpolylnbundix 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfpolylnbundix_ (index)
int     *index;			/* polyline bundle index */
{
    return (polyline_bundle_index (*index));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfpolymkbundix 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfpolymkbundix_(index)
int     *index;			/* polyline bundle index */
{
    return(polymarker_bundle_index (*index));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfflareabundix 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cfflareabundix_(index)
int     *index;			/* polyline bundle index */
{
    return(fill_area_bundle_index(*index));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftextbundix 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int cftextbundix_ (index)
int *index;			/* polyline bundle index */
{
    return(text_bundle_index(*index));
}
