#ifndef lint
static char	sccsid[] = "@(#)negotiate.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
inquire_vdc_type
inquire_output_capabilities
*/

#include "cgipriv.h"		/* defines types used in this file  */

View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */

extern char    *strcpy();

static char    *outlist[] =
{
 "vdc_extent",
 "device_viewport",
 "clip_rectangle",
 "clip_indicator",
 "hard_reset",
 "reset_to_defaults",
 "clear_view_surface",
 "set_error_warning_mask",
 "clear_control",
 "polyline",
 "disjoint_polyline",
 "polymarker",
 "polygon",
 "partial_polygon",
 "rectangle",
 "circle",
 "circular_arc_center",
 "circular_arc_center_close",
 "circular_arc_3pt",
 "circular_arc_3pt_close",
 "ellipse",
 "elliptical_arc",
 "elliptical_arc_close",
 "text",
 "vdm_text",
 "append_text",
 "cell_array",
 "pixel_array",
 "bitblt_source_array",
 "bitblt_pattern_array",
 "bitblt_patterned_source_array",
 "set_drawing_mode",
 "set_aspect_source_flags",
 "polyline_bundle_index",
 "line_type",
 "line_width_specification_mode",
 "line_width",
 "line_color",
 "polymarker_bundle_index",
 "marker_type",
 "marker_width_specification_mode",
 "marker_width",
 "marker_color",
 "fill_area_bundle_index",
 "interior_style",
 "fill_color",
 "hatch_index",
 "pattern_index",
 "pattern_table",
 "pattern_reference_point",
 "pattern_size",
 "perimeter_type",
 "perimeter_width",
 "perimeter_width_specification_mode",
 "perimeter_color",
 "text_bundle_index",
 "text_precision",
 "character_set_index",
 "text_font_index",
 "character_expansion_factor",
 "character_spacing",
 "character_height",
 "text_color",
 "character_orientation",
 "character_path",
 "text_alignment",
 "color_table"
};

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_device_identification		 		    */
/*                                                                          */
/*		Reports SUN Workstation type				    */
/****************************************************************************/

Cerror          inquire_device_identification(name, devid)
Cint            name;		/* device name */
Cchar          *devid;		/* Workstation type */
{
    char           *temp;
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_context(name);
	if (!err)
	{
	    switch (_cgi_view_surfaces[name]->device)
	    {
	    case (BW1DD):
		temp = "BW1DD";
		break;
	    case (BW2DD):
		temp = "BW2DD";
		break;
	    case (CG1DD):
		temp = "CG1DD";
		break;
	    case (GP1DD):
		temp = "GP1DD";
		break;
	    case (CG2DD):
		temp = "CG2DD";
		break;
	    case (CG3DD):
		temp = "CG3DD";
		break;
	    case (CG4DD):
		temp = "CG4DD";
		break;
#ifndef	NO_CG6
	    case (CG6DD):
		temp = "CG6DD";
		break;
#endif	NO_CG6
	    case (BWPIXWINDD):
		temp = "BWPIXWINDD";
		break;
	    case (CGPIXWINDD):
		temp = "GCPIXWINDD";
		break;
	    case (PIXWINDD):
		temp = "PIXWINDD";
		break;
	    default:
		temp = "";
		break;
	    }
	    (void) strcpy(devid, temp);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_device_class		 			    */
/*                                                                          */
/*		Describes general device ability			    */
/****************************************************************************/

Cerror          inquire_device_class(output, input)
Cint           *output, *input;	/* output,input,segment abilities */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	*output = OUTFUNS;
	*input = INFUNS;
    }
    return (_cgi_errhand(err));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_coordinate_system			 	    */
/*                                                                          */
/*		Describes physical dimension of coordinate system.	    */
/****************************************************************************/
Cerror          inquire_physical_coordinate_system
                (name, xbase, ybase, xext, yext, xunits, yunits)
Cint            name;		/* device name */
Cint           *xbase, *ybase;	/* base coordinates */
Cint           *xext, *yext;	/* number of pixels in each direction */
Cfloat         *xunits, *yunits;/* number of pixels per mm. */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_legal_winname(name);
    if (!err)
    {
	err = _cgi_context(name);
	*xbase = _cgi_view_surfaces[name]->vport.r_left;
	*ybase = _cgi_view_surfaces[name]->vport.r_top;
	*xext = _cgi_view_surfaces[name]->vport.r_width;
	*yext = _cgi_view_surfaces[name]->vport.r_height;
	*xunits = 0.0;
	*yunits = 0.0;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_output_function_set	 			    */
/*                                                                          */
/*		inquire_output_function_set	            		    */
/****************************************************************************/
Cerror          inquire_output_function_set(level, support)
Cint            level;		/* level of output */
Csuptype       *support;	/* amount of support */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	if ((level >= 1) && (level <= 6))
	    *support = ALL_NON_REQUIRED_FUNCTIONS;
	else
	    *support = NONE;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_vdc_type	 				    */
/*                                                                          */
/*		inquire_vdc_type					    */
/****************************************************************************/

Cerror          inquire_vdc_type(type)
Cvdctype       *type;		/* type of vdc space */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	*type = INTEGER;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  inquire_output_capabilities		 		    */
/*                                                                          */
/*		Lists output function				            */
/****************************************************************************/

Cerror          inquire_output_capabilities(first, num, list)
Cint            first;		/* first element to be returned */
Cint            num;		/* num of elements to be returned */
Cchar          *list[];		/* returned list */
{
    int             err, i;

    err = _cgi_check_state_5();
    if (!err)
    {
	if ((0 <= first && first < OUTFUNS)
	    &&
	    (0 <= first + num - 1 && first + num - 1 <= OUTFUNS)
	    )
	{
	    for (i = 0; i < num; i++)
		list[i] = outlist[i + first];
	}
	else
	    err = EINQALTL;	/* Args out of range of function list */
    }
    return (_cgi_errhand(err));
}
