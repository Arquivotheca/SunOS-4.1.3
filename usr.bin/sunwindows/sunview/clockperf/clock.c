#ifndef lint
static  char sccsid[] = "@(#)clock.c 1.1 92/07/30 Copyr 1985, 1986, 1987 Sun Micro";
#endif

/*
 * Sun Microsystems, Inc.
 */

/*
 *	Clock:	Display time as text when open and icon when closed.
 *
 * 	Note: 	The industry standard for roman clock
 *		faces show the numeral four as "IIII"
 *		rather than "IV".  This is not an err.
 *						[mjb 6/20/89]
 */

#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <strings.h>
#include <sunwindow/defaults.h>
#include <suntool/sunview.h>
#include <suntool/frame.h>
#include <suntool/icon.h>
#include <suntool/panel.h>
#include "clockhands.h"

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

extern	char *asctime();
extern	int errno;
void	notify_set_signal_check();

void clock_show_date();
void paintface();

static char *optionsptr;
static Pixfont *pfont;
static char *strip_cr();


static	u_short icon_data[300] = {
#include <images/clock.icon>
};
mpr_static(clock_default_mpr, 64, 75, 1, icon_data);

static u_short icon_rom_data[300] = {
#include <images/clock.rom.icon>
};
mpr_static(clock_default_rom_mpr, 64, 75, 1, icon_rom_data);

struct endpoints min_box[4] = {
	0, 0,
	0, 1,
	1, 1,
	1, 0,
};


Icon		clockicon, clockicon_date;
Pixrect 	*ic_mpr, *base_mpr;
struct tm 	*tmp;

static	Frame	frame;
static	Notify_value clock_tool_event();
static	Notify_value clock_itimer_expired();
static	int clock_client;
static  char time_str[128];


unsigned show_seconds, show_date;
static	unsigned testing;
static	unsigned roman;
unsigned face;

static	Panel_item msg_item;
static	Notify_value clock_msg_item_event();
static	Notify_value clock_tool_destroy();

static	int last_hour;

