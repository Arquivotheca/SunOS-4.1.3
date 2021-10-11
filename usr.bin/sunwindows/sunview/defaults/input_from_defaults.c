#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)input_from_defaults.c 1.1 92/07/30";
#endif
#endif
/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * input_from_defaults.c	modify keyboard and mouse behavior
 */

#include <ctype.h>
#include <stdio.h>
#include <strings.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sunwindow/sun.h>
#include "setkey_tables.h"

Bool	defaults_get_boolean();

typedef struct {
    u_int           arrow_keys;
    u_int           sunview_keys;
    u_int           left_handed;
    Ms_parms        ms_parms;
    Ws_scale_list   scaling;
}	Options;

static int            debug_setinput = FALSE;

static Keybd_type     get_type();

static int            setkey_local();

static void           get_default_options(),
		      get_key_info(),
		      get_options_scaling(),
		      lose(),
		      set_keyboard(),
		      set_mouse();

/* 
	Keyboard String Table

 	This defines the strings sent by various keys (as selected in the
	tables above).

	The first byte of each string is its length, the rest is data.
*/

#define	kstescinit(c)	{'\033', '[', 'c', '\0'}
static char keystringtab[16][KTAB_STRLEN] = {
	kstescinit(H) /*home*/,
	kstescinit(A) /*up*/,
	kstescinit(B) /*down*/,
	kstescinit(D) /*left*/,
	kstescinit(C) /*right*/,
};

