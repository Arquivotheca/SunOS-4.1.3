#ifndef lint
static	char sccsid[] = "@(#)gp2test.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <sgtty.h>
#include <ctype.h>
#include <a.out.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <suntool/gfx_hs.h>
#include <sun/fbio.h>
#include "gp2test.h"
#include "gp2test_msgs.h"
#include "sdrtns.h"			/* standard sundiag include */
#include "../../../lib/include/libonline.h"	/* online library include */
extern char *getenv();
extern char *sprintf();
extern void exception();

int             iograb = FALSE;
int      winfd, rootfd, devtopfd, errors;
static unsigned char tred[256], tgrn[256], tblu[256];
Pixrect         *screen;
struct screen   new_screen;
int    tmpplane;
struct          sgttyb  ttybuf;
short		orig_sg_flags;	/* original mode flags for stdin */
int             origsigs, return_code;
int		CgTWO = TRUE;
static int      dummy(){return FALSE;}
/*
 *************************************************************************
 *	Sundiag GP2 test .
 *
 *	This test will perform hardware tests on the GP2 that are embeded
 *	in the microcode. It will test the floating point chip in the XP
 *	by performing GPCI commands that test matrix multiply, cliping,
 *	and point generation. Then it will test the complete GP2 hardware
 *	by having it display 5 sets of polygons on the screen.
 *************************************************************************
 */
/*
 *************************************************************************
 *	Main
 *
 *	This function gets the options from sundiag calling routine and
 *	then calls the routines to setup the window enviroment and call
 *	each test to be executed. Then if passed cleanly will clean up
 *	the window enviroment to the same condition before the test 
 *	started.
 *************************************************************************
 */

main(argc, argv)
    int argc;
    char *argv[];
{
    int	arrcount;

    versionid = "1.1";		/* SCCS version id */
    device_name = "gpone0";
    test_name = "gp2test";

    test_init(argc, argv, dummy, dummy, (char *)NULL); 

/*
 *	Get Sundiag options
 */

    verbose = 0;	/* gp2test verbose mode dees not work well*/
    setup_signals();

/*
 * probe for gp2 & cg5 boards.
 *	exit if none found.
 */
    return_code = check_gp();
    if (probe_cg() < CG_ONLY) {    /* did not find cg */
        reset_signals();
        exit(0);
    }
    run_tests();
}

/*
 ************************************************************************
 *	run_tests.
 *	
 *	This function calls the major test routines to be executed.
 *	and then reports back to the user if any errors are encountered
 *	in debug mode or when not being executed by Sundiag.
 ************************************************************************
 */
run_tests()
{
    unsigned int planes = GP2_BUFSIZ - 1;

    setup_desktop();
    init_static_blk();
    pr_putattributes(screen, &planes);
    test_hardware();
    check_input();
    test_mul_matrix();
    check_input();
    test_mul_point();
    check_input();
    test_proc_line();
    check_input();
    test_polygons();
    check_input();
    if (!CgTWO) 
        test_arbiter();
    check_input();
    if (verbose)
        gp_send_message(0, VERBOSE, "GPCI Hardware Test.");
    clean_up();
    if (verbose)
	gp_send_message(0, VERBOSE, stopped_msg);
    exit(0);
}

/*
 ****************************************************************************
 *	setup_desktop().
 *
 * If (single headed && window system ) we set up a new window. Otherwise 
 * we just do test on GP2. If we are running under window system, open up the 
 * suntools parent environment and get a new window, make 'suntools' the 
 * 'parent'. Set oldest child of new node to be the youngest child of parent.
 * Get screen size of suntools window and set the size of new window same size.
 * Put the new node into the tree. Let the "devtop" window catch the key 
 * strokes. Prevent any cursor from display, this must be done because the 
 * cursor could sneak into the display; remove the new window cursor.
 * Direct all inputs to new window and this cursor displayed over full screen.
 * The input mask would still be available if I had not NULLED it.
 * Remove the mouse. This is necessary because the mouse is still active over 
 * the other windows and clicking the mouse button will cause an interrupt.
 * I will fix it when exiting. Save the current gp2 state and enable all planes.
 ****************************************************************************
 */
