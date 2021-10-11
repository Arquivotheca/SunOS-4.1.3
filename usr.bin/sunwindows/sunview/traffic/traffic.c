#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)traffic.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <sys/socket.h>
#include <rpcsvc/ether.h>
#include "traffic.h"

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

#define WSIZE	TOOL_SWEXTENDTOEDGE

/* 
 *  global variables
 */
static	int	client;
static	int	prevclosed;
static Panel_item	src_item, dst_item, nopkts_item,
			srctoggle_item, dsttoggle_item, proto_item,
			prototoggle_item, lnthtoggle_item, lnth_item;
int debug;

/* 
 *  procedure variables
 */
int	src_toggle(), dst_toggle(), proto_toggle(), lnth_toggle();
Panel_setting src_text(), dst_text(), lnth_text(), proto_text();
Notify_value	frame_interpose();
Notify_value	quit_signal();
void	repaint(), resize();

/* 
 *  pixrects
 */
static short grid_array[] = {
	0x0000,0x0000,0xFFE0,0x0000,0x0000,0xFFE0,0x0000,0x0000,
	0xFFE0,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};
mpr_static(grid_pr, 11, 11, 1, grid_array);

static short icon_image[] ={
#include <images/traffic.icon>
};
DEFINE_ICON_FROM_IMAGE(traffic_icon, icon_image);

#ifdef STANDALONE
main(argc, argv)
#else
int traffic_main(argc, argv)
#endif
	int argc;
	char *argv[];
{
	int i, err;
	struct addrmask am;
	char **tool_attrs = NULL;

	/* 
	 * Initialize variables
	 */
	trf_allocate();
	gethostname(host, sizeof(host));
	for (i = 0; i < MAXSPLIT; i++) {
		timeout[i].it_interval.tv_sec = INITSPEED/10;
		timeout[i].it_interval.tv_usec = (INITSPEED%10) * 100000;
		timeout[i].it_value.tv_sec = INITSPEED/10;
		timeout[i].it_value.tv_usec = (INITSPEED%10) * 100000;
		timeout1[i] = INITSPEED1;
		mode[i] = INITMODE;
		maxabs[i] = INITMAXABS;
	}
	
	argv++;
	argc--;
	while (argc > 0) {
		if (argv[0][0] == '-') {
			switch(argv[0][1]) {
				case 'h':
					if (argc < 2)
						traf_usage();
					strcpy(host, argv[1]);
					argc--;
					argv++;
					break;
				case 's':
					if (argc < 2)
						traf_usage();
					splitcnt = atoi(argv[1]) - 1;
					argc--;
					argv++;
					break;
				case 'd':
					if (argc < 2)
						traf_usage();
					debug = atoi(argv[1]);
					argc--;
					argv++;
					break;
				default:
					if ((i = tool_parse_one(argc, argv,
					    &tool_attrs, "sched")) == -1) {
						tool_usage("traffic");
						EXIT(1);
					} else if (i == 0)
						traf_usage();
					argv += i;
					argc -= i;
					continue;
			}
		}
		argc--;
		argv++;
	}

	/* 
	 * Get fonts
	 */
	fonts[0] = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (fonts[0] == NULL) {
		fprintf(stderr, "Couldn't get little font");
		EXIT(1);
	}
	fonts[1] = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.11");
	if (fonts[1] == NULL) {
		fprintf(stderr, "Couldn't get medium font");
		EXIT(1);
	}
	fonts[2] = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.14");
	if (fonts[2] == NULL) {
		fprintf(stderr, "Couldn't get big font");
		EXIT(1);
	}
	/* 
	 * for now, make curfontht the max possible,
	 * curfontwd the min possible
	 */
	curfontht = fonts[2]->pf_defaultsize.y;
	marginfontwd = fonts[0]->pf_defaultsize.x;
	marginfontht = fonts[0]->pf_defaultsize.y;

	/* 
	 * Create Tool
	 */
	frame = window_create(NULL, FRAME,
		ATTR_LIST,	tool_attrs,
		FRAME_LABEL, "traffic",
		FRAME_ICON, &traffic_icon,
		0);

	if (frame == NULL) {
		fputs("Can't make tool\n", stderr); EXIT(1);
	}
	tool_free_attribute_list(tool_attrs);
	toolrect = *(struct rect *)window_get(frame, WIN_RECT);
	prevclosed = (int)window_get(frame, FRAME_CLOSED);
	/*
	 * create subwindows
	 */
	toppanel = window_create(frame, PANEL, 0);
	if (toppanel == NULL) {
		fputs("Can't make panel\n", stderr); EXIT(1);
	}
	init_toppanel(toppanel);
	for (i = 0; i <= splitcnt; i++)
		makesubwindows(i);
	placesubwindows();

	if ((err = initdevice()) != 0) {
		fprintf(stderr, "Can't contact rpc.etherd running on %s.\n",
		    host);
		clnt_perrno(err);
		fprintf(stderr, "\n");
		EXIT(1);
	}
	am.a_mask = 0;
	setmask(am, ETHERSTATPROC_SELECTDST);
	setmask(am, ETHERSTATPROC_SELECTSRC);
	setmask(am, ETHERSTATPROC_SELECTPROTO);
	setmask(am, ETHERSTATPROC_SELECTLNTH);
	initprototable();

	for (i = 0; i <= splitcnt; i++)
		/* using canvas as a client handle */
		notify_set_itimer_func(canvas[i], timeout_notify, ITIMER_REAL,
		    &timeout[i], 0);

	/* catch going from open to close */
	notify_interpose_event_func(frame, frame_interpose, NOTIFY_SAFE);

	/* 
	 * deinit on SIGTERM or SIGINT
	 */
	notify_set_signal_func(&client, quit_signal,
	    SIGTERM, NOTIFY_ASYNC);
	notify_set_signal_func(&client, quit_signal,
	    SIGINT, NOTIFY_ASYNC);

	window_main_loop(frame);
	deinitdevice();
	
	EXIT(0);
}

