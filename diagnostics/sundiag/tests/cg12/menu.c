
#ifndef lint
static  char sccsid[] = "@(#)menu.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/**********************************************************************/
/* Include files */
/**********************************************************************/

#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <strings.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/textsw.h>
#include <suntool/alert.h>
#include <pixrect/pixrect_hs.h>
#include <esd.h>
#include <esd_icon.h>

/**********************************************************************/
/* External variables */
/**********************************************************************/

extern
char		*sprintf();

extern
void		pmessage();

extern
struct test	sb_testlist[];

extern
char		*timestamp();


/**********************************************************************/
/* Static variables */
/**********************************************************************/

static
struct itimerval	timer;

static
Pixfont		*bigfont,
		*bold_bigfont;

static
Frame		main_frame;

static
Textsw		msg_window;

static
Panel		sel_panel,
		disp_panel;

static
Panel_item	int_button,
		sb_button,
		loops_blck_item,
		loops_brd_item,
		title_text_item,
		stop_button,
		test_title_item,
		timer_item;

static
int		pid,
		*pipe_fd,
		sb_test_select,
		secs,
		swap = 0;

/**********************************************************************/
exec_menu(fildes)
/**********************************************************************/
int *fildes;

{

    extern Notify_value pipe_report();

    /* initialisation of variables */
    bigfont = pf_open("/usr/lib/fonts/fixedwidthfonts/cour.r.24");
    bold_bigfont = pf_open("/usr/lib/fonts/fixedwidthfonts/cour.b.24");

    pipe_fd = fildes;

    sb_test_select = get_test_selection(sb_testlist, LIST_DEFAULT);

    pid = 0;

    /* set timer */
    timer.it_value.tv_usec = 0;
    timer.it_value.tv_sec  = 1;

    /* interval to reload timer */
    timer.it_interval.tv_usec = 0;
    timer.it_interval.tv_sec  = 1;

    main_frame = window_create((Window)NULL, FRAME,
				FRAME_LABEL, "CG12 SYSTEM DIAGNOSTICS",
				FRAME_SUBWINDOWS_ADJUSTABLE, FALSE,
				0);
    build_menu(main_frame);

    (void)notify_set_input_func(main_frame, pipe_report, fildes[0]);

    window_main_loop(main_frame);

}

/**********************************************************************/
build_menu(base_frame)
/**********************************************************************/
Frame base_frame;

