static char sccsid[] = "@(#)sunbuttons.c 1.1 7/30/92 Copyright 1988 Sun Micro";
/*
 *
 * "@(#)sunbutton.c 1.1 12/15/88 Copyright Sun Microsystems"
 *
 *
 * Sunbutton
 *
 * Date : 4/13/89
 * Sunbutton test is for the button box which is connected to the
 *	cpu board through the serial port.  This test verifies
 *	whether the box is functioning or not.  
 *
 * Drivers User : 
 * Released OS : 4.0 and higher, 3.5
 *
 * compile line :
 *   cc sunbutton.c -O1 -o sunbutton -lsuntool -l sunwindow -lpixrect -lm -lc
 *
 * External files used :
 */

#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sun/fbio.h>
#include <sgtty.h>
#include <math.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>

#include <pixrect/pixrect_hs.h>
#include <pixrect/pixrect.h>
#include <pixrect/memreg.h>
#include <pixrect/memvar.h>
#include <pixrect/pr_planegroups.h>

#include <sunwindow/win_cursor.h>
#include <suntool/gfx_hs.h>
#include "sdrtns.h"			/* sundiag standard include */
#include "../../../lib/include/libonline.h"   /* online library include */

#define DEV_NOT_OPEN            3
#define SUNDIAL_ERROR           7
#define SIGBUS_ERROR            8
#define SIGSEGV_ERROR           9
#define SCREEN_NOT_OPEN         10
#define END_ERROR               11
#define SYNC_ERROR              12

/* ------------------------------- Taken from dbio.h ---------------------- */
#define DBIOBUTLITE _IOW(B, 0, int)
/*Macros for the button box */
#define event_is_32_button(event) (vuid_in_range(BUTTON_DEVID, event->ie_code))
 
 
/*These are the bit masks to turn the leds and buttons on/off */
#define BUTTON_MAP(butnum)  (butnum)
#define LED_MAP(butnum)     (1<< (butnum - 1))

#define DB_OK         0  
#define DB_OPEN_ERR  -1
#define DB_IOCTL_ERR -2
#define DB_WRITE_ERR -3
#define DB_READ_ERR  -4
#define DB_COMP_ERR  -5

#define message(s)  fprintf(stderr, (s))

#define DEVICE "/dev/dialbox"		/* the driver name for sunbutton is dialbox */

#define BLACK 0
#define WHITE 255

Window
    frame,
    panel,
    canvas;

Pixwin *dial_pixwin;

int  SpecialFlag;
int win_fd;
struct sgttyb   ttybuf;
int             origsigs;
int		dummy(){};

/*
 *  Main program starts here
 */

main(argc, argv)
    int             argc;
    char          **argv;
 
{
    extern int  process_sunbuttons_args();
    int db_fd, c_cntr = 0;

    static unsigned char diag_cmd[] = {0x10, 0, 0};
    static unsigned char expected_response[] = {0x1, 0x2};
    static unsigned char response[sizeof (expected_response)];
    static unsigned char reset_cmd[] = {0x1F, 0, 0};


    versionid = "1.1";		/* SCCS version id */
    SpecialFlag = FALSE; 

				/* begin Sundiag test */
    test_init(argc, argv, process_sunbuttons_args, dummy, (char *)NULL);
    device_name = DEVICE;
    setup_signals();

    panel_setup();
    if (SpecialFlag) {
	printf("\nSunbutton : Interactive Test.\n");
	interactive_test();  /* interactive mode test */
    } else {
	send_message(0, VERBOSE, "Non-Interactive Test.");
	if (diag_notify()) {
	  reset_signals();
	  exit(2);
	}
	send_message(0, VERBOSE, "Stopped.");
    }
    reset_signals();
    test_end();			/* Sundiag normal exit */
}

/*
 *
 * Process_sunbuttons_args                      
 * 
 * Check the command line for the      
 * argument 'diag'.  If diag is present 
 * the program will execute the manual 
 * intervention test.
 *
 */
