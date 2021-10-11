#ifndef lint
static char	sccsid[] = "@(#)errlst.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Error handling functions
 */
/*
char *errarr[]
_cgi_errhand 
_cgi_print_error 
_cgi_legal_winname 
_cgi_check_state_5 
_cgi_err_check_4 
_cgi_err_check_480 
_cgi_err_check_481
_cgi_err_check_69
_cgi_check_color 
*/

#include "cgipriv.h"

Gstate _cgi_state;		/* CGI global state */
View_surface   *_cgi_vws;	/* current view surface */
Cerrtype _cgi_error_mode;	/* error trap mode */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];

extern char    *sprintf();

static char    *errarr[] =
{
 "1 ENOTCGCL - CGI not in proper state: CGI should be in state CGCL.",
 "2 ENOTCGOP - CGI not in proper state: CGI should be in state CGOP.",
 "3 ENOTVSOP - CGI not in proper state: CGI should be in state VSOP.",
 "4 ENOTVSAC - CGI not in proper state: CGI should be in state VSAC.",
 "5 ENOTOPOP - CGI not in proper state CGI should be in either in state CGOP, VSOP, or in the state VSAC.",
 "10 EVSIDINV - Specified view surface name is invalid.",
 "11 ENOWSTYP - Specified view surface type does not exist.",
 "12 EMAXVSOP - Maximum number of view surfaces already open.",
 "13 EVSNOTOP - Specified view surface not open.",
 "14 EVSISACT - Specified view surface is active.",
 "15 EVSNTACT - Specified view surface is not active.",
 "16 EINQALTL - Inquiry arguments are longer than list.",
 "20 EBADRCTD - Rectangle definition is invalid.",
 "21 EBDVIEWP - Viewport is not within Device Coordinates.",
 "22 ECLIPTOL - Clipping rectangle is too large.",
 "23 ECLIPTOS - Clipping rectangle is too small.",
 "24 EVDCSDIL - VDC space definition is illegal.",
 "30 EBTBUNDL - ASF is BUNDLED.",
 "31 EBBDTBDI - Bundle table index out of range.",
 "32 EBTUNDEF - Bundle table index is undefined.",
 "33 EBADLINX - Polyline index is invalid.",
 "34 EBDWIDTH - Width must be nonnegative.",
 "35 ECINDXLZ - Color index is less than zero.",
 "36 EBADCOLX - Color index is invalid.",
 "37 EBADMRKX - Polymarker index is invalid.",
 "38 EBADSIZE - Size must be nonnegative.",
 "39 EBADFABX - Fill area index is invalid.",
 "40 EPATARTL - Pattern array too large.",
 "41 EPATSZTS - Pattern size too small.",
 "42 ESTYLLEZ - Style (pattern or hatch) index is less than zero.",
 "43 ENOPATNX - Pattern table index not defined.",
 "44 EPATITOL - Pattern table index too large.",
 "45 EBADTXTX - Text index is invalid.",
 "46 EBDCHRIX - Character index is undefined.",
 "47 ETXTFLIN - Text font is invalid.",
 "48 ECEXFOOR - Expansion factor is out of range.",
 "49 ECHHTLEZ - Character height is less than or equal to zero.",
 "50 ECHRUPVZ - Length of character up vector or character base vector is zero.",
 "51 ECOLRNGE - RGB values must be between 0 and 255.",
 "60 ENMPTSTL - Number of points is too large.",
 "61 EPLMTWPT - polylines must have at least two points.",
 "62 EPLMTHPT - Polygons must have at least three points.",
 "63 EGPLISFL - Global polygon list is full.",
 "64 EARCPNCI - Arc points do not lie on circle.",
 "65 EARCPNEL - Arc points do not lie on ellipse.",
 "66 ECELLATS - Cell array dimensions dx,dy are too small.",
 "67 ECELLPOS - Cell array dimensions must be positive.",
 "68 ECELLTLS - Cell array too large for screen.",
 "69 EVALOVWS - Value outside of view surface.",
 "70 EPXNOTCR - Pixrect not created.",
 "80 EINDNOEX - Input device does not exist.",
 "81 EINDINIT - Input device not initialized.",
 "82 EINDALIN - Input device already initialized.",
 "83 EINASAEX - Association already exists.",
 "84 EINAIIMP - Association is impossible.",
 "85 EINNTASD - No triggers associated with device.",
 "86 EINTRNEX - Trigger does not exist.",
 "87 EINNECHO - Input device does not echo.",
 "88 EINECHON - Echo already on.",
 "89 EINEINCP - Echo incompatible with existing echos.",
 "90 EINERVWS - Echoregion larger than view surface.",
 "91 EINETNSU - Echo type not supported.",
 "92 EINENOTO - Echo not on.",
 "93 EIAEVNEN - Events already enabled.",
 "94 EINEVNEN - Events not enabled.",
 "95 EBADDATA - Contents of input data record are invalid.",
 "96 ESTRSIZE - Length of initial string us greater than the implementation defined maximum.",
 "97 EINQOVFL - Input queue has overflowed.",
 "98 EINNTRQE - Input device not in state REQUEST EVENTS.",
 "99 EINNTRSE - Input device not in state RESPOND EVENTS.",
 /* wds 851008: I believe error 99 is no longer used */
 "100 EINNTQUE - Input device not in state EVENT QUEUE.",
 "110 EMEMSPAC - Space allocation has failed.",
 "111 ENOTCSTD - Function or argument not compatible with standard CGI.",
 "112 ENOTCCPW - Function or argument not compatible with CGIPW mode.",
 "113 EFILACC - Unable to access file.",
 "114 ECGIWIN - Ccgiwin descriptor is invalid.",
};
#define	ERR_COUNT		( sizeof(errarr) / sizeof(errarr[0]) )

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_errhand						    */
/*                                                                          */
/*		Implements the CGI error handling			    */
/****************************************************************************/
int             _cgi_errhand(error)
int             error;
{
/*     int     fatal; */
    if (error)
    {				/* don't process error unless there is one */
	switch (_cgi_error_mode)
	{			/* error mode dictates action */
	case INTERRUPT:
	    {
/* 		    fatal = _cgi_fatal_error_check (error); */
		/* check if interrupt */
		_cgi_print_error(error);	/* print error */
/* 		    if (fatal) exit (error); */
		/* stop program on fatal error */
		break;
	    }
	case NO_ACTION:
	    {			/* don't do a thing */
		break;
	    }
	case POLL:
	    {
		_cgi_print_error(error);	/* print error */
		break;
	    }
	}
    }
    return (error);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_print_error	 				    */
/*                                                                          */
/*		Print the error message on standard output		    */
/****************************************************************************/

int             _cgi_print_error(errnum)
int             errnum;
{
    int             add;
    /* turn error number into array index */
    if (errnum < 6)
	add = errnum - 1;
    else if (errnum < 17)
	add = errnum - 5;
    else if (errnum < 25)
	add = errnum - 8;
    else if (errnum < 52)
	add = errnum - 13;
    else if (errnum < 71)
	add = errnum - 21;
    else if (errnum < 101)
	add = errnum - 30;
    else
	add = errnum - 39;

    if (add < ERR_COUNT)
	(void) printf("\007 %s\n", errarr[add]);
    else
	(void) printf("\007 %d - [error code and message unavailable]\n",
		      errnum);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_perror						    */
/*                                                                          */
/*		Front-end to perror() to respect client's error		    */
/*		handling mode						    */
/****************************************************************************/
_cgi_perror(msg)
char           *msg;
{
    char            msgbuf[256];

    (void) sprintf(msgbuf, "CGI: %s", msg);
    switch (_cgi_error_mode)
    {				/* error mode dictates action */
    case INTERRUPT:
	{
	    perror(msgbuf);
	    break;
	}
    case NO_ACTION:
	{			/* don't do a thing */
	    break;
	}
    case POLL:
	{
	    perror(msgbuf);
	    break;
	}
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_legal_winname					    */
/*                                                                          */
/*               determines if a view surface id is legal                   */
/****************************************************************************/
int             _cgi_legal_winname(num)
int             num;
{
    int             err = NO_ERROR;

    if ((num >= 0) && (num < MAXVWS))	/* wds: ...<= MAXVWS)) before 850822 */
    {
	if (_cgi_view_surfaces[num] == 0)
	    err = EVSNOTOP;
    }
    else
	err = EVSIDINV;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_state_5					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_check_state_5()
{
    int             err = ENOTOPOP;

    if (_cgi_state.state != CGCL)
	err = NO_ERROR;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_4				    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_4()
{
    int             err = NO_ERROR;

    if (_cgi_state.state != VSAC)
	err = ENOTVSAC;
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_480				    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_480(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    int             err = NO_ERROR;

    err = _cgi_err_check_4();
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (devnum < 1 || devnum > _CGI_KEYBORDS)
		err = EINDNOEX;
	    break;
	case IC_LOCATOR:
	    if (devnum < 1 || devnum > _CGI_LOCATORS)
		err = EINDNOEX;
	    break;
	case IC_CHOICE:
	    if (devnum < 1 || devnum > _CGI_CHOICES)
		err = EINDNOEX;
	    break;
	case IC_VALUATOR:
	    if (devnum < 1 || devnum > _CGI_VALUATRS)
		err = EINDNOEX;
	    break;
	case IC_STROKE:
	    if (devnum < 1 || devnum > _CGI_STROKES)
		err = EINDNOEX;
	    break;
	case IC_PICK:
	    if (devnum < 1 || devnum > _CGI_PICKS)
		err = EINDNOEX;
	    break;
	default:
	    err = EINDNOEX;
	    break;
	}
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_481				    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_481(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    int             err = NO_ERROR;

    err = _cgi_err_check_480(devclass, devnum);
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (_cgi_keybord[devnum - 1])
	    {
		if (_cgi_keybord[devnum - 1]->subkey.enable == RELEASE)
		    err = EINDINIT;
	    }
	    break;
	case IC_LOCATOR:
	    if (_cgi_locator[devnum - 1])
	    {
		if (_cgi_locator[devnum - 1]->subloc.enable == RELEASE)
		    err = EINDINIT;
	    }
	    break;
	case IC_CHOICE:
	    if (_cgi_button[devnum - 1])
	    {
		if (_cgi_button[devnum - 1]->subchoice.enable == RELEASE)
		    err = EINDINIT;
	    }
	    break;
	case IC_VALUATOR:
	    if (_cgi_valuatr[devnum - 1])
	    {
		if (_cgi_valuatr[devnum - 1]->subval.enable == RELEASE)
		    err = EINDINIT;
	    }
	    break;
	case IC_STROKE:
	    if (_cgi_stroker[devnum - 1])
	    {
		if (_cgi_stroker[devnum - 1]->substroke.enable == RELEASE)
		    err = EINDINIT;
	    }
	    break;
	}
    }
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_69				    	    */
/*                                                                          */
/*          If device point is outside (global) real screen size, return    */
/*          "69 EVALOVWS - Value outside of view surface."                  */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_69(x, y)
Cint            x, y;
{
    if ((x < _cgi_vws->real_screen.r_left + _cgi_vws->real_screen.r_width) &&
	(x >= _cgi_vws->real_screen.r_left) &&
	(y >= _cgi_vws->real_screen.r_top) &&
	(y < _cgi_vws->real_screen.r_top + _cgi_vws->real_screen.r_height))
	return (NO_ERROR);
    else
	return (EVALOVWS);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_color				    	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_check_color(index)
int             index;
{				/* wds 850621: acceptible index is now [0, ...
				 * , MAXCOLOR] (inclusive). */
    int             err = NO_ERROR;

    if (index < 0)
	err = ECINDXLZ;
    else if (index > MAXCOLOR)
	err = EBADCOLX;

    return (err);
}
