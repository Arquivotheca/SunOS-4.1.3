#ifndef lint
static char	sccsid[] = "@(#)negotiate77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Negotiation functions
 */

/*
inquire_device_identification
inquire_device_class
inquire_coordinate_system
inquire_output_function_set
inquire_output_capabilities
*/

#include <cgiconstants.h>
#include "cgidefs.h"
#include "cf77.h"

static char	*f77_outlist[] = {
    "cfaptext",		/* append_text */
    "cfbtblpatarr",	/* bitblt_pattern_array */
    "cfbtblpatsouarr",	/* bitblt_patterned_source_array */
    "cfbtblsouarr",	/* bitblt_source_array */
    "cfcellarr",	/* cell_array */
    "cfcharexpfac",	/* character_expansion_factor */
    "cfcharheight",	/* character_height */
    "cfcharorient",	/* character_orientation */
    "cfcharpath",	/* character_path */
    "cfcharsetix",	/* character_set_index */
    "cfcharspacing",	/* character_spacing */
    "cfcircle",		/* circle */
    "cfcircarcthree",	/* circular_arc_3pt */
    "cfcircarcthreecl",	/* circular_arc_3pt_close */
    "cfcircarccent",	/* circular_arc_center */
    "cfcircarccentcl",	/* circular_arc_center_close */
    "cfclrcont",	/* clear_control */
    "cfclrvws",		/* clear_view_surface */
    "cfclipind",	/* clip_indicator */
    "cfcliprect",	/* clip_rectangle */
    "cfcotable",	/* color_table */
    "cfdevvpt",		/* device_viewport */
    "cfdpolyline",	/* disjoint_polyline */
    "cfellipse",	/* ellipse */
    "cfelliparc",	/* elliptical_arc */
    "cfelliparccl",	/* elliptical_arc_close */
    "cfflareabundix",	/* fill_area_bundle_index */
    "cfflcolor",	/* fill_color */
    "cfhardrst",	/* hard_reset */
    "cfhatchix",	/* hatch_index */
    "cfintstyle",	/* interior_style */
    "cflncolor",	/* line_color */
    "cflntype",		/* line_type */
    "cflnwidth",	/* line_width */
    "cflnspecmode",	/* line_width_specification_mode */
    "cfmkcolor",	/* marker_color */
    "cfmksize",		/* marker_size */
    "cfmkspecmode",	/* marker_size_specification_mode */
    "cfmktype",		/* marker_type */
    "cfppolygon",	/* partial_polygon */
    "cfpatix",		/* pattern_index */
    "cfpatrefpt",	/* pattern_reference_point */
    "cfpatsize",	/* pattern_size */
    "cfpattable",	/* pattern_table */
    "cfperimcolor",	/* perimeter_color */
    "cfperimtype",	/* perimeter_type */
    "cfperimwidth",	/* perimeter_width */
    "cfperimspecmode",	/* perimeter_width_specification_mode */
    "cfpixarr",		/* pixel_array */
    "cfpolygon",	/* polygon */
    "cfpolyline",	/* polyline */
    "cfpolylnbundix",	/* polyline_bundle_index */
    "cfpolymarker",	/* polymarker */
    "cfpolymkbundix",	/* polymarker_bundle_index */
    "cfrectangle",	/* rectangle */
    "cfrsttodefs",	/* reset_to_defaults */
    "cfsaspsouflags",	/* set_aspect_source_flags */
    "cfsdrawmode",	/* set_drawing_mode */
    "cfserrwarnmk",	/* set_error_warning_mask */
    "cftext",		/* text */
    "cftextalign",	/* text_alignment */
    "cftextbundix",	/* text_bundle_index */
    "cftextcolor",	/* text_color */
    "cftextfontix",	/* text_font_index */
    "cftextprec",	/* text_precision */
    "cfvdcext",		/* vdc_extent */
    "cfvdmtext",	/* vdm_text */
};

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqdevid	 		    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int    cfqdevid_ (name,devid,f77strlen)
int    *name;			/* device name */
char   devid[];			/* Workstation type */
int	f77strlen;
{
    int err;
    char	tst[DEVNAMESIZE];

    err = inquire_device_identification (*name,tst);
    RETURN_STRING(devid, tst, f77strlen);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqdevclass	 			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfqdevclass_ (output, input)
int    *output, *input;/* output,input,segment abilities */
{
    return (inquire_device_class (output, input));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqphyscsys                                                */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int 	cfqphyscsys_ (name,xbase, ybase, xext, yext, xunits, yunits)
int    *name;			/* device name */
int    *xbase, *ybase;		/* base coordinates */
int    *xext, *yext;		/* number of pixels in each direction */
float  *xunits, *yunits;	/* number of pixels per mm. */
{
    return (inquire_physical_coordinate_system(*name,
    xbase, ybase, xext, yext, xunits, yunits));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqoutfunset                                               */
/*                                                                          */
/*		inquire_output_function_set	            		    */
/****************************************************************************/
int cfqoutfunset_ (level, support)
int * level;		/* level of output */
int * support;		/* amount of support */
{
    return (inquire_output_function_set (*level, support));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqvdctype 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfqvdctype_ (type)
int * type;			/* type of vdc space */
{
    return (inquire_vdc_type( (Cvdctype*)type ));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfqoutcap	 		    */
/*                                                                          */
/*               Return the FORTRAN binding names for the output            */
/*               capabilities.                                              */
/****************************************************************************/

int     cfqoutcap_ (firstP, numP, list, f77strlen)
int     *firstP;	/* first element of list to be returned */
int     *numP;		/* number of elements of list to be returned */
char   *list;		/* returned list */
int	f77strlen;
{
    int		i,cnt,err;
    char	*listP;
 
    if (!(err = _cgi_check_state_5 ())) {
	i = *firstP;
	cnt = *numP;

	if (0 <= i && i < OUTFUNS && 0 <= i+cnt && i+cnt <= OUTFUNS) {
	    listP = list;
	    for (listP = list; --cnt >= 0; listP += f77strlen, i++)
		RETURN_STRING(listP, f77_outlist[i], f77strlen);
	}
	else
	    err = EINQALTL;
    }

    return (_cgi_errhand(err));
}
