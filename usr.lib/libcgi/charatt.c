#ifndef lint
static char	sccsid[] = "@(#)charatt.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
cgipw_character_set_index
text_font_index
cgipw_text_font_index
_cgi_text_font_index
text_precision
cgipw_text_precision
_cgi_text_precision
character_expansion_factor
cgipw_character_expansion_factor
_cgi_character_expansion_factor
character_spacing
cgipw_character_spacing
_cgi_character_spacing
text_color
cgipw_text_color
_cgi_text_color
character_height
cgipw_character_height
_cgi_character_height
character_orientation
cgipw_character_orientation
_cgi_character_orientation
character_path
cgipw_character_path
text_alignment
cgipw_text_alignment
fixed_font
cgipw_fixed_font
_cgi_reset_all_internal_text
_cgi_reset_internal_text
*/

#include "cgipriv.h"
Gstate          _cgi_state;	/* CGI's global state */
View_surface   *_cgi_vws;	/* current view surface information */
Outatt         *_cgi_att;	/* current attribute information */
#include <math.h>

/****************************************************************************/
/*									    */
/*     FUNCTION: character_set_index					    */
/*									    */
/*	        Selects a set of fonts					    */
/*	This function is basically a no-op because			    */
/*	SunCGI only uses one set of fonts.				    */
/****************************************************************************/

/* ARGSUSED WCL: As documented, character_set_index 1 is the only one used. */
Cerror          character_set_index(index)
Cint            index;		/* font set */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	_cgi_att->text.attr.fontset = 1;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_set_index				    */
/*									    */
/*									    */
/*	        Selects a set of fonts					    */
/*	Like the character_set_index, this function is			    */
/*	basically a no-op because CGI uses only set of			    */
/*	fonts.								    */
/****************************************************************************/

/* ARGSUSED WDS: As documented, character_set_index 1 is the only one used. */
Cerror          cgipw_character_set_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* font set */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->text.attr.fontset = 1;
    return (NO_ERROR);
}


/****************************************************************************/
/*									    */
/*     FUNCTION: text_font_index					    */
/*									    */
/*		This routine resets the current font			    */
/*	The mapping of the font is done in the routine			    */
/*	_cgi_inner_text. It is important to remember that		    */
/*	fonts have different interpretations with			    */
/*	different precisions.						    */
/****************************************************************************/
Cerror          text_font_index(index)
Cint            index;		/* font */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_text_font_index(_cgi_att, index);
	if (!err)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_text_font_index					    */
/*									    */
/*	This is the cgipw version of text_font_index. It is this	    */
/*	function which checks the validity of the font.			    */
/****************************************************************************/

Cerror          cgipw_text_font_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* marker color */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_text_font_index(desc->vws->att, index);
    if (!err)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_text_font_index					    */
/*									    */
/*	This is the internal version of text_font_index. It is this	    */
/*	function which checks the validity of the font.			    */
/****************************************************************************/