process_sunbuttons_args(argv, arrcount)
    char          **argv;
    int           arrcount;

{
    if (strncmp(argv[arrcount], "diag", 4) == 0) {
        SpecialFlag = TRUE;
        return (TRUE);
    }
    return (FALSE);
}

/*****************************************************/
/* Setup_signals                                     */
/* */
/* Catch all interrupts and determine if it is a   */
/* user interrupt, a buserror, or a segmentation     */  
/* violation.                                        */
/* */
/* INTERRUPT HANDLERS:                               */
/* */
/* Finish_it - User interrupts status OK           */
/* Bus       - Bus error status failed.            */
/* Segv      - Segmentation violation status failed */
/* */
/*****************************************************/
setup_signals()
{

    ioctl(0, TIOCGETP, &ttybuf);
    ttybuf.sg_flags |= CBREAK;                 /* non-block mode */
    ttybuf.sg_flags &= ~ECHO;                  /* no echo */
    ioctl(0, TIOCSETP, &ttybuf);

    origsigs = sigsetmask(0);
    signal(SIGINT, finish);
    signal(SIGQUIT, finish);
    signal(SIGHUP, finish);
}

/**********************************************************/
/* Reset_signals                                          */
/* */
/* Reset all interrupt routines back to the state before */
/* the test started.                                      */
/* */
/**********************************************************/
reset_signals()
{
    ioctl(0, TIOCGETP, &ttybuf);
    ttybuf.sg_flags &= ~CBREAK;
    ttybuf.sg_flags |= ECHO;
    ioctl(0, TIOCSETP, &ttybuf);
 
    /* set up original signals */
 
    sigsetmask(origsigs);
    signal ( SIGINT, SIG_IGN );
    signal ( SIGQUIT, SIG_IGN );
    signal ( SIGHUP, SIG_IGN );
}
 


short           tool_image[256] =
{
#include "button_icon.h"
};

#include "buttons.h"

/* Local definitions */
DEFINE_ICON_FROM_IMAGE (tool_icon, tool_image);
#define BUTTON_DEVID		0x7A
#define NBUTTON			32

/* Routines */
void            pw_square (), button_on (), button_off ();
void            init_buttons ();
void            draw_buttons ();
void            no_proc (), button_event_proc ();

/*Global variables */
Window          frame,
                panel,
                canvas;
Pixwin         *button_pixwin;
int             win_fd,
                bb_fd;
long int        led_mask;

panel_setup()
{
    frame = window_create (NULL, FRAME,
                           FRAME_LABEL, "Buttontool",
                           FRAME_ICON, &tool_icon,
                           0);
    panel = window_create (frame, PANEL, 0);
 
    panel_create_item (panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
                       panel_button_image (panel, "Reset", 5, 0),
                       PANEL_NOTIFY_PROC,
                       init_buttons,
                       0);
    panel_create_item(panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
                           panel_button_image(panel, "Diagnostics", 5, 0),
                           PANEL_NOTIFY_PROC,
                           diag_notify,
                           0);
 
    window_fit_height (panel);
    win_fd = (int) window_get (frame, WIN_FD);
    if ((bb_fd = open ("/dev/dialbox", O_RDWR)) == -1) {
        perror ("Open failed for /dev/buttonbox:");
        exit (1);
    }
    win_set_input_device (win_fd, bb_fd, "/dev/dialbox");
    canvas = window_create (frame, CANVAS,
                            CANVAS_WIDTH, 448,
                            CANVAS_HEIGHT, 448,
                            WIN_HEIGHT, 448,
                            WIN_WIDTH, 448,
                            CANVAS_RETAINED, FALSE,
                            CANVAS_REPAINT_PROC, draw_buttons,
                            WIN_EVENT_PROC, button_event_proc,
                            0);
    button_pixwin = canvas_pixwin (canvas);
 
    window_fit (canvas);
    window_fit (frame);
 
    draw_buttons ();
    init_buttons();

}




