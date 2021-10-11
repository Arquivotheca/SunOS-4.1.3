#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)defaults_from_input.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * defaults_from_input.c:    read the kernel to determine the state of
 *	the input system, and set the defaults databaase accordingly
 */

#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#include <sundev/msio.h>
#include <sunwindow/sun.h>
#include <sunwindow/win_input.h>


#ifndef TRUE
#define TRUE	1
#define FALSE	0
#endif

void	defaults_write_changed(),defaults_set_enumeration(),defaults_set_integer();
Bool	defaults_get_boolean();

typedef enum { Unknown, Klunker, Sun1, Sun2, Sun3, Sun4
}	Keybd_type;

typedef struct {
    int		    key;
    int		    code;
}	Pair;

typedef struct {
    u_int           arrow_keys;
    u_int           sunview_keys;
    u_int           left_handed;
    Ms_parms        ms_parms;
    Ws_scale_list   scaling;
}	Options;

static int            debug_setinput = FALSE;

static Keybd_type     get_keyboard_type();

static void           get_file_options(),
		      get_keyboard_options(),
		      get_mouse_options(),
		      get_window_options(),
		      lose(),
		      set_bool_option(),
		      set_int_option(),
		      set_options();

#ifdef STANDALONE
main(argc, argv)
#else
defaults_from_input_main(argc, argv)
#endif    int             argc;
    char          **argv;
{
    int             keyboard, mouse, win0;
    Options         options;

    if (argc-- > 1) {
	if (strcmp(*(++argv), "-d") == 0)
	    debug_setinput = TRUE;
	else {
	    (void)fprintf(stderr, "usage: defaults_from_input [-d]\n");
	    exit(1);
	}
    }
    if ((keyboard = open("/dev/kbd", O_RDONLY, 0)) < 0) {
	(void)fprintf(stderr, "Couldn't open /dev/kbd\n");
	lose(1);
    }
    if ((mouse = open("/dev/mouse", O_RDONLY, 0)) < 0) {
	(void)fprintf(stderr, "Couldn't open /dev/mouse\n");
	lose(1);
    }
    if ((win0 = open("/dev/win0", O_RDONLY, 0)) < 0) {
	(void)fprintf(stderr, "Couldn't open /dev/win0\n");
	lose(1);
    }
    get_keyboard_options(keyboard, &options);
    get_mouse_options(mouse, &options);
    get_window_options(win0, &options);
    
    set_options(&options);
    exit(0);
}

static Keybd_type
get_keyboard_type(kbd)
    int             kbd;
{
    int             type;

    if (ioctl(kbd, KIOCTYPE, &type) == -1) {
	lose(1);
    }
    switch (type) {
      case KB_KLUNK:
	return Klunker;
      case KB_VT100:
	return Sun1;
      case KB_SUN2:
	return Sun2;
      case KB_SUN3:
	return Sun3;
      case KB_SUN4:
	return Sun4;
      case KB_ASCII:
      case -1:
      default:
	return Unknown;
    }
}

static void
get_keyboard_options(kbd, options)
    int                kbd;
    Options           *options;
{
    Keybd_type         type;
    struct kiockeymap  key;

    type = get_keyboard_type(kbd);
    key.kio_tablemask = 0;
    if (type == Sun1) {
	key.kio_station = 55;	/* L5 on the usual Sun-1 arrangement */
    } else {
	key.kio_station = 49;	/* L5 on Klunker, Sun-2, Type 3, and Type 4  */
    }
    if (ioctl(kbd, KIOCGKEY, &key) < 0) {
	goto bad_ioctl;
    }
    switch (key.kio_entry) {
      case LF(5):
	options->left_handed = FALSE;
	options->sunview_keys = TRUE;
	break;
      case RF(9):
	options->left_handed = TRUE;
	options->sunview_keys = TRUE;
	break;
      default:
	options->arrow_keys = TRUE;
	options->left_handed = FALSE;
	options->sunview_keys = FALSE;
	return;
    }
    if (type == Klunker) {
	options->arrow_keys = FALSE;
	return;
    } else if (type == Sun1) {
	key.kio_station = 10;	/* Up-arrow on Sun-1	 */
    } else {
	key.kio_station = 69;	/* on Sun-2, Type 3, and Type 4, regardless of lefty  */
    }
    if (ioctl(kbd, KIOCGKEY, &key) < 0) {
	goto bad_ioctl;
    }
    if (key.kio_entry == STRING + UPARROW)
	options->arrow_keys = TRUE;
    else
	options->arrow_keys = FALSE;
    return;

bad_ioctl:
    (void)fprintf(stderr, "Can't read keyboard mapping.\n");
    lose(2);
}

static void
get_mouse_options(mouse, options)
    int             mouse;
    Options        *options;
{

    if (ioctl(mouse, MSIOGETPARMS, &options->ms_parms)) {
	(void)fprintf(stderr, "Get mouse parms failed\n");
	lose(2);
    }
}

