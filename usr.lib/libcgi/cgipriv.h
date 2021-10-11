/*	@(#)cgipriv.h 1.1 92/07/30 Copyr 1985-9 Sun Micro		*/

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
 * CGI Private (implementation-only) Type definitions 
 */

#include "cgipw.h"		/* defines constants   */
#include "cgiminicon.h"
#include "cgi_gp1_pwpr.h"
#ifndef window_hs_DEFINED
#	include <sunwindow/window_hs.h>
#endif
#ifndef canvas_DEFINED
#	include <suntool/canvas.h>
#endif

/* These constants are used in any case, but are defined by pixwin normally */
#ifdef	NO_PIXWIN_POLYLINE
#	define	POLY_DONTCLOSE	((u_char *) 0)
#	define	POLY_CLOSE	((u_char *) 1)
#endif	NO_PIXWIN_POLYLINE
#define		POLY_DISJOINT   ((u_char *) 2)

/* Externals */

/*
 * Make lint happier by using "most aligned" type, even though it doesn't
 * match llib-lc.
 */
extern double		*malloc(), *calloc(), *realloc();

/* Constants */


#define MIN_MKR_SIZE	6	/* minimum marker size in pixels */
#ifndef	NULL
#	define	NULL	0	/* null pointer; using this value makes it
				 * easier to find pointer 0's by grep'ing. */
#endif	NULL

#define	CC_DC_SHIFT	21	/* shift for char_coord multipliers */
#define	CC_VDC_SHIFT	15	/* shift for char_coord multipliers */

/*Macros*/


#define	SETUP_CGIWIN(ptr)						\
    {									\
	if ((ptr) != (Ccgiwin *) NULL					\
	&&  (ptr)->vws != (View_surface *) NULL)			\
	{								\
	    _cgi_vws = (ptr)->vws;					\
	    _cgi_att = _cgi_vws->att;					\
	}								\
	else								\
	    return(ECGIWIN);						\
    }

#define	START_CRITICAL()						\
    {									\
	++_cgi_criticalcnt;						\
    }

#define	END_CRITICAL()							\
    {									\
	if (--_cgi_criticalcnt <= 0)					\
	    _cgi_dodeferred();						\
    }

/*Enumerated types*/


typedef enum
{
    CGCL, CGOP, VSOP, VSAC
}               Cos;

typedef enum
{
    DONT_XFORM, OFFSET_ONLY, XFORM
}               Cxformflg;

typedef enum
{
    DRAW,
    ERASE,
}               Ceraseflg;

/* Structures */

/* output attribute structures */


typedef struct
{
    Ctextatt        attr;
    short           fixed_font;	/* formerly gs_fixed_font */
    Ccoor           concat_pt;
    char           *astring;	/* formerly gs_astring */
}               TextState;

typedef struct
{
    Cint            r_left, r_bottom;
    Cint            r_width, r_height;
}               VdcRect;

typedef struct
{
    Cclip           indicator;
    VdcRect         rect;
}               ClipDefn;

typedef struct
{
    short           use_pw_size;/* 0==normal VDC; 1==pixwin VDC */
    VdcRect         rect;	/* range of legal VDC values */
    ClipDefn        clip;
}               ViewDefn;

struct outatt
{
    Clinatt         line;
    Cspecmode       line_spec_mode;
    Cmarkatt        marker;
    Cspecmode       mark_spec_mode;
    Cfillatt        fill;
    Cspecmode       perimeter_spec_mode;
    Cpatternatt     pattern;
    short           pattern_with_fill_color;
    Cendstyle       endstyle;
    Casptype        asfs[18];
    Cbunatt        *aes_table[MAXAESSIZE];
    TextState       text;
    ViewDefn        vdc;
};
typedef struct outatt Outatt;

/* view surface structures */


typedef struct
{
    int             windowfd;
    int             tool_pid;
    Canvas          canvas;
    Pixwin         *pw;		/* current pixwin (may be subregion) */
    Pixwin         *orig_pw;	/* original pixwin from open_vws */
    short           depth;
    Rect            lock_rect;	/* formerly _cgi_rect */
    int             (*sig_function) ();	/* formerly _cgi_sig_function */
    Gp1_attr       *gp_att;
}               SunView_if;

typedef struct
{
    Cint            num;	/* numerator of rational fraction */
    Cint            den;	/* denominator of rational fraction */
}               RatFract;

typedef struct
{
    RatFract        x;
    RatFract        y;
}               RatfrScale;

typedef struct
{
    RatfrScale      scale;
    Ccoor           off;
    Ccoor           win_off;
}               Transform;

typedef struct
{
    float           cos_base;
    float           sin_base;
    float           cos_up;
    float           sin_up;
    int             dc_cos_base;
    int             dc_sin_base;
    int             dc_cos_up;
    int             dc_sin_up;
    int             vdc_cos_base;
    int             vdc_sin_base;
    int             vdc_cos_up;
    int             vdc_sin_up;
    float           scale;
}               Conv_text;

typedef struct
{
    Cint            line_width;	/* formerly _cgi_conv_line_width */
    Cint            perimeter_width; /* formerly _cgi_conv_perimeter_width */
    Cint            marker_size;/* formerly _cgi_conv_marker_size */
    Conv_text       text;	/* formerly i_text */
    Rect            clip;	/* pw_region cut out, relative to orig_pw */
}               Conv_vals;