setup_desktop()
{
    Cursor cursor;
    struct inputmask im;
    char *winname, *perrmsg;
    int planes;

    int	mon_color, tmpfd;
    struct	fbtype	fb_type;
    struct	fbgattr	fb_gattr;

      tmpfd=open("/dev/fb", O_RDWR);
      ioctl(tmpfd, FBIOGTYPE, &fb_type);
      if (( fb_type.fb_type == FBTYPE_SUN2GP ) && ( winname = getenv ("WINDOW_PARENT"))) {

        if ((rootfd = open(winname, O_RDONLY)) <= 0) {
	    perrmsg = errmsg(errno);
	    if ( CgTWO ) 
	       (void) sprintf(msg, rootfd_msg, CG_DEV, perrmsg);
            else (void) sprintf(msg, rootfd_msg, CR_DEV, perrmsg);
	    gp_send_message(SCREEN_NOT_OPEN, ERROR, msg);
	    finish_it();
	}
        if ((winfd = win_getnewwindow()) < 0) {
	    perrmsg = errmsg(errno);
            if ( CgTWO )
	       (void) sprintf(msg, winfd_msg, CG_DEV, perrmsg);
            else (void) sprintf(msg, winfd_msg, CR_DEV, perrmsg);
	    gp_send_message(SCREEN_NOT_OPEN, ERROR, msg);
	    finish_it();
	}
        win_setlink(winfd, WL_PARENT,
        	win_nametonumber(winname));
        win_setlink(winfd, WL_OLDERSIB,
        	win_getlink(rootfd, WL_YOUNGESTCHILD));
        win_screenget(rootfd, &new_screen);
        win_setrect(winfd, &new_screen.scr_rect);
        input_imnull(&im);
        im.im_flags |= IM_ASCII;
        win_set_kbd_mask(winfd, &im);
        win_insert(winfd);
        cursor = cursor_create((char *)0);
        win_getcursor(rootfd, cursor);
        cursor_set(cursor, CURSOR_SHOW_CURSOR, 0, 0);
        win_setcursor(rootfd, cursor);
        cursor_copy(cursor);
        cursor_destroy(cursor);
	win_grabio(winfd);
	win_remove_input_device(rootfd, new_screen.scr_msname);
        iograb = TRUE;
    }

    opengp();
    enable_vid();
    pr_getattributes(screen, &tmpplane);
    pr_getcolormap(screen, 0, 256, tred, tgrn, tblu);
    planes = 0xff;
    pr_putattributes(screen, &planes);
    close ( tmpfd );
}

/*
 **************************************************************************
 *	Remove the GP2 test window from the window system.
 ************************************************************************** 
 */
unlock_devtop()
{
    win_remove(winfd);
}
/*
 *************************************************************************
 *	Enable CG5 video so that we can see the polygons being displayed
 *	on the color monitor.
 **************************************************************************
 */
enable_vid()
{
    int fd;
    int vid;
    char fbnamestr[80];

    if (CgTWO) strcpy ( fbnamestr, CG_DEV ); else strcpy ( fbnamestr, CR_DEV );
    vid = FBVIDEO_ON;
    if ((fd = open(fbnamestr, O_RDWR)) != -1) {
        (void) ioctl(fd, FBIOSVIDEO, &vid, 0);
        (void) close(fd);
    }
}

finish_it()
{
    clean_up();
    exit(0);
}
/* 
 **************************************************************************
 *	Restore CG5 color map. Reset the signals for the keyboard.
 *	Display the cursor in proper window. Then re-install the mouse.
 *	Remove the gp2 Test window.
 **************************************************************************\
 */
clean_up()
{
    Cursor  cursor;
    int msfd;
    int flag;

    if (screen <= 0)
        exit(0);

    pr_putattributes(screen, &tmpplane);
    pr_putcolormap(screen, 0, 256, tred, tgrn, tblu);
    reset_signals();

    if (iograb) {
            cursor = cursor_create((char *)0);
            win_getcursor(rootfd, cursor);
            cursor_set(cursor, CURSOR_SHOW_CURSOR, 1, 0);
            win_setcursor(rootfd, cursor);
            cursor_destroy(cursor);
 
            sleep(1);       /* for the heck of it */
 
            flag = win_is_input_device(rootfd, "/dev/mouse");
            if (flag == 1) {
                if ((!exec_by_sundiag) || (debug))
                    gp_send_message (0, DEBUG, 
			"mouse was previously being utilized.\n");
            } else if (flag == 0) {
                if ((!exec_by_sundiag) || (debug))
                    gp_send_message (0, DEBUG, "mouse was previously removed.\n");
            } else if (flag == -1) {
                if ((!exec_by_sundiag) || (debug))
                    gp_send_message (0, DEBUG, "mouse error.\n");
            }
                        /* re-install the mouse */
            msfd = open(new_screen.scr_msname);
            if (msfd >= 0) {
                win_set_input_device(rootfd, msfd, new_screen.scr_msname);
                (void) close (msfd);
            }
            win_releaseio(winfd);
        }


    pr_destroy(screen);
    (void) close(devtopfd);
    (void) close(rootfd);
    (void) close(winfd);

}

