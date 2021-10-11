static char sccsid[] = "@(#)sundials.c 1.1 7/30/92 Copyright 1988 Sun Micro";
/*
 *
 * Sundials
 *
 * Date : 10/12/88
 * Sundial test is for the dials box which is connected to the
 *	cpu board through the serial port.  This test verifies
 *	whether the box is functioning or not.  
 *
 * Drivers User : 
 * Released OS : 4.0 and higher, 3.5
 *
 * compile line :
 *   cc sundials.c -O1 -o sundials -lsuntool -l sunwindow -lpixrect -lm -lc
 *
 * External files used :
 */

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sun/fbio.h>
#include <sgtty.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
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
#include "sundiag_rpcnums.h"
#include "sdrtns.h"		/* sundiag standard header file */ 
#include "../../../lib/include/libonline.h"   /* online library include */


#define DEV_NOT_OPEN            3
#define SUNDIAL_ERROR           7
#define SIGBUS_ERROR            8
#define SIGSEGV_ERROR           9
#define SCREEN_NOT_OPEN         10
#define END_ERROR               11
#define SYNC_ERROR              12

#define DB_OK         0  
#define DB_OPEN_ERR  -1
#define DB_IOCTL_ERR -2
#define DB_WRITE_ERR -3
#define DB_READ_ERR  -4
#define DB_COMP_ERR  -5


#define message(s)  fprintf(stderr, (s))

#define DEVICE "/dev/dialbox"		/* the driver name for sundials is dialbox */

#define NDIALS 8
/* converts dial units (64th of degrees) to radians */
#define DIAL_TO_RADIANS(n) \
        ((float)(n) * 2.0 * M_PI) / (float) DIAL_UNITS_PER_CYCLE

/* these are defined in <sundev/dbio.h>, but since that may not be installed */
/* they are also defined here */

#ifndef DIAL_DEVID
#define DIAL_DEVID              0x7B
#endif  DIAL_DEVID

#define event_is_dial(event)    (vuid_in_range(DIAL_DEVID, event->ie_code))
#define DIAL_NUMBER(event_code) (event_code & 0xFF)
#define DIAL_UNITS_PER_CYCLE (64*360)


#define BLACK 0
#define WHITE 255

Window
    frame,
    panel,
    canvas;

Pixwin *dial_pixwin;

short tool_image[256] =
{
#include "dial_icon.h"
};
 
DEFINE_ICON_FROM_IMAGE(tool_icon, tool_image);
 
void dial_event_proc(), draw_dials();
int diag_notify();
int  SpecialFlag;
int win_fd;
struct sgttyb   ttybuf;
int             origsigs;

char		msg[MESSAGE_SIZE];
int  		dummy(){};

/*
 *  Main program starts here
 */

main(argc, argv)
    int             argc;
    char          **argv;
 
{
    extern int process_sundials_args();
    int db_fd, c_cntr = 0;

    static unsigned char diag_cmd[] = {0x10, 0, 0};
    static unsigned char expected_response[] = {0x1, 0x2};
    static unsigned char response[sizeof (expected_response)];
    static unsigned char reset_cmd[] = {0x1F, 0, 0};


    versionid = "1.1";
    SpecialFlag = FALSE; 

				/* Begin Sundiag test */
    test_init(argc, argv, process_sundials_args, dummy, (char *)NULL);
    device_name = DEVICE;
    setup_signals();

    panel_setup();
    if (SpecialFlag) {
	printf("\nSundials : Interactive Test.\n");
	interactive_test();  /* interactive mode test */
    } else {
	send_message(0, VERBOSE, "\nSundials : Non-Interactive Test.\n");
	if (diag_notify()) {
	  reset_signals();
	  exit(2);
	}
	send_message(0, VERBOSE, "%s: Stopped.\n", test_name);
     }
     reset_signals();
     test_end();		/* Sundiag normal exit */
}

/*
 *
 * Process_sundial_args                      
 * 
 * Check the command line for the      
 * argument 'diag'.  If diag is present 
 * the program will execute the manual 
 * intervention test.
 *
 */
process_sundials_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
    if (strncmp(argv[arrcount], "diag", 4) == 0) 
        SpecialFlag = TRUE;
    else
	return (FALSE);

    return (TRUE);
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
 


/*
 *
 * probe_sundials()
 *
 * This routine determines whether the sundials is
 * installed on system and returns the following codes :
 *
 * 	value = 0 = sundials is not present 
 *	value = 1 = sundials is present
 *
 */
