#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)switcher.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Switcher.c - Switch between plane groups on cgfour frame buffer.
 *	-d /dev/bwtwo0|cgfour0|fb	fb device to switch to
 *	-s n|l|r|i|o|f			switch mode
 *						n(ow)
 *						l(eft wipe)
 *						r(ight wipe)
 *						i(tunnel In)
 *						o(tunnel Out)
 *						f(ade)
 *	-m x y				mouse position after switch
 *					    -1 -1 means don't move, the default
 *	-n				no frame wanted, not interactive frame
 *	-e 0|1				enable plane set to 0 or 1 then exit;
 *					    -d is the device used;
 *					    not dependent on SunView
 */

#include <sunwindow/sun.h>
#include <suntool/sunview.h>
#include <pixrect/pr_planegroups.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

Bool	defaults_get_boolean();

static short ic_image[256] = {
#include <images/switcher.icon>
};
DEFINE_ICON_FROM_IMAGE(icon, ic_image);

static	Frame frame;		/* Frame with no subwindows */
static	base_rootfd;		/* Frames root window */

static	Notify_value event_interposer();
static	void	usage();
static	int	do_switch();
static	void	do_animation();
static	int	find_root();
Bool		deffaults_get_boolean();

/* Values designed to avoid conflict with frame menu return values */
typedef	enum display_transfer_mode {
	NOW=100,
	LEFT_WIPE=101,
	RIGHT_WIPE=102,
	TUNNEL_IN=103,
	TUNNEL_OUT=104,
	FADE=105,
} Display_transfer_mode;

static	Display_transfer_mode	display_transfer_mode;
static	int ms_x_switch;
static	int ms_y_switch;
#define	MS_DEFAULT_VALUE	-1
static	char dev_switch[32];
static	Pixrect *pr;
static	struct screen screen_switch;
static	Menu	wmenu;

#ifdef STANDALONE
main(argc, argv)
#else
switcher_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	extern	char *rindex();
	char	*icon_name;
	int argc_orig = argc;
	char **argv_orig = argv;
	struct	screen	frame_screen;
	char groups[PIX_MAX_PLANE_GROUPS];
	int no_frame;
	int enable_set = -1;
	Rect icon_rect;

	display_transfer_mode = NOW;
	ms_x_switch = MS_DEFAULT_VALUE;
	ms_y_switch = MS_DEFAULT_VALUE;
	no_frame = FALSE;
	(void)strcpy(dev_switch, "/dev/fb");
	/* Get application specific flags */
	argc--;
	argv++;
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'd':	/* fb device to switch to */
			if (argc < 2) {
				(void)fprintf(stderr,
				    "Device filename expected after %s\n",
				    argv[0]);
				goto Arg_Error;
			}
			(void) sscanf(argv[1], "%s", dev_switch);
			argv++;
			argc--;
			break;

		case 's':	/* switch mode */
			if (argc < 2) {
				(void)fprintf(stderr,
				    "Another arg after %s expected\n", argv[0]);
				goto Arg_Error;
			}
			switch (argv[1][0]) {
			case 'n': display_transfer_mode = NOW; break;
			case 'l': display_transfer_mode = LEFT_WIPE; break;
			case 'r': display_transfer_mode = RIGHT_WIPE; break;
			case 'i': display_transfer_mode = TUNNEL_IN; break;
			case 'o': display_transfer_mode = TUNNEL_OUT; break;
			case 'f': display_transfer_mode = FADE; break;
			default:
				(void)fprintf(stderr,
				    "Unexpeced value after %s\n", argv[0]);
				goto Arg_Error;
			}
			argv++;
			argc--;
			break;

		case 'm':	/* mouse position after switch */
			if (argc < 3) {
				(void)fprintf(stderr, "2 args expected after %s\n",
				    argv[0]);
				goto Arg_Error;
			}
			(void) sscanf(argv[1], "%hd", &ms_x_switch);
			(void) sscanf(argv[2], "%hd", &ms_y_switch);
			argv += 2;
			argc -= 2;
			break;

		case 'n':	/* no frame wanted, not interactive frame */
			no_frame = TRUE;
			break;
			
		case 'e':	/* enable set */
			if (argc < 2) {
				(void)fprintf(stderr,
				    "Another arg after %s expected\n", argv[0]);
				goto Arg_Error;
			}
			switch (argv[1][0]) {
			case '0': enable_set = 0; break;
			case '1': enable_set = 1; break;
			default:
				(void)fprintf(stderr,
				    "Unexpeced value after %s\n", argv[0]);
				goto Arg_Error;
			}
			argv++;
			argc--;
			break;

		default:
			{}
		}
		argv++;
		argc--;
		continue;
