#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)cursor_demo.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Cursor_demo -- demonstrate the attribute-value cursor/crosshair
 * interface.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sunwindow/attr.h>
#include <sunwindow/cms_rainbow.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>

static unsigned char	red[CMS_RAINBOWSIZE];
static unsigned char	green[CMS_RAINBOWSIZE];
static unsigned char	blue[CMS_RAINBOWSIZE];

static short k1_image[] = {
#include <images/k1.pr>
};
mpr_static(curdem_icpr1, 64, 64, 1, k1_image);

static short k2_image[] = {
#include <images/k2.pr>
};
mpr_static(curdem_icpr2, 64, 64, 1, k2_image);

static short k3_image[] = {
#include <images/k3.pr>
};
mpr_static(curdem_icpr3, 64, 64, 1, k3_image);

static short k4_image[] = {
#include <images/k4.pr>
};
mpr_static(curdem_icpr4, 64, 64, 1, k4_image);

static short k5_image[] = {
#include <images/k5.pr>
};
mpr_static(curdem_icpr5, 64, 64, 1, k5_image);

static short down_arrow_image[] = {
#include <images/down_arrow.pr>
};
mpr_static(down_arrow, 16, 16, 1, down_arrow_image);

static short off_image[] = {
#include <images/off.pr>
};
mpr_static(off_pr, 64, 16, 1, off_image);

static short on_image[] = {
#include <images/on.pr>
};
mpr_static(on_pr, 64, 16, 1, on_image);

static short ic_image[258] = {
#include <images/cursor_demo.icon>
};
mpr_static(hair_pr, 64, 64, 1, ic_image);

#ifdef notdef
static  struct icon icon = {64, 64, (struct pixrect *)NULL, 0, 0, 64, 64,
            &hair_pr, 0, 0, 0, 0, NULL, (struct pixfont *)NULL,
            ICON_BKGRDCLR};
#endif notdef


static Frame		frame;
static Panel		panel;

static Panel_item	what_to_show;

static Panel_item	horiz_thickness;
static Panel_item	horiz_op;
static Panel_item	horiz_color;
static Panel_item	horiz_gap;
static Panel_item	horiz_length;
static Panel_item	horiz_border_gravity;

static Panel_item	vert_thickness;
static Panel_item	vert_op;
static Panel_item	vert_color;
static Panel_item	vert_gap;
static Panel_item	vert_length;
static Panel_item	vert_border_gravity;

static Panel_item	cursor_op;
static Panel_item	cursor_xhot;
static Panel_item	cursor_yhot;

static Cursor		cursor;

#define	SHOW_CURSOR		0x1
#define	SHOW_HORIZ_HAIR		0x2
#define	SHOW_VERT_HAIR		0x4
#define	SHOW_FULLSCREEN		0x8


/* ARGSUSED */
static
apply_cursor(ip, ie)
Panel_item	ip;
Event		*ie;
{

    unsigned int	bits;

    bits = (unsigned int) panel_get_value(what_to_show);
    (void)cursor_set(cursor,
	       CURSOR_SHOW_CURSOR,	bits & SHOW_CURSOR,
	       CURSOR_SHOW_HORIZ_HAIR,	bits & SHOW_HORIZ_HAIR,
	       CURSOR_SHOW_VERT_HAIR,	bits & SHOW_VERT_HAIR,
	       CURSOR_FULLSCREEN,	bits & SHOW_FULLSCREEN,
	       CURSOR_XHOT,		panel_get_value(cursor_xhot),
	       CURSOR_YHOT, 		panel_get_value(cursor_yhot),
	       CURSOR_OP,   		
	           value_to_op((int)(LINT_CAST(panel_get_value(cursor_op)))),
	       CURSOR_HORIZ_HAIR_THICKNESS, 
		   panel_get_value(horiz_thickness) + 1,
	       CURSOR_HORIZ_HAIR_OP,  
		   value_to_op((int)(LINT_CAST(panel_get_value(horiz_op)))),
	       CURSOR_HORIZ_HAIR_COLOR, 	panel_get_value(horiz_color),
	       CURSOR_HORIZ_HAIR_GAP, 		panel_get_value(horiz_gap),
	       CURSOR_HORIZ_HAIR_LENGTH, 	panel_get_value(horiz_length),
	       CURSOR_HORIZ_HAIR_BORDER_GRAVITY, 
		   panel_get_value(horiz_border_gravity),
	       CURSOR_VERT_HAIR_THICKNESS,  
		   panel_get_value(vert_thickness) + 1,
	       CURSOR_VERT_HAIR_OP,  
		   value_to_op((int)(LINT_CAST(panel_get_value(vert_op)))),
	       CURSOR_VERT_HAIR_COLOR,  	panel_get_value(vert_color),
	       CURSOR_VERT_HAIR_GAP,  		panel_get_value(vert_gap),
	       CURSOR_VERT_HAIR_LENGTH, 	panel_get_value(vert_length),
	       CURSOR_VERT_HAIR_BORDER_GRAVITY, 
		   panel_get_value(vert_border_gravity),
	       0);

    (void)window_set(panel, WIN_CURSOR, cursor, 0);
}


