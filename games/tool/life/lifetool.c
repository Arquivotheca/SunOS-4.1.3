#ifndef lint
static  char sccsid[] = "@(#)lifetool.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#include <sunwindow/cms.h>
#include <suntool/walkmenu.h>
#include "life.h"

#define abs(x) (((x) > 0) ? (x) : -(x))
#ifndef max
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif

#define ROWPAD 10		/* extra space between rows in panel */

/* tool and sunwindows-specific data */
static	Frame	frame;
static	Panel	panel;
static	Canvas	canvas;
static struct pixwin		*board_pixwin;
static int			gencnt;
static	Panel_item mode_item, zoom_item, gen_item, speed_item, grid_item;

static	mode_choice(), grid_toggle(), speed_slider(),  zoom_slider();
int	cmdpanel_event();
int	clear_button(), quit_button(), reset_button(), find_button();
static	board_repaint(), board_event();
Notify_value clock_notify();

struct pixrect *mem_create();
int life_spacing;			/* distance between 2 lines */
static	struct pixrect *piecepix;

static short grid_array[] = {
	0x2480,0x2480,0xFFE0,0x2480,0x2480,0xFFE0,0x2480,0x2480,
	0xFFE0,0x2480,0x2480,0x0000,0x0000,0x0000,0x0000,0x0000
};
mpr_static(grid_pr, 11, 11, 1, grid_array);

static	int	runmode;		/* am I in run mode? */
static	int	gridon;			/* is grid visible? */
static	int	speed = 1;
static	int	client;
int	leftoffset, upoffset;

static short icon_image[] ={
#include <images/life.icon>
};
DEFINE_ICON_FROM_IMAGE(life_icon, icon_image);

#define MAXLENGTH 64
extern char
circlearr[MAXLENGTH][MAXLENGTH]; /* where drawcircle puts its ouput */
#define SPACING 15		/* diameter = 12 */
static	int ozoom = SPACING;
int amicolor;
static	int toolfd;
static	Menu	panelmenu;
static	Menu	menu;
static	Scrollbar vertsb, horizsb;

static	char cmsname[CMS_NAMESIZE];
static	u_char	red[CMSIZE];
static	u_char	green[CMSIZE];
static	u_char	blue[CMSIZE];
static char tool_name[] = "life";

/* command line args */
static	int	genlim;
static	int	initpattern;

FILE *debugfp;