/* ARGSUSED */
Notify_value
frame_interpose(clnt, event, arg, when)
	Notify_client clnt;
	Event *event;
	Notify_arg arg;
	Notify_event_type when;
{
	int closed;
	Notify_value rc;

	rc = notify_next_event_func(clnt, event, arg, NOTIFY_SAFE);
	if ((event_action(event) == WIN_REPAINT)) {
		closed = (int)window_get(clnt, FRAME_CLOSED);
		/*
		 * check to see if it was closed, as opposed to a
		 * repaint pevent
		 */		
		if (closed && !prevclosed) {
			deinitdevice();
		}
		if (!closed && prevclosed) {
			initdevice();
		}
		prevclosed = closed;
	}
	return (rc);
}

/* ARGSUSED */
Notify_value
timeout_notify(clnt, which)
	Notify_client	clnt;
	int	which;
{
	int	i;
	
	i = canvastoint(clnt);
	if (i > splitcnt) {
		fprintf(stderr, "got i that was too big\n");
	}
	else {
		draw(i);
	}
	
}

/* ARGSUSED */
Notify_value
quit_signal(clnt, sig)
	Notify_client clnt;
	int sig;
{
	deinitdevice();
	exit(0);
}

/* ARGSUSED */
void
repaint(canv, pw, repaint_area)
	Canvas	canv;
	Pixwin	*pw;
	Rectlist *repaint_area;
{
	traf_board_init(canvastoint(canv));
}

/* ARGSUSED */
void
resize(canv, wd, ht)
	Canvas	canv;
{
	tswrect[canvastoint(canv)] = *(struct rect *)window_get(canv,WIN_RECT);
}

canvastoint(canv)
	Canvas	canv;
{
	int	i;
	
	for (i = 0; i < MAXSPLIT; i++) {
		if (canvas[i] == canv) {
			return(i);
		}
	}
	fprintf(stderr, "traffic: help! unknown canvas\n");
	return(-1);
}