void
button_event_proc (window, event, arg)
    Window          window;
    Event          *event;
    caddr_t         arg;
{
    int             button,
                    down;

    if (event_is_32_button (event)) {
	down = (int) window_get (window, WIN_EVENT_STATE, event->ie_code);
	button = BUTTON_MAP((event->ie_code - (BUTTON_DEVID << 8)));
	if (down) {
	    button_on (button);
	}
	else {
	    button_off (button);
	}
    }
}


struct pr_pos   origins_32[33] =
{
    {0, 0},
    {104, 40},
    {168, 40},
    {232, 40},
    {296, 40},
    {40, 104},
    {104, 104},
    {168, 104},
    {232, 104},
    {296, 104},
    {360, 104},
    {40, 168},
    {104, 168},
    {168, 168},
    {232, 168},
    {296, 168},
    {360, 168},
    {40, 232},
    {104, 232},
    {168, 232},
    {232, 232},
    {296, 232},
    {360, 232},
    {40, 296},
    {104, 296},
    {168, 296},
    {232, 296},
    {296, 296},
    {360, 296},
    {104, 360},
    {168, 360},
    {232, 360},
    {296, 360},
};


struct pr_pos  *but_origin = origins_32;
void
draw_buttons ()
{
    int             i;

    for (i = 1; i < NBUTTON + 1; i++) {
	pw_rop (button_pixwin, but_origin[i].x, but_origin[i].y, 64, 64,
		PIX_SRC, &off_off_icon, 0, 0);
    }
}
void
init_buttons()
{
    led_mask = 0;
    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
	perror ("Ioctl(DBIOBUTLITE)");
    }	
    draw_buttons();
}

void
button_on (but_num)
    int             but_num;
{
    int             x1,
                    y1,
                    x2,
                    y2;

    led_mask |= LED_MAP(but_num);
    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
	perror ("Ioctl(DBIOBUTLITE)");
    }
    pw_rop (button_pixwin, but_origin[but_num].x,
	    but_origin[but_num].y, 64, 64,
	    PIX_SRC, &on_on_icon, 0, 0);
}


void
button_off (but_num)
    int             but_num;
{
    int             x1,
                    y1,
                    x2,
                    y2;

    led_mask &= (~LED_MAP(but_num));
    if (ioctl (bb_fd, DBIOBUTLITE, &led_mask) == -1) {
	perror ("Ioctl(DBIOBUTLITE)");
    }
    pw_rop (button_pixwin, but_origin[but_num].x,
	    but_origin[but_num].y, 64, 64,
	    PIX_SRC, &off_off_icon, 0, 0);
}
void
pw_square (x1, y1, x2, y2, r, color)
    int             x1,
                    y1,
                    x2,
                    y2,
                    r,
                    color;
{
    struct rect     rect;

    rect.r_left = x1;
    rect.r_top = y1;
    rect.r_width = rect.r_height = 2 * r;

    pw_lock (button_pixwin, &rect);

    /* Draw the box */
    pw_vector (button_pixwin, x1, y1, x2, y1, PIX_SRC, color);
    pw_vector (button_pixwin, x2, y1, x2, y2, PIX_SRC, color);
    pw_vector (button_pixwin, x2, y2, x1, y2, PIX_SRC, color);
    pw_vector (button_pixwin, x1, y2, x1, y1, PIX_SRC, color);

    pw_unlock (button_pixwin);
}

interactive_test()
{
    window_main_loop(frame);
}


clean_up()
{
	reset_signals();
}


