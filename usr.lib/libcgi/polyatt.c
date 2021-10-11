#ifndef lint
static char	sccsid[] = "@(#)polyatt.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
line_type
cgipw_line_type
_cgi_line_type
line_width
cgipw_line_width
_cgi_check_line_width
_cgi_set_conv_line_width
line_color
cgipw_line_color
_cgi_line_color
line_width_specification_mode
cgipw_line_width_specification_mode
line_endstyle
cgipw_line_endstyle
_cgi_line_endstyle
interior_style
cgipw_interior_style
_cgi_interior_style
fill_color
cgipw_fill_color
_cgi_fill_color
hatch_index
cgipw_hatch_index
_cgi_hatch_index
pattern_index
cgipw_pattern_index
_cgi_pattern_index
pattern_table
pattern_reference_point
cgipw_pattern_reference_point
pattern_size
cgipw_pattern_size
perimeter_type
cgipw_perimeter_type
_cgi_perimeter_type
perimeter_width
cgipw_perimeter_width
_cgi_check_perimeter_width
_cgi_set_conv_perimeter_width
perimeter_color
cgipw_perimeter_color
_cgi_perimeter_color
perimeter_width_specification_mode
cgipw_perimeter_width_specification_mode
*/

#include "cgipriv.h"

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;
Cint           *_cgi_pattern_table[MAXNUMPATS];	/* pattern table */

/* WDS: most of these routines don't check incoming attributes for validity.
 * And there are no error codes defined suitable for most of the invalidities!
 * So for now, quiet change them to the default if the argument is invalid.
 */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_type                                           	    */
/*                                                                          */
/*		Sets line type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          line_type(ttyp)
Clintype        ttyp;		/* style of line */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_line_type(_cgi_att, ttyp);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_line_type                                     	    */
/*                                                                          */
/*                                                                          */
/*		Sets line type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          cgipw_line_type(desc, ttyp)
Ccgiwin        *desc;
Clintype        ttyp;		/* style of line */