#ifdef STANDALONE
main(argc, argv)
#else
int life_main(argc, argv)
#endif
	char **argv;
{
	int k;
	struct itimerval nextupdate;

	frame = window_create(NULL, FRAME,
		FRAME_ARGC_PTR_ARGV, &argc, argv,
		FRAME_LABEL, tool_name,
		FRAME_ICON, &life_icon,
		0);
	if (frame == 0) {
		fprintf(stderr, "life: couldn't create window\n");
		exit(1);
	}
	toolfd = (int)window_get(frame, WIN_FD);
	life_spacing = SPACING;
	debugfp = stdout;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			if (argc < 3)
				life_usage();
			switch(argv[1][1]) {
			case 'p': /* pattern */
				initpattern = atoi(argv[2]);
				break;
			case 'm': /* pattern */
				runmode = atoi(argv[2]);
				break;
			case 'g': /* generations */
				genlim = atoi(argv[2]);
				break;
			case 'i': /* interval */
				speed = atoi(argv[2]);
				if (speed < 1 || speed > 200) {
					fprintf(stderr, "%d Illegal speed\n",
					    atoi(argv[2]));
					exit(1);
				}
				break;
			case 's': /* spacing */
				life_spacing = atoi(argv[2]);
				if (life_spacing < 4 || life_spacing > 40) {
					fprintf(stderr, "%d Illegal spacing\n",
					    atoi(argv[2]));
					exit(1);
				}
				break;
			default:
				life_usage();
			}
			argc--;
			argv++;
		} else
			life_usage();
		argc--;
		argv++;
	}

	/* create and initialize the option subwindow */
	panel = window_create(frame, PANEL, 0);
	init_panel();

	/* create and initialize the board subwindow */
	vertsb = scrollbar_create(0);
	horizsb = scrollbar_create(0);
	canvas = window_create(frame, CANVAS,
		CANVAS_AUTO_SHRINK, 0,
		CANVAS_RETAINED, 0,
		CANVAS_FIXED_IMAGE, 0,
		CANVAS_REPAINT_PROC, board_repaint,
		WIN_VERTICAL_SCROLLBAR, vertsb,
		WIN_HORIZONTAL_SCROLLBAR, horizsb,
		WIN_EVENT_PROC, board_event,
		CANVAS_HEIGHT, 2*CANVAS_ORIGIN,
		CANVAS_WIDTH, 2*CANVAS_ORIGIN,
		0);
	scrollbar_scroll_to(vertsb, CANVAS_ORIGIN);
	scrollbar_scroll_to(horizsb, CANVAS_ORIGIN);
	board_pixwin = (Pixwin *)window_get(canvas, CANVAS_PIXWIN);

	menu =  menu_create(
		MENU_STRING_ITEM, "Glider", (char *)GLIDER,
		MENU_STRING_ITEM, "8", (char *)EIGHT,
		MENU_STRING_ITEM, "Pulsar", (char *)PULSAR,
		MENU_STRING_ITEM, "Gun", (char *)GUN,
		MENU_STRING_ITEM, "Escort", (char *)ESCORT,
		MENU_STRING_ITEM, "Barber", (char *)BARBER,
		MENU_STRING_ITEM, "Puffer", (char *)PUFFER,
		MENU_STRING_ITEM, "Hertz", (char *)HERTZ,
		MENU_STRING_ITEM, "Tumbler", (char *)TUMBLER,
		MENU_STRING_ITEM, "Muchnick", (char *)MUCHNICK,
            0);
             
	circle();

	/* set up color map */
	amicolor = board_pixwin->pw_pixrect->pr_depth > 1;
	if (amicolor) {
		sprintf(cmsname, "lifetool%d", getpid());
		pw_setcmsname(board_pixwin, cmsname);
		red[0] = 255;
		blue[0] = 255;
		green[0] = 255;
		red[CMSIZE-1] = 0;
		blue[CMSIZE-1] = 0;
		green[CMSIZE-1] = 0;
		for(k = 1; k <= CMSIZE-1; k++) {
			red[k] = (k-1)*(256/CMSIZE);
			green[k] = 0;
			blue[k] = 255 - red[k];
		}
		pw_putcolormap(board_pixwin, 0, CMSIZE, red, green, blue);
	}
	loaditimerval(speed, &nextupdate);
	notify_set_itimer_func(&client, clock_notify, ITIMER_REAL,
	    &nextupdate, 0);
	if (initpattern)
		showit(initpattern);
	initstack();
	
	window_main_loop(frame);
}

static
init_panel()
{
	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, " ", 0);

	grid_item = panel_create_item(panel, PANEL_TOGGLE,
		PANEL_CHOICE_IMAGES, &grid_pr, 0,
		PANEL_NOTIFY_PROC, grid_toggle, 0);
	setupmenu(grid_item, "Turn Grid On");

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "   ", 0);

	mode_item = panel_create_item(panel, PANEL_CHOICE,
		PANEL_ITEM_Y, ATTR_ROW(0) - 3,
		PANEL_VALUE, !runmode,
		PANEL_CHOICE_STRINGS, "Run", "Step", 0,
		PANEL_NOTIFY_PROC, mode_choice, 0);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ATTR_ROW(0) + 2,
		PANEL_LABEL_STRING, "    Gen", 0);

	gen_item = panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ATTR_ROW(0) + 2,
		PANEL_LABEL_STRING, "     0", 0);
	
	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "    ", 0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Find", 0, 0),
		PANEL_NOTIFY_PROC, find_button, 0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Clear", 0, 0),
		PANEL_NOTIFY_PROC, clear_button, 0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Reset", 0, 0),
		PANEL_NOTIFY_PROC, reset_button, 0);

	panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(panel, "Quit", 0, 0),
		PANEL_NOTIFY_PROC, quit_button, 0);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "Fast",
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD, 0);
	
	speed_item = panel_create_item(panel, PANEL_SLIDER,
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD,
		PANEL_MIN_VALUE, 1,
		PANEL_MAX_VALUE, 200,/* hundredths of a sec */
		PANEL_VALUE, speed,
		PANEL_NOTIFY_LEVEL, PANEL_DONE,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_PROC, speed_slider, 0);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD,
		PANEL_LABEL_STRING, "Slow", 0);
	
	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD,
		PANEL_LABEL_STRING, "     Zoom Out", 0);
	
	zoom_item = panel_create_item(panel, PANEL_SLIDER,
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD,
		PANEL_MIN_VALUE, 4, /* minimum that circle() can deal with */
		PANEL_MAX_VALUE, 40,
		PANEL_VALUE, life_spacing,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_LEVEL, PANEL_DONE,
		PANEL_NOTIFY_PROC, zoom_slider, 0);

	panel_create_item(panel, PANEL_MESSAGE,
		PANEL_ITEM_Y, ATTR_ROW(1) + ROWPAD,
		PANEL_LABEL_STRING, "Zoom In", 0);

	window_fit_height(panel);	
}

