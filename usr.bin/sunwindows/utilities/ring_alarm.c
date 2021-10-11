#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ring_alarm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif
#endif

#include	<stdio.h>
#include	<string.h>
#include	<fcntl.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/file.h>
#include        <sys/ioctl.h>
#include        <sys/param.h>
#include        <sundev/kbd.h>
#include        <sundev/kbio.h>
#include        <pixrect/pixrect.h>
#include        <sunwindow/defaults.h>
#include        <sunwindow/rect.h>
#include        <sunwindow/rectlist.h>
#include 	<sunwindow/cms.h>
#include	<sunwindow/pixwin.h>
#include 	<sunwindow/win_screen.h>
#include 	<sunwindow/win_struct.h>
#include	<suntool/window.h>

/*
 * ring_alarm
 */

main()
{
	extern	char	*getenv();
	extern	int	select(), win_get_alarm();
        register struct pixwin  *pw;
	int     result, kbdfd, windowfd, framenumber, ret, cmd, pid;
	char	name[BUFSIZ], *winname;
	static	Win_alarm	ra_alarm;
	int	beeps, flashes;
	struct	timeval	tv;
	struct screen screen;
	Rect	r;
	Bool	win_do_audible_bell;
	Bool	win_do_visible_bell;

	result = win_get_alarm(&ra_alarm);
	if (result == -1) {
		fprintf (stderr, "ring_alarm: WINDOW_ALARM is NULL; using default values.\n");

	}

	if (result == -2) {
                fprintf (stderr, "ring_alarm: WINDOW_ALARM has illegal values; using default values.\n");
 
        }

	beeps = ra_alarm.beep_num;
	flashes = ra_alarm.flash_num;
	tv = ra_alarm.beep_duration;

	if (beeps<=0 && flashes<=0)
                exit(1);

	/* Get defaults for bell if first time */
        win_do_audible_bell = defaults_get_boolean(
            "/SunView/Audible_Bell", (Bool)TRUE, (int *)NULL);
        win_do_visible_bell = defaults_get_boolean(
            "/SunView/Visible_Bell", (Bool)TRUE, (int *)NULL);

	if (( pid = fork ()) == -1) {
                perror ("ring_alarm: fork failed.");
                exit(1);
        }
 
        if (pid != 0) {
                exit(1);
        }

	if ( (winname = getenv ("WINDOW_ME")) == NULL ) {
                (void)fprintf (stderr, "ring_alarm:  WINDOW_ME not found!\n");
                exit(1);
        }
        if ( (windowfd = open (winname, O_RDWR, 0)) == -1 ) {
                exit(1);
        }
        if ( (framenumber = win_getlink (windowfd, WL_ENCLOSING)) == -1 ) {
                fprintf (stderr, "ring_alarm:  cannot get framenumber.\n");
                exit(1); 
        }
        (void)close (windowfd);         /* Need to reuse this one.*/ 
         
        (void)win_numbertoname (framenumber, name);
	if ( (windowfd = open (name, O_RDWR, 0)) == -1 ) {
                perror ("ring_alarm:  open window border");
                exit(1);
        }        
        if ( (pw = pw_open (windowfd)) == (Pixwin *)NULL ) {
                fprintf (stderr, "ring_alarm:  cannot get pixwin.\n");
                exit(1);
        }

	(void)win_getsize (windowfd, &r);

	if (windowfd >= 0 ) {
                /* Get screen keyboard name */
                win_screenget(windowfd, &screen);
        }

        /*
         * Open the keyboard.
         */
	if (beeps > 0 && win_do_audible_bell) {
        	if ( (kbdfd = open (screen.scr_kbdname, O_RDWR, 0)) == -1 ) {
               		perror ("ring_alarm:  cannot open keyboard");
                	exit(1);
		}
        }

	 while (beeps>0 || flashes>0) {
		if (flashes>0 && win_do_visible_bell) {
                        /* Flash pw */
                       	(void)pw_writebackground(pw, 0, 0, r.r_width, r.r_height, PIX_NOT(PIX_DST));
                       	pw_show(pw);
                }
                if (beeps>0 && win_do_audible_bell) {
                       	/* Start bell */
                       	cmd = KBD_CMD_BELL;
                       	(void) ioctl(kbdfd, KIOCCMD, &cmd);
                }

		/* Wait here for duration time. */
                ret = select (0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &tv);
		if (beeps>0 && win_do_audible_bell) {
			beeps--;
                        /* Stop bell */
                        cmd = KBD_CMD_NOBELL;
                        (void) ioctl(kbdfd, KIOCCMD, &cmd);
                }
                if (flashes>0 && win_do_visible_bell) {
			flashes--;
                        /* Turn off flash */
                        (void)pw_writebackground(pw, 0, 0, r.r_width, r.r_height, PIX_NOT(PIX_DST));
                        pw_show(pw);
                }
		if (beeps>0 || flashes>0)
			ret = select (0, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &tv);
	    }
	if (win_do_audible_bell) (void) close (kbdfd);
	exit(0);
}

