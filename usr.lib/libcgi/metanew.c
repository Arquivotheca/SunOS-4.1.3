#ifndef lint
static char	sccsid[] = "@(#)metanew.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI viewsurface initialization & termination functions
 */

/*
open_vws
_cgi_new_vwsurf_name
_cgi_get_dev
_cgi_get_device_vwsurf
_cgi_initialize_gp1
_cgi_chkdev
_cgi_getfb
_cgi_shared_screen
_cgi_color_init
_cgi_get_window_vwsurf
chkscreen
blanket_a_window
open_gfx_window
fbtype_colorflag_check
_cgi_fb_colorflag_mismatch
open_vws_special
close_vws
_cgi_free_name
_cgi_bump_vws
_cgi_bump_all_vws
_cgi_context
*/

#include "cgipriv.h"
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sun/fbio.h>
#include <sun/gpio.h>
#include <sys/ioctl.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_rgb.h>

Gstate          _cgi_state;	/* CGI global state */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
View_surface   *_cgi_vws;	/* current view surface */
int             _cgi_vwsurf_count;	/* num. of view surfaces ever opened */
int             _cgi_gfxenvfd = -1;	/* to be fd of WINDOW_GFX envir. var. */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_retained;	/* Bears user request to SIGWINCH handler */
int             _cgi_criticalcnt;	/* critical region protection */
Notify_value    _cgi_sigchild();

extern char   **environ;
extern int      errno;
extern char    *getenv();
int             (*win_errorhandler()) ();
extern char    *strcpy(), *strncpy();
extern char    *rindex(), *sprintf();

Outatt         *_cgi_alloc_output_att();
int             _cgi_shared_screen();
int             _cgi_shared_screen_state;
static char    *srchptr;
static struct screen *scrptr;
static int      foundscreen;