lock()
{
	struct rect rect;

	rect_construct(&rect, 0, 0, 2*CANVAS_ORIGIN, 2*CANVAS_ORIGIN);
	pw_lock(board_pixwin, &rect);
}

unlock()
{
	pw_unlock(board_pixwin);
}

/* VARARGS */
/* ARGSUSED */
/* also called from zoom_slider */
static
board_repaint(canv, pw, repaint_area)
	Canvas canv;
	Pixwin *pw;
	Rectlist *repaint_area;
{
	pw_writebackground(board_pixwin, 0, 0, 2*CANVAS_ORIGIN,
	    2*CANVAS_ORIGIN, PIX_CLR);
	if (gridon)
		drawgrid();
	paint_board();
}

drawgrid()
{
	int	j;
	
	lock();
	for (j = 0; j <= 2*CANVAS_ORIGIN; j+=life_spacing)
		pw_vector(board_pixwin, 0, j, 2*CANVAS_ORIGIN, j,
		    PIX_SRC^PIX_DST, 1);
	for (j = 0; j <= 2*CANVAS_ORIGIN; j+=life_spacing)
		pw_vector(board_pixwin, j, 0, j, 2*CANVAS_ORIGIN,
		    PIX_SRC^PIX_DST, 1);
	unlock();
}

/* VARARGS */
/* ARGSUSED */
/* also called from reset_button */
static
grid_toggle(item, value, event)
	Panel_item item;
	struct inputevent event;
{
	static Menu_item menu_on, menu_off;
	
	if (menu_on == NULL) {
		menu_on = menu_create_item(MENU_STRING, "Turn Grid On", 0);
		menu_off = menu_create_item(MENU_STRING, "Turn Grid Off", 0);
	}
	gridon ^= 1;
	if (gridon)
		menu_set(panelmenu, MENU_REPLACE, 1, menu_off, 0);
	else
		menu_set(panelmenu, MENU_REPLACE, 1, menu_on, 0);
	panel_set_value(grid_item, gridon);
	drawgrid();
}

/* ARGSUSED */
static
mode_choice(item, value, event)
	Panel_item item;
	struct inputevent event;
{
	struct itimerval nextupdate;

	if (value == 0) {
		loaditimerval(speed, &nextupdate);
		notify_set_itimer_func(&client, clock_notify, ITIMER_REAL,
		    &nextupdate, 0);
		runmode = 1;
	}
	else {
		runmode = 0;
		newgen();
		updategen();
		loaditimerval(0, &nextupdate);
		notify_set_itimer_func(&client, clock_notify, ITIMER_REAL,
		    &nextupdate, 0);
	}
}

/* ARGSUSED */
static
speed_slider(item, value, event)
	Panel_item item;
	int value;
	Event	*event;
{
	struct itimerval nextupdate;

	speed = value;
	loaditimerval(speed, &nextupdate);
	notify_set_itimer_func(&client, clock_notify, ITIMER_REAL,
	    &nextupdate, 0);
}

