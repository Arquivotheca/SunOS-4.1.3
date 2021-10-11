#ifndef lint
static  char sccsid[] = "@(#)lifetool.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include <sunwindow/cms.h>
#include <suntool/tool_hs.h>
#include <suntool/optionsw.h>
#include <suntool/msgsw.h>
#include <suntool/menu.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include "life.h"

#define abs(x) (((x) > 0) ? (x) : -(x))
#ifndef max
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif
struct timeval tv;
extern struct pixfont	*pw_pfsysopen(), *pf_open();
struct pixfont	*littlefont;

/* tool and sunwindows-specific data */
static struct tool				*tool;
static struct toolsw		*msg_tsw, *opt_tsw, *board_tsw;
static struct pixwin		*board_pixwin;
static struct msgsubwindow	*mswerror;
static int 			newmessage;/* set when change mswerror */
static int			gencnt;
Panel_item mode_item, zoom_item, gen_item;
Panel osw;
Scrollbar leftscroll, upscroll;

static struct pixfont  *font;

int doscroll();
static	mode_proc(), grid_proc(), speed_proc(), move_proc(),
    sigwinched(), board_selected(), board_sighandler(), zoom_proc();
int reset_proc(), quit_proc();

static int clock_selected();
struct pixrect *mem_create();
int spacing;				/* distance between 2 lines */
struct pixrect *piecepix;
static short    tri_right_dat[] = {
	0xC000,0xF000,0xFC00, 0xFE00,0xFC00,0xF000,0xC000,0x0000,
};
mpr_static(tri_right, 16, 8, 1, tri_right_dat);

static short grid_array[] = {
	0x2480,0x2480,0xFFE0,0x2480,0x2480,0xFFE0,0x2480,0x2480,
	0xFFE0,0x2480,0x2480,0x0000,0x0000,0x0000,0x0000,0x0000
};
mpr_static(grid_pr, 11, 11, 1, grid_array);

int runmode;				/* am I in run mode? */
int gridon;				/* is grid visible? */
struct timeval nextupdate = {0, 100000};
static short icon_data[256]={
#include <images/life.icon>
};
mpr_static(base_mpr, 64, 64, 1, icon_data);
static	struct icon goicon = {64, 64, (struct pixrect *)0,
	    {0, 0, 64, 64}, &base_mpr,
	    {0, 0, 0, 0}, 0, (struct pixfont *)0, 0};

char board[NLINES][NLINES];
#define SCROLLWIDTH 18
#define MAXLENGTH 64
extern char
circlearr[MAXLENGTH][MAXLENGTH]; /* where drawcircle puts its ouput */
#define SPACING 15		/* diameter = 12 */

struct rect currect;
int rightedge;
int bottomedge;
int leftoffset;
int upoffset;
int ozoom = SPACING;
int color;

static struct menuitem menu_items[] = {
  {MENU_IMAGESTRING, "glider", (char *)GLIDER},
  {MENU_IMAGESTRING, "8", (char *)EIGHT},
  {MENU_IMAGESTRING, "pulsar", (char *)PULSAR},
  {MENU_IMAGESTRING, "gun", (char *)GUN},
  {MENU_IMAGESTRING, "escort", (char *)ESCORT},
  {MENU_IMAGESTRING, "barber", (char *)BARBER},
  {MENU_IMAGESTRING, "puffer", (char *)PUFFER},
  {MENU_IMAGESTRING, "hertz", (char *)HERTZ},
  {MENU_IMAGESTRING, "tumbler", (char *)TUMBLER},
  {MENU_IMAGESTRING, "muchnick", (char *)MUCHNICK},
  {MENU_IMAGESTRING, "reset", (char *)RESET},
  {MENU_IMAGESTRING, "run", (char *)RUN},
};

static struct menu menu_body = {
	MENU_IMAGESTRING, "patterns",
	sizeof(menu_items)/sizeof(struct menuitem), menu_items,
	NULL, NULL
};
static struct menu *menu_ptr = &menu_body;