traf_board_init(i)
{
	static int oldmode[MAXSPLIT];
	int speed, setspeed;
	char buf[20];

	if (i > splitcnt)
		return;
	/* XXX shouldn't use 1000 */
	pw_writebackground(pixwin[i], 0, 0, 1000, 1000, PIX_CLR);
	traf_clearstate(i);
	if (mode[i] == DISPLAY_SIZE)
		drawsizetext(i);
	else if (mode[i] == DISPLAY_PROTO)
		drawprototext(i);
	if (mode[i] == DISPLAY_LOAD) {
		curht[i] = tswrect[i].r_height - TOPGAP;
		curht[i] = (curht[i]/10)*10;
	} else {
		curht[i] = tswrect[i].r_height - curfontht - TOPGAP;
		curht[i] = (curht[i]/10)*10;
	}
	setspeed = 0;
	if (mode[i] == DISPLAY_LOAD) {
		/* 
		 * if changing to load, set speed to .1 sec
		 */
		if (oldmode[i] != DISPLAY_LOAD) {
			setspeed = 1;
			speed = 1;
		}
		panel_set(speed_item1[i], PANEL_SHOW_ITEM, 0, 0);
		panel_set(speed_item[i], PANEL_SHOW_ITEM, 1, 0);
		panel_set_value(slider_item[i], 0);
		panel_set_value(scale_item[i], 0);
		traf_absolute[i] = 0;
		panel_set(slider_item[i], PANEL_SHOW_ITEM, 0, 0);
		panel_set(scale_item[i], PANEL_SHOW_ITEM, 0, 0);
	}
	if (oldmode[i] == DISPLAY_LOAD && mode[i] != DISPLAY_LOAD) {
		/* 
		 * if it was measuring load, speed was probably .1 sec
		 * so turn it up to something reasonable
		 */
		setspeed = 1;
		speed = INITSPEED;
		panel_set(slider_item[i], PANEL_SHOW_ITEM, 1, 0);
		panel_set(scale_item[i], PANEL_SHOW_ITEM, 1, 0);
	}
	oldmode[i] = mode[i];
	if (setspeed) {
		timeout[i].it_interval.tv_sec = speed/10;
		timeout[i].it_interval.tv_usec = (speed%10) * 100000;
		timeout[i].it_value.tv_sec = speed/10;
		timeout[i].it_value.tv_usec = (speed%10) * 100000;
		notify_set_itimer_func(canvas[i], timeout_notify, ITIMER_REAL,
		    &timeout[i], 0);
		sprintf(buf, "  %d.%01d secs", speed/10, speed%10);
		panel_set(speedvalue_item[i], PANEL_LABEL_STRING, buf, 0);
		panel_set_value(speed_item[i], speed);
	}
	drawleftmargin(i);
	if (gridon[i])
		drawgrid(i);
	draw(i);
}

init_vertpanel(i, panel)
	Panel panel;
{
	mode_item[i] = panel_create_item(panel, PANEL_CHOICE,
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_FEEDBACK, PANEL_MARKED,
		PANEL_LABEL_STRING, "Display", 
		PANEL_VALUE, INITMODE,
		PANEL_CHOICE_STRINGS, "Load", "Size", "Proto", "Src", "Dst", 0,
		PANEL_NOTIFY_PROC, mode_choice, 0);

	window_fit_width(panel);
}

init_horizpanel(i, panel)
	Panel panel;
{
	char buf[20];

	slider_item[i] = panel_create_item(panel, PANEL_CYCLE,
		PANEL_CHOICE_YS,    ATTR_ROW(0) + 4, 0,
		PANEL_CHOICE_IMAGES, proof_pr, proof_pr1, 0,
		PANEL_NOTIFY_PROC, slider_cycle, 0);

	sprintf(buf, "%d.%01d secs", INITSPEED/10, INITSPEED%10);
	speedvalue_item[i] = panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, buf, 0);
	
	speed_item[i] = panel_create_item(panel, PANEL_SLIDER,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, 100,/* tenths of a sec */
		PANEL_SHOW_ITEM, 1,
		PANEL_VALUE, INITSPEED,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_PROC, speed_slider, 0);

	scale_item[i] = panel_create_item(panel, PANEL_CYCLE,
		PANEL_CHOICE_YS,    ATTR_ROW(0) + 4, 0,
		PANEL_CHOICE_STRINGS, "Rel", "Abs", 0,
		PANEL_NOTIFY_PROC, scale_cycle, 0);

	grid_item[i] = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ATTR_ROW(0),	/* kludge! */
		PANEL_CHOICE_IMAGES, &grid_pr, 0,
		PANEL_NOTIFY_PROC, grid_toggle, 0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Delete Me", 0, 0),
		PANEL_NOTIFY_PROC, delete_button, 0);

	speed_item1[i] = panel_create_item(panel, PANEL_SLIDER,
		PANEL_MIN_VALUE, 10,
		PANEL_MAX_VALUE, 300,/* secs */
		PANEL_SHOW_ITEM, 0,
		PANEL_VALUE, INITSPEED1,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_ITEM_X, panel_get(speed_item[i], PANEL_ITEM_X),
		PANEL_ITEM_Y, panel_get(speed_item[i], PANEL_ITEM_Y),
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_PROC, speed1_slider, 0);

	window_fit(panel);
}