static int
value_to_op(value)
register int	value;
{
    switch (value) {
	case 0: return PIX_SRC; 
	case 1: return PIX_DST; 
	case 2: return PIX_SRC | PIX_DST;
	case 3: return PIX_SRC & PIX_DST;
	case 4: return PIX_NOT(PIX_SRC & PIX_DST);
	case 5: return PIX_NOT(PIX_DST);
	case 6: return PIX_SRC ^ PIX_DST;
    }
    /* NOTREACHED */
}

    
/* ARGSUSED */
static
quit_out(panel_local,item, event)
Panel		panel_local;
Panel_item	item;
Event		*event;
{
    (void)window_destroy(frame);
}


#ifdef STANDALONE
main(argc, argv)
#else
cursor_demo_main(argc,argv)
#endif
int argc;
char **argv;
{
	static  Notify_value my_destroy_func();

	struct pixrect	*orig_pr, *new_pr;
	Pixwin		*pixwin;

	/* create the frame and panel */
        frame = window_create((Window)0, FRAME,
	    FRAME_ARGS,		argc, argv,
            FRAME_LABEL,        "cursor_demo",
	    FRAME_ICON,		icon_create(ICON_IMAGE, &hair_pr, 0),
	    FRAME_NO_CONFIRM,	TRUE,
            0);

	(void) notify_interpose_destroy_func(frame, my_destroy_func);
	
	panel = window_create(frame, PANEL, 0);

	/* copy the orignal cursor image */
	orig_pr = (struct pixrect *) (LINT_CAST(
	    cursor_get(window_get(panel, WIN_CURSOR), CURSOR_IMAGE)));
	new_pr = (struct pixrect *)(LINT_CAST(mem_create(
		orig_pr->pr_width, orig_pr->pr_height, orig_pr->pr_depth)));
	(void)pr_rop(new_pr, 0, 0, new_pr->pr_width, new_pr->pr_height, PIX_SRC,
	    orig_pr, 0, 0);

	/* create a cursor */
	cursor = cursor_create(
	    CURSOR_IMAGE, new_pr,
	    CURSOR_OP,    PIX_SRC ^ PIX_DST, 
	    0);
        (void)window_set(panel, WIN_CURSOR, cursor, 0); 

	/* setup the color map */
	cms_rainbowsetup(red, green, blue);
	pixwin = (Pixwin *) (LINT_CAST(window_get(panel, WIN_PIXWIN)));
	(void)pw_setcmsname(pixwin, CMS_RAINBOW);
	(void)pw_putcolormap(pixwin, 0, CMS_RAINBOWSIZE, red, green, blue);

	what_to_show = 
	    panel_create_item(panel, PANEL_TOGGLE, 
			      PANEL_LABEL_X, ATTR_COL(0),
			      PANEL_LABEL_Y, ATTR_ROW(1),
			      PANEL_VALUE_X, ATTR_COL(3),
			      PANEL_VALUE_Y, ATTR_ROW(2),
		              PANEL_LABEL_STRING, "What to show:",
		              PANEL_LABEL_BOLD, TRUE,
		              PANEL_CHOICE_STRINGS, 
			         "Show Cursor", "Show Horizontal Hair", 
			         "Show Vertical Hair", "Fullscreen",
			         0,
			      PANEL_VALUE, 0x1,
		              PANEL_LAYOUT, PANEL_VERTICAL,
		              PANEL_FEEDBACK, PANEL_MARKED,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	(void)panel_create_item(panel, PANEL_MESSAGE,
			  PANEL_LABEL_X, ATTR_COL(30),
			  PANEL_LABEL_Y, ATTR_ROW(1),
			  PANEL_LABEL_STRING, "Cursor Info:",
			  PANEL_LABEL_BOLD, TRUE,
			  0);

	cursor_xhot = 
	    panel_create_item(panel, PANEL_CYCLE,
			      PANEL_LABEL_X, ATTR_COL(34),
			      PANEL_LABEL_Y, ATTR_ROW(2),
			      PANEL_LABEL_STRING, "X Hot Spot",
			      PANEL_CHOICE_STRINGS,
				 "0", "1", "2", "3", "4", "5",
				 "6", "7", "8", "9", "10", "11",
				 "12", "13", "14", "15", 0,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	cursor_yhot = 
	    panel_create_item(panel, PANEL_CYCLE,
			      PANEL_LABEL_X, ATTR_COL(34),
			      PANEL_LABEL_Y, ATTR_ROW(3),
			      PANEL_LABEL_STRING, "Y Hot Spot",
			      PANEL_CHOICE_STRINGS,
				 "0", "1", "2", "3", "4", "5",
				 "6", "7", "8", "9", "10", "11",
				 "12", "13", "14", "15", 0,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	cursor_op = 
	    panel_create_item(panel, PANEL_CYCLE,
			      PANEL_LABEL_X, ATTR_COL(34),
			      PANEL_LABEL_Y, ATTR_ROW(4),
			      PANEL_LABEL_STRING, "Drawing OP",
			      PANEL_CHOICE_STRINGS,
				"PIX_SRC", 
				"PIX_DST", 
				"PIX_SRC | PIX_DST",
				"PIX_SRC & PIX_DST", 
				"PIX_NOT(PIX_SRC & PIX_DST)", 
				"PIX_NOT(PIX_DST)",
				"PIX_SRC ^ PIX_DST",
				0,
			      PANEL_VALUE, 2,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	(void)panel_create_item(panel, PANEL_MESSAGE,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(8),
			  PANEL_LABEL_STRING, "Horizontal Hair Info:",
			  PANEL_LABEL_BOLD, TRUE,
			  0);

	horiz_thickness = 
	    panel_create_item(panel, PANEL_CHOICE,
                              PANEL_LABEL_X, ATTR_COL(5),
			      PANEL_LABEL_Y, ATTR_ROW(9),
			      PANEL_LABEL_STRING, "Thickness",
			      PANEL_CHOICE_IMAGES,
				&curdem_icpr1, &curdem_icpr2, &curdem_icpr3,
				&curdem_icpr4, &curdem_icpr5, 0,
			      PANEL_FEEDBACK, PANEL_MARKED,
			      PANEL_MARK_IMAGES, &down_arrow, 0,
			      PANEL_NOMARK_IMAGES, 0,
			      PANEL_LAYOUT, PANEL_VERTICAL,
			      PANEL_MARK_XS, ATTR_COL(9), 0,
			      PANEL_MARK_YS, ATTR_ROW(10), 0,
			      PANEL_CHOICE_XS, ATTR_COL(6), 0,
			      PANEL_CHOICE_YS, ATTR_ROW(10) + 16, 0,
			      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	horiz_op = 
	    panel_create_item(panel, PANEL_CYCLE,
                              PANEL_LABEL_X, ATTR_COL(21),
			      PANEL_LABEL_Y, ATTR_ROW(9),
			      PANEL_LABEL_STRING, "Drawing OP",
			      PANEL_CHOICE_STRINGS,
				"PIX_SRC", 
				"PIX_DST", 
				"PIX_SRC | PIX_DST",
				"PIX_SRC & PIX_DST", 
				"PIX_NOT(PIX_SRC & PIX_DST)", 
				"PIX_NOT(PIX_DST)",
				"PIX_SRC ^ PIX_DST",
				0,
			      PANEL_VALUE, 2,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);


	horiz_gap = 
	    panel_create_item(panel, PANEL_SLIDER, 
                              PANEL_LABEL_X, ATTR_COL(28),
			      PANEL_LABEL_Y, ATTR_ROW(10),
			      PANEL_LABEL_STRING, "Gap:",
			      PANEL_SLIDER_WIDTH, 250,
			      PANEL_VALUE, 0,
			      PANEL_MIN_VALUE, -1,
			      PANEL_MAX_VALUE, 400,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	horiz_length = 
	    panel_create_item(panel, PANEL_SLIDER, 
                              PANEL_LABEL_X, ATTR_COL(25),
			      PANEL_LABEL_Y, ATTR_ROW(11),
			      PANEL_LABEL_STRING, "Length:",
			      PANEL_SLIDER_WIDTH, 250,
			      PANEL_MIN_VALUE, -1,
			      PANEL_MAX_VALUE, 800,
			      PANEL_VALUE, -1,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	horiz_color = 
	    panel_create_item(panel, PANEL_CHOICE, 
                              PANEL_LABEL_X, ATTR_COL(26),
			      PANEL_LABEL_Y, ATTR_ROW(12),
			      PANEL_LABEL_STRING, "Color:",
			      PANEL_VALUE, 3,
			      PANEL_CHOICE_STRINGS,
				 "0", "1", "2", "3", "4",
				 "5", "6", "7", 0,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	horiz_border_gravity =
	    panel_create_item(panel, PANEL_CHOICE,
                              PANEL_LABEL_X, ATTR_COL(17),
			      PANEL_LABEL_Y, ATTR_ROW(13),
			      PANEL_LABEL_STRING, "Border Gravity:",
			      PANEL_CHOICE_IMAGES,
				  &off_pr, &on_pr, 0,
			      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			      PANEL_FEEDBACK, PANEL_NONE,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	(void)panel_create_item(panel, PANEL_MESSAGE,
			  PANEL_LABEL_X, ATTR_COL(0),
			  PANEL_LABEL_Y, ATTR_ROW(15),
			  PANEL_LABEL_STRING, "Vertical Hair Info:",
			  PANEL_LABEL_BOLD, TRUE,
			  0);

	vert_thickness = 
	    panel_create_item(panel, PANEL_CHOICE,
                              PANEL_LABEL_X, ATTR_COL(5),
			      PANEL_LABEL_Y, ATTR_ROW(16),
			      PANEL_LABEL_STRING, "Thickness",
			      PANEL_CHOICE_IMAGES,
				&curdem_icpr1, &curdem_icpr2, &curdem_icpr3,
				&curdem_icpr4, &curdem_icpr5, 0,
			      PANEL_FEEDBACK, PANEL_MARKED,
			      PANEL_MARK_IMAGES, &down_arrow, 0,
			      PANEL_NOMARK_IMAGES, 0,
			      PANEL_LAYOUT, PANEL_VERTICAL,
			      PANEL_MARK_XS, ATTR_COL(9), 0,
			      PANEL_MARK_YS, ATTR_ROW(17), 0,
			      PANEL_CHOICE_XS, ATTR_COL(6), 0,
			      PANEL_CHOICE_YS, ATTR_ROW(17) + 16, 0,
			      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	vert_op = 
	    panel_create_item(panel, PANEL_CYCLE,
                              PANEL_LABEL_X, ATTR_COL(21),
			      PANEL_LABEL_Y, ATTR_ROW(16),
			      PANEL_LABEL_STRING, "Drawing OP",
			      PANEL_CHOICE_STRINGS,
				"PIX_SRC", 
				"PIX_DST", 
				"PIX_SRC | PIX_DST",
				"PIX_SRC & PIX_DST", 
				"PIX_NOT(PIX_SRC & PIX_DST)", 
				"PIX_NOT(PIX_DST)",
				"PIX_SRC ^ PIX_DST",
				0,
			      PANEL_VALUE, 2,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);


	vert_gap = 
	    panel_create_item(panel, PANEL_SLIDER, 
                              PANEL_LABEL_X, ATTR_COL(28),
			      PANEL_LABEL_Y, ATTR_ROW(17),
			      PANEL_LABEL_STRING, "Gap:",
			      PANEL_SLIDER_WIDTH, 250,
			      PANEL_VALUE, 0,
			      PANEL_MIN_VALUE, -1,
			      PANEL_MAX_VALUE, 400,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	vert_length = 
	    panel_create_item(panel, PANEL_SLIDER, 
                              PANEL_LABEL_X, ATTR_COL(25),
			      PANEL_LABEL_Y, ATTR_ROW(18),
			      PANEL_LABEL_STRING, "Length:",
			      PANEL_SLIDER_WIDTH, 250,
			      PANEL_MIN_VALUE, -1,
			      PANEL_MAX_VALUE, 800,
			      PANEL_VALUE, -1,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	vert_color = 
	    panel_create_item(panel, PANEL_CHOICE, 
                              PANEL_LABEL_X, ATTR_COL(26),
			      PANEL_LABEL_Y, ATTR_ROW(19),
			      PANEL_LABEL_STRING, "Color:",
			      PANEL_SLIDER_WIDTH, 200,
			      PANEL_VALUE, 3,
			      PANEL_CHOICE_STRINGS,
				 "0", "1", "2", "3", "4",
				 "5", "6", "7", 0,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	vert_border_gravity =
	    panel_create_item(panel, PANEL_CHOICE,
                              PANEL_LABEL_X, ATTR_COL(17),
			      PANEL_LABEL_Y, ATTR_ROW(20),
			      PANEL_LABEL_STRING, "Border Gravity:",
			      PANEL_CHOICE_IMAGES,
				  &off_pr, &on_pr, 0,
			      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
			      PANEL_FEEDBACK, PANEL_NONE,
			      PANEL_NOTIFY_PROC, apply_cursor,
			      0);

	(void)panel_create_item(panel, PANEL_BUTTON, 
                     PANEL_LABEL_X, ATTR_COL(70),
		     PANEL_LABEL_Y, ATTR_ROW(1),
		     PANEL_LABEL_IMAGE,
			panel_button_image(panel, "Quit", 5, (Pixfont *)0),
		     PANEL_NOTIFY_PROC, quit_out,
		     0);

	(void)window_fit(panel);
	(void)window_fit(frame);

	window_main_loop(frame);

#ifdef STANDALONE
	exit(0);
#else
	return 0;
#endif
}
static Notify_value
my_destroy_func(client, status)
    Notify_client	client;
    Destroy_status	status;
{
    int		windowfd = (int)window_get(
    		    (Window)(LINT_CAST(client)), WIN_FD);

    if (status == DESTROY_CHECKING) {
	int	result;
	Event	event;

	result = alert_prompt(
		(Frame)client,
		&event,
		ALERT_MESSAGE_STRINGS,
		    "Are you sure you want to Quit?",
	            0,
		ALERT_BUTTON_YES,	"Confirm",
		ALERT_BUTTON_NO,	"Cancel",
		ALERT_OPTIONAL,		1,
		ALERT_NO_BEEPING,	1,
		0);
	if (result == ALERT_YES) {
	    return(notify_next_destroy_func(client, status));
	} else {
	    (void) notify_veto_destroy(
	    	(Notify_client)(LINT_CAST(client)));
	    return(NOTIFY_DONE);
	}
    }
    return(notify_next_destroy_func(client, status));
}