Arg_Error:
		usage();
		EXIT(-1);
		/* NOTREACHED */
	}
	/* open frame buffer */
	if ((pr = pr_open(dev_switch)) == 0) {
		(void)fprintf(stderr, "pixrect open failed for %s\n", dev_switch);
		EXIT(1);
		/* NOTREACHED */
	}
	if (enable_set != -1) {
		(void) pr_available_plane_groups(pr, sizeof groups, groups);
		if (groups[PIXPG_OVERLAY_ENABLE] == 0) {
			(void)fprintf(stderr,
			    "%s does not have an overlay enable plane\n",
			    dev_switch);
			EXIT(1);
			/* NOTREACHED */
		}
		pr_set_plane_group(pr, PIXPG_OVERLAY_ENABLE);
		rect_construct(&icon_rect, 0, 0, pr->pr_width, pr->pr_height);
		(void) do_animation(display_transfer_mode, enable_set,
		    icon_rect);
		EXIT(0);
		/* NOTREACHED */
	}
	
	/* Set up icon */
	rect_construct(&icon_rect, 2, 46, 60, 16);
	icon_name = rindex(dev_switch, '/');
	if (!icon_name)
		icon_name = dev_switch;
	else
		icon_name++;
	icon.ic_flags = ICON_BKGRDCLR;
	(void) icon_set(&icon,
	    ICON_LABEL,		icon_name,
	    ICON_LABEL_RECT,	&icon_rect,
	    0);

	/* Create frame */
	frame = window_create((Window)0, FRAME,
	    FRAME_LABEL,	"switcher",
	    FRAME_ICON,		&icon,
	    FRAME_CLOSED,	TRUE,
	    FRAME_ARGC_PTR_ARGV,&argc_orig, argv_orig,
	    0);
	
	/* Get root window from frame's desktop */
	(void)win_screenget((int)window_get(frame, WIN_FD), &frame_screen);
	base_rootfd = open(frame_screen.scr_rootname, O_RDONLY, 0);
	if (base_rootfd == -1) {
		(void)fprintf(stderr, "couldn't open %s\n",frame_screen.scr_rootname);
		return (-1);
	}

	/* Get pixrect that can access the enable plane */
	(void) pr_available_plane_groups(pr, sizeof groups, groups);
	if (groups[PIXPG_OVERLAY_ENABLE] == 0) {
		(void)pr_destroy(pr);
		if ((pr = pr_open(frame_screen.scr_fbname)) == 0) {
			(void)fprintf(stderr, "Couldn't open pixrect on %s\n",
			    frame_screen.scr_fbname);
			EXIT(1);
			/* NOTREACHED */
		}
		(void) pr_available_plane_groups(pr, sizeof groups, groups);
		if (groups[PIXPG_OVERLAY_ENABLE] == 0) {
			(void)fprintf(stderr,
			    "%s does not have an overlay enable plane\n",
			    frame_screen.scr_fbname);
			EXIT(1);
			/* NOTREACHED */
		}

	}
	pr_set_plane_group(pr, PIXPG_OVERLAY_ENABLE);
	
	/* If no frame wanted then do switching operation right now */
	if (no_frame) {
		(void) do_switch(display_transfer_mode);
		EXIT(0);
		/* NOTREACHED */
	}
	/* Interpose frame's event stream */
	(void) notify_interpose_event_func(frame, event_interposer,NOTIFY_SAFE);

	/* Initialize menu */
	{
		Menu fmenu;

		fmenu = (Menu) window_get(frame, WIN_MENU);
		wmenu = menu_create(
		    MENU_STRING_ITEM,	"Now",	NOW,
		    MENU_STRING_ITEM,	"Left",	LEFT_WIPE,
		    MENU_STRING_ITEM,	"Right",RIGHT_WIPE,
		    MENU_STRING_ITEM,	"In",	TUNNEL_IN,
		    MENU_STRING_ITEM,	"Out",	TUNNEL_OUT,
		    MENU_STRING_ITEM,	"Fade",	FADE,
		    0);
		(void)menu_set(wmenu, MENU_INSERT, 0,
			 menu_create_item(MENU_PULLRIGHT_ITEM, "Frame",
			    fmenu, 0), 0);
		(void) window_set(frame, WIN_MENU, wmenu, 0);
	}
	/* Wait for user action */
	window_main_loop(frame);
	EXIT(0);
	/* NOTREACHED */
}