typedef enum
{
    ANY = PIXWINDD, MONO = BWPIXWINDD, COLOR = CGPIXWINDD,
}               colorflag_enum;

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_vws	 					    */
/*                                                                          */
/*               Opens and activiates a view surface. 			    */
/*		 Pertainent view surface information is stored		    */
/*		 and the view surface is added to the list of view surfaces.*/
/****************************************************************************/
Cerror          open_vws(name, devdd)
Cint           *name;		/* internal name */
Cvwsurf        *devdd;		/* view surface descriptor */
{
    register View_surface *cur_vws = NULL;
    int             err = NO_ERROR;
    int             new_name;

    START_CRITICAL();

    if (_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = ENOTCCPW;		/* Function not compatible with CGIPW mode */
    else
    {
	err = _cgi_check_state_5();
	if (!err)
	{
	    new_name = _cgi_new_vwsurf_name();
	    if (new_name < 0)
		err = EMAXVSOP;	/* Max no. of view surfaces already open. */
	    else
	    {
		/* Don't malloc unless everything else is OK */
		cur_vws = (View_surface *) calloc(1, sizeof(View_surface));
		if (cur_vws == (View_surface *) NULL)
		    err = EMEMSPAC;
	    }
	}
    }

    if (!err)
    {
	cur_vws->att = _cgi_att = _cgi_state.common_att;
	_cgi_vws = _cgi_view_surfaces[new_name] = cur_vws;
	_cgi_retained = (devdd->retained);
	err = _cgi_get_dev(devdd);	/* get actual device */
	(void) fcntl(cur_vws->sunview.windowfd, F_SETFL, FASYNC | FNDELAY);
    }
    if (!err)
    {
	Cos             incoming_cgi_state = _cgi_state.state;

	_cgi_state.state = VSAC;
	cur_vws->active = 1;
	/*
	 * if first view surface opened, set defaults, initialize input stats,
	 * and clear screen.  Hard_reset does _cgi_windowset via vdc_extent.
	 */
	if (incoming_cgi_state == CGOP)
	    (void) hard_reset();
	else
	    (void) _cgi_windowset(cur_vws);
	*name = new_name;
    }
    else
    {
	if (new_name >= 0)
	{
	    /* Make surface as if it didn't exist */
	    _cgi_free_name(new_name);
	}
	_cgi_vws = (View_surface *) NULL;
    }

    END_CRITICAL();
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_new_vwsurf_name					    */
/*                                                                          */
/*		Returns available view surface "name" (or -1 failure).	    */
/*		A "name" is an index into the _cgi_view_surfaces array.	    */
/*		see also _cgi_free_name, which frees up the "name".	    */
/****************************************************************************/
int             _cgi_new_vwsurf_name()
{
    int             i, index = -1;

    if (_cgi_vwsurf_count < MAXVWS)	/* OK to give out MAXVWS-1 */
	index = _cgi_vwsurf_count++;	/* Now count may be MAXVWS. */
    else
    {
	for (i = 0; i < MAXVWS; i++)
	{
	    if (!_cgi_view_surfaces[i])	/* Reuse this closed vws */
	    {
		index = i;
		break;
	    }
	}
    }

    return (index);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_get_dev						    */
/*                                                                          */
/*		sets the current device 				    */
/****************************************************************************/
static int      _cgi_get_dev(devdd)
Cvwsurf        *devdd;		/* view surface descriptor */
{
    register View_surface *cur_vws = _cgi_vws;
    int             err = NO_ERROR;
    int             gp_flag;	/* returned gp1 existence proof */
    int             dd = devdd->dd;

    /* static int  bcolor = 0; */
    /* set device internals */
    cur_vws->sunview.pw = (Pixwin *) NULL;

    switch (devdd->dd)
    {
    case BW1DD:
	{
	    static char     bwonedev[] = "/dev/bwone?";
	    char            devstring[sizeof(bwonedev)];

	    (void) strcpy(devstring, bwonedev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN1BW, &gp_flag);
	    cur_vws->sunview.depth = 1;
	    break;
	}
    case BW2DD:
	{
	    static char     bwtwodev[] = "/dev/bwtwo?";
	    char            devstring[sizeof(bwtwodev)];

	    (void) strcpy(devstring, bwtwodev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN2BW, &gp_flag);
	    cur_vws->sunview.depth = 1;
	    break;
	}
    case CG1DD:
	{
	    static char     cgonedev[] = "/dev/cgone?";
	    char            devstring[sizeof(cgonedev)];

	    (void) strcpy(devstring, cgonedev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN1COLOR, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
    case CG2DD:
	{
	    static char     cgtwodev[] = "/dev/cgtwo?";
	    char            devstring[sizeof(cgtwodev)];

	    (void) strcpy(devstring, cgtwodev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN2COLOR, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
    case CG3DD:
	{
	    static char     cgthreedev[] = "/dev/cgthree?";
	    char            devstring[sizeof(cgthreedev)];

	    (void) strcpy(devstring, cgthreedev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN3COLOR, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
    case CG4DD:
	{
	    static char     cgfourdev[] = "/dev/cgfour?";
	    char            devstring[sizeof(cgfourdev)];

	    (void) strcpy(devstring, cgfourdev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN4COLOR, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
#ifndef	NO_CG6
    case CG6DD:
	{
	    static char     cgsixdev[] = "/dev/cgsix?";
	    char            devstring[sizeof(cgsixdev)];

	    (void) strcpy(devstring, cgsixdev);
	    err = _cgi_get_device_vwsurf(devstring, devdd,
 	    	FBTYPE_SUNFAST_COLOR, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
/* This define is used later in this file */
#define	FBTYPE_IS_COLOR(fbtype)	(  ((fbtype) == FBTYPE_SUN1COLOR)	\
				|| ((fbtype) == FBTYPE_SUN2COLOR)	\
				|| ((fbtype) == FBTYPE_SUN3COLOR)	\
				|| ((fbtype) == FBTYPE_SUN4COLOR)	\
				|| ((fbtype) == FBTYPE_SUNFAST_COLOR)	\
				|| ((fbtype) == FBTYPE_SUN2GP)		)
#else	NO_CG6
/* This define is used later in this file */
#define	FBTYPE_IS_COLOR(fbtype)	(  ((fbtype) == FBTYPE_SUN1COLOR)	\
				|| ((fbtype) == FBTYPE_SUN2COLOR)	\
				|| ((fbtype) == FBTYPE_SUN3COLOR)	\
				|| ((fbtype) == FBTYPE_SUN4COLOR)	\
				|| ((fbtype) == FBTYPE_SUN2GP)		)
#endif	NO_CG6
    case GP1DD:
	{
	    static char     gponedev[] = "/dev/gpone?@";	/* e.g., /dev/gpone0a */
	    char            devstring[sizeof(gponedev)];

	    (void) strcpy(devstring, gponedev);
	    err = _cgi_get_device_vwsurf(devstring, devdd, FBTYPE_SUN2GP, &gp_flag);
	    cur_vws->sunview.depth = 8;
	    break;
	}
    case BWPIXWINDD:
	{
	    colorflag_enum  colorflg = MONO;

	    err = _cgi_get_window_vwsurf(&cur_vws->sunview.windowfd,
					 &cur_vws->sunview.tool_pid,
					 &colorflg, devdd, &gp_flag);
	    if (colorflg == MONO)
		cur_vws->sunview.depth = 1;
	    break;
	}
    case CGPIXWINDD:
	{
	    colorflag_enum  colorflg = COLOR;

	    cur_vws->sunview.depth = 8;
	    err = _cgi_get_window_vwsurf(&cur_vws->sunview.windowfd,
					 &cur_vws->sunview.tool_pid,
					 &colorflg, devdd, &gp_flag);
	    if (colorflg == COLOR)
		cur_vws->sunview.depth = 8;
	    break;
	}
    default:
	{
	    if ((devdd->windowname != '\0') || (devdd->screenname != '\0'))
	    {
		dd = PIXWINDD;
		goto case_PIXWINDD;
	    }
	    else
		err = ENOWSTYP;
	    break;
	}
    case PIXWINDD:
case_PIXWINDD:
	{
	    colorflag_enum  colorflg = ANY;

	    err = _cgi_get_window_vwsurf(&cur_vws->sunview.windowfd,
					 &cur_vws->sunview.tool_pid,
					 &colorflg, devdd, &gp_flag);
	    if (colorflg == MONO)
		cur_vws->sunview.depth = 1;
	    else
		cur_vws->sunview.depth = 8;
	    break;
	}
    }
    if (!err)
    {
	if (cur_vws->sunview.pw)
	{
	    pw_writebackground(cur_vws->sunview.pw, 0, 0,
			       cur_vws->vport.r_width,
			       cur_vws->vport.r_height,
			       PIX_COLOR(0) | PIX_SRC);
	}
	else
	    err = ENOWSTYP;
    }
    /*
     * Duplicate cases of gp init from _cgi_get_device_vwsurf and
     * _cgi_get_window_vwsurf were consolidated here. wcl860919
     */
    if (!err)
    {
	if (gp_flag)
	{
	    /* wds: better than no test at all */
	    if (_cgi_initialize_gp1(cur_vws) != 0)
		cur_vws->sunview.gp_att = (Gp1_attr *) NULL;
	}
	cur_vws->device = dd;
    }
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_get_device_vwsurf					    */
/*                                                                          */
/*		Opens a view surface on a "raw device".			    */
/*		Tries (in order) user's screenname (if any),		    */
/*		/dev/fb, devstring (iterated over possible values).	    */
/*		Will not run on a raw device already running windows.	    */
/*		Should allow some value of fbtype to mean "use devstring".  */
/****************************************************************************/
static int      _cgi_get_device_vwsurf(devstring, vsurfp, fbtype, gpflg)
char           *devstring;
Cvwsurf        *vsurfp;
int             fbtype;
int            *gpflg;
{
    struct screen   screen;
    int             oldsigmask;
    int             err = NO_ERROR;
    static char     default_fb_string[] = "/dev/fb";
    static char     none_string[] = "NONE";

    win_initscreenfromargv(&screen, (char **) NULL);	/* get default screen
							 * struct */
    if (*vsurfp->screenname != '\0')
    {
	if ((_cgi_chkdev(vsurfp->screenname, fbtype)) >= 0)
	    (void) strncpy(screen.scr_fbname, vsurfp->screenname, SCR_NAMESIZE);
	else
	    err = ENOWSTYP;
    }
    else
    {
	if ((_cgi_chkdev(default_fb_string, fbtype)) >= 0)
	    (void) strncpy(screen.scr_fbname, default_fb_string, SCR_NAMESIZE);
	else if ((_cgi_getfb(devstring, fbtype)) >= 0)
	    (void) strncpy(screen.scr_fbname, devstring, SCR_NAMESIZE);
	else
	    err = ENOWSTYP;
    }


    if (getenv("WINDOW_ME"))
    {
	(void) strncpy(screen.scr_kbdname, none_string, SCR_NAMESIZE);
	/* Related to core's _core_setadjacent for moving mouse among screens */
	_cgi_vws->sunview.windowfd = 99;	/* Need to fool test in
						 * _cgi_new_window_init? */
	oldsigmask = sigblock((1 << (SIGWINCH - 1)) | (1 << (SIGCHLD - 1)));
	if ((_cgi_vws->sunview.windowfd = win_screennew(&screen)) < 0)
	    err = ENOWSTYP;
	else
	    win_screenget(_cgi_vws->sunview.windowfd, &screen);
	(void) sigsetmask(oldsigmask);
    }
    else			/* Really shouldn't be "else": should always
				 * test the device? */
    {
	/* Refuse to use a device that is already running windows. */
	{
	    int             (*olderr) ();

	    _cgi_vws->sunview.windowfd = 99;	/* Need to fool test in
						 * _cgi_new_window_init? */
	    _cgi_shared_screen_state = 0;
	    (void) strncpy(screen.scr_kbdname, none_string, SCR_NAMESIZE);
/***	    (void) strncpy(screen.scr_msname, none_string, SCR_NAMESIZE); ***/
	    oldsigmask = sigblock((1 << (SIGWINCH - 1)) | (1 << (SIGCHLD - 1)));
	    olderr = win_errorhandler(_cgi_shared_screen);
	    /*
	     * Apparently, we catch errors so we can determine if window was
	     * shared, and to avoid "an error message is displayed to indicate
	     * ... problem"
	     */
	    if ((_cgi_vws->sunview.windowfd = win_screennew(&screen)) >= 0)	/* Was < 0 */
	    {
		if (_cgi_shared_screen_state)
		    err = ENOWSTYP;
		else
		{		/* screen ok: not shared */
		    /*
		     * if _cgi_vws->sunview.windowfd above was < 0, the fd is
		     * no good for the win_screenget below
		     */
		    win_screenget(_cgi_vws->sunview.windowfd, &screen);
		    err = NO_ERROR;
		}
	    }
	    (void) win_errorhandler(olderr);
	    (void) sigsetmask(oldsigmask);
	}
    }

    if (!err)
    {
	/*
	 * _cgi_new_window_init sets _cgi_vws->sunview.pw from
	 * _cgi_vws->sunview.windowfd
	 */
	err = _cgi_new_window_init(_cgi_vws);
    }

    if (!err)
    {
	if (FBTYPE_IS_COLOR(fbtype))
	    _cgi_color_init(vsurfp);
	if (fbtype == FBTYPE_SUN2GP)
	    *gpflg = 1;
	else
	    *gpflg = 0;
    }
    return (err);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_initialize_gp1					    */
/*                                                                          */
/*		sets up Graphics Processor structures for CGI use	    */
/*		wds Pulled out of _cgi_get_device_vwsurf_gp1dd, 850611	    */
/*		Returns 0 if it succeeds, nonzero if fails.		    */
/****************************************************************************/
int             _cgi_initialize_gp1(vws)
View_surface   *vws;		/* view surface for output */
{
    int             planes;

    if ((vws == (View_surface *) NULL)
	|| vws->sunview.pw == (Pixwin *) NULL
	|| vws->sunview.gp_att == (Gp1_attr *) NULL
	|| _cgi_gp1_attr_init(vws->sunview.pw, &vws->sunview.gp_att) < 0)
	return 1;

    /* Note that the return code is not checked. */
    _cgi_update_gp1_xform(vws);
    pw_getattributes(vws->sunview.pw, &planes);
    _cgi_gp1_set_pixplanes(planes, vws->sunview.gp_att);
    if (ioctl(((struct gp1pr *) (vws->sunview.pw->pw_pixrect->pr_data))->ioctl_fd,
	      GP1IO_GET_RESTART_COUNT, (char *) &_cgi_state.gp1resetcnt))
    {
	_cgi_gp1_attr_close(vws->sunview.pw, &vws->sunview.gp_att);
	return 1;
    }
    else
    {
	vws->sunview.gp_att->resetcnt = _cgi_state.gp1resetcnt;
	/* wds 851009: Maybe _cgi_gp1_snd_attr should be called? */
	/* I think that we just delay sending the attrs until first primitive */
	return 0;
    }
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_chkdev	 					    */
/*                                                                          */
/*		Checks the device dev's frame buffer			    */
/*		After checking its type against the one input.		    */
/*		Return the fd for the device dev's frame buffer		    */
/*		After checking its type against the one input.		    */
/****************************************************************************/
static int      _cgi_chkdev(dev, type)
char           *dev;
int             type;
{
    int             fbfd, fbtype;

    if ((fbfd = open(dev, O_RDWR, 0)) < 0)
	return (-1);
    if ((fbtype = pr_getfbtype_from_fd(fbfd)) == -1 || (fbtype != type))
    {
	(void) close(fbfd);
	return (-1);
    }
    (void) close(fbfd);
    return (1);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_getfb		                                    */
/*                                                                          */
/*		Return the fd for the device with correct frame buffer type */
/*		Possible device names are formed from devstring:	    */
/*		Rightmost ? may be replaced by any digit 0-9. @ with a-d.   */
/*		If 1 is returned, devstring is the one that worked.	    */
/*		-1 is returned for failure & devstring has been changed.    */
/****************************************************************************/
static int      _cgi_getfb(devstring, type)
char           *devstring;
int             type;
{
    char           *zero_ptr, digit, *at_ptr, letter;

    zero_ptr = rindex(devstring, '?');
    at_ptr = rindex(devstring, '@');

    /* Pointer {zero,at}_ptr either position to iterate, or NULL pointer */
    digit = '0';
    do
    {
	if (zero_ptr)
	    *zero_ptr = digit;

	letter = 'a';
	do
	{
	    if (at_ptr)
		*at_ptr = letter;

	    if ((_cgi_chkdev(devstring, type)) >= 0)
		return (1);
	}
	while ((++letter <= 'd') && at_ptr);
	/* Once through is enough, if (at_ptr == NULL) */
    }
    while ((++digit <= '9') && zero_ptr);

    return (-1);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_shared_screen                                         */
/*                                                                          */
/*          if called, the screen is shared                                 */
/*                                                                          */
/****************************************************************************/
static int      _cgi_shared_screen()
{
    if (errno == EBUSY)
	_cgi_shared_screen_state = EBUSY;
    return (0);
}



/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_color_init 					    */
/*                                                                          */
/*		_cgi_color_init	initializes the colormap		    */
/****************************************************************************/
_cgi_color_init(vsurf)
Cvwsurf        *vsurf;
{
    register View_surface *vwsP = _cgi_vws;
    u_char          red[256], green[256], blue[256];
    int             cmapsize;
    char            cmsname[DEVNAMESIZE];
    struct colormapseg cms;
    struct cms_map  cmap;

    cmapsize = vsurf->cmapsize;
    if (cmapsize == 0)
	cmapsize = 2;		/* Use monochrome by default */
    if (vsurf->cmapname[0] == '\0')
    {
	/* Build unique color map segment name */
	(void) sprintf(cmsname, "cmap%10D%5D", getpid(), vwsP->sunview.windowfd);
	cmap.cm_red = red;
	cmap.cm_green = green;
	cmap.cm_blue = blue;
	cms.cms_size = CMS_RGBSIZE;
	cms_rgbsetup(cmap.cm_red, cmap.cm_green, cmap.cm_blue);
	cmapsize = cms.cms_size;
	pw_setcmsname(vwsP->sunview.pw, cmsname);
	pw_putcolormap(vwsP->sunview.pw, 0, cmapsize,
		       cmap.cm_red, cmap.cm_green, cmap.cm_blue);
	if (vwsP->sunview.orig_pw != vwsP->sunview.pw)
	{
	    pw_setcmsname(vwsP->sunview.orig_pw, cmsname);
	    pw_putcolormap(vwsP->sunview.orig_pw, 0, cmapsize,
			   cmap.cm_red, cmap.cm_green, cmap.cm_blue);
	}
    }
    else
    {
	(void) strncpy(cmsname, vsurf->cmapname, DEVNAMESIZE);	/* necessary? */
	if (_cgi_sharedcmap(vwsP->sunview.windowfd, cmsname, &cms, &cmap))
	{
	    pw_setcmsname(vwsP->sunview.pw, cmsname);
	    if (vwsP->sunview.orig_pw != vwsP->sunview.pw)
		pw_setcmsname(vwsP->sunview.orig_pw, cmsname);
	}
	else
	{
	    pw_setcmsname(vwsP->sunview.pw, cmsname);
/* 		cms_rgbsetup (cmap.cm_red, cmap.cm_green, cmap.cm_blue); */
	    /*
	     * wds 850626: Turned back on (was previously commented out) needed
	     * so upcoming pw_writebackground will refill the "screen" with
	     * color zero of new color map size, not merely default size. Note:
	     * color map is the correct size, but values may be trash,
	     * depending on whether this is the first window using the cmsname.
	     */
	    pw_putcolormap(vwsP->sunview.pw, 0, cmapsize, red, green, blue);
	    if (vwsP->sunview.orig_pw != vwsP->sunview.pw)
	    {
		pw_setcmsname(vwsP->sunview.orig_pw, cmsname);
		pw_putcolormap(vwsP->sunview.orig_pw,
			       0, cmapsize, red, green, blue);
	    }
	}
    }
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_get_window_vwsurf					    */
/*                                                                          */
/*		Opens a view surface in a window.			    */
/*		Tries (in order):                                           */
/*		    view_surface (tool) on user's screenname (if any),	    */
/*		    blanketing window* in environment variable WINDOW_GFX,  */
/*		    view_surface (tool) on same screen* as WINDOW_GFX,	    */
/*		Idea: Could also try a view_surface (tool) on /dev/fb*.	    */
/*                                                                          */
/*		* above only if colorflg matches color attribute of screen. */
/*                                                                          */
/****************************************************************************/
static int      _cgi_get_window_vwsurf(pw_fdp, pidp, colorflg, vsurfp, gpflg)
int            *pw_fdp, *pidp;
colorflag_enum *colorflg;
Cvwsurf        *vsurfp;
int            *gpflg;
{
    int             i1, i2;
    static char     name[DEVNAMESIZE];
    static char    *argvarray[] =
    {
	"view_surface", name, 0
    };

    static char     initstring[80] = "WINDOW_INITIALDATA=";
    static char    *envparray[] =
    {
	initstring, 0
    };
    char	  **childenvp, *pname;
    int             fd, pipefd[2];
    int             pid = -1;
    struct screen   screen;
    int             chkscreen();
    Cerror          err;

    /*
     * Old test: if ((_cgi_gfxenvfd == -1) && !(vsurfp->flags & VWSURF_NEWFLG)
     * && (*vsurfp->screenname == '\0'))
     */

    /* Set pw to 0 so we can tell later if somebody successfully opened it */
    _cgi_vws->sunview.pw = (Pixwin *) 0;

    /* User has demanded a "new" view surface, so fork 'em */
    if (vsurfp->flags & VWSURF_NEWFLG)	goto fork_vwsurftool;

    /* User has NOT demanded a "new" view surface */
    if (vsurfp->windowname[0] != '\0')
    {				/* Try to open user-specified windowname */
	if (we_getgfxwindow(name) == 0 && strcmp(name, vsurfp->windowname) == 0)
	{
	    if (_cgi_gfxenvfd != -1)
		return (ENOWSTYP);	/* WINDOW_GFX==windowname & is already
					 * in use */
	    else if ((fd = blanket_a_window(vsurfp->windowname, colorflg)) < 0)
		return (ENOWSTYP);
	    else
		_cgi_gfxenvfd = *pw_fdp = fd;	/* Give caller the fd to use */
	    /*
	     * _cgi_new_window_init call below sets _cgi_vws->sunview.pw from
	     * _cgi_vws->sunview.windowfd
	     */
	}
	else if ((*pw_fdp = blanket_a_window(vsurfp->windowname, colorflg)) < 0)
	    return (ENOWSTYP);
	pid = 0;		/* no view_surface "tool" forked, so there is
				 * no "tool" pid */
    }
    else if ((_cgi_gfxenvfd == -1) && (vsurfp->screenname[0] == '\0'))
    {				/* If user specified no screen, try WINDOW_GFX
				 * environment variable */
	if (we_getgfxwindow(name))
	    return (ENOWSTYP);	/* WINDOW_GFX environment value could not be
				 * found */
	if ((fd = blanket_a_window(name, colorflg)) < 0)
	    return (ENOWSTYP);
	else
	    _cgi_gfxenvfd = *pw_fdp = fd;	/* Let the caller know what fd
						 * to use */
	/*
	 * _cgi_new_window_init below sets _cgi_vws->sunview.pw from
	 * _cgi_vws->sunview.windowfd
	 */
	pid = 0;		/* no view_surface "tool" forked, so there is
				 * no "tool" pid */
    }
    else			/* we'll have to make a view_surface tool, but
				 * on what screen? */
    {
fork_vwsurftool:
	/* If the user is specifying what view surface he wants, try to find it */
	if (vsurfp->screenname[0] != '\0')
	{
	    foundscreen = FALSE;
	    srchptr = vsurfp->screenname;
	    scrptr = &screen;
	    win_enumall(chkscreen);
	    if (!foundscreen)
		return (ENOWSTYP);	/* No such screen running windows */
	    /* Core goes on to demand that the screen found matches "colorflg" */
	    if (*colorflg == ANY)
	    {
		/* set *colorflg from fbtype & set *gpflg if GP1. */
		if (fbtype_colorflag_check(screen.scr_fbname, colorflg, gpflg)
		    < 0)
		    return (ENOWSTYP);
	    }
	}
	else
	    /*
	     * Use the same screen as vsurfp->windowname, if the user gave us
	     * one. Otherwise, use the same screen as WINDOW_GFX. Core tries to
	     * reopen _cgi_gfxenvfd & check it against colorflg.
	     */
	{
	    /* open_gfx_window demands the correct colorflg. */
	    if ((fd = open_gfx_window(vsurfp->windowname, colorflg)) < 0)
		return (ENOWSTYP);	/* else try to open /dev/fb perhaps? */
	    /* _cgi_vws->sunview.windowfd = fd; necessary? */
	    /* _cgi_new_window_init should set _cgi_vws->sunview.pw */
	    win_screenget(fd, &screen);
	    (void) close(fd);
	}

	/* we know what screen, now we have to fork the view_surface tool */
	if (pipe(pipefd) < 0)
	    return (ENOWSTYP);
	/* (void) fprintf (stderr,"BEFORE errno=%d \n", errno); */
	errno = 0;
	if ((pid = vfork()) == 0)
	{			/* child process */
	    (void) strncpy(name, screen.scr_rootname, DEVNAMESIZE);
	    /* e.g., /dev/cgtwo0 */
	    if (vsurfp->ptr == 0)
		childenvp = environ;	/* core uses NULL envir. ptr. here */
	    else
	    {
		childenvp = envparray;
		(void) strncpy(initstring + 19, *vsurfp->ptr, 60);
		initstring[sizeof(initstring)-1] = '\0';
	    }
	    if (dup2(pipefd[1], 1) < 0)
		_exit(0);

	    (void) close(0);	/* assume fds 0,1,2 are left */
	    /* Other file descripters not needed by view_surface, so close them */
	    i2 = getdtablesize();
	    for (i1 = 3; i1 < i2; i1++)
		(void) fcntl(i1, F_SETFD, 1);	/* close on exec */

	    if (access((pname="/usr/bin/view_surface"), F_OK|X_OK))
		    pname ="/usr/lib/view_surface";
	    execve(pname, argvarray, childenvp);
	    _exit(0);		/* only return if unable to exec */
	}
	notify_set_wait3_func(_cgi_state.notifier_client_handle,
			      _cgi_sigchild, pid);

	(void) close(pipefd[1]);/* parent process */
	if (pid < 0)
	{
	    (void) close(pipefd[0]);
	    return (ENOWSTYP);
	}
	i1 = read(pipefd[0], name, DEVNAMESIZE);	/* e.g., /dev/win23 */
	(void) close(pipefd[0]);
	if ((i1 < DEVNAMESIZE) || ((fd = open(name, O_RDWR, 0)) < 0))
	{
	    (void) kill(pid, 9);
	    return (ENOWSTYP);
	}

	*pw_fdp = fd;
	_cgi_vws->sunview.windowfd = fd;
	win_setowner(fd, getpid());
    }				/* end making a view_surface tool */

    /*
     * _cgi_new_window_init sets _cgi_vws->sunview.pw from
     * _cgi_vws->sunview.windowfd
     */
    if ((err = _cgi_new_window_init(_cgi_vws)) != NO_ERROR)
	return (err);
    if (_cgi_vws->sunview.pw == (Pixwin *) 0)
    {
	if (pid > 0)
	    (void) kill(pid, 9);
	return (ENOWSTYP);
    }
    else
    {
	*pidp = pid;
    }

    /* Finish off window viewsurface initialization */
    if (*colorflg == COLOR)
    {
	_cgi_color_init(vsurfp);
    }
    return (NO_ERROR);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: chkscreen	 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
static int      chkscreen(windowfd)
int             windowfd;
{
    win_screenget(windowfd, scrptr);
    if (strcmp(srchptr, scrptr->scr_fbname) == 0)
    {
	foundscreen = TRUE;
	return (TRUE);
    }
    return (FALSE);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: blanket_a_window 					    */
/*                                                                          */
/*               Tries to open windowname given.			    */
/*               Checks frame buffer's capabilities against "colorflag".    */
/*               If colorflag == ANY, it is replaced with MONO or COLOR.    */
/*               Returns nonnegative open window fd if OK, -1 for failure.  */
/****************************************************************************/
static int      blanket_a_window(windowname, colorflg)
char           *windowname;
colorflag_enum *colorflg;
{
    int             parentfd, fd;

    if (!windowname || *windowname == '\0')
	return (-1);		/* WINDOW_GFX environment value could not be
				 * found */
    if ((parentfd = open_gfx_window(windowname, colorflg)) < 0)
	return (-1);
    if ((fd = win_getnewwindow()) < 0)	/* get another /dev/winXX */
    {
	(void) close(parentfd);
	return (-1);
    }
    _cgi_vws->sunview.windowfd = fd;
    /*
     * _cgi_new_window_init (called from this function's caller) sets
     * _cgi_vws->sunview.pw from _cgi_vws->sunview.windowfd
     */
    if (win_insertblanket(fd, parentfd))
    {
	(void) close(parentfd);
	(void) close(fd);
	return (-1);
    }
    (void) close(parentfd);
    return (fd);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_gfx_window 					    */
/*                                                                          */
/*               If windowname is given, try to open it, else WINDOW_GFX.   */
/*               Checks frame buffer's capabilities against "colorflag".    */
/*               If colorflag == ANY, it is replaced with MONO or COLOR.    */
/*               Returns nonnegative open window fd if OK, -1 for failure.  */
/****************************************************************************/
static int      open_gfx_window(windowname, colorflg)
char           *windowname;
colorflag_enum *colorflg;
{
    int             gwfd;
    char            name[DEVNAMESIZE];
    int             gp_flag;
    struct screen   screen;

    if (windowname && *windowname)
    {
    }				/* Then use windowname as is */
    else
    {
	if (we_getgfxwindow(windowname = name))
	    return (-1);	/* WINDOW_GFX environment value could not be
				 * found */
    }
    if ((gwfd = open(windowname, O_RDWR, 0)) < 0)	/* get window fd */
	return (-1);
    win_screenget(gwfd, &screen);	/* get screen structure */
    if (fbtype_colorflag_check(screen.scr_fbname, colorflg, &gp_flag) < 0)
    {
	(void) close(gwfd);
	return (-1);
    }
    return (gwfd);		/* return window fd */
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: fbtype_colorflag_check(screenname, colorflg)               */
/*                                                                          */
/*               Checks a frame buffer's inherent capabilities against	    */
/*               the input "colorflag" demands.				    */
/*               If colorflag == ANY, it is replaced with MONO or COLOR.    */
/*               If fbtype == FBTYPE_SUN2GP, sets global *gpflg.	    */
/*               Returns nonnegative fbtype code if OK, -1 for failure.	    */
/****************************************************************************/

/*
 * ## If this is called from both _cgi_get_window_vwsurf and
 * _cgi_get_device_vwsurf, then _cgi_get_dev also does the test and
 * sets *gpflg superfluously.
 */
static int      fbtype_colorflag_check(screenname, colorflg, gpflg)
char           *screenname;
colorflag_enum *colorflg;
int            *gpflg;
{
    int             fbtype, fbfd;

    if ((fbfd = open(screenname, O_RDWR, 0)) < 0)	/* get FB fd */
	return (-1);
    if ((fbtype = _cgi_fb_colorflag_mismatch(fbfd, colorflg)) < 0)
    {
	(void) close(fbfd);
	return (-1);
    }
    if (fbtype == FBTYPE_SUN2GP)
	*gpflg = 1;
    else
	*gpflg = 0;
    (void) close(fbfd);		/* close FB fd */
    return (fbtype);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_fb_colorflag_mismatch				    */
/*                                                                          */
/*               Checks a frame buffer's inherent capabilities against	    */
/*               the input "colorflag" demands.				    */
/*               If colorflag == ANY, it is replaced with MONO or COLOR.    */
/*               Returns nonnegative fbtype code if OK, -1 for failure.	    */
/****************************************************************************/
static int      _cgi_fb_colorflag_mismatch(fbfd, colorflg)
int             fbfd;
colorflag_enum *colorflg;
{
    int             fbtype;
    int             returnvalue = -1;	/* Assume the worst */

    if ((fbtype = pr_getfbtype_from_fd(fbfd)) == -1)
	return (-1);
    else
	switch (fbtype)
	{
	case FBTYPE_SUN1BW:
	case FBTYPE_SUN2BW:
	    switch (*colorflg)
	    {
	    default:
	    case ANY:
		*colorflg = MONO;
		/* continue thru next case */
	    case MONO:
		returnvalue = fbtype;
		break;
	    case COLOR:
		break;
	    }
	    break;
	case FBTYPE_SUN1COLOR:
	case FBTYPE_SUN2COLOR:
	case FBTYPE_SUN3COLOR:
	case FBTYPE_SUN4COLOR:
#ifndef	NO_CG6
	case FBTYPE_SUNFAST_COLOR:
#endif	NO_CG6
	case FBTYPE_SUN2GP:
	    switch (*colorflg)
	    {
	    case MONO:
		break;
	    default:
	    case ANY:
		*colorflg = COLOR;
		/* continue thru next case */
	    case COLOR:
		returnvalue = fbtype;
		break;
	    }
	    break;
	}
    return (returnvalue);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_vws_special 					    */
/*                                                                          */
/*               Opens, but does not activiate, a view surface. All 	    */
/*		 pertainent view surface information is stored and the view */
/*		 surface is added to the list of view surfaces 		    */
/****************************************************************************/

Cerror          open_vws_special(name, devdd, atarg)
Cint           *name;		/* internal name */
Cvwsurf        *devdd;		/* view surface descriptor */
Ccgiwin        *atarg;		/* returned descriptor of new view surface */
{
    register View_surface *cur_vws = 0;	/* ptr to current view surface info */
    register Outatt *cur_att = 0;	/* ptr to current attribute info */
    int             err;
    int             new_name;	/* newly allocated view surface */

    START_CRITICAL();

    /* Don't malloc unless everything else is OK */
    err = _cgi_check_state_5();
    if (!err)
    {
	new_name = _cgi_new_vwsurf_name();
	if (new_name < 0)
	    err = EMAXVSOP;	/* Maximum no. of view surfaces already open. */
    }
    if (!err)
    {
	cur_vws = (View_surface *) calloc(1, sizeof(View_surface));
	_cgi_view_surfaces[new_name] = cur_vws;
	if (!cur_vws)
	    err = EMEMSPAC;
    }
    if (!err)
    {
	cur_att = _cgi_alloc_output_att(_cgi_state.common_att);
	if (cur_att == (Outatt *) 0)
	{			/* malloc failed */
	    err = EMEMSPAC;
	    _cgi_free_name(new_name);
	}
    }
    if (!err)
    {				/* if malloc worked */
	_cgi_vws = cur_vws;
	cur_vws->att = _cgi_att = cur_att;
	_cgi_retained = (devdd->retained);
	err = _cgi_get_dev(devdd);	/* get actual device */
	(void) fcntl(cur_vws->sunview.windowfd, F_SETFL, FASYNC | FNDELAY);
    }
    if (!err)			/* (no err from _cgi_get_dev) */
    {
	Cos             incoming_cgi_state = _cgi_state.state;

	if (_cgi_state.state != VSAC)
	    _cgi_state.state = VSOP;
	cur_vws->active = 0;
	/*
	 * if first view surface opened, set defaults, initialize input stats,
	 * and clear screen.  Hard_reset does _cgi_windowset via vdc_extent.
	 */
	if (incoming_cgi_state == CGOP)
	    (void) hard_reset();
	else
	    (void) _cgi_windowset(cur_vws);

	/* Return handles for new view surface */
	*name = new_name;	/* Only if no errors */
	atarg->vws = cur_vws;
    }
    else
    {
	if (new_name >= 0)
	{
	    /* Make surface as if it didn't exist */
	    _cgi_free_name(new_name);
	}
	/*
	 * Explicitly invalidate the contents of the descriptor being returned
	 * to forestall later segmentation violations when the client calls
	 * CGIPW functions with this bogus descriptor.
	 */
	*name = -1;
	_cgi_vws = atarg->vws = (View_surface *) NULL;
    }

    END_CRITICAL();
    return (_cgi_errhand(err));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: close_vws	 					    */
/*                                                                          */
/*		deselects viewsurface		  			    */
/****************************************************************************/

Cerror          close_vws(name)
Cint            name;		/* view surface name */

{
    register View_surface *vwsP;
    int             err;

    /*
     * Can't detect error 112, ENOTCCPW, not compatible with Standard CGI,
     * since close_vws is called by close_cgi_pw, without setting another
     * global.
     */
    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_context(name);
    if (!err)
    {
	vwsP = _cgi_vws;
	if (vwsP->sunview.gp_att != (Gp1_attr *) NULL)
	{
	    _cgi_gp1_attr_close(vwsP->sunview.pw, &vwsP->sunview.gp_att);
	}
	if (vwsP->sunview.pw != vwsP->sunview.orig_pw)
	{
	    pw_close(vwsP->sunview.orig_pw);
	    vwsP->sunview.orig_pw = (Pixwin *) NULL;
	}

	/*
	 * If this view surface had its own attribute set allocated, then free
	 * it.  Common attribute structure is freed in close_cgi.
	 */
	if (vwsP->att != _cgi_state.common_att)
	{
	    _cgi_free_output_att(vwsP->att);
	    vwsP->att = (Outatt *) NULL;
	}

	/*
	 * ## WCL860925: is this true?? This should hold off SIGWINCH &
	 * SIGCHILD handling, which could cause errors after closing the fd.
	 */
	_cgi_free_name(name);

	if (!_cgi_state.cgipw_mode)
	{
	    /* wds 860513: don't clear cgipw user's pw or close his windowfd */
	    pw_close(vwsP->sunview.pw);

	    if (_cgi_gfxenvfd == vwsP->sunview.windowfd)	/* WDS: was != before
								 * 860104 */
	    {
		win_removeblanket(vwsP->sunview.windowfd);
		(void) close(_cgi_gfxenvfd);	/* WDS: was commented out. */
		_cgi_gfxenvfd = -1;
	    }
	    else if (vwsP->sunview.tool_pid > 0)	/* 0 is not a valid PID */
	    {
		win_setowner(vwsP->sunview.windowfd, vwsP->sunview.tool_pid);
		(void) close(vwsP->sunview.windowfd);
		(void) kill(vwsP->sunview.tool_pid, 9);
		vwsP->sunview.tool_pid = 0;
	    }
	    else
	    {
		if (win_isblanket(vwsP->sunview.windowfd))
		    win_removeblanket(vwsP->sunview.windowfd);
		(void) close(vwsP->sunview.windowfd);
	    }
	}
	_cgi_vws = (View_surface *) NULL;
	_cgi_att = _cgi_state.common_att;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_free_name	 					    */
/*                                                                          */
/*		frees up a viewsurface "name" (_cgi_view_surfaces index).   */
/*		see also _cgi_new_vwsurf_name, which allocates names.	    */
/*		_cgi_vwsurf_count is index such that it and all entries	    */
/*		above it are definitely NULL.  Lower indices may be NULL.   */
/****************************************************************************/

Cerror          _cgi_free_name(name)
Cint            name;		/* view surface name */
{
    View_surface   *cur_vws = _cgi_view_surfaces[name];

    if (cur_vws != (View_surface *) 0)
    {
	free((char *) cur_vws);
	_cgi_view_surfaces[name] = (View_surface *) 0;
    }

    /* If name is highest in array, find new highest entry in array */
    if (name == _cgi_vwsurf_count - 1)
    {
	while (--name >= 0 && _cgi_view_surfaces[name] == (View_surface *) 0)
	    ;
	_cgi_vwsurf_count = name + 1;
	/*
	 * If there are still view surfaces open, set global pointers
	 * to point to an open one.
	 */
	if (_cgi_vwsurf_count > 0)
	{
	    _cgi_vws = _cgi_view_surfaces[name];
	    _cgi_att = _cgi_vws->att;
	}
    }
    _cgi_reset_vws_based_state();
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_bump_vws                                              */
/*                                                                          */
/*               gets next active view surface				    */
/****************************************************************************/
int             _cgi_bump_vws(num)
int            *num;
{
    int             i = *num;
    int             ret = 0;

    for (; i < _cgi_vwsurf_count; i++)
    {
	ret = _cgi_context(i);
	if ((ret == 0) && _cgi_view_surfaces[i]->active)
	{
	    ret = 1;
	    break;
	}
    }
    *num = i + 1;		/* Caller should try next surface next time */
    return (ret);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_bump_all_vws					    */
/*                                                                          */
/*               gets next open view surface				    */
/****************************************************************************/
int             _cgi_bump_all_vws(num)
int            *num;
{
    int             i;
    int             ret = 0;

    for (i = *num; i < _cgi_vwsurf_count; i++)
    {
	if (_cgi_context(i) == 0)
	{
	    ret = 1;
	    break;
	}
    }
    *num = i + 1;		/* Caller should try next surface next time */
    return (ret);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_context                                               */
/*                                                                          */
/*               changes context of view surface.                           */
/****************************************************************************/
int             _cgi_context(num)
int             num;
{
    int             err;

    err = _cgi_legal_winname(num);	/* check num, _cgi_view_surfaces[num] */
    if (!err)
    {
	_cgi_vws = _cgi_view_surfaces[num];
	_cgi_att = _cgi_vws->att;
    }
    return (err);
}
