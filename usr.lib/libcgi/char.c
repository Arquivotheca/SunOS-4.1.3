#ifndef lint
static char	sccsid[] = "@(#)char.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Text functions
 */

/*
text
cgipw_text
append_text
cgipw_append_text
inquire_text_extent
cgipw_inquire_text_extent
_cgi_inquire_text_extent
vdm_text
cgipw_vdm_text
*/

#include "cgipriv.h"

Gstate          _cgi_state;	/* CGI global state */
View_surface   *_cgi_vws;	/* current view surface information */
Outatt         *_cgi_att;	/* structure containing current attributes */
Cerror          _cgi_append_to_saved_text();
Ccoor           _cgi_00;	/* coordinate pair (0,0) */
extern char    *strcat(), *strcpy();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: text	 					    	    */
/*                                                                          */
/*	        Writes text at postion c1				    */
/****************************************************************************/
Cerror          text(c1, tstring)
Ccoor          *c1;		/* starting point of text (in VDC Space) */
Cchar          *tstring;	/* text */
{
    int             ivw;
    Cerror          err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_inner_text(_cgi_vws, c1, tstring, (Ccoor *) 0);
	}
	if (_cgi_att->text.astring != (char *) 0)
	    *_cgi_att->text.astring = 0;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_text 					    	    */