struct view_surface		/* formerly 'struct essential' */
{
    short           active;
    int             device;	/* BW1DD, BW2DD, CG1DD, CG2DD, CG4DD, GP1DD */
    SunView_if      sunview;
    Outatt         *att;
    Rect            real_screen;/* formerly _cgi_r_screen */
    Rect            vport;	/* formerly _cgi_vport */
    Transform       xform;	/* scaling and offset */
    Conv_vals       conv;	/* converted values */
};
typedef struct view_surface View_surface;

typedef struct
{
    short           ft_flag;
    int             ft_top;
    int             ft_cap;
    int             ft_bottom;
    int             ft_xmin;
    int             ft_xmax;
    Ccoor           ft_topline;	/* VDC movement to topline from ref */
    Ccoor           ft_capline;	/* VDC movement to capline from ref */
    Ccoor           ft_bottomline;	/* VDC movement to bottom from ref */
    Ccoor           ft_height;	/* VDC movement up a char box */
    Ccoor           ft_left;	/* VDC movement to left,base from ref */
    Ccoor           ft_right;	/* VDC movement to rght,base from ref */
    long            ft_vdc_change_cnt;	/* check against gs_vdc_change_cnt */
    long            ft_text_change_cnt;	/* check against gs_text_change_cnt */
}               Cfontinfo;

/* attribute structures */


struct toutatt
{
    int             ok;
    struct outatt  *cont;
};

#define NO_FONT		-1	/* value for num when ptr == NULL */
typedef struct
{
    int             num;
    struct pixfont *ptr;
}               OpenFont;

/*
 * Structure containing CGI global state that is once-per-CGI, rather
 * than once-per-viewsurface.
 */
typedef struct
{
    Cos             state;	/* CGI's state (CGCL, CGOP, VSOP, VSAC) */
    short           cgipw_mode; /* CGI is in CGIPW mode */
    Notify_client   notifier_client_handle;	/* CGI's handle */
    OpenFont        open_font;	/* open firmware font */
    int             gp1resetcnt;
    long            vdc_change_cnt;	/* _cgi_windowset calls */
    long            text_change_cnt;	/* _cgi_reset_internal_text calls */
    Outatt         *common_att;		/* attr. common to all view surfaces */
}               Gstate;

/* Input structures */


struct echorec
{
    Clogical        echo;
    Clogical        track;
    int            *etypes;
};

struct device
{				/* INPUT DEVICE DESCRIPTORS */
    Clidstate       enable;	/* bit 0: 0 => uninit, 1 => init */
    short           echo;	/* type of echo, 0=noecho,... */
    int             echopos[2];	/* NDC pt for echo reference */
    Cackstate       ack;	/* acknowldegement state */
};

struct locatstr
{				/* locator */
    struct device   subloc;
    int             setpos[2];	/* set position of locator */
    int             x, y;	/* device measure */
    int             rx, ry;	/* request buffer */
    int             trig;
};

struct keybstr
{				/* keyboard */
    struct device   subkey;
    char           *initstring;	/* initial echo string */
    int             initpos;	/* initial cursor position */
    int             bufsize;	/* user keyboard buffer size  */
    int             maxbufsz;	/* max buffer size = 80 chars */
    char           *rinitstring;/* request buffer */
    int             trig;
    Ccoor           echo_base;	/* base of echoing region */
    Ccoor           echo_pt;	/* current point within echoing region */
};

struct strokstr
{				/* stroke */
    struct device   substroke;
    Ccoorlist      *sarray;	/* stroke array */
    Ccoorlist      *rsarray;	/* request buffer */
    int             trig;
};

struct valstr
{				/* valuator */
    struct device   subval;
    float           vlinit;	/* set initial value */
    float           vlmin;	/* minimum value */
    float           vlmax;	/* maximum value */
    float           curval;	/* current value */
    float           rcurval;	/* request buffer */
    int             trig;
    Ccoor           echo_base;	/* base of echoing region */
    float           echo_val;	/* value currently drawn in echo region */
    int             echo_flag;	/* echoed value is currently displayed */
};

struct choicestr
{				/* buttons */
    struct device   subchoice;
    int             curval;	/* current value */
    int             rcurval;	/* request buffer */
    int             trig;
};

struct pickstr
{				/* buttons */
    struct device   subpick;
    Cpick          *pck;
    Cpick          *rpck;
    int             trig;
};
struct asstype
{				/* associations (currently unused) */
    int             evenclas;
    int             eventnam;
    int             samplcls;
    int             samplnam;
};

struct reportpk
{				/* REPORTS EVENTS QUEUES */
    short           segname;
    int             pickid;
};

struct reportky
{
    int             length;
    char           *text;
};

struct eventype
{
    struct reportpk pickrept;
    struct reportky keybrept;
};

struct samptype
{
    int             location[2];
    float           value;
};

struct smpldata
{
    int             samplcls;
    int             samplnam;
    struct samptype onefsmpl;
};

struct inqueue
{
    int             evenclas;
    int             eventnam;
    struct eventype onefevnt;
    int             samplnum;
    struct smpldata smplinfo;
};

struct evqueue
{
    int             n;
    Cinrep         *contents;
};