Cerror          _cgi_text_font_index(att, index)
Outatt         *att;
Cint            index;		/* marker color */
{
    int             err = NO_ERROR;

    if (att->asfs[13] == INDIVIDUAL)
    {
	if (index >= ROMAN && index <= SYMBOLS)
	{
	    att->text.attr.current_font = index;
	}
	else
	    err = ETXTFLIN;
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: text_precision						    */
/*									    */
/*	This function determines whether raster or vector text		    */
/*	is to be rendered. If the precision is STRING, raster		    */
/*	(pixrect) text is displayed, otherwise vector text		    */
/*	is displayed.							    */
/****************************************************************************/
Cerror          text_precision(ttyp)
Cprectype       ttyp;		/* text type */

{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_text_precision(_cgi_att, ttyp);
	if (!err && ttyp != STRING)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_text_precision					    */
/*									    */
/*	This function actually resets the text precision.		    */
/*	Since text precision is a bundled attribute, the text precision	    */
/*	is only reset if ASF 14 is individual.				    */
/*	_cgi_reset_internal_text is called in case			    */
/*	attributes which affect non-STRING text have been called.	    */
/*									    */
/****************************************************************************/

Cerror          cgipw_text_precision(desc, ttyp)
Ccgiwin        *desc;
Cprectype       ttyp;		/* text type */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_text_precision(desc->vws->att, ttyp);
    if (!err && ttyp != STRING)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_text_precision					    */
/*									    */
/*	This function actually resets the text precision.		    */
/*	Since text precision is a bundled attribute, the text precision	    */
/*	is only reset if ASF 14 is individual.				    */
/*	_cgi_reset_internal_text is called in case			    */
/*	attributes which affect non-STRING text have been called.	    */
/*									    */
/****************************************************************************/

Cerror          _cgi_text_precision(att, ttyp)
Outatt         *att;
Cprectype       ttyp;		/* text type */
{
    int             err = NO_ERROR;

    if (att->asfs[14] == INDIVIDUAL)
    {
	att->text.attr.precision = ttyp;
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: character_expansion_factor				    */
/*									    */
/*	This function sets the width to height ratio. The actual	    */
/*	width of characters is determined by both the character		    */
/*	expansion factor and the character height.			    */
/****************************************************************************/
Cerror          character_expansion_factor(efac)
Cfloat          efac;		/* width factor */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_character_expansion_factor(_cgi_att, efac);
	if (!err)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_expansion_factor			    */
/*									    */
/*	This function implements the character expansion factor function.   */
/*	Two error conditions are checked: 1) the ASF (15) must		    */
/*	be set to INDIVIDUAL, and 2) the character exp. factor must	    */
/*	be between .01 and 10. This is a SUN restriction and		    */
/*	is imposed because char. exp. factors outside of this		    */
/*	range would produce unreadable characters.			    */
/*	The function _cgi_reset_internal_text is called to update	    */
/*	the appropriate internal variables.				    */
/*									    */
/****************************************************************************/

Cerror          cgipw_character_expansion_factor(desc, efac)
Ccgiwin        *desc;
Cfloat          efac;		/* width factor */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_character_expansion_factor(desc->vws->att, efac);
    if (!err)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_character_expansion_factor			    */
/*									    */
/*	This function implements the character expansion factor function.   */
/*	Two error conditions are checked: 1) the ASF (15) must		    */
/*	be set to INDIVIDUAL, and 2) the character exp. factor must	    */
/*	be between .01 and 10. This is a SUN restriction and		    */
/*	is imposed because char. exp. factors outside of this		    */
/*	range would produce unreadable characters.			    */
/*	The function _cgi_reset_internal_text is called to update	    */
/*	the appropriate internal variables.				    */
/*									    */
/****************************************************************************/

Cerror          _cgi_character_expansion_factor(att, efac)
Outatt         *att;
Cfloat          efac;		/* width factor */
{
    int             err = NO_ERROR;

    if (att->asfs[15] == INDIVIDUAL)
    {
	if ((efac >= 0.01) && (efac <= 10.0))
	{
	    att->text.attr.exp_factor = efac;
	}
	else
	    err = ECEXFOOR;
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: character_spacing					    */
/*									    */
/*	This function sets the amount of space between characters.	    */
/*	The spacing is expressed as a percentage of character height.	    */
/****************************************************************************/
Cerror          character_spacing(efac)
Cfloat          efac;		/* spacing ratio */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_character_spacing(_cgi_att, efac);
	if (!err)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_spacing				    */
/*									    */
/*	This function implements the character spacing function.	    */
/*	In addition to check the ASF(16), the spacing factor		    */
/*	must be between 10 and -10. Negative spacing factors		    */
/*	permit overstriking and an alternative method for		    */
/*	reversing the path.						    */
/*	The function _cgi_reset_internal_text is called to update	    */
/*	the appropriate internal variables.				    */
/*									    */
/****************************************************************************/

Cerror          cgipw_character_spacing(desc, efac)
Ccgiwin        *desc;
Cfloat          efac;		/* width factor */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_character_spacing(desc->vws->att, efac);
    if (!err)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_character_spacing					    */
/*									    */
/*	This function implements the character spacing function.	    */
/*	In addition to check the ASF(16), the spacing factor		    */
/*	must be between 10 and -10. Negative spacing factors		    */
/*	permit overstriking and an alternative method for		    */
/*	reversing the path.						    */
/*	The function _cgi_reset_internal_text is called to update	    */
/*	the appropriate internal variables.				    */
/*									    */
/****************************************************************************/

Cerror          _cgi_character_spacing(att, efac)
Outatt         *att;
Cfloat          efac;		/* width factor */
{
    int             err = NO_ERROR;

    if (att->asfs[16] == INDIVIDUAL)
    {
	if ((efac >= -10.0) && (efac <= 10.0))
	{
	    att->text.attr.space = efac;
	}
	else
	    err = ECEXFOOR;
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: text_color						    */
/*									    */
/*		Selects text color					    */
/****************************************************************************/
Cerror          text_color(index)
Cint            index;		/* color */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_text_color(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_text_color					    */
/*									    */
/*	This function actually changes the text color index. The ASF(17)    */
/*	must be set to INDIVIDUAL and the color index must be valid.	    */
/****************************************************************************/

Cerror          cgipw_text_color(desc, index)
Ccgiwin        *desc;
Cint            index;		/* text color */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_text_color(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_text_color					    */
/*									    */
/*	This function actually changes the text color index. The ASF(17)    */
/*	must be set to INDIVIDUAL and the color index must be valid.	    */
/****************************************************************************/

Cerror          _cgi_text_color(att, index)
Outatt         *att;
Cint            index;		/* text color */
{
    int             err = NO_ERROR;

    if (att->asfs[17] == INDIVIDUAL)
    {
	err = _cgi_check_color(index);
	if (!err)
	{
	    att->text.attr.color = index;
	}
    }
    else
    {
	err = EBTBUNDL;
    }
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: character_height					    */
/*									    */
/*	This function sets the character height. The character		    */
/*	height is the basis of the character width (expansion factor)	    */
/*	and the character spacing.					    */
/****************************************************************************/

Cerror          character_height(height)
Cint            height;		/* height in VDC */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_character_height(_cgi_att, height);
	if (!err)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_height					    */
/*									    */
/*	This function actually sets the character height. The height	    */
/*	must be positive. This function calls the _cgi_reset_internal_text  */
/*	function which resets the expansion factor, and spacing.	    */
/****************************************************************************/

Cerror          cgipw_character_height(desc, height)
Ccgiwin        *desc;
Cint            height;		/* height */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_character_height(desc->vws->att, height);
    if (!err)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_character_height					    */
/*									    */
/*	This function actually sets the character height. The height	    */
/*	must be positive. This function calls the _cgi_reset_internal_text  */
/*	function which resets the expansion factor, and spacing.	    */
/****************************************************************************/

Cerror          _cgi_character_height(att, height)
Outatt         *att;
Cint            height;		/* height */
{
    int             err = NO_ERROR;

    if (height > 0)
	att->text.attr.height = height;
    else
	err = ECHHTLEZ;
    return (err);
}


/****************************************************************************/
/*									    */
/*     FUNCTION: character_orientation					    */
/*									    */
/*	Specifies the skew and direction of text			    */
/*	The character orientation is determined by two vectors: the	    */
/*	up vector and the base vector.					    */
/****************************************************************************/

Cerror          character_orientation(bx, by, dx, dy)
 /* charprecision incompatibility */
Cfloat          bx, by, dx, dy;	/* base component and up component of up vector */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_character_orientation(_cgi_att, bx, by, dx, dy);
	if (!err)
	    _cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_orientation				    */
/*									    */
/*	This function implements the character_orientation function.	    */
/*	The routine checks to make sure that the up and base		    */
/*	vectors are not null.						    */
/****************************************************************************/

Cerror          cgipw_character_orientation(desc, bx, by, dx, dy)
Ccgiwin        *desc;
Cfloat          bx, by, dx, dy;	/* base component and up component of up vector */

{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_character_orientation(desc->vws->att, bx, by, dx, dy);
    if (!err)
	_cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_character_orientation				    */
/*									    */
/*	This function implements the character_orientation function.	    */
/*	The routine checks to make sure that the up and base		    */
/*	vectors are not null.						    */
/****************************************************************************/

Cerror          _cgi_character_orientation(att, bx, by, dx, dy)
Outatt         *att;
Cfloat          bx, by, dx, dy;	/* base component and up component of up vector */

{
    int             err = NO_ERROR;

    if (((bx) || (dx)) || ((by) || (dy)))
    {
	att->text.attr.basex = bx;
	att->text.attr.basey = by;
	att->text.attr.upx = dx;
	att->text.attr.upy = dy;
    }
    else
	err = ECHRUPVZ;
    return (err);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: character_path						    */
/*									    */
/*	This function specifies the direction in which text is		    */
/*	written. This attribute affects the interpretation of character	    */
/*	orientation and text alignment.					    */
/****************************************************************************/
Cerror          character_path(path)
Cpathtype       path;		/* text direction */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_att->text.attr.path = path;
	_cgi_reset_all_internal_text();
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_character_path					    */
/*									    */
/*	This function actually implements the character path.		    */
/*	no error checking is required.					    */
/*									    */
/****************************************************************************/

Cerror          cgipw_character_path(desc, path)
Ccgiwin        *desc;
Cpathtype       path;		/* text direction */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->text.attr.path = path;
    _cgi_reset_internal_text(desc->vws);
    return (NO_ERROR);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: text_alignment						    */
/*									    */
/*		Specifies criterion used to align text.			    */
/*	Alignment is done in both the vertical and horizontal directions    */
/*	The continuous alignment parameters allow variable		    */
/*	alignment.							    */
/****************************************************************************/
Cerror          text_alignment(halign, valign, hcalind, vcalind)
Chaligntype     halign;		/* horizontal alignment type */
Cvaligntype     valign;		/* vertical alignment type */
Cfloat          hcalind, vcalind;	/* continuous alignment indicators */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_att->text.attr.halign = halign;
	_cgi_att->text.attr.valign = valign;
	_cgi_att->text.attr.hcalind = hcalind;
	_cgi_att->text.attr.vcalind = vcalind;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_text_alignment					    */
/*									    */
/*	This function just implements text_alignment for		    */
/*	an individual view surface. No error checking is		    */
/*	required.							    */
/*									    */
/****************************************************************************/

Cerror          cgipw_text_alignment(desc, halign, valign, hcalind, vcalind)
Ccgiwin        *desc;
Chaligntype     halign;		/* horizontal alignment type */
Cvaligntype     valign;		/* vertical alignment type */
Cfloat          hcalind, vcalind;	/* continuous alignment indicators */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->text.attr.halign = halign;
    desc->vws->att->text.attr.valign = valign;
    desc->vws->att->text.attr.hcalind = hcalind;
    desc->vws->att->text.attr.vcalind = vcalind;
    return (NO_ERROR);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: fixed_font						    */
/*									    */
/*	This is a SunCGI extension which allows fixed or variable	    */
/*	width font to be implemented.					    */
/****************************************************************************/
Cerror          fixed_font(index)
Cint            index;		/* font set */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
	_cgi_att->text.fixed_font = index;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*									    */
/*     FUNCTION: cgipw_fixed_font					    */
/*									    */
/*	This function just implements fixed_font for			    */
/*	an individual view surface. No error checking is		    */
/*	required.							    */
/****************************************************************************/

Cerror          cgipw_fixed_font(desc, index)
Ccgiwin        *desc;
Cint            index;		/* font set */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->text.fixed_font = index;
    return (NO_ERROR);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_reset_all_internal_text				    */
/*									    */
/*	Call _cgi_reset_internal_text for all active view surfaces.	    */
/*									    */
/****************************************************************************/
_cgi_reset_all_internal_text()
{
    int             ivw;

    ivw = 0;
    while (_cgi_bump_all_vws(&ivw))
	_cgi_reset_internal_text(_cgi_vws);
}

/****************************************************************************/
/*									    */
/*     FUNCTION: _cgi_reset_internal_text				    */
/*									    */
/*	This is the most important text attribute function because	    */
/*	it combines the effects of character height, expansion factor,	    */
/*	spacing, path, and orientation. The first step is		    */
/*	to convert the orientation vectors into angles. The next	    */
/*	step is to combine the character size with the			    */
/*	angle and store the result					    */
/****************************************************************************/
_cgi_reset_internal_text(vws)
register View_surface *vws;
{
    register Conv_text *textP = &vws->conv.text;
    register Outatt *attP = vws->att;
    double          sin_base, sin_up, a1, ax, ay, cos_base, cos_up;
    double          xsign, ysign;
    double          k180 = 3.14159;
    Cfloat          conv_height, vdc_scale, cc_dc_range, cc_vdc_range;
    Cfontinfo       font_info;
    extern Cfontinfo _cgi_inq_font();

    /*
     * find sin_base: the sine of the angle between the x-axis and the base
     * vector
     */
    if (attP->text.attr.basey == 0.)
    {
	sin_base = 0.0;
	if (attP->text.attr.basex >= 0.)
	    cos_base = 1.;
	else
	    cos_base = -1.;
    }
    else
    {
	ax = attP->text.attr.basex;
	ay = attP->text.attr.basey;
	if (ax != 0.)
	{
	    a1 = atan(ay / ax);
	    if (ax < 0.0)
		a1 += k180;
	    sin_base = sin(a1);
	    cos_base = cos(a1);
	}
	else
	{			/* take care of +/- 90 degrees */
	    cos_base = 0.0;
	    if (ay < 0.)
		sin_base = -1.0;
	    else
		sin_base = 1.0;
	}
    }
    textP->sin_base = sin_base * attP->text.attr.exp_factor;
    textP->cos_base = cos_base * attP->text.attr.exp_factor;

    /*
     * find sin_up: the sine of the angle between the x-axis and the up vector.
     * negative sine reverses the sense of y-axis between screen and CGI
     * coordinate systems.
     */
    if (attP->text.attr.upy == 0.)
    {
	sin_up = 0.0;
	if (attP->text.attr.upx >= 0.)
	    cos_up = 1.;
	else
	    cos_up = -1.;
    }
    else
    {
	ax = attP->text.attr.upx;
	ay = attP->text.attr.upy;
	if (ax != 0.)
	{
	    a1 = atan(ay / ax);
	    if (ax < 0.0)
		a1 += k180;
	    sin_up = sin(a1);
	    cos_up = cos(a1);
	}
	else
	{			/* take care of +/- 90 degrees */
	    cos_up = 0.;
	    if (ay < 0.)
		sin_up = -1.0;
	    else
		sin_up = 1.0;
	}
    }
    textP->sin_up = sin_up;
    textP->cos_up = cos_up;

    /* calculate pixel scale factors */
    font_info = _cgi_inq_font(attP->text.attr.current_font);
    _cgi_f_devscalen(attP->text.attr.height, conv_height);
    textP->scale = conv_height / (float) font_info.ft_cap;
    vdc_scale = attP->text.attr.height / (float) font_info.ft_cap;
    cc_dc_range = (float) (1 << CC_DC_SHIFT);
    cc_vdc_range = (float) (1 << CC_VDC_SHIFT);
    if (((float) vws->xform.scale.x.num / (float) vws->xform.scale.x.den) < 0.)
	xsign = -1.;
    else
	xsign = 1.;
    if (((float) vws->xform.scale.y.num / (float) vws->xform.scale.y.den) < 0.)
	ysign = -1.;
    else
	ysign = 1.;

    /*
     * The xsign/ysign values are DC-relative, so mirror y to change to VDC.
     */
    textP->vdc_cos_base = textP->cos_base * vdc_scale * cc_vdc_range * xsign;
    textP->vdc_sin_up = -textP->sin_up * vdc_scale * cc_vdc_range * ysign;
    textP->vdc_sin_base = -textP->sin_base * vdc_scale * cc_vdc_range * ysign;
    textP->vdc_cos_up = textP->cos_up * vdc_scale * cc_vdc_range * xsign;

    if (attP->vdc.use_pw_size)
	ysign = -ysign;
    textP->dc_cos_base = textP->cos_base * textP->scale * cc_dc_range * xsign;
    textP->dc_sin_up = textP->sin_up * textP->scale * cc_dc_range * ysign;
    textP->dc_sin_base = textP->sin_base * textP->scale * cc_dc_range * ysign;
    textP->dc_cos_up = textP->cos_up * textP->scale * cc_dc_range * xsign;

    /*
     * Update time stamp only after all work is done.  This keeps
     * _cgi_inq_font() call above from updating internal font info based on
     * partially complete set of text numbers.
     */
    _cgi_state.text_change_cnt++;
}