static void
get_window_options(win, options)
    int             win;
    Options        *options;
{
    int 	    order;

    order = win_get_button_order(win);
    if (order == WS_ORDER_RML) {
	if (!options->left_handed) {
	    (void)fprintf(stderr, "Mouse is left-handed, but keyboard isn't.  ");
	    (void)fprintf(stderr, "Database will say right-handed.\n");
	}
    } else {
	if (options->left_handed) {
	    (void)fprintf(stderr, "Keyboard is right-handed, but mouse isn't.  ");
	    (void)fprintf(stderr, "Database will say right-handed.\n");
	    options->left_handed = FALSE;
	}
    }
    if (win_get_scaling(win, &options->scaling)) {
        (void)fprintf(stderr, "Get mouse scaling failed");
        lose(2);
    }
}

static void
lose(code)
    int             code;
{
    perror("input_from_defaults");
    exit(code);
}

static void
set_options(options)
    Options        *options;
{
    char	   *yes = "Yes",
		   *no  = "No",
		   *on  = "On",
		   *off = "Off";

    if ((int)defaults_get_boolean("/Input/Arrow_Keys", (Bool)TRUE, (int *)NULL)
	!= options->arrow_keys)
	set_bool_option("/Input/Arrow_Keys",
			(int)options->arrow_keys, yes, no);
    if ((int)defaults_get_boolean("/Input/SunView_Keys", (Bool)TRUE, (int *)NULL)
	!= options->sunview_keys)
	set_bool_option("/Input/SunView_Keys",
			(int)options->sunview_keys, yes, no);
    if ((int)defaults_get_boolean("/Input/Left_Handed", (Bool)FALSE, (int *)NULL)
	!= options->left_handed)
	set_bool_option("/Input/Left_Handed",
			(int)options->left_handed, yes, no);
    if ((int)defaults_get_boolean("/Input/Jitter_Filter", (Bool)FALSE, (int *)NULL)
	!= options->ms_parms.jitter_thresh)
	set_bool_option("/Input/Jitter_Filter",
			(int)options->ms_parms.jitter_thresh, on, off);
    if ((int)defaults_get_boolean("/Input/Speed_Enforced", (Bool)FALSE, (int *)NULL)
	!= options->ms_parms.speed_law)
	set_bool_option("/Input/Speed_Enforced",
			(int)options->ms_parms.speed_law, yes, no);
    if (defaults_get_integer("/Input/Speed_Limit", 48, (int *)NULL)
	!= options->ms_parms.speed_limit)
	set_int_option("/Input/Speed_Limit",
			(int)options->ms_parms.speed_limit);
    if (defaults_get_integer("/Input/1st_ceiling", 65535, (int *)NULL)
	!= options->scaling.scales[0].ceiling)
	set_int_option("/Input/1st_ceiling",
			(int)options->scaling.scales[0].ceiling);
    if (defaults_get_integer("/Input/1st_factor", 2, (int *)NULL)
	!= options->scaling.scales[0].factor)
	set_int_option("/Input/1st_factor",
			(int)options->scaling.scales[0].factor);
    if (defaults_get_integer("/Input/2nd_ceiling", 0, (int *)NULL)
	!= options->scaling.scales[1].ceiling)
	set_int_option("/Input/2nd_ceiling",
			(int)options->scaling.scales[1].ceiling);
    if (defaults_get_integer("/Input/2nd_factor", 0, (int *)NULL)
	!= options->scaling.scales[1].factor)
	set_int_option("/Input/2nd_factor",
			(int)options->scaling.scales[1].factor);
    if (defaults_get_integer("/Input/3rd_ceiling", 0, (int *)NULL)
	!= options->scaling.scales[2].ceiling)
	set_int_option("/Input/3rd_ceiling",
			(int)options->scaling.scales[2].ceiling);
    if (defaults_get_integer("/Input/3rd_factor", 0, (int *)NULL)
	!= options->scaling.scales[2].factor)
	set_int_option("/Input/3rd_factor",
			(int)options->scaling.scales[2].factor);
    if (defaults_get_integer("/Input/4th_ceiling", 0, (int *)NULL)
	!= options->scaling.scales[3].ceiling)
	set_int_option("/Input/4th_ceiling",
			(int)options->scaling.scales[3].ceiling);
    if (defaults_get_integer("/Input/4th_factor", 0, (int *)NULL)
	!= options->scaling.scales[3].factor)
	set_int_option("/Input/4th_factor",
			(int)options->scaling.scales[3].factor);
    defaults_write_changed((char *)NULL, (int *)NULL);
}

static void
set_bool_option(name, value, t_name, f_name)
    char           *name, *t_name, *f_name;
    int             value;
{
    if (debug_setinput)
	(void)printf("Set %s to %s.\n", name, value ? t_name : f_name);
    else
	defaults_set_enumeration(name, (value ? t_name : f_name), (int *)NULL);
}


static void
set_int_option(name, value)
    char           *name;
    int             value;
{
    if (debug_setinput)
	(void)printf("Set %s to %d.\n", name, value);
    else
	defaults_set_integer(name, value, (int *)NULL);
}