probe_sundials()
{
    int             db_fd, count, vuid_format;

    int	dials;
    static unsigned char reset_cmd[] = {0x1F, 0, 0};

    count = 0;
    dials = 0; /* 0 = dialbox is not present */
	       /* 1 = dialbox is present */

    if ((open("/dev/dialbox", O_RDWR)) == -1) {
	send_message(-DEV_NOT_OPEN, ERROR, "Open failed for /dev/dialbox.");
    }
  
    win_remove_input_device(win_fd, "/dev/dialbox");

    /* set to "native mode", (This does not work for the 3.5 driver yet */
    vuid_format = VUID_NATIVE;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
        send_message(-SUNDIAL_ERROR, ERROR, "ioctl(VUIDSFORMAT, VUID_NATIVE) error.\n");
    }
 
    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
	count = 1;
    }
    vuid_format = VUID_FIRM_EVENT;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
	send_message(-SUNDIAL_ERROR, ERROR, "ioctl(VUIDSFORMAT, VUID_EVENT) error.\n");
    }
    if (count)
	send_message(-SUNDIAL_ERROR, ERROR, "Could not talk to Dialbox.\n"); 
    dials = 1;    /* dialbox is present */
    return(dials);
}

panel_setup()
{
    int db_fd;

    dial_reset();
    frame = window_create(NULL, FRAME,
                          FRAME_LABEL, "Dialtool",
                          FRAME_ICON, &tool_icon,
                          0);
    panel = window_create(frame, PANEL, 0);
    panel_create_item(panel, PANEL_BUTTON, PANEL_LABEL_IMAGE,
                      panel_button_image(panel, "Diagnostics", 5, 0),
                      PANEL_NOTIFY_PROC,
                      diag_notify,
                      0);
    window_fit_height(panel);
    win_fd = (int)window_get(frame, WIN_FD);
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1)  {
	perror("Open failed for /dev/dialbox:");
        exit(1);
    }

    win_set_input_device(win_fd, db_fd, "/dev/dialbox");
    close(db_fd); /* window system now has its own copy */
 
    canvas = window_create(frame, CANVAS,
                            CANVAS_WIDTH, 256,
                            CANVAS_HEIGHT, 512,
                            WIN_HEIGHT, 512,
                            WIN_WIDTH, 256,
                            CANVAS_RETAINED, FALSE,
                            CANVAS_REPAINT_PROC, draw_dials,
                            WIN_EVENT_PROC, dial_event_proc,
                            0);
    dial_pixwin = canvas_pixwin(canvas);
 
    window_fit(canvas);
    window_fit(frame);
 
    draw_dials();
}
interactive_test()
{
    window_main_loop(frame);
}

static float angle[9];
 
dial_reset()
{
    int i;
    for (i=0; i < NDIALS; i++) {
        angle[i] = M_PI;
    }
}      
 
pw_circle(pw, cx, cy, r, color)
        Pixwin *pw;
        int cx, cy, r, color;
{
        int x, y, s;
        struct rect rect;

        x = 0;
        y = r;
        s = 3 - (2 *y);

        rect.r_left = cx - r;
        rect.r_top = cy - r;
        rect.r_width = rect.r_height = 2 * r;

        pw_lock(pw, &rect);
        while(y>x)
        {
                put(pw, cx, cy, x, y, color);
                if (s<0)
                        s+=4*(x++)+6;
                else
                        s+=4*((x++)-(y--))+10;
        }
        if (x==y)
                put(pw, cx, cy, x, y, color);
        pw_unlock(pw);
}

put(pw,cx, cy, x, y, color)
Pixwin *pw;
int cx, cy, x, y, color;
{
        int i,j,k,tmp;

        for (i=0; i<2; i++)
        {
                for (j=0; j<2; j++)
                {
                        for (k=0; k<2; k++)
                        {
                                pw_put(pw, x+cx, y+cy, color);
                                x = (-x);
                        }
                        y = (-y);
                }
                tmp=x;
                x=y;
                y=tmp;
        }
}

void
dial_event_proc(window, event, arg)
    Window window;
    Event *event;
    caddr_t arg;
{
    int i;
    static int delta, absolute, my_absolute;

    if (event_is_dial(event)) {
            i = DIAL_NUMBER(event->ie_code);
            delta = (int)window_get(canvas, WIN_EVENT_STATE, event->ie_code);
            draw_dial(i, angle[i], BLACK);
            angle[i] += DIAL_TO_RADIANS(delta);
            draw_dial(i, angle[i], WHITE);
    }
}

