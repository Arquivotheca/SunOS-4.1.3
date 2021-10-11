#ifndef lint
static  char sccsid[] = "@(#)desktop.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <sys/file.h>
#include <sys/filio.h>
#include <string.h>
#include <suntool/sunview.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */

#define	DEVNAMESIZE	20
#define	FB_DEVICE_NAME	"/dev/fb"

int 		window_locked = FALSE;
static
int		root_fd = -1,
		winfd = -1;

static
char		devname[DEVNAMESIZE];

static
struct screen	new_screen;

static 
struct fbgattr fbgattr;

/* this is a special version of the send_message just for frame buffer
 * tests */
void
fb_send_message(where, msg_type, msg)
    int             where;
    int             msg_type;
    char           *msg;    
{
    if (window_locked)  (void)win_releaseio(winfd);
    send_message (where, msg_type, msg);
    if (window_locked) (void)win_grabio(winfd);
}


/**********************************************************************/
lock_desktop(dev)
/**********************************************************************/
char *dev;
/* Determine if SunView (or XView) is running on the the current
 * frame buffer. If yes, lock the screen for test operations and
 * return 0. Otherwise return -1.
 */

{
    int match_name();
    Cursor cursor;
    struct inputmask im;

    func_name = "lock_desktop";
    TRACE_IN

    root_fd = -1;
    (void)strcpy(devname, FB_DEVICE_NAME);
    if (win_enumall(match_name) == -1) {
	(void)fb_send_message(SKIP_ERROR, CONSOLE, "problem enumerating windows\n");
	TRACE_OUT
	return -1;
    }
    if (root_fd >= 0) {
	int fb_fd;
	fb_fd = open(devname, O_RDONLY);
	if (ioctl(fb_fd, FBIOGATTR, &fbgattr) < 0) {
	    (void)fb_send_message(SKIP_ERROR, CONSOLE, "problem getting frame buffer type.\n");
	    (void)close(fb_fd);
	    (void)close(root_fd);
	    root_fd = -1;
	    TRACE_OUT
	    return -1;
	 } else if (fbgattr.real_type!= FBTYPE_SUNGP3) {
	    (void)close(root_fd);
	    (void)close(fb_fd);
	    root_fd = -1;
	}
    }

    if (root_fd < 0 ) {
	root_fd = -1;
	(void)strcpy(devname, dev);
	if (win_enumall(match_name) == -1) {
	    (void)fb_send_message(SKIP_ERROR, CONSOLE, "problem enumerating windows\n");
	    TRACE_OUT
	    return -1;
	}
    }

    /* No need to lock if window system is not running */
    if (root_fd < 0 ) {
	TRACE_OUT
	return -1;
    }
	
    /* get new window */
    winfd = win_getnewwindow();
    if (winfd < 0) {
	(void)fb_send_message(SKIP_ERROR, CONSOLE, "problem getting frame buffer type.\n");
	TRACE_OUT
	return -1;
    }
    /* link new window to the existing window tree on frame buffer */
    (void)win_setlink(winfd, WL_PARENT, win_fdtonumber(root_fd));
    (void)win_setlink(winfd, WL_OLDERSIB,
				win_getlink(root_fd, WL_YOUNGESTCHILD));

    /* set new window as big as the whole screen */
    (void)win_screenget(root_fd, &new_screen);
    (void)win_setrect(winfd, &new_screen.scr_rect);


    /* set kbd input mask */
    (void)input_imnull(&im);
    im.im_flags |= IM_ASCII; /* catch 0-127 */
    (void)win_set_kbd_mask(winfd, &im);

    /* put the new node into the tree. */
    (void)win_insert(winfd);

    /* Prevent any cursor from display.
     * This must be done because the cursor
     * could sneak into the display.
     */
    /* remove the new window cursor */
    cursor = cursor_create(0);
    (void)win_getcursor(winfd, cursor);
    (void)cursor_set(cursor, CURSOR_SHOW_CURSOR, FALSE, 0);
    (void)win_setcursor(winfd, cursor);

    /* remove the mouse.
     * this is necessary because the mouse
     * is still active over the other windows and
     * clicking the mouse button will cause an interrupt.
     */
    (void)win_remove_input_device(root_fd,
			    new_screen.scr_msname);
    (void)win_grabio(winfd);
    window_locked = TRUE;

    TRACE_OUT
    return root_fd;
}

/**********************************************************************/
unlock_desktop()
/**********************************************************************/
/* Unlock the desktop */
{
    int msfd;
    Cursor cursor;

    func_name = "unlock_desktop";
    TRACE_IN

    if (root_fd >= 0) {
	(void)win_releaseio(winfd);
	window_locked = FALSE;
	/* restore the window cursor */
	cursor = cursor_create(0);
	(void)win_getcursor(winfd, cursor);
	(void)cursor_set(cursor, CURSOR_SHOW_CURSOR, TRUE, 0);
	(void)win_setcursor(winfd, cursor);


	/* re-install the mouse */
	msfd = open(new_screen.scr_msname, O_RDWR );
	if (msfd >= 0) {
		(void)win_set_input_device(root_fd, msfd,
				new_screen.scr_msname);
		(void)close (msfd);
	}

	(void)close(winfd);
	(void)close(root_fd);
	root_fd = winfd = -1;
    }

    TRACE_OUT
    return 0;
}
     
/**********************************************************************/
static
match_name(fd)
/**********************************************************************/
int fd;

{
    struct  screen screen;

    func_name = "match_name";
    TRACE_IN

    if (root_fd >= 0) {
	TRACE_OUT
	return 0;
    }

    (void)win_screenget(fd, &screen);
    if (strncmp(devname, screen.scr_fbname, SCR_NAMESIZE) == 0) {
	root_fd = open(screen.scr_rootname, O_RDWR);
    }

    TRACE_OUT
    return 0;

}

/**********************************************************************/
check_key()
/**********************************************************************/
/* exit immediately if Ctrl-C has been hit during a screen lock,
 * otherwise, return 0 */

{
    Event event;
    int count;

    func_name = "check_key";
    TRACE_IN

    if (winfd < 0) {
	TRACE_OUT
	return 0;
    }

    count = 0;

    (void)ioctl(winfd, FIONREAD, &count);
    if (count > 0) {
	do {
	    (void)input_readevent(winfd, &event);
	    if (event_id(&event) == 0x3) { /* Ctrl-C */
		/*
		(void)unlock_desktop();
		*/
		/* Exit with no error */
		(void)stop_draw();
		(void)finish();
	    }
	    (void)ioctl(winfd, FIONREAD, &count);
	} while (count > 0);
    }

    TRACE_OUT
    return 0;
}