#ifdef STANDALONE
main(argc, argv)
#else
int clocktool_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	*name = "clock";
	char	*tool_name = argv[0];
	struct	inputmask im;
	Panel	panel;
	char	**tool_attrs = NULL;
	Pixrect *cmd_line_mpr;
	Rect    *screen_rect, *frame_closed_rect;

	optionsptr = "";
	last_hour = -1;
	/*
	 * Create and customize tool
	 */
	argc--;   argv++;		/*	skip over program name	*/
    
	clockicon = icon_create(
		ICON_WIDTH,	64,
		ICON_HEIGHT,	64,
		0);
		   
        frame = window_create(0, FRAME,
            FRAME_LABEL,		name,
            FRAME_CLOSED,		TRUE,
            FRAME_ICON,			clockicon,
            WIN_ROWS,			2,
            WIN_COLUMNS,		26,
	    WIN_ERROR_MSG, 		"Unable to create frame\n",
            FRAME_ARGC_PTR_ARGV,	&argc, argv,
            0);
            
        if (frame == (Frame)NULL)
            EXIT(1);
            
        icon_destroy(clockicon);     
    
        while (argc-- > 0) {
                switch ((*argv)[1]) {
		  case 'S':
		  case 's': show_seconds = TRUE;
			    break;
		  case 'T':
		  case 't': testing = TRUE;
			    break;
		  case 'r': roman = TRUE;
			    break;
		  case 'd': show_date = TRUE;
			    argv++;
			    argc--;
			    if (argc < 0) {
				    (void)fprintf(stderr,
				    	"invalid -d option to clock\n");
				    break;
			    }
			    optionsptr = *argv;
			    break;
		  case 'f': face = TRUE;
		  	    break;
		  default:  (void)fprintf(stderr,
				"clock doesn't recognize '%s' argument\n",
				    *argv);
                }
                argv++;
        }
	/*
	 * Set up clock icon
	 */
	if (show_date) {
		ic_mpr = mem_create(64, 75, 1);
		base_mpr = mem_create(64, 75, 1);
	} else {
		ic_mpr = mem_create(64, 64, 1);
		base_mpr = mem_create(64, 64, 1);
	}
	
	(void)pr_rop(base_mpr, 0, 0, 64, 64, PIX_SRC, 
	    (roman == TRUE)? &clock_default_rom_mpr: &clock_default_mpr, 0, 0);
	    
	if (show_date) {
		(void)pr_rop(base_mpr, 0, 62, 64, 13, PIX_SET, (Pixrect *)0, 0, 0);
		(void)pr_rop(base_mpr, 0+2, 62+2, 64-4, 13-4, PIX_CLR, (Pixrect *)0, 0, 0);
	}
	
	/* Get the user specified icon image */
        clockicon = window_get(frame, FRAME_ICON);
	cmd_line_mpr = (Pixrect *) icon_get(clockicon, ICON_IMAGE);
	if (cmd_line_mpr) {
	     (void)pr_rop(base_mpr, 0, 0, cmd_line_mpr->pr_width,
		    cmd_line_mpr->pr_height, PIX_SRC, cmd_line_mpr, 0, 0);
	     (void) icon_set(clockicon, ICON_IMAGE, NULL, 0);	    
	     (void)pr_destroy(cmd_line_mpr);	    
	}
	
	/* Tell the frame about the new icon height */
	if (show_date) {
	    Rect	*temp_rect = (Rect *)icon_get(clockicon, ICON_IMAGE_RECT);
	    
	    temp_rect->r_height = 75;
	    icon_set(clockicon, 
		 ICON_HEIGHT,		75,
		 ICON_IMAGE_RECT, temp_rect,
		 0);           		  
            window_set(frame,  FRAME_ICON, clockicon, 0);
        }
        pfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.7");
	if (pfont == NULL) {
		(void)fprintf(stderr, "can't find screen.r.7\n");
		EXIT(1);
	}
	
        clockicon = window_get(frame, FRAME_ICON);
        
        icon_set(clockicon, 
		 ICON_IMAGE,	ic_mpr,
		 ICON_FONT,	pfont,
		 0);
        
	(void) notify_interpose_event_func((Notify_client)frame,
			clock_tool_event, NOTIFY_SAFE);
			

	
	/*
	 * Create and customize msg subwindow.
	 * Even though the panel only has a message item, it still
	 * accepts keystrokes.
	 */
	panel = window_create(frame, PANEL,
		    WIN_ERROR_MSG,              "Unable to create frame\n", 
		    PANEL_ACCEPT_KEYSTROKE,  	TRUE,
		    0);
	
	if (panel == (Panel)NULL)
		EXIT(1);

	(void) notify_interpose_event_func((Notify_client)panel,
			clock_msg_item_event, NOTIFY_SAFE);

	/* Initialize tmp and set the size of the panel item */	
	clock_update_tmp();	
	msg_item = panel_create_item(panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, strip_cr(asctime(tmp)),
			PANEL_PAINT, PANEL_NO_CLEAR,
			0);
	if (msg_item == (Panel_item)NULL)
		EXIT(1);
	window_fit(panel);
	window_fit(frame);

	/*
	 * Notice when tool dies so that can remove interval timer.
	 */
	(void) notify_interpose_destroy_func((Notify_client)(frame), 
		clock_tool_destroy);
	/*
	 * Simulate a interval timer expiration to prime image.
	 */
	(void) clock_itimer_expired((Notify_client)(&clock_client), 
		ITIMER_REAL);
		
	screen_rect = (Rect *)window_get(frame, WIN_SCREEN_RECT);
	frame_closed_rect = (Rect *)window_get(frame, FRAME_CLOSED_RECT);
	
	if (rect_bottom(frame_closed_rect) > screen_rect->r_height) {
	    frame_closed_rect->r_top = screen_rect->r_height - frame_closed_rect->r_height;
	    (void) window_set(frame, FRAME_CLOSED_RECT, frame_closed_rect, 0);
	}
	     
	
	/*
	 * Main loop
	 */
	window_main_loop(frame);
	/*
	 * Cleanup
	 */
	(void)pf_close(pfont);
	(void)pr_destroy(ic_mpr);
	(void)pr_destroy(base_mpr);
	
	EXIT(0);
}