char cmsname[CMS_NAMESIZE];
u_char	red[CMSIZE];
u_char	green[CMSIZE];
u_char	blue[CMSIZE];
static char tool_name[] = "Game of Life 2.0";

FILE *debugfp;

main(argc, argv)
	char **argv;
{
	char **tool_attrs = NULL;
	int k;

	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		tool_usage(tool_name);
		exit(1);
	}
	spacing = SPACING;
	debugfp = stdout;
	/*
	 * create the tool
	 */
	tool = tool_make(WIN_NAME_STRIPE, 1,
	    		 WIN_BOUNDARY_MGR, 1,
			 WIN_LABEL, tool_name,
			 WIN_ATTR_LIST,	tool_attrs,
			 WIN_ICON, &goicon, 0);
	if (tool == (struct tool *)NULL)  lose("Couldn't create tool");
	tool_free_attribute_list(tool_attrs);
	/*font = pw_pfsysopen(); */
	font = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.11");
	if (font == (struct pixfont *)NULL)
		lose("Couldn't get default font");

	littlefont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (littlefont == (struct pixfont *)NULL)
		lose("Couldn't get little font");

	/* create and initialize the message subwindow */
	msg_tsw = msgsw_createtoolsubwindow(tool, "", TOOL_SWEXTENDTOEDGE,
	    (font->pf_defaultsize.y * 3) / 2,
	    "Left paints", font);
	if (msg_tsw == (struct toolsw *)NULL)
		lose("Couldn't create message subwindow");
	mswerror = (struct msgsubwindow *)msg_tsw->ts_data;

	/* 
	 * picking msg_tsw for one second interrupt was an
	 * arbitrary choice
	 */
	msg_tsw->ts_io.tio_selected = clock_selected;
	msg_tsw->ts_io.tio_timer = &tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	/* create and initialize the option subwindow */
	opt_tsw = panel_create(tool,
		PANEL_HEIGHT, TOOL_SWEXTENDTOEDGE,
		PANEL_FONT, font,
		0);
	if (opt_tsw == (struct toolsw *)NULL)
		lose("Couldn't create option subwindow");
	init_options();

	/* create and initialize the board subwindow */
	board_tsw = tool_createsubwindow(tool, "",
			TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE);
	if (board_tsw == (struct toolsw *)NULL)
		lose("Couldn't create board subwindow");
	win_getsize(board_tsw->ts_windowfd, &currect);
	rightedge = (currect.r_width - SCROLLWIDTH)/spacing;
	bottomedge = (currect.r_height - SCROLLWIDTH)/spacing;

	circle();
	init_board();
	leftscroll = scrollbar_create(SCROLL_PIXWIN, board_pixwin,
		SCROLL_NOTIFY_PROC, doscroll,
		SCROLL_BAR_DISPLAY_LEVEL, SCROLL_ALWAYS, 
		SCROLL_BUBBLE_DISPLAY_LEVEL, SCROLL_ALWAYS,
		SCROLL_DIRECTION, SCROLL_HORIZONTAL,
		SCROLL_TOP, currect.r_height - SCROLLWIDTH,
		SCROLL_LEFT, 0,
		SCROLL_WIDTH, currect.r_width - SCROLLWIDTH,
		SCROLL_HEIGHT, SCROLLWIDTH,
		0);
	upscroll = scrollbar_create(SCROLL_PIXWIN, board_pixwin,
		SCROLL_NOTIFY_PROC, doscroll,
		SCROLL_BAR_DISPLAY_LEVEL, SCROLL_ALWAYS, 
		SCROLL_BUBBLE_DISPLAY_LEVEL, SCROLL_ALWAYS,
		SCROLL_DIRECTION, SCROLL_VERTICAL,
		SCROLL_TOP, 0,
		SCROLL_LEFT, currect.r_width - SCROLLWIDTH,
		SCROLL_WIDTH, SCROLLWIDTH,
		SCROLL_HEIGHT, currect.r_height - SCROLLWIDTH,
		0);
	board_init();
	signal(SIGWINCH, sigwinched);

	/* install tool */
	tool_install(tool);

	/* set up color map */
	color = amicolor();
	if (color) {
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

	tool_select(tool, 0);

	/* terminate tool */
	tool_destroy(tool);
	pw_pfsysclose();
	exit(0);
}