int
diag_notify()
{
    switch(diag_test()) {
        case DB_OK:
	    if (SpecialFlag) 
	      message("Sunbutton O.K.\n");
            send_message(0, VERBOSE, "Sunbutton O.K.");
	    return(0);
            break;
        case DB_OPEN_ERR:
	    if (SpecialFlag)
              message("Sunbutton : Cannot open device.\n");
            else
	      send_message(-DEV_NOT_OPEN, ERROR, "Cannot open device.");
	    return(-1);
            break;
        case DB_IOCTL_ERR:
	   if (SpecialFlag)
              message("Sunbutton : Ioctl failed.\n");
	   else
              send_message(-SUNDIAL_ERROR, ERROR, "ioctl(VUIDSFORMAT, VUID_NATIVE)"); 
	    return(-1);
            break;
        case DB_WRITE_ERR:
	    if (SpecialFlag)
              message("Sunbutton : Write Error.\n");
            else
	      send_message(-SUNDIAL_ERROR, ERROR, "Writing Diag Command Failed.");
	    return(-1);
            break;
        case DB_READ_ERR:
	    if (SpecialFlag)
              message("Sunbutton : No Response from Dialbox.\n");
	    else
	      send_message(-SUNDIAL_ERROR, ERROR, "No Response from Dialbox.");
	    return(-1);
            break;
        case DB_COMP_ERR:
	    if (SpecialFlag) 
              message("Sunbutton : Selftest Failed.\n");
	    else
	      send_message(-SUNDIAL_ERROR, ERROR, "Selftest Failed."); 
	    return(-1);
            break;
        default:
	    if (SpecialFlag)  
              message("Sunbutton : Weird unknown kind of error.\n");
	    else 
	      send_message(-SUNDIAL_ERROR, ERROR, "Weird unknown kind of error.");
            return(-1);
            break;
    }
}

int
diag_test()
{
    int             db_fd, c, err, vuid_format;

    /* 0x20 for no error, 0x21 for fail */
    static unsigned char expected_response_ok[] = {0x20};
    static unsigned char expected_response_er[] = {0x21};
    static unsigned char response[sizeof (expected_response_ok)];
    static unsigned char reset_cmd[] = {0x20};
    static unsigned char light_cmd_on[] = {0x75, 0xff, 0xff,0xff,0xff};
    static unsigned char light_cmd_off[] = {0x75, 0x00, 0x00,0x00,0x00};

    err = DB_OK;
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1) {
	  perror("open");
	  return(DB_OPEN_ERR);
    }
    win_remove_input_device(win_fd, "/dev/dialbox");
    
    /* set to "native mode", (This does not work for the 3.5 driver yet! */
    vuid_format = VUID_NATIVE;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
	  perror("ioctl(VUIDSFORMAT, VUID_NATIVE)");
	  err = DB_IOCTL_ERR;
    }
    /* Set up for "non-blocking" reads */
    if (fcntl(db_fd, F_SETFL, fcntl(db_fd, F_GETFL, 0) | O_NDELAY) == -1) {
	  perror("fctntl()");
	  err = DB_IOCTL_ERR;
    }
    while (read(db_fd, response, 1) > 0) {
	  ; /* just flush */
    }
    sleep(1);
    /* This does not work for the 3.5 driver yet. */
    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
	  perror("write()");
	  err = DB_WRITE_ERR;
    } else {
	  sleep(1);
	  if ((c = read(db_fd, response, 1)) != 1) {
	    err = DB_READ_ERR;
	  } else if (response[0] == expected_response_er[0]) {
	     err = DB_COMP_ERR;
	  } else if (response[0] == expected_response_ok[0]) {
	   /* OK */
           sleep(1);
           write(db_fd, light_cmd_on, sizeof (light_cmd_on));
           sleep(3);
           write(db_fd, light_cmd_off, sizeof (light_cmd_off));
	  }
    }
    sleep(1);
    write(db_fd, reset_cmd, sizeof (reset_cmd)) ;
    /* back to VUID mode */
    vuid_format = VUID_FIRM_EVENT;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
	  perror("ioctl (VUIDSFORMAT, VUID_FIRM_EVENT)");
	  err = DB_IOCTL_ERR;
    }
    while (read(db_fd, response, 1) > 0) {
	  ; /* just flush (again) */
    }
    /* re install device to window system */
    win_set_input_device(win_fd, db_fd, "/dev/dialbox");
    close(db_fd);
    return(err);

}

	