init_toppanel(panel)
	Panel panel;
{
	Panel_item split_item;
	
	srctoggle_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ATTR_ROW(0),	/* kludge! */
		PANEL_CHOICE_STRINGS, "Filter", 0,
		PANEL_NOTIFY_PROC, src_toggle, 0);

	src_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_Y, ATTR_ROW(0) + 5,	/* kludge! */
		PANEL_LABEL_STRING, "Src:",
		PANEL_VALUE_DISPLAY_LENGTH, 12, 
		PANEL_NOTIFY_PROC, src_text, 0);

	dsttoggle_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ATTR_ROW(0),	/* kludge! */
		PANEL_CHOICE_STRINGS, "Filter", 0,
		PANEL_NOTIFY_PROC, dst_toggle, 0);

	dst_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_Y, ATTR_ROW(0) + 5,	/* kludge! */
		PANEL_LABEL_STRING, "Dst:",
		PANEL_VALUE_DISPLAY_LENGTH, 12, 
		PANEL_NOTIFY_PROC, dst_text, 0);

	split_item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0) + 3,	/* kludge! */
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Split", 0, 0),
		PANEL_NOTIFY_PROC, split_button, 0);

	quit_item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_ITEM_Y, ATTR_ROW(0) + 3,	/* kludge! */
		PANEL_LABEL_IMAGE, panel_button_image(panel,
		    "Quit", 0, 0),
		PANEL_NOTIFY_PROC, quit_button, 0);

	prototoggle_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1) + 10,
		PANEL_CHOICE_STRINGS, "Filter", 0,
		PANEL_NOTIFY_PROC, proto_toggle, 0);

	proto_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_Y, ATTR_ROW(1) + 15,	/* kludge! */
		PANEL_LABEL_STRING, "Proto:",
		PANEL_VALUE_DISPLAY_LENGTH, 12, 
		PANEL_NOTIFY_PROC, proto_text, 0);

	lnthtoggle_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ATTR_ROW(1) + 10,	/* kludge! */
		PANEL_ITEM_X, panel_get(dsttoggle_item, PANEL_ITEM_X),
		PANEL_CHOICE_STRINGS, "Filter", 0,
		PANEL_NOTIFY_PROC, lnth_toggle, 0);

	lnth_item = panel_create_item(panel, PANEL_TEXT,
		PANEL_ITEM_Y, ATTR_ROW(1) + 15,	/* kludge! */
		PANEL_LABEL_STRING, "Lnth:",
		PANEL_VALUE_DISPLAY_LENGTH, 12, 
		PANEL_NOTIFY_PROC, lnth_text, 0);

	nopkts_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_ITEM_Y, ATTR_ROW(1) + 13,	/* kludge! */
		PANEL_ITEM_X, panel_get(split_item, PANEL_ITEM_X),
		PANEL_SHOW_ITEM, 0,
		PANEL_PAINT, PANEL_NO_CLEAR,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_CHOICE_IMAGES, panel_button_image(panel,
		    "no packets rcved", 0, 0), 0, 0);

	window_fit_height(panel);
}

nopkts(x)
{
	static int show = 0;
	
	if (x) {
		panel_set(nopkts_item, PANEL_SHOW_ITEM, x, 0);
		panel_set_value(nopkts_item, 0);
		panel_set_value(nopkts_item, 1);
		show = 1;
	}
	else if (!x && show) {
		panel_set(nopkts_item, PANEL_SHOW_ITEM, x, 0);
		show = 0;
	}
}

/* ARGSUSED */
Panel_setting
src_text(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char *addrstr;
	struct addrmask amask;
	int ok;

	addrstr = panel_get_value(src_item);
	if (addrstr && addrstr[0])
		ok = traf_addrtomask(&amask, addrstr);
	else {
		amask.a_mask = 0;
		ok = 0;
	}
	setmask(amask, ETHERSTATPROC_SELECTSRC);
	if (ok)
		panel_set_value(srctoggle_item, 1);
	else
		panel_set_value(srctoggle_item, 0);
	return (PANEL_NONE);
}

/* ARGSUSED */
src_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	struct addrmask amask;
	char *addrstr;
	int ok;

	if (value == 1) {
		addrstr = panel_get_value(src_item);
		if (addrstr && addrstr[0])
			ok = traf_addrtomask(&amask, addrstr);
		else {
			amask.a_mask = 0;
			ok = 0;
		}
		if (!ok)
			panel_set_value(srctoggle_item, 0);
	}
	else
		amask.a_mask = 0;
	setmask(amask, ETHERSTATPROC_SELECTSRC);
}

/* ARGSUSED */
Panel_setting
dst_text(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char *addrstr;
	struct addrmask amask;
	int ok;

	addrstr = panel_get_value(dst_item);
	if (addrstr && addrstr[0])
		ok = traf_addrtomask(&amask, addrstr);
	else {
		amask.a_mask = 0;
		ok = 0;
	}
	setmask(amask, ETHERSTATPROC_SELECTDST);
	if (ok)
		panel_set_value(dsttoggle_item, 1);
	else
		panel_set_value(dsttoggle_item, 0);
	return (PANEL_NONE);
}