static
Notify_value
clock_msg_item_event(msg_item_local, event, arg, type)
	Panel	msg_item_local;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	switch (event_id(event)) {
	  case 's':	/* toggle second */
	  case 'S':	/*    hand	*/
		show_seconds = !show_seconds;
                if (!show_seconds)
                   if (roman)
                      clock_rom_sethands(frame);
                   else
                      clock_sethands(frame);
		/* Simulate a interval timer expiration to prime image */
		(void) clock_itimer_expired((Notify_client)(
			&clock_client), ITIMER_REAL);
		break;
	  case 't':	/* toggle testing */
	  case 'T':
		testing = !testing;
		/* Simulate a interval timer expiration to prime image */
		(void) clock_itimer_expired((Notify_client)(
			&clock_client), ITIMER_REAL);
		break;
	  default: /*  pass on the rest */
		return(notify_next_event_func((Notify_client)(
			msg_item_local), (Notify_event)event, 
			arg, type));
	}
	return(NOTIFY_DONE);
} 

static
Notify_value
clock_tool_event(frame_local, event, arg, type)
	Frame	frame_local;
	Event	*event;
	Notify_arg arg;
	Notify_event_type type;
{
	short iconic_start = (int)window_get(frame_local, FRAME_CLOSED);

	Notify_value value = notify_next_event_func((Notify_client)(
		frame_local), (Notify_event)(event), arg, type);

	if (iconic_start != (int)window_get(frame_local, FRAME_CLOSED))
		/*
		 * Simulate interval timer expiration so that the correct
		 * time is shown after changing state.
		 */
		(void) clock_itimer_expired((Notify_client)(
			&clock_client), ITIMER_REAL);
	return(value);
}

static
Notify_value
clock_tool_destroy(frame_local, status)
	Frame	*frame_local;
	Destroy_status status;
{
	Notify_value value = notify_next_destroy_func((Notify_client)(
		frame_local), status);

	if (((status == DESTROY_CLEANUP) || (status == DESTROY_PROCESS_DEATH))&&
	    (value == NOTIFY_DONE))
		/*
		 * Remove clock related conditions (e.g., interval timer)
		 * if tool going away.  Could just let it expire & ignore it
		 * but then the tool's process would be a long time terminating
		 * (it would be waiting to the timeout).
		 */
		(void)notify_remove((Notify_client)(&clock_client));
	return(value);
} 

/* ARGSUSED */
static
Notify_value
clock_itimer_expired(clock, which)
	Notify_client	clock;
	int		which;
{
	struct	itimerval itimer;
	struct	timeval c_tv;

	clock_update_tmp();
	if ((int)window_get(frame, FRAME_CLOSED)) {
		if (roman) {
			clock_rom_sethands(frame);
		} else {
			clock_sethands(frame);
		}
	}  else 
		panel_set(msg_item, 
			PANEL_LABEL_STRING, strip_cr(asctime(tmp)),
			0);
	/* Reset itimer everytime */
	if (testing)
		/* Poll at top speed */
		itimer = NOTIFY_POLLING_ITIMER;
	else {
		/* Compute next timeout from current time */
		itimer.it_value.tv_usec = 0;
		itimer.it_value.tv_sec = ((show_seconds) ? 1: 60 - tmp->tm_sec);
	}
	/* Don't utilize subsequent interval */
	itimer.it_interval.tv_usec = 0;
	itimer.it_interval.tv_sec = 0;
	/* Utilize notifier hack to avoid stopped clock bug */
	c_tv = itimer.it_value;
	c_tv.tv_sec *= 2;
	notify_set_signal_check(c_tv);
	/* Set itimer event handler */
	(void) notify_set_itimer_func((Notify_client)(&clock_client), 
		clock_itimer_expired, ITIMER_REAL, &itimer,(struct itimerval *)0);
	return(NOTIFY_DONE);
}

/* Call when tmp needs to be up-to-date */
static
clock_update_tmp()
{
	int	clock;

	if (testing) {
		if (!tmp) {
			/* Get local time from kernel */
			clock = time((time_t *)0);
			tmp = localtime (&clock);
		}
		/* Increment local time */
		if (++(tmp->tm_min) == 60) {
			tmp->tm_min = 0;
			if (++(tmp->tm_hour) == 24)
				tmp->tm_hour = 0;
		}
	} else {
		/* Get local time from kernel */
		clock = time((time_t *)0);
		tmp = localtime (&clock);
	}
}