lose(str)
char	*str;
{	fprintf(stderr, str); exit(1);
}

static
sigwinched()
{
	tool_sigwinch(tool);
}

static
init_options()
{
	osw = (Panel)opt_tsw->ts_data;

	/* 
	 * create optsw items
	 */
	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, " ", 0);

	panel_create_item(osw, PANEL_TOGGLE,
		PANEL_FEEDBACK, PANEL_INVERTED,
		PANEL_CHOICE_IMAGES, &grid_pr, 0,
		PANEL_NOTIFY_PROC, grid_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "   ", 0);

	mode_item = panel_create_item(osw, PANEL_CHOICE,
		PANEL_FEEDBACK, PANEL_MARKED,
		PANEL_LABEL_STRING, "mode:", 
		PANEL_MARK_IMAGES, &tri_right,0,
		PANEL_NOMARK_IMAGES, 0,
		PANEL_VALUE, 1,
		PANEL_CHOICE_STRINGS, "run", "step", 0,
		PANEL_NOTIFY_PROC, mode_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "   ", 0);

	panel_create_item(osw, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(osw,
		    "reset", 0, littlefont),
		PANEL_NOTIFY_PROC, reset_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "    ", 0);

	panel_create_item(osw, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, panel_button_image(osw,
		    "quit", 0, littlefont),
		PANEL_NOTIFY_PROC, quit_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, " ", 0);

	gen_item = panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "     0", 0);
	
	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "fast",
		PANEL_ITEM_X, PANEL_CU(0),
		PANEL_ITEM_Y, PANEL_CU(1), 0);
	
	panel_create_item(osw, PANEL_SLIDER,
		PANEL_MIN_VALUE, 0,
		PANEL_MAX_VALUE, 200,/* hundredths of a sec */
		PANEL_NOTIFY_LEVEL, PANEL_DONE,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_PROC, speed_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "slow", 0);
	
	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "   zoom out", 0);
	
	zoom_item = panel_create_item(osw, PANEL_SLIDER,
		PANEL_MIN_VALUE, 4, /* minimum that circle() can deal with */
		PANEL_MAX_VALUE, 40,
		PANEL_VALUE, SPACING,
		PANEL_SHOW_VALUE, FALSE,
		PANEL_SHOW_RANGE, FALSE,
		PANEL_NOTIFY_LEVEL, PANEL_DONE,
		PANEL_NOTIFY_PROC, zoom_proc, 0);

	panel_create_item(osw, PANEL_MESSAGE,
		PANEL_LABEL_STRING, "zoom in", 0);
	
	panel_fit_height(osw);
}

static
init_board()
{
	struct inputmask  mask;
	register int i,j;

	for(i = 0; i < rightedge; i++)
	for(j = 0; j < bottomedge; j++)
		board[i][j] = 0;
	board_pixwin = pw_open(board_tsw->ts_windowfd);
	if (board_pixwin == (struct pixwin *)NULL)
		lose("Couldn't create board pixwin");
	board_tsw->ts_io.tio_handlesigwinch = board_sighandler;
	board_tsw->ts_io.tio_selected = board_selected;
	input_imnull(&mask);
	win_setinputcodebit(&mask, MS_LEFT);
	win_setinputcodebit(&mask, MS_MIDDLE);
	win_setinputcodebit(&mask, MENU_BUT);
	win_setinputcodebit(&mask, LOC_MOVE);
	win_setinputcodebit(&mask, LOC_MOVEWHILEBUTDOWN);
	mask.im_flags |= IM_NEGEVENT;
	win_setinputmask(board_tsw->ts_windowfd, &mask,
	    (struct inputmask *)NULL, WIN_NULLLINK);
}