static	void
usage()
{
(void)fprintf(stderr,
"Usage:\n-d\t/dev/bwtwo0|cgfour0|fb\n-s\tn|l|r|i|o|f\n-m\tx y\n-n\n-e\t0|1\n");
}

static	int
do_switch(mode)
	Display_transfer_mode	mode;
{
	char available[PIX_MAX_PLANE_GROUPS];
	int rootfd_switch;
	int enable_color;
	int x, y;
	
	/*
	 * Find screen_switch if haven't already.  This delayed binding
	 * is useful when starting switcher before the target desktop has
	 * been created.
	 */
	if (screen_switch.scr_rootname[0] == NULL) {
		/* Find target root window */
		switch (win_enumall(find_root)) {
		case 0:	/* Not found */
			(void)fprintf(stderr, "couldn't find %s's screen\n",
			    dev_switch);
			return (-1);

		case 1: /* Found */
			break;

		case -1:/* Error */
		default:
			(void)fprintf(stderr, "problem enumerating windows\n");
			perror("switcher");
			return (-1);
		}
	}
	
	rootfd_switch = open(screen_switch.scr_rootname, O_RDONLY, 0);
	if (rootfd_switch == -1) {
		(void)fprintf(stderr, "couldn't open %s\n",
		    screen_switch.scr_rootname);
		return (-1);
	}
	win_get_plane_groups_available(rootfd_switch, pr, available);
	if (available[PIXPG_OVERLAY] && available[PIXPG_8BIT_COLOR]) {
		(void)fprintf(stderr,
		    "target device is accessing multiple plane groups\n");
		(void)close(rootfd_switch);
		return (-1);
	}
	if (available[PIXPG_OVERLAY])
		enable_color = 1;
	else if (available[PIXPG_8BIT_COLOR])
		enable_color = 0;
	else if (available[PIXPG_MONO])
		enable_color = 1;
	else {
		(void)fprintf(stderr, "couldn't figure enable color\n");
		(void)close(rootfd_switch);
		return (-1);
	}

	do_animation(mode, enable_color, screen_switch.scr_rect);

	/*
	 * Set mouse on other desktop.  Do after animation because
	 * setting the mouse before the animation looses the animation
	 * because a desktop with TOGGLE_ENABLE switches automatically
	 * using the fast mode.
	 */
	/* Get current postion on frame's desktop */
	x = win_get_vuid_value(base_rootfd, LOC_X_ABSOLUTE);
	y = win_get_vuid_value(base_rootfd, LOC_Y_ABSOLUTE);
	/* Override the current value if specified by the user */
	if (ms_x_switch != MS_DEFAULT_VALUE)
		x = ms_x_switch;
	if (ms_y_switch != MS_DEFAULT_VALUE)
		y = ms_y_switch;
	(void)win_setmouseposition(rootfd_switch, x, y);

	(void)close(rootfd_switch);
	return (0);
}

