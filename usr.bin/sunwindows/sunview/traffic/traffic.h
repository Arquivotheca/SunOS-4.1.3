/*	@(#)traffic.h 1.1 92/07/30 SMI */

/*
 *	1985 by Sun Microsystems Inc.
 *
 */

#define INITSPEED 20		/* 2 secs */
#define INITSPEED1 120		/* 2 mins */
#define DISPLAY_LOAD 0
#define DISPLAY_SIZE 1
#define DISPLAY_PROTO 2
#define DISPLAY_SRC 3
#define DISPLAY_DST 4
#define INITMODE DISPLAY_PROTO
#define NFONTS 3
#define MAXBARS 8		/* max of NUMNAMES, NPROTOS, NBUCKETS/2 */
#define NUMNAMES 8
#define MAXSPLIT 4
#define INITMAXABS 100		/* init max of scale when in abs mode */
#define LEFTMARGIN 5		/* numbers of chars wide */
#define TOPGAP 10		/* gap between top of graph and subwindow */

struct rank {
	int cnt, addr;
};

/* 
 *  pixrects
 */
extern struct pixrect	*proof_pr1;
extern struct pixrect	*proof_pr;

/* 
 *  global variables
 */
Frame			frame;
struct rect		toolrect;
struct rect		tswrect[MAXSPLIT];
Canvas			canvas[MAXSPLIT];
Panel			toppanel;
Panel			horizpanel[MAXSPLIT];
Panel			vertpanel[MAXSPLIT];
Panel_item		quit_item;
Panel_item		mode_item[MAXSPLIT];
Panel_item		grid_item[MAXSPLIT];
Panel_item		speed_item[MAXSPLIT];
Panel_item		speed_item1[MAXSPLIT];
Panel_item		speedvalue_item[MAXSPLIT];
Panel_item		slider_item[MAXSPLIT];
Panel_item		scale_item[MAXSPLIT];
struct pixwin		*pixwin[MAXSPLIT];
struct itimerval	timeout[MAXSPLIT];
int			timeout1[MAXSPLIT];
struct pixfont		*fonts[NFONTS];
char			host[256];
int			splitcnt;
int			gridon[MAXSPLIT];		/* is grid on? */
int			curfontht;
int			marginfontwd;
int			marginfontht;
int			running;
int			traf_absolute[MAXSPLIT];
int			mode[MAXSPLIT];
int			curht[MAXSPLIT];
int			maxabs[MAXSPLIT];

/* 
 *  procedure variables
 */

int grid_toggle(), speed_slider(), speed1_slider(), mode_choice(),
	split_button(), quit_button(), quit(), delete_button(),
	slider_cycle(), scale_cycle();

Notify_value timeout_notify();