/* ARGSUSED */
static
zoom_slider(item, zoom, event)
	Panel_item item;
	int zoom;
	Event	*event;
{
	int	off, ht, wd;
	
	life_spacing = zoom;
	canvassize(&ht, &wd);
	off = getvoff() + ht/2;
	upoffset += (off/zoom) - (off/ozoom);
	off = gethoff() + wd/2;
	leftoffset += (off/zoom) - (off/ozoom);
	circle();
	board_repaint();
	ozoom = zoom;
}

/* ARGSUSED */
quit_button(item, event)
	Panel_item item;
	struct inputevent *event;
{
		fullscreen_prompt("\
Press the left mouse button\n\
to confirm Quit.  To cancel,\n\
press the right mouse button.", "Really Quit", "Cancel Quit", event, toolfd);
	if (event_id(event) == MS_LEFT)
		exit(0);
}

/* ARGSUSED */
clear_button(item, event)
	Panel_item	item;
	Event		*event;
{
	zerolist();
	runmode = 0;
	gencnt = 0;
	panel_set_value(mode_item, 1);
	panel_paint(mode_item, PANEL_CLEAR);
	panel_set(gen_item, PANEL_LABEL_STRING, "     0", 0);
	board_repaint();
}

/* ARGSUSED */
find_button(item, event)
	Panel_item	item;
	Event		*event;
{
	int	x, y, ht, wd;
	
	if (life_getpoint(&x, &y) == 0)
		return;
	canvassize(&ht, &wd);
	scrollbar_scroll_to(vertsb, (y+upoffset)*life_spacing - ht/2);
	scrollbar_scroll_to(horizsb, (x+leftoffset)*life_spacing - wd/2);
}

/* ARGSUSED */
reset_button(item, event)
	Panel_item	item;
	Event		*event;
{
	Panel_item	dummyitem;
	Event		*dummyevent;

	if (gridon)
		grid_toggle();	
	runmode = 0;
	gencnt = 0;
	panel_set_value(mode_item, 1);
	panel_paint(mode_item, PANEL_CLEAR);
	panel_set(gen_item, PANEL_LABEL_STRING, "     0", 0);
	speed_slider(dummyitem, 1, dummyevent);
	panel_set_value(speed_item, 1);
	zoom_slider(dummyitem, SPACING, dummyevent);
	panel_set_value(zoom_item, SPACING);
	leftoffset = 0;
	upoffset = 0;
	scrollbar_scroll_to(vertsb, CANVAS_ORIGIN);
	scrollbar_scroll_to(horizsb, CANVAS_ORIGIN);
}

static
board_event(canv, event)
	Canvas canv;
	Event *event;
{
	int	x, y;
	int	cur_x, cur_y;
	static int erasing, painting;

	if (event_is_down(event) && event_id(event) == MS_RIGHT) {
	    /* translate the event to window space,
	     * then show the menu.
	     */
	    showit(menu_show(menu, canv,
		canvas_window_event(canv, event), 0));
	    return;
	}
	
	x = event->ie_locx; y = event->ie_locy;
	cur_x = (x/life_spacing)*life_spacing;
	cur_y = (y/life_spacing)*life_spacing;
	if (event_is_up(event)) {
		erasing = 0;
		painting = 0;
		return;
	}

	if (event->ie_code == MS_MIDDLE ||
	    (event->ie_code == LOC_MOVE && erasing)) {
		erasing = 1;
		if (life_get(cur_x/life_spacing,cur_y/life_spacing)== 0)
			return;
		deletepoint(cur_x/life_spacing - leftoffset,
		    cur_y/life_spacing - upoffset);
		erase_stone(cur_x/life_spacing, cur_y/life_spacing);
	}
	else if (event->ie_code == MS_LEFT ||
	    (event->ie_code == LOC_MOVE && painting)) {
		painting = 1;
		addpoint(cur_x/life_spacing - leftoffset,
		    cur_y/life_spacing - upoffset);
		paint_stone(cur_x/life_spacing, cur_y/life_spacing, INITCOLOR);
	}
}