lock()
{
	struct rect rect;

	rect = currect;
	rect.r_width = rightedge*spacing;
	rect.r_height = bottomedge*spacing;
	pw_lock(board_pixwin, &rect);
}

unlock()
{
	pw_unlock(board_pixwin);
}

static
board_init()
{
	register int i,j;

	pw_writebackground(board_pixwin, 0, 0, currect.r_width,
	    currect.r_height, PIX_CLR);
	scrollbar_set(leftscroll, SCROLL_TOP, currect.r_height - SCROLLWIDTH,
		SCROLL_WIDTH, currect.r_width - SCROLLWIDTH, 0);
	scrollbar_set(upscroll, SCROLL_LEFT, currect.r_width - SCROLLWIDTH,
		SCROLL_HEIGHT, currect.r_height - SCROLLWIDTH, 0);

	leftbubble();
	upbubble();
	scrollbar_paint_all_clear(board_pixwin);

	/* 
	 * should use batchrop here
	 */
	if (gridon)
		drawgrid();
	for (i = 0; i < rightedge; i++) {
		for (j = 0; j < bottomedge; j++) {
			if (board[i][j])
				paint_stone(i, j, board[i][j]);
		}
	}
	msgsw_setstring(mswerror, "Left paints, middle erases");
}

drawgrid()
{
	int wd, ht, i, j;
	
	wd = rightedge * spacing;
	ht = bottomedge * spacing;
	lock();
	for (j = 0; j <= ht; j+=spacing)
		pw_vector(board_pixwin, 0, j, wd, j,
		    PIX_SRC^PIX_DST, 1);
	for (j = 0; j <= wd; j+=spacing)
		pw_vector(board_pixwin, j, 0, j, ht,
		    PIX_SRC^PIX_DST, 1);
	unlock();
}

/* respond to damage to board subwindow */
static
board_sighandler(sw)
caddr_t sw;
{
	struct rect	r;

	win_getsize(board_tsw->ts_windowfd, &r);
	/* 
	 * resize whole thing if size change, otherwise just
	 * redraw damaged part
	 */
	if (r.r_width != currect.r_width || r.r_height != currect.r_height) {
		currect.r_width = r.r_width;
		currect.r_height = r.r_height;
		rightedge = (currect.r_width-SCROLLWIDTH)/spacing;
		bottomedge = (currect.r_height-SCROLLWIDTH)/spacing;
		refreshboard();
		board_init();
	}
	else {
		pw_damaged(board_pixwin);
		board_init();
		pw_donedamaged(board_pixwin);

	}
}

/* get notification from option subwindow */
static
grid_proc(item, value, event)
	Panel_item item;
	struct inputevent event;
{
	gridon ^= 1;
	drawgrid();
}

static
mode_proc(item, value, event)
	Panel_item item;
	struct inputevent event;
{
	if (value == 0)
		runmode = 1;
	else {
		runmode = 0;
		newgen();
		updategen();
	}
}

static
speed_proc(item, value, event)
	Panel_item item;
	int value;
	struct inputevent event;
{
	nextupdate.tv_sec = value/100;
	nextupdate.tv_usec = (value%100)*10000;
}

static
zoom_proc(item, zoom, event)
	Panel_item item;
	int zoom;
{
	spacing = zoom;
	rightedge = (currect.r_width-SCROLLWIDTH)/spacing;
	bottomedge = (currect.r_height-SCROLLWIDTH)/spacing;
	leftoffset += ((currect.r_width/zoom) - (currect.r_width/ozoom))/2;
	upoffset += ((currect.r_height/zoom) - (currect.r_height/ozoom))/2;
	circle();
	refreshboard();
	board_init();
	ozoom = zoom;
}

quit_proc()
{
	exit(0);
}