{
    extern Notify_value my_frame_destroyer();

    extern int stop_proc(),
	       int_exec_proc(),
	       sb_exec_proc();

    extern Panel_item create_button();
    extern Panel_item create_cycle_item();

    int x;
    int y;
    int h;
    int w;
    int y1;
    int fd_tmp;
    int screen_width;
    int screen_height;
    Rect *r;

    r = (Rect *)window_get(base_frame, WIN_SCREEN_RECT);
    screen_width = r->r_width;
    screen_height = r->r_height;

    (void)window_set(base_frame, WIN_X, 0,
				WIN_Y, 0,
				WIN_WIDTH, screen_width,
				WIN_HEIGHT, screen_height/2,
				0);

    sel_panel = window_create(base_frame, PANEL,
				WIN_X, 0,
				WIN_Y, 0,
				PANEL_ITEM_X_GAP, 15,
				0);

    int_button = create_button(sel_panel, &int_button_image,
				INTEGRATION_TEST, int_exec_proc,
				"Run Integration Test", (char *)NULL, (char *)NULL);


    sb_button = create_button(sel_panel, &sb_button_image,
				SINGLE_BLOCK_TEST, sb_exec_proc,
				"Default", "All", "Select");

    title_text_item = panel_create_item(sel_panel, PANEL_MESSAGE,
				PANEL_ITEM_X, 0,
				PANEL_ITEM_Y, 0,
				PANEL_LABEL_FONT, bold_bigfont,
				PANEL_LABEL_STRING, "CG12 SYSTEM DIAGNOSTICS",
				0);


    loops_blck_item = create_cycle_item(sel_panel, "#loops/block : ");

    loops_brd_item = create_cycle_item(sel_panel, "#loops/board : ");

    
    window_fit_height(sel_panel);

    /* relocate items */
    r = (Rect *)panel_get(title_text_item, PANEL_ITEM_RECT);
    w = r->r_width;
    h = r->r_height;
    r = (Rect *)window_get(sel_panel, WIN_RECT);

    (void)panel_set(title_text_item, PANEL_ITEM_X, (screen_width-w)/2,
				PANEL_ITEM_Y, (r->r_height-h)/2,
				0);
    
    h = r->r_height;
    r = (Rect *)panel_get(loops_blck_item, PANEL_ITEM_RECT);
    x = screen_width - (r->r_width+(int)panel_get(sel_panel, PANEL_ITEM_X_GAP)*2);
    y = (h-((r->r_height*2)+(int)panel_get(sel_panel, PANEL_ITEM_Y_GAP)))/2;
    (void)panel_set(loops_blck_item, PANEL_ITEM_X, x,
				PANEL_ITEM_Y, y,
				PANEL_VALUE, 0,
				0);
    y1 = y;
    y = y+r->r_height+(int)panel_get(loops_blck_item, PANEL_ITEM_Y_GAP);
    (void)panel_set(loops_brd_item, PANEL_ITEM_X, x,
				PANEL_ITEM_Y, y,
				PANEL_VALUE, 0,
				0);

    /* create new panel */
    r = (Rect *)window_get(sel_panel, WIN_RECT);
    disp_panel = window_create(base_frame, PANEL,
				WIN_X, r->r_left,
				WIN_Y, r->r_top,
				WIN_WIDTH, r->r_width,
				WIN_HEIGHT, r->r_height,
				PANEL_ITEM_X_GAP, 15,
				WIN_SHOW, FALSE,
				0);

    stop_button = create_button(disp_panel, &stop_button_image,
				NULL, stop_proc, (char *)NULL, (char *)NULL, (char *)NULL);

    test_title_item = panel_create_item(disp_panel, PANEL_MESSAGE,
				PANEL_ITEM_X, x,
				PANEL_ITEM_Y, y1,
				PANEL_LABEL_FONT, bold_bigfont,
				0);

    timer_item = panel_create_item(disp_panel, PANEL_MESSAGE,
				PANEL_LABEL_STRING, "000:00:00",
				PANEL_LABEL_FONT, bold_bigfont,
				0);

    /* reposition items */
    r = (Rect *)panel_get(timer_item, PANEL_ITEM_RECT);
    x = (screen_width-r->r_width)/2;
    (void)panel_set(timer_item, PANEL_ITEM_X, x,
				PANEL_ITEM_Y, y,
				0);

    /* message window */
    (void)unlink(TEXTSW_TMP_FILE);
    fd_tmp = open(TEXTSW_TMP_FILE, O_RDWR | O_CREAT, 0666);
    if (fd_tmp)
        (void)close(fd_tmp);
    msg_window = (Textsw) window_create(base_frame, TEXTSW,
				WIN_BELOW, sel_panel,
				TEXTSW_MENU, (Menu)NULL,
				TEXTSW_BROWSING, TRUE,
				TEXTSW_IGNORE_LIMIT, TEXTSW_INFINITY,
				TEXTSW_FILE, TEXTSW_TMP_FILE,
				0);
    
    (void)notify_interpose_destroy_func(base_frame, my_frame_destroyer);
}

/**********************************************************************/
static Notify_value
my_frame_destroyer(frame, status)
/**********************************************************************/
Frame frame;
Destroy_status status;

{
    int result;

    if (status == DESTROY_CHECKING) {
	result = alert_prompt(main_frame, (Event *) NULL,
				ALERT_MESSAGE_STRINGS, "Do you want to quit CG12 Diagnostics tool ?", 0,
				ALERT_BUTTON_YES, "Confirm Quit",
				ALERT_BUTTON_NO, "Cancel",
				ALERT_MESSAGE_FONT, bold_bigfont,
				ALERT_BUTTON_FONT, bold_bigfont,
				0);

	if (result == ALERT_YES) {
	    (void)unlink(TEXTSW_TMP_FILE);
	    (void)window_set(main_frame, FRAME_NO_CONFIRM, TRUE, 0);
	} else {
	    (void)notify_veto_destroy(main_frame);
	    return(NOTIFY_DONE);
	}
    }

    return(notify_next_destroy_func(frame, status));
}

/*ARGSUSED*/
/**********************************************************************/
Notify_value
pipe_report(me, fd)
/**********************************************************************/
int *me;
int fd;

{
    int cc, bcount;
    char textbuf[512];

    /* read message from pipe */
    while ((ioctl(fd, FIONREAD, &cc) != -1) && cc > 0) {
            bcount = read(fd, textbuf, (cc <= 511) ? cc : 511);
	    textbuf[bcount] = '\0';
            pmessage(textbuf);
    }
    return(NOTIFY_DONE);
}

/*ARGSUSED*/
/**********************************************************************/
static Panel_item
create_button(panel, image, client_data, proc, m1, m2, m3)
/**********************************************************************/
Panel panel;
int client_data;
Pixrect *image;
int (*proc)();
char *m1, *m2, *m3;