/*                                                                          */
/*	This function renders text on a given view surface.		    */
/*	The actual rendering is done by the function _cgi_inner_text	    */
/*	which is in the file charsubs.c					    */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_text(desc, c1, tstring)
Ccgiwin        *desc;
Ccoor          *c1;		/* starting point of text (in VDC Space) */
Cchar          *tstring;	/* text */
{
    Cerror          err;

    SETUP_CGIWIN(desc);
    err = _cgi_inner_text(desc->vws, c1, tstring, (Ccoor *) 0);
    if (desc->vws->att->text.astring != (char *) 0)
	*desc->vws->att->text.astring = 0;
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: append_text	 					    */
/*                                                                          */
/*		appends text to most recently appended text		    */
/****************************************************************************/
Cerror          append_text(flag, tstring)
Cchar          *tstring;	/* text */
Ctextfinal      flag;		/* final text for alignment */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	err = _cgi_append_to_saved_text(_cgi_state.common_att, tstring);
	if (!err && flag == FINAL)
	{
	    ivw = 0;
	    while (_cgi_bump_vws(&ivw))
		err = _cgi_inner_text(_cgi_vws, &_cgi_att->text.concat_pt,
				      _cgi_att->text.astring, (Ccoor *) 0);
	    *_cgi_att->text.astring = 0;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_append_text 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_append_text(desc, flag, tstring)
Ccgiwin        *desc;
Cchar          *tstring;	/* text */
Ctextfinal      flag;		/* final text for alignment */
{
    Cerror          err = NO_ERROR;

    SETUP_CGIWIN(desc);
    err = _cgi_append_to_saved_text(desc->vws->att, tstring);

    /* draw text */
    if (!err && flag == FINAL)
    {
	err = _cgi_inner_text(desc->vws, &desc->vws->att->text.concat_pt,
			      desc->vws->att->text.astring, (Ccoor *) 0);
	*desc->vws->att->text.astring = 0;
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: inquire_text_extent	 				    */
/*                                                                          */
/*		 inquire_text_extent					    */
/****************************************************************************/
/* ARGSUSED WCL: nextchar unused */
Cerror          inquire_text_extent(tstring, nextchar, concat, lleft, uleft, uright)
Cchar          *tstring;	/* text */
Cchar           nextchar;	/* exact place of last character */
Ccoor          *concat;		/* concatentation point */
Ccoor          *lleft, *uleft, *uright;
{
    int             err, ivw;

    err = _cgi_err_check_4();
    if (!err)
    {
	/*
	 * Look for the first active view surface. This "can't fail", since
	 * VSAC is checked above.
	 */
	ivw = 0;
	if (_cgi_bump_vws(&ivw))
	{
	    err = _cgi_inquire_text_extent
		(_cgi_vws, tstring, concat, lleft, uleft, uright);
	}
	else
	    err = ENOTVSAC;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_inquire_text_extent 				    */
/*                                                                          */
/*		 cgipw_inquire_text_extent				    */
/****************************************************************************/
/* ARGSUSED WCL: nextchar unused */
Cerror          cgipw_inquire_text_extent
                (desc, tstring, nextchar, concat, lleft, uleft, uright)
Ccgiwin        *desc;
Cchar          *tstring;	/* text */
Cchar           nextchar;	/* which character would be next */
/* Could proportionally-space or kern based on next char.  Unused for now. */
Ccoor          *concat;		/* concatentation point */
Ccoor          *lleft, *uleft, *uright;	/* coordinates of text bounding box */
{
    SETUP_CGIWIN(desc);
    return (_cgi_inquire_text_extent(desc->vws, tstring,
				     concat, lleft, uleft, uright));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_inquire_text_extent 				    */
/*                                                                          */
/*		 _cgi_inquire_text_extent				    */
/****************************************************************************/
Cerror          _cgi_inquire_text_extent
                (vws, tstring, concat, lleft, uleft, uright)
View_surface   *vws;
Cchar          *tstring;	/* text */
/* Could proportionally-space or kern based on next char.  Unused for now. */
Ccoor          *concat;		/* concatentation point */
Ccoor          *lleft, *uleft, *uright;	/* coordinates of text bounding box */
{
    Ccoor           saved_concat;
    Ccoor           extent[3];	/* for _cgi_inner_text to return corners */
    Cerror          err;

    saved_concat = vws->att->text.concat_pt;

    /*
     * _cgi_inner_text maintains oldx, oldy in VDC, and returns extent in VDC.
     * Since cgipw_ functions really use VDC (which is forced to be 1::1 with
     * pixels), we just return the VDC values.
     */
    err = _cgi_inner_text(vws, &_cgi_00, tstring, extent);
    *lleft = extent[0];
    *uleft = extent[1];
    *uright = extent[2];
    *concat = vws->att->text.concat_pt;
    vws->att->text.concat_pt = saved_concat;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: vdm_text	 				    	    */
/*                                                                          */
/*	        Writes text at postion c1				    */
/****************************************************************************/
Cerror          vdm_text(c1, flag, tstring)
Ccoor          *c1;		/* starting point of text (in VDC Space) */
/* Should be a pointer, but the manual accidentally said it is a Ccoor */
Ctextfinal      flag;		/* final text for alignment */
Cchar          *tstring;	/* text */
{
    int             ivw, err;	/* error */

    err = _cgi_err_check_4();
    if (!err)
    {
	ivw = 0;
	while (_cgi_bump_vws(&ivw))
	    _cgi_att->text.concat_pt = *c1;

	/* append_text does _cgi_errhand if it generates errors */
	return (append_text(flag, tstring));
    }
    else
	return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cgipw_vdm_text 				    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Cerror          cgipw_vdm_text(desc, c1, flag, tstring)
Ccgiwin        *desc;
Ccoor          *c1;		/* starting point of text (in VDC Space) */
/* Should be a pointer, but the manual accidentally said it is a Ccoor */
Ctextfinal      flag;		/* final text for alignment */
Cchar          *tstring;	/* text */

{
    Cerror          err;

    SETUP_CGIWIN(desc);
    desc->vws->att->text.concat_pt = *c1;
    err = cgipw_append_text(desc, flag, tstring);
    return (err);
}

static Cerror   _cgi_append_to_saved_text(att, tstring)
register Outatt *att;
Cchar          *tstring;
{
    if (att->text.astring != (char *) 0 && *att->text.astring != '\0')
    {
	att->text.astring = (char *) realloc(att->text.astring,
					 ((unsigned) strlen(att->text.astring)
					  + (unsigned) strlen(tstring) + 1)
					     * sizeof(char));
    }
    else
    {
	att->text.astring = (char *) malloc((unsigned) (strlen(tstring)
							* sizeof(char)));
	if (att->text.astring != (char *) 0)
	    *att->text.astring = 0;
    }
    if (att->text.astring == (char *) 0)
	return (EMEMSPAC);
    else
    {
	(void) strcat(att->text.astring, tstring);
	return (NO_ERROR);
    }
}