reset_proc(panel, item)
	Panel panel;
	Panel_item item;
{
	int i,j;
	
	for(i = 0; i < rightedge; i++)
	for(j = 0; j < bottomedge; j++)
		board[i][j] = 0;
	zerolist();
	runmode = 0;
	gencnt = 0;
	ozoom = SPACING;
	spacing = SPACING;
	rightedge = (currect.r_width-SCROLLWIDTH)/spacing;
	bottomedge = (currect.r_height-SCROLLWIDTH)/spacing;
	leftoffset = 0;
	upoffset = 0;
	panel_set_value(mode_item, 1);
	panel_set_value(zoom_item, SPACING);
	panel_paint(mode_item, PANEL_CLEAR);
	panel_paint(zoom_item, PANEL_CLEAR);
	panel_set(gen_item, PANEL_LABEL_STRING, "     0", 0);
	circle();
	refreshboard();
	board_init();
}

run_proc()
{
	panel_set_value(mode_item, 0);
	panel_paint(mode_item, PANEL_CLEAR);
	runmode = 1;
}

/*
 * respond to user inputs in board area
 */
static
board_selected(sw, ibits, obits, ebits, timer)
caddr_t			sw;
int				*ibits, *obits, *ebits;
struct timeval	**timer;
{
	int	x, y;
	struct inputevent	ie;
	struct menuitem *mi;
	int	cur_x, cur_y, entered;
	static int erasing, painting;
	Scrollbar sb;

	if (input_readevent(board_tsw->ts_windowfd, &ie) == -1)  {
		perror("input_readevent failed");
		abort();
	}
	*ibits = *obits = *ebits = 0;
	sb = scrollbar_for_point(board_pixwin, ie.ie_locx, ie.ie_locy,
	    &entered);
	if (sb) {
		if (entered) {
			if (sb == leftscroll)
				leftbubble();
			else
				upbubble();
		}
		scrollbar_interpret(board_pixwin, &ie);
		return;
	}
	scrollbar_interpret(board_pixwin, &ie);

	if (ie.ie_code == MENU_BUT && win_inputposevent(&ie) &&
	    (mi = menu_display(&menu_ptr, &ie, board_tsw->ts_windowfd))) {
		ie.ie_code = (short) mi->mi_data;
		handlemenu(ie);
		*ibits = *obits = *ebits = 0;
		return;
	}
	
	x = ie.ie_locx; y = ie.ie_locy;
	cur_x = (x/spacing)*spacing;
	cur_y = (y/spacing)*spacing;
	if (cur_x > spacing*(rightedge-1) || cur_y > spacing*(bottomedge-1))
		return;
	if (win_inputnegevent(&ie)) {
		erasing = 0;
		painting = 0;
		return;
	}

	if (ie.ie_code == MS_MIDDLE ||
	    (ie.ie_code == LOC_MOVEWHILEBUTDOWN && erasing)) {
		erasing = 1;
		if (board[cur_x/spacing][cur_y/spacing]== 0)
			return;
		deletepoint(cur_x/spacing - leftoffset,
		    cur_y/spacing - upoffset);
		erase_stone(cur_x/spacing, cur_y/spacing);
	}
	else if (ie.ie_code == MS_LEFT ||
	    (ie.ie_code == LOC_MOVEWHILEBUTDOWN && painting)) {
		painting = 1;
		addpoint(cur_x/spacing - leftoffset,
		    cur_y/spacing - upoffset);
		paint_stone(cur_x/spacing, cur_y/spacing, INITCOLOR);
	}
}

paint_stone(i, j, color)
{ 
	int diameter;
	
	diameter = spacing - 3;
	if (color)
		pw_write(board_pixwin, i*spacing+2, j*spacing+2, diameter,
		    diameter, (PIX_COLOR(color)|PIX_SRC), piecepix, 0, 0);
	else
		pw_write(board_pixwin, i*spacing+2, j*spacing+2, diameter,
		    diameter, PIX_SRC, piecepix, 0, 0);
	board[i][j] = color;
} 