{
    extern int show_menu_proc();
    Panel_item item;
    Menu menu;
    Menu_item mi;
    register char **p;
    register int i;
    static int mv[] = {0, LIST_DEFAULT, LIST_ALL, LIST_SELECT};

    item = panel_create_item(panel, PANEL_BUTTON,
				PANEL_LABEL_IMAGE, image,
				PANEL_NOTIFY_PROC, proc,
				0);
    if (m1 != NULL) {
	menu = menu_create(MENU_CLIENT_DATA, client_data,
				MENU_LEFT_MARGIN, 10,
				0);
	for (p = &m1, i = 0; i < 3 && *p != NULL; p++, i++) {
	    mi = menu_create_item( MENU_STRING_ITEM, *p, mv[i+1],
				MENU_FONT, (i==0) ? bold_bigfont :
							bigfont,
				MENU_SELECTED, (i==0) ? TRUE : FALSE,
				MENU_BOXED, (i==0) ? TRUE : FALSE,
				MENU_MARGIN, 10,
				0);
	    (void)menu_set(menu, MENU_APPEND_ITEM, mi, 0);
	}

	(void)panel_set(item, PANEL_EVENT_PROC, show_menu_proc,
				PANEL_CLIENT_DATA, menu,
				0);
    }

    return(item);
}

/**********************************************************************/
static Panel_item
create_cycle_item(panel, label)
/**********************************************************************/
Panel panel;
char *label;
{
    
    Panel_item item;

    item = panel_create_item(panel, PANEL_CYCLE,
				PANEL_LABEL_STRING, label,
				PANEL_LABEL_FONT, bold_bigfont,
				PANEL_CHOICE_FONTS, bold_bigfont, 0,
				PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
				PANEL_CHOICE_STRINGS, " 1",
						      " 2",
						      " 3",
						      " 4",
						      " 5",
						      " 6",
						      " 7",
						      " 8",
						      " 9",
						      " Endless", 0,
				0);
    
    return(item);
}

/**********************************************************************/
show_menu_proc(item, event)
/**********************************************************************/
Panel_item item;
Event *event;
{
    Menu me;
    Menu_item mi;
    int mv;

    if (event_id(event) == MS_RIGHT && event_is_down(event)) {
	me = (Menu)panel_get(item, PANEL_CLIENT_DATA);
	mv = (int)menu_show(me, sel_panel, event, 0);
	if (mv != NULL) {
	    /* hightlight choice with bold_bigfont */
	    mi = menu_find(me, MENU_FONT, bold_bigfont, 0);
	    (void)menu_set(mi, MENU_FONT, bigfont, MENU_BOXED, FALSE, 0);
	    mi = menu_find(me, MENU_VALUE, mv, 0);
	    (void)menu_set(mi, MENU_FONT, bold_bigfont, MENU_BOXED, TRUE, 0);

	    /* set test selection for single board test */
	    switch ((int)menu_get(me, MENU_CLIENT_DATA)) {
		    case INTEGRATION_TEST:
			exec_test(INTEGRATION_TEST, NULL);
			break;
		    case SINGLE_BLOCK_TEST:
			sb_test_select = get_test_selection(sb_testlist, mv);
			break;
	    }
	}
    } else {
	(void)panel_default_handle_event(item, event);
    }
}

/**********************************************************************/
show_msg(text)
/**********************************************************************/
char *text;

{
    int length;

    length = (int)textsw_insert(msg_window, text, (Textsw_index)strlen(text));

    if (length != strlen(text)) {
        (void)fprintf(stderr, "Failed to insert text.\n");
    }
}

/*ARGSUSED*/
/**********************************************************************/
int_exec_proc(item, event)
/**********************************************************************/
Panel_item item;
Event *event;
{
    exec_test(INTEGRATION_TEST, NULL);
}
/*ARGSUSED*/
/**********************************************************************/
sb_exec_proc(item, event)
/**********************************************************************/
Panel_item item;
Event *event;
{
    exec_test(SINGLE_BLOCK_TEST, sb_test_select);
}
/**********************************************************************/
exec_test(type, mode)
/**********************************************************************/
int type, mode;

{
    extern char *fork_test();
    int blck_cnt, brd_cnt;
    char *errmsg;

    blck_cnt = (int)panel_get_value(loops_blck_item) + 1;
    if (blck_cnt == 10) blck_cnt = -1;
    brd_cnt = (int)panel_get_value(loops_brd_item) + 1;
    if (brd_cnt == 10) brd_cnt = -1;

    errmsg = fork_test(pipe_fd, type, blck_cnt, brd_cnt, mode, &pid);
    if (errmsg) {
	pmessage(errmsg);
	return;
    }

    /* start timer */
    start_timer();

    display_test_title(type);

    (void)window_set(sel_panel, WIN_SHOW, FALSE, 0);
    (void)window_set(disp_panel, WIN_SHOW, TRUE, 0);
}