struct pr_pos origins_8[8] =
{
    {64,  64},
    {64, 192},
    {64, 320},
    {64, 448},
    {192,  64},
    {192, 192},
    {192, 320},
    {192, 448},
};

struct pr_pos origins_9[9] =
{
    {64,  64},  {192, 64}, {320, 64},
    {64, 192},  {192, 192}, {320, 192},
    {64, 320},  {192, 320}, {320, 320}
};

struct pr_pos *dial_origin = origins_8;

void
draw_dials()
{
    int i;
    for (i=0; i < 8; i++) {
        pw_circle(dial_pixwin, dial_origin[i].x, dial_origin[i].y,
                  48, WHITE);
        draw_dial(i, angle[i], WHITE);
    }
}


draw_dial(dialnum, angle, color)
    int dialnum;
    float angle;
    int color;
{
    struct pr_pos to;

    to.x = dial_origin[dialnum].x + (int)(sin(-angle) * 46.0);
    to.y = dial_origin[dialnum].y + (int)(cos(-angle) * 46.0);
    pw_vector(dial_pixwin, dial_origin[dialnum].x,
          dial_origin[dialnum].y,
          to.x,
          to.y,
          PIX_SRC, color);
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
	      message("Sundials O.K.\n");
	    send_message(0, VERBOSE, "%s:Sundials O.K.\n", test_name);
	    return(0);
            break;
        case DB_OPEN_ERR:
	    if (SpecialFlag)
              message("Sundials : Cannot open device.\n");
            else
	      send_message(-DEV_NOT_OPEN, ERROR, "Cannot open device.\n");
	    return(-1);
            break;
        case DB_IOCTL_ERR:
	   if (SpecialFlag)
              message("Sundials : Ioctl failed.\n");
	   else
              send_message(-SUNDIAL_ERROR, ERROR, "ioctl(VUIDSFORMAT, VUID_NATIVE)\n"); 
	    return(-1);
            break;
        case DB_WRITE_ERR:
	    if (SpecialFlag)
              message("Sundials : Write Error.\n");
            else
	      send_message(-SUNDIAL_ERROR, ERROR, "Writing Diag Command Failed.\n");
	    return(-1);
            break;
        case DB_READ_ERR:
	    if (SpecialFlag)
              message("Sundials : No Response from Dialbox.\n");
	    else
	      send_message(-SUNDIAL_ERROR, ERROR, "No Response from Dialbox.\n");
	    return(-1);
            break;
        case DB_COMP_ERR:
	    if (SpecialFlag) 
              message("Sundials : Selftest Failed.\n");
	    else
	      send_message(-SUNDIAL_ERROR, ERROR, "Selftest Failed.\n"); 
	    return(-1);
            break;
        default:
	    if (SpecialFlag)  
              message("Sundials : Weird unknown kind of error.\n");
	    else 
	      send_message(-SUNDIAL_ERROR, ERROR, "Weird unknown kind of error.\n");
            return(-1);
            break;
    }
}

int
diag_test()
{
    int             db_fd, c, err, vuid_format;

    static unsigned char diag_cmd[] = {0x10, 0, 0};
    static unsigned char expected_response[] = {0x1, 0x2};
    static unsigned char response[sizeof (expected_response)];
    static unsigned char reset_cmd[] = {0x1F, 0, 0};

    err = DB_OK;
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1) {
        return(DB_OPEN_ERR);
    }
    win_remove_input_device(win_fd, "/dev/dialbox");

    /* set to "native mode", (This does not work for the 3.5 driver yet! */
    vuid_format = VUID_NATIVE;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
        err = DB_IOCTL_ERR;
    }
    /* Set up for "non-blocking" reads */
    if (fcntl(db_fd, F_SETFL, fcntl(db_fd, F_GETFL, 0) | O_NDELAY) == -1) {
        err = DB_IOCTL_ERR;
    }
    while (read(db_fd, response, 1) > 0) {
        ; /* just flush */
    }
    /* This does not work for the 3.5 driver yet. */
    if (write(db_fd, diag_cmd, sizeof (diag_cmd)) != sizeof (diag_cmd)) {
        err = DB_WRITE_ERR;
    } else {
        sleep(1);
        if ((c = read(db_fd, response, 2)) != 2) {
            err = DB_READ_ERR;
        } else if (strncmp(response, expected_response, sizeof (expected_response))) {
             err = DB_COMP_ERR;
        } else {
            ; /* OK */
        }
    }    
    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
         err = DB_WRITE_ERR;
    }
    /* back to VUID mode */
    vuid_format = VUID_FIRM_EVENT;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
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

	