paint_stone(i, j, color)
{ 
	int diameter;
	
	diameter = life_spacing - 3;
	if (amicolor)
		pw_write(board_pixwin, i*life_spacing+2, j*life_spacing+2, diameter,
		    diameter, (PIX_COLOR(color)|PIX_SRC), piecepix, 0, 0);
	else
		pw_write(board_pixwin, i*life_spacing+2, j*life_spacing+2, diameter,
		    diameter, PIX_SRC, piecepix, 0, 0);
} 

erase_stone(i, j)
{ 
	int diameter, z;
	
	diameter = life_spacing - 3;
	pw_writebackground(board_pixwin, i*life_spacing+2, j*life_spacing+2,
	    diameter, diameter, PIX_CLR);
} 


circle()
{
	int i,j, diameter;
	
	diameter = life_spacing - 3;
	if (diameter >= MAXLENGTH) {
	    fprintf(stderr, "help!\n");
	    exit(1);
	}
	drawcircle(diameter, 0, 1);

	if (piecepix != NULL)
		pr_destroy(piecepix);
	piecepix = mem_create(diameter, diameter, 1);
	for (j = 0; j < diameter; j++) {
		for (i = 0;  i < diameter;   i++)
			pr_put(piecepix, i, j, circlearr[i][j]);
	}
}

/* 
 * 1 sec timer: If in runmode, advance one cycle
 */
/* ARGSUSED */
static Notify_value
clock_notify(clnt, which)
	Notify_client clnt;
	int which;
{
	if (runmode) {
		newgen();
		updategen();
	}
	return (NOTIFY_DONE);
}

updategen()
{
	char buf[7];
	
	gencnt++;
	sprintf(buf, "%6d", gencnt);
	panel_set(gen_item, PANEL_LABEL_STRING, buf, 0);
	if (genlim && gencnt > genlim)
		exit(0);
}

loaditimerval(val, it)
	int	val;
	struct	itimerval *it;
{

	it->it_interval.tv_sec = val/100;
	it->it_interval.tv_usec = (val%100)*10000;
	it->it_value.tv_sec = val/100;
	it->it_value.tv_usec = (val%100)*10000;
}

setupmenu(item, str)
	Panel_item	item;
	char	*str;
{
	panelmenu = menu_create(MENU_CLIENT_DATA, item,
	    MENU_NOTIFY_PROC, menu_return_item, 
	    0);
	menu_set(panelmenu, MENU_STRING_ITEM, str, 0, 0);
	panel_set(item,
	    PANEL_EVENT_PROC, cmdpanel_event,
	    PANEL_CLIENT_DATA, panelmenu,
	    0);
}

/*
 * Handle input events when over panel items.
 * Menu request gives menu of possible options.
 */
cmdpanel_event(item, ie)
	Panel_item item;
	Event *ie;
{
	Menu_item mi;
	typedef int (*func)();
	func proc;

	if (event_id(ie) == MENU_BUT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(
		    (Menu)panel_get(item, PANEL_CLIENT_DATA), panel,
		  	panel_window_event(panel, ie) , 0);
		if (mi != NULL) {
			event_set_shiftmask(ie, (int)menu_get(mi, MENU_VALUE));
			proc = (func)panel_get(item, PANEL_NOTIFY_PROC);
			(*proc)(item, ie);
		}
	} else
		panel_default_handle_event(item, ie);
}

canvassize(htp, wdp)
	int	*htp, *wdp;
{
	*htp = (int)window_get(canvas, WIN_HEIGHT);
	*wdp = (int)window_get(canvas, WIN_WIDTH);
}

getvoff()
{
	return ((int)scrollbar_get(vertsb, SCROLL_VIEW_START));
}

gethoff()
{
	return ((int)scrollbar_get(horizsb, SCROLL_VIEW_START));
}

life_usage()
{
	fprintf(stderr,
    "life [-p pattern] [-g gens] [-i interval] [-m mode] [-s spacing]\n");
	exit(1);
}