{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_line_type(desc->vws->att, ttyp);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_line_type                                     	    */
/*                                                                          */
/*                                                                          */
/*		Sets line type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          _cgi_line_type(att, ttyp)
Outatt         *att;
Clintype        ttyp;		/* style of line */

{
    Cerror          err = NO_ERROR;

    if (att->asfs[0] == INDIVIDUAL)
    {
	if (((int) ttyp < (int) SOLID) || ((int) ttyp > (int) LONG_DASHED))
	    ttyp = SOLID;
	att->line.style = (Clintype) ttyp;
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_width                                           	    */
/*                                                                          */
/*		Sets line width 					    */
/****************************************************************************/

Cerror          line_width(width)
Cfloat          width;		/* line width */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_line_width(_cgi_att, width);
    if (!err)
    {
	_cgi_att->line.width = width;
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_line_width(_cgi_vws);
    }

    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_line_width                                     	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_line_width(desc, width)
Ccgiwin        *desc;
Cfloat          width;		/* line width */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_check_line_width(desc->vws->att, width);
    if (!err)
    {
	desc->vws->att->line.width = width;
	_cgi_set_conv_line_width(desc->vws);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_line_width					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

static Cerror   _cgi_check_line_width(att, width)
Outatt         *att;
Cfloat          width;		/* line width */
{
    if (att->asfs[1] != INDIVIDUAL)
	return (EBTBUNDL);
    else if (width < 0)
	return (EBDWIDTH);	/* width must be positive */
    else
	return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_set_conv_line_width				    */
/*                                                                          */
/*   Created 851009 (wds) to organize setting the current line.width        */
/****************************************************************************/
_cgi_set_conv_line_width(vws)
View_surface   *vws;
{
    Outatt         *attP = vws->att;

    if (attP->line_spec_mode == SCALED)
    {
	register float  wtemp;	/* temporary result */

	/* wds 850617: viewport coords are "VDC" for pixwin surfaces */
	if (attP->vdc.use_pw_size)
	    wtemp = vws->vport.r_width;
	else
	    wtemp = abs(attP->vdc.rect.r_width);
	_cgi_devscalen((int) (wtemp * (attP->line.width / 100.) + 0.5), wtemp);
	if (wtemp < 1)
	    wtemp = 1;
	vws->conv.line_width = wtemp;
    }
    else
    {
	vws->conv.line_width = attP->line.width;
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_color                                          	    */
/*                                                                          */
/*		Sets line color 					    */
/****************************************************************************/

Cerror          line_color(index)
Cint            index;		/* line color */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_line_color(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_line_color					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_line_color(desc, index)
Ccgiwin        *desc;
Cint            index;		/* line color */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_line_color(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_line_color					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_line_color(att, index)
Outatt         *att;
Cint            index;		/* line color */
{
    Cerror          err = NO_ERROR;

    if (att->asfs[2] == INDIVIDUAL)
    {
	err = _cgi_check_color(index);
	if (!err)
	{
	    att->line.color = index;
	}
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_width_specification_mode	 			    */
/*                                                                          */
/*		sets line_width_specification_mode			    */
/****************************************************************************/

Cerror          line_width_specification_mode(mode)
Cspecmode       mode;		/* pixels or percent */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	if (mode != ABSOLUTE)
	    mode = SCALED;
	_cgi_att->line_spec_mode = mode;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_line_width_specification_mode 			    */
/*                                                                          */
/*		sets line_width_specification_mode			    */
/****************************************************************************/

Cerror          cgipw_line_width_specification_mode(desc, mode)
Ccgiwin        *desc;
Cspecmode       mode;		/* pixels or percent */
{
    SETUP_CGIWIN(desc);
    if (mode != ABSOLUTE)
	mode = SCALED;
    desc->vws->att->line_spec_mode = mode;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: line_endstyle			 			    */
/*                                                                          */
/*		sets line endstyle					    */
/****************************************************************************/

Cerror          line_endstyle(mode)
Cendstyle       mode;		/* pixels or percent */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	(void) _cgi_line_endstyle(_cgi_att, mode);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_line_endstyle 			 		    */
/*                                                                          */
/*		sets line endstyle					    */
/****************************************************************************/

Cerror          cgipw_line_endstyle(desc, mode)
Ccgiwin        *desc;
Cendstyle       mode;		/* pixels or percent */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_line_endstyle(desc->vws->att, mode);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_line_endstyle 			 		    */
/*                                                                          */
/*		sets line endstyle					    */
/****************************************************************************/

Cerror          _cgi_line_endstyle(att, mode)
Outatt         *att;
Cendstyle       mode;		/* pixels or percent */
{
    /* wds 851010: mode is not checked for validity */
    if (((int) mode < (int) NATURAL) || ((int) mode > (int) BEST_FIT))
	mode = BEST_FIT;
    att->endstyle = mode;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: interior_style	 					    */
/*                                                                          */
/*		Sets fill style for polygon				    */
/****************************************************************************/

Cerror          interior_style(istyle, perimvis)
Cintertype      istyle;		/* fill style */
Cflag           perimvis;	/* perimeter visibility */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_interior_style(_cgi_att, istyle, perimvis);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_interior_style                                       */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_interior_style(desc, istyle, perimvis)
Ccgiwin        *desc;
Cintertype      istyle;		/* fill style */
Cflag           perimvis;	/* perimeter visibility */

{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_interior_style(desc->vws->att, istyle, perimvis);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_interior_style					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_interior_style(att, istyle, perimvis)
Outatt         *att;
Cintertype      istyle;		/* fill style */
Cflag           perimvis;	/* perimeter visibility */

{
    Cerror          err = NO_ERROR;

    att->fill.visible = perimvis;
    if (att->asfs[6] == INDIVIDUAL)
    {
	if (((int) istyle < (int) HOLLOW) || ((int) istyle > (int) HATCH))
	    istyle = HOLLOW;
	att->fill.style = istyle;
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: fill_color                                          	    */
/*                                                                          */
/*		Sets fill color 					    */
/****************************************************************************/

Cerror          fill_color(index)
Cint            index;		/* fill color */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_fill_color(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_fill_color					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_fill_color(desc, index)
Ccgiwin        *desc;
Cint            index;		/* fill color */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_fill_color(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fill_color					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_fill_color(att, index)
Outatt         *att;
Cint            index;		/* fill color */
{
    Cerror          err = NO_ERROR;

    if (att->asfs[9] == INDIVIDUAL)
    {
	err = _cgi_check_color(index);
	if (!err)
	{
	    att->fill.color = index;
	}
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: hatch_index                                          	    */
/*                                                                          */
/****************************************************************************/

Cerror          hatch_index(index)
Cint            index;		/* fill color */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_hatch_index(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_hatch_index                                         */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_hatch_index(desc, index)
Ccgiwin        *desc;
Cint            index;		/* fill color */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_hatch_index(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_hatch_index					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_hatch_index(att, index)
Outatt         *att;
Cint            index;		/* fill color */
{
    Cerror          err = NO_ERROR;

    if (att->asfs[7] == INDIVIDUAL)
    {
	if (index >= 0)
	{
	    if (index < MAXNUMPATS)
	    {
		if (_cgi_pattern_table[index])
		{
		    att->fill.hatch_index = index;
		}
		else
		    err = ENOPATNX;
	    }
	    else
		err = EPATITOL;
	}
	else
	    err = ESTYLLEZ;
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pattern_index                                         	    */
/*                                                                          */
/****************************************************************************/

Cerror          pattern_index(index)
Cint            index;
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_pattern_index(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_pattern_index                                        */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_pattern_index(desc, index)
Ccgiwin        *desc;
Cint            index;
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_pattern_index(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_pattern_index                                        */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_pattern_index(att, index)
Outatt         *att;
Cint            index;
{
    Cerror          err = NO_ERROR;

    if (att->asfs[8] == INDIVIDUAL)
    {
	if (index >= 0)
	{
	    if (index < MAXNUMPATS)
	    {
		if (_cgi_pattern_table[index])
		{
		    att->fill.pattern_index = index;
		    att->pattern.cur_index = index;
		    att->pattern.row = _cgi_pattern_table[index][1];
		    att->pattern.column = _cgi_pattern_table[index][2];
		    att->pattern.colorlist = &(_cgi_pattern_table[index][3]);
		    /* really an array */
		}
		else
		    err = ENOPATNX;
	    }
	    else
		err = EPATITOL;
	}
	else
	    err = ESTYLLEZ;
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pattern_table	 					    */
/*                                                                          */
/*		defines an entry in the pattern table			    */
/****************************************************************************/

Cint            pattern_table(index, m, n, colorind)
Cint            index;		/* entry in table */
Cint            m, n;		/* number of rows and columns */
Cint           *colorind;	/* array containing pattern */
{
    int             i, k;
    Cerror          err;
    int            *newset;

    err = _cgi_check_state_5();
    if (!err)
    {
	k = m * n;
	if (k > MAXPATSIZE)
	    err = EPATARTL;	/* error pattern too large */
	else if (index >= MAXNUMPATS)
	    err = EPATITOL;	/* pattern index too large */
	else
	{
	    if (m || n)
	    {
		if (index >= 0)
		{
		    newset = (int *) malloc((unsigned) ((k + 3)
							* sizeof(int)));
		    newset[0] = 1;
		    newset[1] = m;
		    newset[2] = n;
		    for (i = 3; i < k + 3; i++)
		    {
			newset[i] = *colorind;
			colorind++;
		    }
		    _cgi_pattern_table[index] = newset;
		}
		else
		    err = ESTYLLEZ;
	    }
	    else
		err = EPATSZTS;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pattern_reference_point                               	    */
/*                                                                          */
/****************************************************************************/

Cerror          pattern_reference_point(begin)
Ccoor          *begin;
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	_cgi_att->pattern.point = begin;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_pattern_reference_point                              */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_pattern_reference_point(desc, begin)
Ccgiwin        *desc;
Ccoor          *begin;
{
    SETUP_CGIWIN(desc);
    desc->vws->att->pattern.point = begin;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: pattern_size						    */
/*                                                                          */
/*		pattern_size defines the size of the pattern array	    */
/*		in screen coordinates.					    */
/****************************************************************************/

Cerror          pattern_size(dx, dy)
Cint            dx, dy;		/* size of pattern in VDC space */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_att->pattern.dx = dx;
	_cgi_att->pattern.dy = dy;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_pattern_size                              */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_pattern_size(desc, dx, dy)
Ccgiwin        *desc;
Cint            dx, dy;		/* size of pattern in VDC space */
{
    SETUP_CGIWIN(desc);
    desc->vws->att->pattern.dx = dx;
    desc->vws->att->pattern.dy = dy;
    return (NO_ERROR);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: perimeter_type                                        	    */
/*                                                                          */
/*		Sets perimeter type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          perimeter_type(ttyp)
Clintype        ttyp;		/* style of perimeter */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_perimeter_type(_cgi_att, ttyp);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_perimeter_type                                  	    */
/*                                                                          */
/*                                                                          */
/*		Sets perimeter type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          cgipw_perimeter_type(desc, ttyp)
Ccgiwin        *desc;
Clintype        ttyp;		/* style of perimeter */

{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_perimeter_type(desc->vws->att, ttyp);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_perimeter_type                                  	    */
/*                                                                          */
/*                                                                          */
/*		Sets perimeter type to one of four enumerated CGI types	    */
/****************************************************************************/

Cerror          _cgi_perimeter_type(att, ttyp)
Outatt         *att;
Clintype        ttyp;		/* style of perimeter */

{
    Cerror          err = NO_ERROR;

    if (att->asfs[10] == INDIVIDUAL)
    {
	if (((int) ttyp < (int) SOLID) || ((int) ttyp > (int) LONG_DASHED))
	    ttyp = SOLID;
	att->fill.pstyle = (Clintype) ttyp;
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: perimeter_width                                       	    */
/*                                                                          */
/*		Sets perimeter width 					    */
/****************************************************************************/

Cerror          perimeter_width(width)
Cfloat          width;		/* perimeter width */
{
    int             ivw;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_check_perimeter_width(_cgi_att, width);
    if (!err)
    {
	_cgi_att->fill.pwidth = width;
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	    _cgi_set_conv_perimeter_width(_cgi_vws);
    }

    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_perimeter_width                                 	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_perimeter_width(desc, width)
Ccgiwin        *desc;
Cfloat          width;		/* perimeter width */
{
    Cerror          err = NO_ERROR;

    SETUP_CGIWIN(desc);
    if (!err)
	err = _cgi_check_perimeter_width(desc->vws->att, width);
    if (!err)
    {
	desc->vws->att->fill.pwidth = width;
	_cgi_set_conv_perimeter_width(desc->vws);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_perimeter_width				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_check_perimeter_width(att, width)
Outatt         *att;
Cfloat          width;		/* perimeter width */
{
    if (att->asfs[11] != INDIVIDUAL)
	return (EBTBUNDL);
    else if (width < 0)
	return (EBDWIDTH);	/* width must be positive */
    else
	return (NO_ERROR);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_set_conv_perimeter_width				    */
/*                                                                          */
/*   Created 851009 (wds) to organize setting the current fill.pwidth       */
/****************************************************************************/
_cgi_set_conv_perimeter_width(vws)
View_surface   *vws;
{
    Outatt         *attP = vws->att;

    if (attP->perimeter_spec_mode == SCALED)
    {
	register float  wtemp;	/* temporary result */

	/* wds 850617: viewport coords are "VDC" for pixwin surfaces */
	if (attP->vdc.use_pw_size)
	    wtemp = vws->vport.r_width;
	else
	    wtemp = abs(attP->vdc.rect.r_width);
	_cgi_devscalen((int) (wtemp * (attP->fill.pwidth / 100.) + 0.5), wtemp);
	if (wtemp < 1)
	    wtemp = 1;
	vws->conv.perimeter_width = wtemp;
    }
    else
    {
	vws->conv.perimeter_width = attP->fill.pwidth;
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: perimeter_color                                       	    */
/*                                                                          */
/*		Sets perimeter color 					    */
/****************************************************************************/

Cerror          perimeter_color(index)
Cint            index;		/* perimeter color */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_perimeter_color(_cgi_att, index);
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_perimeter_color                                      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          cgipw_perimeter_color(desc, index)
Ccgiwin        *desc;
Cint            index;		/* perimeter color */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_perimeter_color(desc->vws->att, index);
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_perimeter_color                                      */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

Cerror          _cgi_perimeter_color(att, index)
Outatt         *att;
Cint            index;		/* perimeter color */
{
    Cerror          err = NO_ERROR;

    if (att->asfs[12] == INDIVIDUAL)
    {
	err = _cgi_check_color(index);
	if (!err)
	{
	    att->fill.pcolor = index;
	}
    }
    else
	err = EBTBUNDL;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: perimeter_width_specification_mode	 		    */
/*                                                                          */
/*		sets perimeter_width_specification_mode			    */
/****************************************************************************/

Cerror          perimeter_width_specification_mode(mode)
Cspecmode       mode;		/* pixels or percent */
{
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
    {
	if (mode != ABSOLUTE)
	    mode = SCALED;
	_cgi_att->perimeter_spec_mode = mode;
    }
    return (_cgi_errhand(err));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_perimeter_width_specification_mode 		    */
/*                                                                          */
/*		sets perimeter_width_specification_mode			    */
/****************************************************************************/

Cerror          cgipw_perimeter_width_specification_mode(desc, mode)
Ccgiwin        *desc;
Cspecmode       mode;		/* pixels or percent */
{
    SETUP_CGIWIN(desc);
    if (mode != ABSOLUTE)
	mode = SCALED;
    desc->vws->att->perimeter_spec_mode = mode;
    return (NO_ERROR);
}