erase_stone(i, j)
{ 
	int diameter;
	
	diameter = spacing - 3;
	if (color)
		pw_write(board_pixwin, i*spacing+2, j*spacing+2,
		    diameter, diameter,
		    (PIX_SRC|PIX_COLOR(board[i][j])) ^ PIX_DST,
		    piecepix, 0, 0);
	else
		pw_write(board_pixwin, i*spacing+2, j*spacing+2,
		    diameter, diameter, PIX_SRC ^ PIX_DST, piecepix, 0, 0);
	board[i][j] = 0;
} 


circle()
{
	int i,j, diameter;
	
	diameter = spacing - 3;
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
static
clock_selected(msgsw, ibits, obits, ebits, timer)
	struct	msgsubwindow *msgsw;
	int	*ibits, *obits, *ebits;
	struct	timeval **timer;
{
	if (*timer && ((*timer)->tv_sec == 0) && ((*timer)->tv_usec == 0)
	    && *ibits == 0) {
		if (runmode) {
			newgen();
			updategen();
		}
		tv = nextupdate;
	}
	*ibits = 0;
	*obits = 0;
	*ebits = 0;
}

amicolor()
{
	struct screen screen;

	return (tool->tl_pixwin->pw_pixrect->pr_depth > 1);
}

doscroll(pw, sb, notify_data, offset, length, motion)
	struct pixwin *pw;
	Scrollbar sb;
	char *notify_data;
	Scroll_motion motion;
{
	int left, right, top, bottom;
	int oldleftoffset, oldupoffset;
	float x;
	
	switch (motion) {
		case SCROLL_FORWARD:
			if (offset <= spacing)
				return;
			if (sb ==leftscroll)
				leftoffset -= offset/spacing;
			else
				upoffset -= offset/spacing;
			break;
		case SCROLL_BACKWARD:
			if (offset <= spacing)
				return;
			if (sb ==leftscroll)
				leftoffset += offset/spacing;
			else
				upoffset += offset/spacing;
			break;
		case SCROLL_ABSOLUTE:
			box(&left, &right, &top, &bottom);
			x = offset/(float)length;
			if (sb == leftscroll) {
				oldleftoffset = leftoffset;
				if ((right - left) > rightedge)
					leftoffset = rightedge/2 -
					    (left + x*(right - left));
				else
					leftoffset = x*rightedge -
					    (left + right)/2;
				if (oldleftoffset == leftoffset)
					return;
			}
			else {
				oldupoffset = upoffset;
				if ((bottom - top) > bottomedge)
					upoffset = bottomedge/2 -
					    (top + x*(bottom - left));
				else
					upoffset = x*bottomedge -
					    (top + bottom)/2;
				if (oldupoffset == upoffset)
					return;
			}
			break;
	}
	refreshboard();
	board_init();
}

/* 
 * update bubble in left scrollbar
 */
leftbubble()
{
	int left, right, top, bottom;
	int start;

	box(&left, &right, &top, &bottom);
	start = max(-leftoffset - left, 0);
	start = min(start, right - left);
	scrollbar_set(leftscroll,
		SCROLL_VIEW_START, start,
		SCROLL_VIEW_LENGTH, rightedge,
		SCROLL_OBJECT_LENGTH, right - left, 0);
}

upbubble()
{
	int left, right, top, bottom;
	int start;

	box(&left, &right, &top, &bottom);
	start = max(-upoffset - top, 0);
	start = min(start, bottom - top);
	scrollbar_set(upscroll,
		SCROLL_VIEW_START, start,
		SCROLL_VIEW_LENGTH, bottomedge,
		SCROLL_OBJECT_LENGTH, bottom - top, 0);
}

updategen()
{
	char buf[7];
	
	gencnt++;
	sprintf(buf, "%6d", gencnt);
	panel_set(gen_item, PANEL_LABEL_STRING, buf, 0);
}