/* 
 **************************************************************************
 * set up signals for non-blocking
 * input.  If this is not done,
 * the windows will have a queue of
 * input events to handle.
 **************************************************************************
 */
setup_signals()
{
    (void) ioctl(0, TIOCGETP, &ttybuf);
    orig_sg_flags = ttybuf.sg_flags;/* save original mode flags from stdin */
    ttybuf.sg_flags |= CBREAK;      /* non-block mode */
    ttybuf.sg_flags &= ~ECHO;       /* no echo */
    (void) ioctl(0, TIOCSETP, &ttybuf);

    origsigs = sigsetmask(0);
    (void) signal(SIGINT, finish_it);
    (void) signal(SIGQUIT, finish_it);
    (void) signal(SIGHUP, finish_it);
    (void) signal(SIGXCPU, exception );
}
/* 
 **************************************************************************
 *	Release the keyboard to accept all key strokes.
 *	set signals to the origianal signals.
 **************************************************************************
 */

reset_signals()
{
    if(!exec_by_sundiag)
        (void) printf ("resetting signals...");
    (void) ioctl(0, TIOCGETP, &ttybuf);
    ttybuf.sg_flags = orig_sg_flags; /* restore orig mode flags for stdin */
    (void) ioctl(0, TIOCSETP, &ttybuf);

    /* set up original signals */

    (void) sigsetmask(origsigs);
    /* flush the buffer */

    flush_io();
    if(!exec_by_sundiag)
        (void) printf ("done.\n");
}
/* 
 **************************************************************************
 * 	Remove extra keystrokes that have been qued up.
 **************************************************************************
 */
flush_io()
{
    int tfd, arg, knt;
    static char tmpbuf[CANBSIZ];
 
    arg = 0;
    if (devtopfd)
        tfd = devtopfd;
    else
        tfd = 0;
 
    (void) ioctl(tfd, FIONREAD, &arg);
 
    knt = CANBSIZ;
    while (arg > 0) {
        if (arg < knt)
            knt = arg;
        knt = read (tfd, tmpbuf, knt);
        arg -= knt;
    }
}

/*
 * send_message the routine which logs messages to the logfile.
 * If (exec_by_sundiag == FALSE) then the message is sent to stderr
 * instead of to sundiag. There are 3 variables for 
 *	send_message: where, msg_type,
 * and msg. "where" can indicate a defined error number. The numbers 96-100
 * are reserved by sundiag for bus errors in bus(), segmentation errors in
 * segv(), interrupts (<ctrl>c's) in finish(), command-line argument usage
 * errors (use USAGE_ERROR in your main(), see newtest.c), and RPC errors in
 * transmitting the message. If "where" is negative, the "send_message"
 * function will exit with the "where" exit code.
 * For "msg_type", use INFO, WARNING, FATAL, ERROR, or LOGFILE.
 * For msg, use a pointer (char *) to a message buffer which includes a \n.
 * The logfile message is formatted as follows:
 * (void) sprintf(fmt_msg, "%s:%s:%s:%s", device, test_name, msg_type_id, msg);
 * where device is set up by you (see process_newtest_args() in newtest.c),
 * test_name is argv[0], msg_type_id is the "msg_type" 
 * from send_message (INFO->I,
 * WARNING->W, ERROR->E, and LOGFILE->L), and msg is the msg from send_message.
 */
gp_send_message(where, msg_type, msg)
    int             where;
    int             msg_type;
    char           *msg;
{
    if ( iograb ) {
        win_releaseio( devtopfd );
        sleep(15);
    }

    send_message ( where, msg_type, msg );

    if ( iograb ) {
	win_grabio( devtopfd );
	sleep(5);
    }
}