/*ARGSUSED*/
/**********************************************************************/
Notify_value
child_finished(me, cpid, status)
/**********************************************************************/
int *me;
int cpid;
union wait *status;

{
    char errtxt[256];

    /* unregister function */
    (void)notify_set_wait3_func(main_frame, NOTIFY_FUNC_NULL, cpid);

    if (WIFSTOPPED(*status)) {
	(void)sprintf(errtxt,  "Test has been stopped by signal %d.\n", status->w_stopsig);
	pmessage(timestamp(errtxt));
    }
    if (WIFSIGNALED(*status)) {
	(void)sprintf(errtxt,  "Test has been terminated by signal %d.\n", status->w_termsig);
	pmessage(timestamp(errtxt));
    }
    if (WIFSIGNALED(*status) || WIFSTOPPED(*status) || WIFEXITED(*status))  {
	/* swap command panel */
	if (!(int)window_get(main_frame, FRAME_CLOSED)) {
	    (void)window_set(disp_panel, WIN_SHOW, FALSE, 0);
	    (void)window_set(sel_panel, WIN_SHOW, TRUE, 0);
	} else {
	    swap = 1;
	}

	/* stop timer */
	stop_timer();

	/* done */
        return(NOTIFY_DONE);
    }

    return(NOTIFY_IGNORED);
}

/*ARGSUSED*/
/**********************************************************************/
Notify_value
count_time(me, which)
/**********************************************************************/
Notify_client me;
int which;

{
    extern struct tm *gmtime();
    struct tm *tm;
    char text[10];

    secs++;
    tm = gmtime((long *)&secs);
    
    (void)sprintf(text, "%03d:%02d:%02d", tm->tm_hour, tm->tm_min,
                                                            tm->tm_sec);
    (void)panel_set(timer_item, PANEL_LABEL_STRING, text, 0);

    return(NOTIFY_DONE);
}


/*ARGSUSED*/
/**********************************************************************/
stop_proc(item, event)
/**********************************************************************/
Panel_item item;
Event *event;

{
    if (swap) {
	(void)panel_set(timer_item, PANEL_LABEL_STRING, "000:00:00", 0);
	(void)window_set(disp_panel, WIN_SHOW, FALSE, 0);
	(void)window_set(sel_panel, WIN_SHOW, TRUE, 0);
	swap = 0;
    } else {
	if (kill(pid, SIGKILL)) {
	    perror("kill");
	    pmessage("Can't stop test.\n");
	}
    }
}

/**********************************************************************/
start_timer()
/**********************************************************************/

{
    extern Notify_value count_time();

    secs = 0;
    (void)notify_set_itimer_func(main_frame, count_time,
		    ITIMER_REAL, &timer, ITIMER_NULL);
}

/**********************************************************************/
stop_timer()
/**********************************************************************/

{
    extern Notify_value count_time();

    (void)notify_set_itimer_func(main_frame, count_time,
		    ITIMER_REAL, ITIMER_NULL, ITIMER_NULL);
    if (!swap) {
	(void)panel_set(timer_item, PANEL_LABEL_STRING, "000:00:00", 0);
    }
}

/**********************************************************************/
display_test_title(type)
/**********************************************************************/
int type;

{
    char *title;
    int w;
    Rect *r;

    static char int_test[] = "CG12 Integration Test";
    static char sb_test[] = "CG12 Functional Blocks Test";

    switch (type) {
	case INTEGRATION_TEST:
		title = int_test;
		break;
	case SINGLE_BLOCK_TEST:
		title = sb_test;
		break;
    }

    (void)panel_set(test_title_item, PANEL_LABEL_STRING, title, 0);

    w = (int)window_get(main_frame, WIN_WIDTH);
    r = (Rect *)panel_get(test_title_item, PANEL_ITEM_RECT);
    (void)panel_set(test_title_item, PANEL_ITEM_X, (w-r->r_width)/2, 0);


}

/**********************************************************************/
int
get_test_selection(tl, m)
/**********************************************************************/
struct test tl[];
int m;

{
    int s;
    int i;
    struct test *tp;
    Rect *r;

    if (m == LIST_ALL) {
	return -1;
    }

    if (m == LIST_SELECT) {
	r = (Rect *)window_get(main_frame, WIN_RECT);
	s = popup_select(tl, r, bold_bigfont);
	return s;
    }

    for (tp = &tl[0], s=0, i=1; tp ; tp = tp->nexttest) {
	if (tp->dfault) {
		s |= i;
		i <<= 1;
	}
    } 
    return s;
}

/**********************************************************************/
register_notifier(npid)
/**********************************************************************/
int npid;

{
    extern Notify_value pipe_report();
    extern Notify_value child_finished();

    /* prepare to catch SIGCHLD */
    (void)notify_set_wait3_func(main_frame, child_finished, npid);
}