/* ARGSUSED */
dst_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	struct addrmask amask;
	char *addrstr;
	int ok;

	if (value == 1) {
		addrstr = panel_get_value(dst_item);
		if (addrstr && addrstr[0])
			ok = traf_addrtomask(&amask, addrstr);
		else {
			amask.a_mask = 0;
			ok = 0;
		}
		if (!ok)
			panel_set_value(dsttoggle_item, 0);
	}
	else
		amask.a_mask = 0;
	setmask(amask, ETHERSTATPROC_SELECTDST);
}

/* ARGSUSED */
Panel_setting
proto_text(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char *protostr;
	struct addrmask amask;
	int ok;

	protostr = panel_get_value(proto_item);
	if (protostr && protostr[0])
		ok = prototomask(&amask, protostr);
	else {
		amask.a_mask = 0;
		ok = 0;
	}
	setmask(amask, ETHERSTATPROC_SELECTPROTO);
	if (ok)
		panel_set_value(prototoggle_item, 1);
	else
		panel_set_value(prototoggle_item, 0);
	return (PANEL_NONE);
}

/* ARGSUSED */
proto_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	struct addrmask amask;
	char *protostr;
	int ok;

	if (value == 1) {
		protostr = panel_get_value(proto_item);
		if (protostr && protostr[0])
			ok = prototomask(&amask, protostr);
		else {
			amask.a_mask = 0;
			ok = 0;
		}
		if (!ok)
			panel_set_value(prototoggle_item, 0);
	}
	else
		amask.a_mask = 0;
	setmask(amask, ETHERSTATPROC_SELECTPROTO);
}

/* ARGSUSED */
Panel_setting
lnth_text(item, event)
	Panel_item item;
	struct inputevent *event;
{
	char *lnthstr;
	struct addrmask amask;
	int ok;

	lnthstr = panel_get_value(lnth_item);
	if (lnthstr && lnthstr[0])
		ok = lnthtomask(&amask, lnthstr);
	else {
		amask.a_mask = 0;
		ok = 0;
	}
	setmask(amask, ETHERSTATPROC_SELECTLNTH);
	if (ok)
		panel_set_value(lnthtoggle_item, 1);
	else
		panel_set_value(lnthtoggle_item, 0);
	return (PANEL_NONE);
}

/* ARGSUSED */
lnth_toggle(item, value, event)
	Panel_item item;
	struct inputevent *event;
{
	struct addrmask amask;
	char *lnthstr;
	int ok;

	if (value == 1) {
		lnthstr = panel_get_value(lnth_item);
		if (lnthstr && lnthstr[0])
			ok = lnthtomask(&amask, lnthstr);
		else {
			amask.a_mask = 0;
			ok = 0;
		}
		if (!ok)
			panel_set_value(lnthtoggle_item, 0);
	}
	else
		amask.a_mask = 0;
	setmask(amask, ETHERSTATPROC_SELECTLNTH);
}

makesubwindows(i)
{
	if (horizpanel[i])
		return;

	horizpanel[i] = window_create(frame, PANEL, 
		WIN_X, 0,
		WIN_BELOW, i ? canvas[i-1] : toppanel,
		PANEL_ITEM_X_GAP, 26,  /* experimentally determined constant */
		0);
	if (horizpanel[i] == NULL)
		fputs("Can't make panel\n", stderr), exit(1);
	init_horizpanel(i, horizpanel[i]);

	vertpanel[i] = window_create(frame, PANEL,
			WIN_BELOW, i? vertpanel[i-1] : toppanel,
			WIN_RIGHT_OF, horizpanel[i],
		 0);

	if (vertpanel[i] == NULL)
		fputs("Can't make panel\n", stderr), exit(1);
	init_vertpanel(i, vertpanel[i]);
	
	canvas[i] = window_create(frame, CANVAS,
		CANVAS_RETAINED, 0,
		CANVAS_FIXED_IMAGE, 0,
		CANVAS_REPAINT_PROC, repaint,
		CANVAS_RESIZE_PROC, resize,
		WIN_X, 0,
		WIN_BELOW, horizpanel[i],
		0);

	if (canvas[i] == NULL)
		fputs("Can't create subwindow\n", stderr), exit(1);
	pixwin[i] = (Pixwin *)window_get(canvas[i], CANVAS_PIXWIN);
}

traf_usage()
{
	fprintf(stderr,
	    "Usage: traffic [-h host] [-s subwindowcnt]\n");
	exit(1);
}
