#ifndef lint
static char	sccsid[] = "@(#)bundle.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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

/*
set_aspect_source_flags
cgipw_set_aspect_source_flags
_cgi_set_aspect_source_flags
define_bundle_index
cgipw_define_bundle_index
_cgi_check_bundle_index
_cgi_define_bundle_index
polyline_bundle_index
cgipw_polyline_bundle_index
_cgi_check_polyline_bundle_index
_cgi_polyline_bundle_index
polymarker_bundle_index
cgipw_polymarker_bundle_index
_cgi_check_polymarker_bundle_index
_cgi_polymarker_bundle_index
fill_area_bundle_index
cgipw_fill_area_bundle_index
_cgi_check_fill_area_bundle_index
_cgi_fill_area_bundle_index
text_bundle_index
cgipw_text_bundle_index
_cgi_check_text_bundle_index
_cgi_text_bundle_index
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;
Outatt         *_cgi_att;

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_aspect_source_flags				    */
/*                                                                          */
/*		Determines which set of attributes are to be used.	    */
/****************************************************************************/
Cerror          set_aspect_source_flags(flags)
Cflaglist      *flags;		/* value of attribute (INDIVIDUAL or BUNDLED) */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	/* put in error for flag value out of range */
	if (_cgi_set_aspect_source_flags(_cgi_att, flags))
	{
	    ivw = 0;
	    while (_cgi_bump_all_vws(&ivw))
		_cgi_reset_intatt(_cgi_vws);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_set_aspect_source_flags				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_set_aspect_source_flags(desc, flags)
Ccgiwin        *desc;
Cflaglist      *flags;		/* value of attribute (INDIVIDUAL or BUNDLED) */
{
    SETUP_CGIWIN(desc);
    /* put in error for flag value out of range */
    if (_cgi_set_aspect_source_flags(desc->vws->att, flags))
	_cgi_reset_intatt(desc->vws);
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_set_aspect_source_flags				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_set_aspect_source_flags(att, flags)
register Outatt *att;
Cflaglist      *flags;		/* value of attribute (INDIVIDUAL or BUNDLED) */
{
    int             i;
    int             flag = 0;

    for (i = 0; i < flags->n; i++)
	/* wds 851009: there was NO CHECK for valid flag number! */
	if (flags->num[i] >= 0 && flags->num[i] <= 17)
	{
	    att->asfs[flags->num[i]] = flags->value[i];
	    if (flags->value[i] == BUNDLED)
		switch (flags->num[i])
		{
		case 0:
		    att->line.style =
			att->aes_table[att->line.index - 1]->line_type;
		    break;
		case 1:
		    att->line.width =
			att->aes_table[att->line.index - 1]->line_width;
		    flag = 1;
		    break;
		case 2:
		    att->line.color
			= att->aes_table[att->line.index - 1]->line_color;
		    break;
		case 3:
		    att->marker.type
			= att->aes_table[att->marker.index - 1]->marker_type;
		    break;
		case 4:
		    att->marker.size
			= att->aes_table[att->marker.index - 1]->marker_size;
		    flag = 1;
		    break;
		case 5:
		    att->marker.color =
			att->aes_table[att->marker.index - 1]->marker_color;
		    break;
		case 6:
		    att->fill.style =
			att->aes_table[att->fill.index - 1]->interior_style;
		    break;
		case 7:
		    att->fill.hatch_index =
			att->aes_table[att->fill.index - 1]->hatch_index;
		    break;
		case 8:
		    att->fill.pattern_index =
			att->aes_table[att->fill.index - 1]->pattern_index;
		    break;
		case 9:
		    att->fill.color =
			att->aes_table[att->fill.index - 1]->fill_color;
		    break;
		case 10:
		    att->fill.pstyle =
			att->aes_table[att->fill.index - 1]->perimeter_type;
		    break;
		case 11:
		    att->fill.pwidth =
			att->aes_table[att->fill.index - 1]->perimeter_width;
		    flag = 1;
		    break;
		case 12:
		    att->fill.pcolor =
			att->aes_table[att->fill.index - 1]->perimeter_color;
		    break;
		case 13:
		    att->text.attr.current_font =
			att->aes_table[att->text.attr.index - 1]->text_font;
		    break;
		case 14:
		    att->text.attr.precision =
			att->aes_table
			[att->text.attr.index - 1]->text_precision;
		    break;
		case 15:
		    att->text.attr.exp_factor
			= att->aes_table
			[att->text.attr.index - 1]->character_expansion;
		    flag = 1;
		    break;
		case 16:
		    att->text.attr.space
			= att->aes_table
			[att->text.attr.index - 1]->character_spacing;
		    flag = 1;
		    break;
		case 17:
		    att->text.attr.color =
			att->aes_table[att->text.attr.index - 1]->text_color;
		    break;
		default:
		    break;
		}
	    /*
	     * wds 851010: note that switching flag to INDIVIDUAL doesn't
	     * change attr. User must call individual attribute routine again. 
	     */
	}
    return (flag);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: define_bundle_index					    */
/*                                                                          */
/*                                                                          */
/*		Defines an entry in the AES table.			    */
/****************************************************************************/
Cerror          define_bundle_index(index, entry)
Cint            index;		/* entry in AES table */
Cbunatt        *entry;		/* new attribute values */

{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	err = _cgi_check_bundle_index(_cgi_att, index);
	if (!err && _cgi_define_bundle_index(_cgi_att, index, entry))
	{
	    ivw = 0;
	    while (_cgi_bump_all_vws(&ivw))
		_cgi_reset_intatt(_cgi_vws);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_define_bundle_index 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_define_bundle_index(desc, index, entry)
Ccgiwin        *desc;
Cint            index;		/* polyline bundle index */
Cbunatt        *entry;		/* new attribute values */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_bundle_index(desc->vws->att, index);
    if (!err)
	if (_cgi_define_bundle_index(desc->vws->att, index, entry))
	    _cgi_reset_intatt(desc->vws);

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror _cgi_check_bundle_index(att, index)
Outatt         *att;
Cint            index;		/* polyline bundle index */
{
    if (index >= MAXAESSIZE || index < 1)	/* 850815 off-by-one fix */
	return (EBBDTBDI);
    else
    {				/* Valid bundle definition */
	if (att->aes_table[index - 1] == (Cbunatt *) NULL)
	    att->aes_table[index - 1] = (Cbunatt *) malloc(sizeof(Cbunatt));
	if (att->aes_table[index - 1] == (Cbunatt *) NULL)
	    return (EMEMSPAC);
    }
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_define_bundle_index 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_define_bundle_index(att, index, entry)
register Outatt *att;
Cint            index;		/* polyline bundle index */
Cbunatt        *entry;		/* new attribute values */
{
    int             flag = 0;

    *(att->aes_table[index - 1]) = *entry;

    /* wds 851009: NO CHECK for valid attribute values! */
    /*
     * Can't use all the attribute routines to do this, because they only allow
     * INDIVIDUAL ASFs. Both this code and the (individual) attribute-setting
     * routines should call some internal routines to do the value checking. 
     */

    /*
     * If bundle is one of the current bundle indices (in use), then we test
     * ASFs and possibly set bundle values into current values. 
     */
    if (att->line.index == index)
    {
	if (att->asfs[0] == BUNDLED)
	    att->line.style = entry->line_type;
	if (att->asfs[1] == BUNDLED)
	{
	    att->line.width = entry->line_width;
	    flag = 1;
	}
	if (att->asfs[2] == BUNDLED)
	    att->line.color = entry->line_color;
    }
    if (att->marker.index == index)
    {
	if (att->asfs[3] == BUNDLED)
	    att->marker.type = entry->marker_type;
	if (att->asfs[4] == BUNDLED)
	{
	    att->marker.size = entry->marker_size;
	    flag = 1;
	}
	if (att->asfs[5] == BUNDLED)
	    att->marker.color = entry->marker_color;
    }
    if (att->fill.index == index)
    {
	if (att->asfs[6] == BUNDLED)
	    att->fill.style = entry->interior_style;
	if (att->asfs[7] == BUNDLED)
	    att->fill.hatch_index = entry->hatch_index;
	if (att->asfs[8] == BUNDLED)
	    att->fill.pattern_index = entry->pattern_index;
	if (att->asfs[9] == BUNDLED)
	    att->fill.color = entry->fill_color;
	if (att->asfs[10] == BUNDLED)
	    att->fill.pstyle = entry->perimeter_type;
	if (att->asfs[11] == BUNDLED)
	{
	    att->fill.pwidth = entry->perimeter_width;
	    flag = 1;
	}
	if (att->asfs[12] == BUNDLED)
	    att->fill.pcolor = entry->perimeter_color;
    }
    if (att->text.attr.index == index)
    {
	if (att->asfs[13] == BUNDLED)
	    att->text.attr.current_font = entry->text_font;
	if (att->asfs[14] == BUNDLED)
	    att->text.attr.precision = entry->text_precision;
	if (att->asfs[15] == BUNDLED)
	    att->text.attr.exp_factor = entry->character_expansion;
	if (att->asfs[16] == BUNDLED)
	    att->text.attr.space = entry->character_spacing;
	if (att->asfs[17] == BUNDLED)
	    att->text.attr.color = entry->text_color;
	flag = 1;
    }
    return (flag);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polyline_bundle_index	 				    */
/*                                                                          */
/*               Defines current polyline_bundle_index                      */
/****************************************************************************/
Cerror          polyline_bundle_index(index)
Cint            index;		/* polyline bundle index */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_polyline_bundle_index(_cgi_att, index);
    if (!err && _cgi_polyline_bundle_index(_cgi_att, index))
    {
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_line_width(_cgi_vws);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_polyline_bundle_index 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_polyline_bundle_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* polyline bundle index */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_polyline_bundle_index(desc->vws->att, index);
    if (!err)
	if (_cgi_polyline_bundle_index(desc->vws->att, index))
	    _cgi_set_conv_line_width(desc->vws);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_polyline_bundle_index			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror _cgi_check_polyline_bundle_index(att, index)
Outatt         *att;
Cint            index;		/* polyline bundle index */
{
    if (index >= MAXAESSIZE || index < 1)	/* 850815 off-by-one fix */
	return (EBBDTBDI);
    else if (!att->aes_table[index - 1])
	return (EBADLINX);
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_polyline_bundle_index 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_polyline_bundle_index(att, index)
register Outatt *att;
Cint            index;		/* polyline bundle index */
{
    int             flag = 0;

    att->line.index = index;

    if (att->asfs[0] == BUNDLED)
	att->line.style = att->aes_table[index - 1]->line_type;
    if (att->asfs[1] == BUNDLED)
    {
	att->line.width = att->aes_table[index - 1]->line_width;
	flag = 1;
    }
    if (att->asfs[2] == BUNDLED)
	att->line.color = att->aes_table[index - 1]->line_color;
    return (flag);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: polymarker_bundle_index	 			    */
/*                                                                          */
/*               Defines current polymarker_bundle_index                    */
/****************************************************************************/
Cerror          polymarker_bundle_index(index)
Cint            index;		/* polymarker bundle index */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_polymarker_bundle_index(_cgi_att, index);
    if (!err && _cgi_polymarker_bundle_index(_cgi_att, index))
    {
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_marker_size(_cgi_vws);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_polymarker_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_polymarker_bundle_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* polymarker bundle index */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_polymarker_bundle_index(desc->vws->att, index);
    if (!err)
	if (_cgi_polymarker_bundle_index(desc->vws->att, index))
	    _cgi_set_conv_marker_size(desc->vws);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_polymarker_bundle_index			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror _cgi_check_polymarker_bundle_index(att, index)
Outatt         *att;
Cint            index;		/* polymarker bundle index */
{
    if (index >= MAXAESSIZE || index < 1)	/* 850815 off-by-one fix */
	return (EBBDTBDI);
    else if (!att->aes_table[index - 1])
	return (EBADMRKX);
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_polymarker_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_polymarker_bundle_index(att, index)
register Outatt *att;
Cint            index;		/* polymarker bundle index */
{
    int             flag = 0;

    att->marker.index = index;
    if (att->asfs[3] == BUNDLED)
	att->marker.type = att->aes_table[index - 1]->marker_type;
    if (att->asfs[4] == BUNDLED)
    {
	att->marker.size = att->aes_table[index - 1]->marker_size;
	flag = 1;
    }
    if (att->asfs[5] == BUNDLED)
	att->marker.color = att->aes_table[index - 1]->marker_color;
    return (flag);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: fill_area_bundle_index	 				    */
/*                                                                          */
/*               Defines current fill_area_bundle_index                     */
/****************************************************************************/
Cerror          fill_area_bundle_index(index)
Cint            index;		/* fill_area bundle index */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_fill_area_bundle_index(_cgi_att, index);
    if (!err && _cgi_fill_area_bundle_index(_cgi_att, index))
    {
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_perimeter_width(_cgi_vws);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_fill_area_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_fill_area_bundle_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* fill_area bundle index */
{
    int             err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_fill_area_bundle_index(desc->vws->att, index);
    if (!err)
	if (_cgi_fill_area_bundle_index(desc->vws->att, index))
	    _cgi_set_conv_perimeter_width(desc->vws);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_fill_area_bundle_index			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror _cgi_check_fill_area_bundle_index(att, index)
Outatt         *att;
Cint            index;		/* fill_area bundle index */
{
    if (index >= MAXAESSIZE || index < 1)	/* 850815 off-by-one fix */
	return (EBBDTBDI);
    else if (!att->aes_table[index - 1])
	return (EBADFABX);
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_area_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_fill_area_bundle_index(att, index)
register Outatt *att;
Cint            index;		/* fill_area bundle index */
{
    int             flag = 0;

    att->fill.index = index;
    if (att->asfs[6] == BUNDLED)
	att->fill.style = att->aes_table[index - 1]->interior_style;
    if (att->asfs[7] == BUNDLED)
	att->fill.hatch_index = att->aes_table[index - 1]->hatch_index;
    if (att->asfs[8] == BUNDLED)
	att->fill.pattern_index = att->aes_table[index - 1]->pattern_index;
    if (att->asfs[9] == BUNDLED)
	att->fill.color = att->aes_table[index - 1]->fill_color;
    if (att->asfs[10] == BUNDLED)
	att->fill.pstyle = att->aes_table[index - 1]->perimeter_type;
    if (att->asfs[11] == BUNDLED)
    {
	att->fill.pwidth = att->aes_table[index - 1]->perimeter_width;
	flag = 1;
    }
    if (att->asfs[12] == BUNDLED)
	att->fill.pcolor = att->aes_table[index - 1]->perimeter_color;
    return (flag);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: text_bundle_index	 				    */
/*                                                                          */
/*               Defines current text_bundle_index                          */
/****************************************************************************/
Cerror          text_bundle_index(index)
Cint            index;		/* text bundle index */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_text_bundle_index(_cgi_att, index);
    if (!err)
    {
	_cgi_text_bundle_index(_cgi_att, index);
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_reset_internal_text(_cgi_vws);
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_text_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_text_bundle_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* text bundle index */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_text_bundle_index(desc->vws->att, index);
    if (!err)
	_cgi_text_bundle_index(desc->vws->att, index);
    _cgi_reset_internal_text(desc->vws);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_text_bundle_index				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror _cgi_check_text_bundle_index(att, index)
Outatt         *att;
Cint            index;		/* text bundle index */
{
    if (index >= MAXAESSIZE || index < 1)	/* 850815 off-by-one fix */
	return (EBBDTBDI);
    else if (!att->aes_table[index - 1])
	return (EBADTXTX);
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_text_bundle_index					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_text_bundle_index(att, index)
register Outatt *att;
Cint            index;		/* text bundle index */
{
    att->text.attr.index = index;
    if (att->asfs[13] == BUNDLED)
	att->text.attr.current_font = att->aes_table[index - 1]->text_font;
    if (att->asfs[14] == BUNDLED)
	att->text.attr.precision = att->aes_table[index - 1]->text_precision;
    if (att->asfs[15] == BUNDLED)
	att->text.attr.exp_factor = att->aes_table
	    [index - 1]->character_expansion;
    if (att->asfs[16] == BUNDLED)
	att->text.attr.space = att->aes_table[index - 1]->character_spacing;
    if (att->asfs[17] == BUNDLED)
	att->text.attr.color = att->aes_table[index - 1]->text_color;
}