#ifdef STANDALONE
main(argc, argv)
#else
input_from_defaults_main(argc, argv)
#endif    int             argc;
    char          **argv;
{
    int             keyboard, mouse, win0;
    Options         options;
    Key_info       *info_ptr;
    Keybd_type      keybd_type;

    if (argc-- > 1) {
	if (strcmp(*(++argv), "-d") == 0)
	    debug_setinput = TRUE;
	else {
	    (void)fprintf(stderr, "usage: input_from_defaults [-d]\n");
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
    keybd_type = get_type(keyboard);
    get_default_options(&options);

    get_key_info(keybd_type, &options, &info_ptr);
    set_keyboard(keybd_type, keyboard, info_ptr);
    set_mouse(mouse, win0, &options);

    exit(0);
}

static void
get_default_options(options)
    Options        *options;
{
    options->arrow_keys =
	(int)defaults_get_boolean("/Input/Arrow_Keys", (Bool)TRUE, (int *)NULL);
    options->sunview_keys =
	(int)defaults_get_boolean("/Input/SunView_Keys", (Bool)TRUE, (int *)NULL);
    options->left_handed =
	(int)defaults_get_boolean("/Input/Left_Handed", (Bool)FALSE, (int *)NULL);
    options->ms_parms.jitter_thresh =
	(int)defaults_get_boolean("/Input/Jitter_Filter", (Bool)FALSE, (int *)NULL);
    options->ms_parms.speed_law =
	(int)defaults_get_boolean("/Input/Speed_Enforced", (Bool)FALSE, (int *)NULL);
    options->ms_parms.speed_limit =
	defaults_get_integer("/Input/Speed_Limit", 48, (int *)NULL);
    get_options_scaling(&options->scaling);
}

static void
get_key_info(type, options, info_ptr)
    Keybd_type      type;
    Options        *options;
    Key_info      **info_ptr;
{
    register Keybd_info *kbd_ptr;

    if (!options->sunview_keys) {
	if (type != Sun2_kbd && type != Sun3_kbd && type != Sun4_kbd) {
	    (void)fprintf(stderr,
		    "No-SunView is valid only on Sun2, Type 3, or Type 4 keyboards\n");
	    exit(4);
	}
	if (options->left_handed)
	    (void)fprintf(stderr, "Left-handedness ignored on No-SunView keyboard\n");
	if (options->arrow_keys)
	    *info_ptr = ktbl_no_sunview_info.standard;
	else
	    *info_ptr = ktbl_no_sunview_info.standard_noarrow;
	return;
    }
    switch (type) {
      case Klunker_kbd:
	kbd_ptr = &ktbl_klunker_info;
	break;
      case Sun1_kbd:
	kbd_ptr = &ktbl_sun1_info;
	break;
      case Sun2_kbd:
	kbd_ptr = &ktbl_sun2_info;
	break;
      case Sun3_kbd:
	kbd_ptr = &ktbl_sun3_info;
	break;
      case Sun4_kbd:
	kbd_ptr = &ktbl_sun4_info;
	break;
      default:
	(void)fprintf(stderr, "unknown keyboard type\n");
	exit(2);
    }
    if (options->left_handed) {
	if (options->arrow_keys)
	    *info_ptr = kbd_ptr->lefty;
	else
	    *info_ptr = kbd_ptr->lefty_noarrow;
    } else {
	if (options->arrow_keys)
	    *info_ptr = kbd_ptr->standard;
	else
	    *info_ptr = kbd_ptr->standard_noarrow;
    }
}

static void
get_options_scaling(scale_list)
    Ws_scale_list	*scale_list;
{
    scale_list->scales[0].ceiling = 
	defaults_get_integer("/Input/1st_ceiling", 65535, (int *)NULL);
    scale_list->scales[0].factor = 
	defaults_get_integer("/Input/1st_factor", 2, (int *)NULL);
    scale_list->scales[1].ceiling = 
	defaults_get_integer("/Input/2nd_ceiling", 0, (int *)NULL);
    scale_list->scales[1].factor = 
	defaults_get_integer("/Input/2nd_factor", 0, (int *)NULL);
    scale_list->scales[2].ceiling = 
	defaults_get_integer("/Input/3rd_ceiling", 0, (int *)NULL);
    scale_list->scales[2].factor = 
	defaults_get_integer("/Input/3rd_factor", 0, (int *)NULL);
    scale_list->scales[3].ceiling = 
	defaults_get_integer("/Input/4th_ceiling", 0, (int *)NULL);
    scale_list->scales[3].factor = 
	defaults_get_integer("/Input/4th_factor", 0, (int *)NULL);
}

static Keybd_type
get_type(kbd)
    int             kbd;
{
    int             type;

    if (ioctl(kbd, KIOCTYPE, &type) == -1) {
	lose(1);
    }
    switch (type) {
      case KB_KLUNK:
	return Klunker_kbd;
      case KB_VT100:
	return Sun1_kbd;
      case KB_SUN2:
	return Sun2_kbd;
      case KB_SUN3:
	return Sun3_kbd;
      case KB_SUN4:
	return Sun4_kbd;
      case KB_ASCII:
      case -1:
      default:
	return Unknown_kbd;
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
set_mouse(mouse, win0, options)
    int		     mouse, win0;
    Options	    *options;
{
    int		    err;

    if (debug_setinput) {
	(void)printf("Set button order to %s.\n",
		(options->left_handed) ? "RML" : "LMR");
	(void)printf("Set jitter filter %s.\n",
		(options->ms_parms.jitter_thresh) ? "on" : "off");
	(void)printf("Speed law is %s enforced.\n",
		(options->ms_parms.speed_law) ? "" : "not");
	(void)printf("Speed limit is %d.\n", options->ms_parms.speed_limit);
	(void)printf("Scaling ceiling / factor pairs are:\n");
	(void)printf("    %5d : %3d\n",
		options->scaling.scales[0].ceiling,
		options->scaling.scales[0].factor);
	(void)printf("    %5d : %3d\n",
		options->scaling.scales[1].ceiling,
		options->scaling.scales[1].factor);
	(void)printf("    %5d : %3d\n",
		options->scaling.scales[2].ceiling,
		options->scaling.scales[2].factor);
	(void)printf("    %5d : %3d\n",
		options->scaling.scales[3].ceiling,
		options->scaling.scales[3].factor);
	return;
    }
    err = ioctl(mouse, MSIOSETPARMS, &options->ms_parms);
    if (err) {
	(void)fprintf(stderr, "Set mouse parms failed");
	lose(2);
    }
    err = win_set_button_order(win0, (options->left_handed) ?
					WS_ORDER_RML : WS_ORDER_LMR);
    if (err) {
	(void)fprintf(stderr, "Set mouse button-order failed");
	lose(2);
    }
    err = win_set_scaling(win0, &options->scaling);
    if (err) {
	(void)fprintf(stderr, "Set mouse scaling failed");
	lose(2);
    }
}

static void
set_keyboard(type, keyboard, info_ptr)
    Keybd_type      type;
    int             keyboard;
    Key_info       *info_ptr;
{
    register int    count;
    register Pair  *pairs;

    pairs = info_ptr->down_only;
    count = info_ptr->count_downs;
    while (count-- > 0) {
	if (setkey_local(keyboard, 0, pairs[count].key,
		   pairs[count].code) == -1)
	    lose(3);
	if (setkey_local(keyboard, CTRLMASK, pairs[count].key,
		   pairs[count].code) == -1)
	    lose(3);
	if (setkey_local(keyboard, SHIFTMASK, pairs[count].key,
		   pairs[count].code) == -1)
	    lose(3);
	if (setkey_local(keyboard, CAPSMASK, pairs[count].key,
		   pairs[count].code) == -1)
	    lose(3);
	if (type == Sun4_kbd) {
	    if (setkey_local(keyboard, ALTGRAPHMASK, pairs[count].key,
		   pairs[count].code) == -1)
		lose(3);
	}
    }
    pairs = info_ptr->ups;
    count = info_ptr->count_ups;
    while (count-- > 0) {
	if (setkey_local(keyboard, UPMASK, pairs[count].key,
		   pairs[count].code) == -1)
	    lose(3);
    }
}

static int
setkey_local(keyboard, table, position, code)
    int                keyboard, table, position, code;
{
    struct kiockeymap  key;

    if (code >= STRING && code <= STRING + 15)
	(void)strcpy(key.kio_string, keystringtab[code - STRING]);
    else
	key.kio_string[0] = '\0';

    if (debug_setinput) {
	(void)printf("Set station %d in %s table to %d.\n",
	       position,
	       ((table == 0)	 	 ? "base"	 :
		(table == CAPSMASK)	 ? "caps-locked" :
		(table == SHIFTMASK) 	 ? "shifted"	 :
		(table == CTRLMASK)	 ? "controlled"	 :
		(table == ALTGRAPHMASK)	 ? "alt-graphed" :
		(table == UPMASK)	 ? "key-up"	 :  "unknown"),
	       code);
	if (code >= STRING && code <= STRING + 15) {
	    (void)printf("\t(string[%d]: \"%s\")\n", code - STRING, key.kio_string);
	}
 	return 0;
    } else {
	key.kio_tablemask = table;
	key.kio_station = position;
	key.kio_entry = code;
	return ((ioctl((keyboard), KIOCSKEY, &key)));
    }
}