static	char *monthstr[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static	char *daystr[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};



void
clock_show_date()
{
	char datestr[20];
	register char *datep, *optp;
	struct pr_prpos where;

	/*
	 * Update the date field once an hour.
	 */
	if (last_hour != tmp->tm_hour) {
		last_hour = tmp->tm_wday;
		datep = datestr;
		optp = optionsptr;
		while (*optp) {
			switch (*optp++) {
			case 'm':		/* Month */
				(void)strcpy(datep, monthstr[tmp->tm_mon]);
				datep += 3;
				break;
			case 'w':		/* day of Week */
				(void)strcpy(datep, daystr[tmp->tm_wday]);
				datep += 3;
				break;
			case 'd':		/* Day of month */
				if (tmp->tm_mday >= 10) {
					*datep++ = (tmp->tm_mday / 10 + '0');
				}
				*datep++ = tmp->tm_mday % 10 + '0';
				break;
			case 'y':		/* Year */
				*datep++ = tmp->tm_year / 10 + '0';
				*datep++ = tmp->tm_year % 10 + '0';
				break;
			case 'a':		/* Am/pm */
				*datep++ = tmp->tm_hour > 11 ? 'P' : 'A';
				*datep++ = 'M';
				break;
			}
			*datep++ = ' ';
		}
		*--datep = '\0';
		where.pr = base_mpr;
		where.pos.y = 71;
		where.pos.x = 2;
		(void)pf_text(where, PIX_SRC, pfont, "          "); /* clear */
		where.pos.x = 31 - 3 * strlen(datestr);
		if (where.pos.x < 2) {
			where.pos.x = 2;
		}
		(void)pf_text(where, PIX_SRC, pfont, datestr);
	/*	(void)icon_set(clockicon, ICON_LABEL, datestr, 0);	 */	
	}
}
	
static char *date[] = {"", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10",
	"11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
	"21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31"};
	
#define TOP   0
#define LEFT  1
#define BOT   2
#define RIGHT 3

static struct pr_pos pos[] = {{(64-3*6)/2, 24},	/* top */
			  {8, 32+3},		/* left */
			  {(64-2*6)/2, 64-24+6},  /* bottom */
			  {32+8, 32 +3}};	/* right */

static int minq[]  =  { TOP, TOP, TOP, TOP, TOP, TOP, TOP, TOP,
			RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
			     RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT, RIGHT,
			BOT, BOT, BOT, BOT, BOT, BOT, BOT, BOT,BOT,
			     BOT, BOT, BOT, BOT, BOT, BOT,
			LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
			     LEFT, LEFT, LEFT, LEFT, LEFT, LEFT,
			TOP, TOP, TOP, TOP, TOP, TOP, TOP};

void
paintface(mpr)
	Pixrect *mpr;
{
	static struct pr_prpos where;
	int arr[4], z;
	
	where.pr = mpr;
	arr[0] = arr[1] = arr[2] = arr[3] = 0;

	z = 60*tmp->tm_hour + tmp->tm_min;
	if (z > 720)
		z -= 720;
	if (90 <= z && z <= 270)
		arr[RIGHT] = 1;
	else if (270 <= z && z <= 450)
		arr[BOT] = 1;
	else if (450 <= z && z <= 630)
		arr[LEFT] = 1;
	else
		arr[TOP] = 1;
	arr[minq[tmp->tm_min]] = 1;
	if (arr[LEFT] == 0 && arr[RIGHT] == 0 && !roman) {
		where.pos = pos[LEFT];
		(void)pf_text(where, PIX_SRC, pfont, daystr[tmp->tm_wday]);
		where.pos = pos[RIGHT];
		(void)pf_text(where, PIX_SRC, pfont, date[tmp->tm_mday]);
	}
	else {
		where.pos = pos[TOP];
		(void)pf_text(where, PIX_SRC, pfont, daystr[tmp->tm_wday]);
		where.pos = pos[BOT];
		(void)pf_text(where, PIX_SRC, pfont, date[tmp->tm_mday]);
	}
}




static char *
strip_cr(str)
	char *str;
{
	strcpy(time_str, str);
	time_str[strlen(time_str) - 1] = '\0';
	return(time_str);
}