static	void
do_animation(mode, enable_color, r)
	Display_transfer_mode mode;
	register int enable_color;
	Rect r;
{
	register int i;
	register int rl, rr;
	int rt, rb;

	/*
	 * Do transition animation in the enable plane
	 */
#define	TSTEP	4
#define	STEP	8
	switch (mode) {
	case LEFT_WIPE:
		for (i = r.r_left; i <= rect_right(&r)+STEP; i+=STEP)
			(void)pr_rop(pr, i, r.r_top, STEP, r.r_height,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
		break;
		
	case RIGHT_WIPE:
		for (i = rect_right(&r); i >= r.r_left-STEP; i-=STEP)
			(void)pr_rop(pr, i, r.r_top, STEP, r.r_height,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
		break;
		
	case TUNNEL_IN:
		rl = r.r_left; rt = r.r_top;
		rr = rect_right(&r); rb = rect_bottom(&r);
		while (rl <= rr+TSTEP && rt <= rb+TSTEP) {
			(void)pr_rop(pr, rl, rt, rr-rl, TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rr, rt, TSTEP, rb-rt+TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rl, rb, rr-rl+TSTEP, TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rl, rt, TSTEP, rb-rt,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			rl+=TSTEP;
			rt+=TSTEP;
			rr-=TSTEP;
			rb-=TSTEP;
		}
		break;
		
	case TUNNEL_OUT: {
		int hor_mid = (rect_right(&r) - r.r_left) / 2;
		int ver_mid = (rect_bottom(&r) - r.r_top) / 2;
		
		if (hor_mid > ver_mid) {
			rl = r.r_left + ver_mid;
			rr = rect_right(&r) - ver_mid;
			rt = rb = ver_mid;
		} else {
			rt = r.r_top + hor_mid;
			rb = rect_bottom(&r) - hor_mid;
			rl = rr = hor_mid;
		}
		while (rl >= r.r_left-TSTEP && rt >= r.r_top-TSTEP) {
			(void)pr_rop(pr, rl, rt, rr-rl, TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rr, rt, TSTEP, rb-rt+TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rl, rb, rr-rl+TSTEP, TSTEP,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			(void)pr_rop(pr, rl, rt, TSTEP, rb-rt,
			    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
			rl-=TSTEP;
			rt-=TSTEP;
			rr+=TSTEP;
			rb+=TSTEP;
		}
		break;
		}
		
	case FADE:
#define	INCS 20
		for (i = 0; i < INCS+STEP; i+=STEP) {
			for (rl = r.r_left+i; rl <= rect_right(&r); rl+=INCS)
				(void)pr_rop(pr, rl, r.r_top, STEP, r.r_height,
				    PIX_COLOR(enable_color) | PIX_SRC, 
				    	(Pixrect *)0, 0, 0);
			for (rt = r.r_top+i; rt <= rect_bottom(&r); rt+=INCS)
				(void)pr_rop(pr, r.r_left, rt, r.r_width, STEP,
				    PIX_COLOR(enable_color) | PIX_SRC, 
				    	(Pixrect *)0, 0, 0);
		}
		break;
		
	case NOW:
	default:
		(void)pr_rop(pr, r.r_left, r.r_top, r.r_width, r.r_height,
		    PIX_COLOR(enable_color) | PIX_SRC, (Pixrect *)0, 0, 0);
		break;
	}
}

static	int
find_root(windowfd)
	int	windowfd;
{
	struct	screen screen;

	(void)win_screenget(windowfd, &screen);
	/* Match device names */
	if (strncmp(dev_switch, screen.scr_fbname, SCR_NAMESIZE) == 0) {
		screen_switch = screen;
		return (1);
	}
	return(0);
}

static	Notify_value
event_interposer(frame_local, event, arg, type)
	Window	frame_local;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	if (event_is_down(event) && (event_action(event) == MS_LEFT)) {
		(void) do_switch(display_transfer_mode);
		return (NOTIFY_DONE);
	} else if (event_is_down(event) && (event_action(event) == MS_RIGHT)) {
		Display_transfer_mode possible_mode;

		possible_mode = (Display_transfer_mode)menu_show(wmenu, frame_local,
		    event, 0);
		switch (possible_mode) {
		case NOW:
		case LEFT_WIPE:
		case RIGHT_WIPE:
		case TUNNEL_IN:
		case TUNNEL_OUT:
		case FADE:
			(void) do_switch(possible_mode);
		}
		return (NOTIFY_DONE);
	} else
		return (notify_next_event_func(frame_local, (Notify_event)
				(LINT_CAST(event)), arg, type));
}



