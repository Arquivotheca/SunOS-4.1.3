#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)overview.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Overview.c - Run the second argument to overall in an environment that
 *		overlays a running SunWindows environment (e.g., suntools).
 *		Programs read ASCII from stdin.
 *
 *		Can run SunWindows based programs (read input from a windowfd)
 *		with overall if use -w flag.  The -w must come before the
 *		command.
 */

#include <suntool/sunview.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sundev/kbd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

struct cbuf {
    char               *cb_rbp;	   /* read pointer */
    char               *cb_wbp;	   /* write pointer */
    char               *cb_ebp;	   /* end of buffer */
    char                cb_buf[8192];
};

typedef	struct	ptytty {
    struct cbuf         inbuf;			/* input buffer */
    struct cbuf         outbuf;			/* output buffer */
    /* pty and subprocess */
    int                 pty;			/* pty file descriptor */
    int                 tty;			/* tty file descriptor */
    int                 ttyslot;		/* ttyslot in utmp for tty */
    int                 (*output) ();		/* handle ouput from program */
    int			waiting_for_pty_input;	/* Accelerator to avoid
						 * excessive notifier activity*/
    int			waiting_for_pty_output;	/* Accelerator to avoid
						 * excessive notifier activity*/
    Notify_func		cached_pri;		/* Default prioritizer */
} Ptytty;

static Ptytty * ptytty_create();

static short ic_image[258] = {
#include <images/overview.icon>
};
mpr_static(overic_mpr, 64, 64, 1, ic_image);

static	struct icon icon = {64, 64, (struct pixrect *)NULL, 0, 0, 64, 64,
	    &overic_mpr, 0, 0, 0, 0, NULL, (struct pixfont *)NULL,
	    ICON_BKGRDCLR};

static	struct pixrect pr = {0, 0, 0, 0, 0};
static	struct cursor newcurs = {0, 0, 0, &pr};	/* Blank cursor */

#ifdef  get_cursor
static	Cursor oldcurs_ptr;	/* Remembered cursor */
#endif  get_cursor

static	int winfd = -1;		/* Frame window fd */
static	struct screen screen;	/* Screen data */
static	Frame frame;		/* Frame with no subwindows */
static	int taken_over_screen;	/* Has frame taken over screen? */

static	char *command;
static	char **arglist;

static	int over_pid;		/* pid of child */
static	int suspended;		/* Is child in suspended state? */

static	int window_prog;	/* Is program a SunWindows based program? */

static	Notify_value	event_interposer();
static	Notify_value	child_process_state_change();

static	Ptytty *pty_tty;	/* pty assistance */
static	int overview_handle_output();
static	void send_escape_sequence();
static	char *strsetwithdecimal();

char	*calloc(), *strcpy();

#ifdef STANDALONE
main(argc, argv)
#else
overview_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char	name[WIN_NAMESIZE];	
	struct	icon *icon_use;

	/* Create frame */
	argc--; argv++;
	frame = window_create((Window)0, FRAME,
	    FRAME_ICON,		&icon,
	    FRAME_ARGC_PTR_ARGV,&argc, argv,
	    0);
	/* Look for flags before command name */
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'w':
			window_prog = 1;
			break;
		default:
			{}
		}
		argv++;
		argc--;
	}
	/* Get program name and args */
	if (argc > 0) {
	    extern	char *rindex();

	    arglist = argv;
	    command = *arglist;
	    /* Get icon that are using */
	    icon_use = (struct icon *)(LINT_CAST(window_get(frame, FRAME_ICON)));
	    /* Strip off command back to first backslash */
	    icon_use->ic_text = rindex(command, '/');
	    if (!icon_use->ic_text)
		icon_use->ic_text = command;
	    else
		icon_use->ic_text++;
	} else {
	    (void)fprintf(stderr,
		"Need to specify program name to run under overview.\n");
	    exit(1);
	}
	/* Modify frame */
	(void)window_set(frame,
	    FRAME_OPEN_RECT,	window_get(frame, WIN_SCREEN_RECT),
	    FRAME_ICON,		icon_use,
	    0);
	/* Interpose frame's event stream */
	(void) notify_interpose_event_func(frame,
	    event_interposer, NOTIFY_SAFE);
	winfd = (int) window_get(frame, WIN_FD);
	/* Set environment so that can run SunWindows programs */
	(void)win_fdtoname(winfd, name);	
	(void)we_setgfxwindow(name);	
	(void)we_setmywindow(name);	
	/* Get screen */
	(void)win_screenget(winfd, &screen);
	/* Make pty */
	pty_tty = ptytty_create(overview_handle_output);
	/*
	 * Set so mouse and ascii input will get stopped at this window
	 * instead of being directed to underlaying windows.
	 */
	(void)window_set(frame,
	    WIN_CONSUME_PICK_EVENT,	WIN_MOUSE_BUTTONS,
	    WIN_CONSUME_KBD_EVENTS,
		WIN_ASCII_EVENTS, WIN_LEFT_KEYS, WIN_RIGHT_KEYS,
		WIN_TOP_KEYS, 0,
						/* See event_interposer */
	    WIN_SHOW,			TRUE,
	    0);
	if (!window_get(frame, FRAME_CLOSED))
		takeover();
	(void)notify_start();
	if (suspended) {
		(void)killpg(suspended, SIGKILL);
		suspended = 0;
	}
	exit(0);
}
static
Notify_value
event_interposer(frame_local, event, arg, type)
    Window    frame_local;
    Event    *event;
    Notify_arg arg;
    Notify_event_type type;
{
    if (taken_over_screen) {
        char    c = event_id(event);

        /* Send ASCII to child */
        if (event_is_down(event)) {
            if (event_is_ascii(event))
                (void) ptytty_input(pty_tty, &c, 1);
            else if (event_is_key_left(event))
                send_escape_sequence(
                    event_id(event) - KEY_LEFTFIRST + LEFTFUNC);
            else if (event_is_key_right(event))
                send_escape_sequence(
                    event_id(event) - KEY_RIGHTFIRST + RIGHTFUNC);
            else if (event_is_key_top(event))
                send_escape_sequence(
                    event_id(event) - KEY_TOPFIRST + TOPFUNC);
            else
                return (NOTIFY_IGNORED);
        } else
            return (NOTIFY_IGNORED);

        return (NOTIFY_DONE);
    } else {
        Notify_value value;

        value = notify_next_event_func(frame_local, (Notify_event)(
            LINT_CAST(event)), arg, type);
        /* See if not iconic */
        if (!window_get(frame_local, FRAME_CLOSED)) {
            /* Takeover if not already */
            takeover();
        }
        return (value);
    }
}

static	void
send_escape_sequence(id)
	short	id;
{
	char	buf[10];
	char	*cp;
	u_int	entry;
	char	c;

	/* Get string going to send */
	entry = id;
	cp = strsetwithdecimal(&buf[0], entry, sizeof (buf) - 1);
	/* Send escape sequence */
	c = '\033'; /*Esc*/
	(void) ptytty_input(pty_tty, &c, 1);
	c = '[';
	(void) ptytty_input(pty_tty, &c, 1);
	while (*cp != '\0') {
		(void) ptytty_input(pty_tty, cp, 1);
		cp++;
	}
	c = 'z';
	(void) ptytty_input(pty_tty, &c, 1);
}

static	char *	/* From sundev/kbd.c */
strsetwithdecimal(buf, val, maxdigs)
	char	*buf;
	u_int	val, maxdigs;
{
	int	hradix = 5;
	char	*bp;
	int	lowbit;
	char	*tab = "0123456789abcdef";

	bp = buf + maxdigs;
	*(--bp) = '\0';
	while (val) {
		lowbit = val & 1;
		val = (val >> 1);
		*(--bp) = tab[val % hradix * 2 + lowbit];
		val /= hradix;
	}
	return (bp);
}

static
takeover()
{
	if (taken_over_screen)
		return;
	if (over_pid == 0) {
		if ((over_pid = ptytty_fork(pty_tty, command, arglist)) < 0) {
			perror("fork");
			exit(1);
		}
		/* Wait for child process to die */
		(void) notify_set_wait3_func(frame,
		    child_process_state_change, over_pid);
	}
#ifdef	get_cursor
	/* Remember old cursor */
	oldcurs_ptr = (Cursor) window_get(frame, WIN_CURSOR);
	/* Give frame an invisible cursor */
	(void)window_set(frame, WIN_CURSOR, &newcurs, 0);
#else
	/* Give frame an invisible cursor */
	(void)win_setcursor(winfd, (Cursor)(LINT_CAST(&newcurs)));
#endif
	/* Prepare framebuffer */
	clear_full_framebuffer();
	if (!window_prog) {
		int fd;
		int fdflags;
		char buf[30];

		/* Direct all keyboard input to over window */
		(void)win_grabio(winfd);
		/* Turn off mouse (application will read directly) */
		(void)win_remove_input_device(winfd, screen.scr_msname);
		/* flush mouse */
		fd = open(screen.scr_msname, 0);
		/* make non-blocking so will generate an error when empty */
		if ((fdflags = fcntl(fd, F_GETFL, 0)) == -1)
			exit(1);
		fdflags |= FNDELAY;
		if (fcntl(fd, F_SETFL, fdflags) == -1)
			exit(1);
		while (read(fd, buf, sizeof buf) > 0)
			;
		(void)close(fd);
	}
	taken_over_screen = 1;
	if (suspended) {
		(void)killpg(suspended, SIGCONT);
		suspended = 0;
	}
}

static
clear_full_framebuffer()
{
	int	fullplanes = 255;
	struct	pixwin *pw = pw_open(winfd);

	/*
	 * Lock screen before clear so don't clobber frame buffer while
	 * cursor moving.
	 */
	(void)pw_lock(pw, &screen.scr_rect);
	/* Enable writing to entire depth of frame buffer.  */
	(void)pr_putattributes(pw->pw_pixrect, &fullplanes);
	/* Clear entire frame buffer */
	(void)pr_rop(pw->pw_pixrect, screen.scr_rect.r_left, screen.scr_rect.r_top,
	 screen.scr_rect.r_width, screen.scr_rect.r_height, PIX_CLR, 
	 (Pixrect *)0, 0, 0);
	/* Unlock screen  */
	(void)pw_unlock(pw);
	(void)pw_close(pw);
}

/* ARGSUSED */
static
Notify_value
child_process_state_change(frame_local, pid, status, rusage)
	Window	frame_local;
	int	pid;
	union	wait *status;
	struct	rusage *rusage;
{
	if (WIFSTOPPED(*status)) {
		/* suspended */
		if (window_get(frame_local, FRAME_CLOSED))
			return (NOTIFY_IGNORED);
		suspended = pid;
		putback();
		(void)window_set(frame_local, FRAME_CLOSED, TRUE, 0);
		return (NOTIFY_DONE);
	}
	/* dead child */
	putback();
	exit((int)(status->w_retcode));
	/* NOREACH */
	return (NOTIFY_DONE);
}

static
putback()
{
	if (!taken_over_screen)
		return;
	if (!window_prog) {
		/* Turn on input devices */
		(void)win_setms(winfd, &screen);
		/*
		 * 3.0 arbitrary input device addition with:
		 * win_set_input_device(winfd, fd, name);
		 *
		 * kbd equivalent of mouse:
		 * win_setkbd(winfd, &s);
		 */
	} else
		/* Grab io because about to release it */
		(void)win_grabio(winfd);
	/* Release io has hidden semantic of reloading colormap */
	(void)win_releaseio(winfd);
	/* Go iconic */
	(void)window_set(frame, FRAME_CLOSED, TRUE, 0);
#ifdef	get_cursor
	/* Give frame old cursor back */
	(void)window_set(frame, WIN_CURSOR, oldcurs_ptr, 0);
#else
	{
	extern struct  cursor tool_cursor;

	/* Give frame a std cursor */
	(void)win_setcursor(winfd, (Cursor)(LINT_CAST(&tool_cursor)));
	}
#endif
	taken_over_screen = 0;
}

/* ARGSUSED */
static int 
overview_handle_output(ptytty, addr, len)
	Ptytty	*ptytty;
	register char *addr;
	register int len;
{
	register i;

	for (i = 0; i <= len; i++)
		putc(*(addr+i), stdout);
	return (len);
}

static	Notify_value	ptytty_pty_input_pending();
static	Notify_value	ptytty_pty_output_pending();
static	Notify_value	ptytty_prioritizer();

#define	iwbp	ptytty->inbuf.cb_wbp
#define	irbp	ptytty->inbuf.cb_rbp
#define	iebp	ptytty->inbuf.cb_ebp
#define	ibuf	ptytty->inbuf.cb_buf
#define	owbp	ptytty->outbuf.cb_wbp
#define	orbp	ptytty->outbuf.cb_rbp
#define	oebp	ptytty->outbuf.cb_ebp
#define	obuf	ptytty->outbuf.cb_buf

static
Ptytty *
ptytty_create(output)
    int    (*output) ();
{
    Ptytty *ptytty = (Ptytty *) (LINT_CAST(calloc(1, sizeof(Ptytty))));

    if (ptytty_init(ptytty) < 0)
	exit(1);
    ptytty->inbuf.cb_rbp = ptytty->inbuf.cb_buf;
    ptytty->inbuf.cb_wbp = ptytty->inbuf.cb_buf;
    ptytty->inbuf.cb_ebp = &ptytty->inbuf.cb_buf[sizeof (ptytty->inbuf.cb_buf)];
    ptytty->outbuf.cb_rbp = ptytty->outbuf.cb_buf;
    ptytty->outbuf.cb_wbp = ptytty->outbuf.cb_buf;
    ptytty->outbuf.cb_ebp =
	&ptytty->outbuf.cb_buf[sizeof (ptytty->outbuf.cb_buf)];
    (void) notify_set_input_func((Notify_client)(LINT_CAST(ptytty)), 
    	ptytty_pty_input_pending, ptytty->pty);
    ptytty->waiting_for_pty_input = 1;
    ptytty->cached_pri = notify_set_prioritizer_func(
    	(Notify_client)(LINT_CAST(ptytty)),ptytty_prioritizer);
    ptytty->output = output;
    return (ptytty);
}

/* ARGSUSED */
static
int
ptytty_fork(ptytty, prog, argv)
    Ptytty               *ptytty;
    char                 *prog;
    char                **argv;
{
    int                   pidchild;
    struct sigvec vec, ovec;
    
    (void)unsetenv("TERMCAP");
    pidchild = fork();
    if (pidchild < 0)
	return (-1);
    if (pidchild) {
	int                   on = 1;
	int                   fdflags;

	if ((fdflags = fcntl(ptytty->pty, F_GETFL, 0)) == -1) {
		perror("fcntl");
		return (-1);
	}
	fdflags |= FNDELAY;
	if (fcntl(ptytty->pty, F_SETFL, fdflags) == -1) {
		perror("fcntl");
		return (-1);
	}
	(void)ioctl(ptytty->pty, TIOCPKT, &on);

	/* (void)signal(SIGTSTP, SIG_IGN); */

	vec.sv_handler = SIG_IGN;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(SIGTSTP, &vec, 0);

	return (pidchild);
    }

    /* (void)signal(SIGWINCH, SIG_DFL); /* we don't care about SIGWINCH's */

    vec.sv_handler = SIG_DFL;
    vec.sv_mask = vec.sv_onstack = 0;
    sigvec(SIGWINCH, &vec, 0);

    /*
     * Change process group of child process (me at this point in code) so its
     * signal stuff doesn't affect the terminal emulator. 
     */
    pidchild = getpid();

    vec.sv_handler = SIG_IGN;
    sigvec(SIGTTOU, &vec, &ovec);
    {
	/* int (*presigval) () = signal(SIGTTOU, SIG_IGN); */

	if ((ioctl(ptytty->tty, TIOCSPGRP, &pidchild)) == -1) {
	    perror("TIOCSPGRP");
	}
	(void)setpgrp(pidchild, pidchild);

	/* (void)signal(SIGTTOU, presigval); */
	sigvec(SIGTTOU, &ovec, 0);
    }
    /*
     * Set up file descriptors 
     */
    (void)close(ptytty->pty);
    (void)dup2(ptytty->tty, 0);
    (void)dup2(ptytty->tty, 1);
    (void)dup2(ptytty->tty, 2);
    (void)close(ptytty->tty);
    execvp(prog, argv);
    perror(*argv);
    exit(1);
    /* NOTREACHED */
}

static
int
ptytty_init(ptytty)
    Ptytty  *ptytty;
{
    int                   tt, tmpfd, ptynum = 0;
    int                   pty = 0, tty = 0;
    char                  linebuf[20], *line = &linebuf[0];
    char		  *ptyp = "pqrstuvwxyzPQRST";
    struct stat           stb;

    /*
     * find unopened pty 
     */
needpty:
    while (*ptyp) {
	(void)strcpy(line, "/dev/ptyXX");
	line[strlen("/dev/pty")] = *ptyp;
	line[strlen("/dev/ptyp")] = '0';
	if (stat(line, &stb) < 0)
	    break;
	while (ptynum < 16) {
	    line[strlen("/dev/ptyp")] = "0123456789abcdef"[ptynum];
	    pty = open(line, 2);
	    if (pty > 0)
		goto gotpty;
	    ptynum++;
	}
	ptyp++;
    }
    (void)fprintf(stderr, "All pty's in use\n");
    return (-1);
    /* NOTREACHED */
gotpty:
    line[strlen("/dev/")] = 't';
    tt = open("/dev/tty", 2);
    if (tt > 0) {
	(void)ioctl(tt, TIOCNOTTY, 0);
	(void)close(tt);
    }
    tty = open(line, 2);
    if (tty < 0) {
	ptynum++;
	(void)close(pty);
	goto needpty;
    }
    (void)ttysw_restoreparms(tty); /* Not ttysw specific */
    /*
     * Copy stdin.  Set stdin to tty so ttyslot in updateutmp will think this
     * is the control terminal.  Restore state. Note: ttyslot should have
     * companion ttyslotf(fd). 
     */
    tmpfd = dup(0);
    (void)close(0);
    (void)dup(tty);
    ptytty->ttyslot = updateutmp((char *)0, 0, tty);
    (void)close(0);
    (void)dup(tmpfd);
    (void)close(tmpfd);
    if (ptytty->ttyslot == 0) {
	(void)close(tty);
	(void)close(pty);
	return (-1);
    }
    ptytty->tty = tty;
    ptytty->pty = pty;
    return (0);
}

/* Write user input to pty */
static Notify_value
ptytty_pty_output_pending(ptytty, pty)
    Ptytty               *ptytty;
    int                   pty;
{
    register int        cc;

    if (iwbp > irbp) {
	cc = write(pty, irbp, iwbp - irbp);
	if (cc > 0) {
	    irbp += cc;
	    if (irbp == iwbp)
		irbp = iwbp = ibuf;
	} else if (cc < 0) {
	    perror("pty write failure");
	}
    }
    return (NOTIFY_DONE);
}

/* Read pty's input (which is output from program) */
static Notify_value
ptytty_pty_input_pending(ptytty, pty)
    Ptytty               *ptytty;
    int                   pty;
{
    register int        cc;
    extern int errno;

    /* use readv so can read packet header into separate char? */
    cc = read(pty, owbp, oebp - owbp);
    /* System V compatibility requires recognition of 4.2 and Sys5 returns:
     * 4.2: cc == -1 and EWOULDBLOCK; Sys5: cc == 0 and EWOULDBLOCK
     */
    if (cc <= 0 && errno == EWOULDBLOCK)
		cc = 0;
    else if (cc <= 0)
		cc = -1;
    else if (*owbp == 0)
		cc--;
    else
		cc = 0;
    if (cc > 0) {
		bcopy(owbp + 1, owbp, cc);
		owbp += cc;
    }
    return (NOTIFY_DONE);
}

static Notify_value
ptytty_prioritizer(ptytty, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		  nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events)
    register Ptytty      *ptytty;
    fd_set                *ibits_ptr, *obits_ptr, *ebits_ptr;
    int			  nsig, *sigbits_ptr, *event_count_ptr;
    register int         *auto_sigbits_ptr, nfd;
    Notify_event         *events;
{
    register int          bit;
    register int          pty = ptytty->pty;

    /* if (*obits_ptr & (bit = FD_BIT(pty))) { */
    if (FD_SET(pty, obits_ptr)) {
	(void) notify_output((Notify_client)(LINT_CAST(ptytty)), pty);
	FD_CLR(pty, obits_ptr);
    }
    /* if (*ibits_ptr & (bit = FD_BIT(pty))) { */
    if (FD_SET(pty, ibits_ptr)) {
	(void) notify_input((Notify_client)(LINT_CAST(ptytty)), pty);
	FD_CLR(pty, ibits_ptr);
    }
    (void) ptytty->cached_pri(ptytty, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events);
    ptytty_reset_conditions(ptytty);
    return (NOTIFY_DONE);
}

/*
 * Conditionally set conditions
 */
static
ptytty_reset_conditions(ptytty)
    register Ptytty      *ptytty;
{
    register int          pty = ptytty->pty;

    /* Send program output to terminal emulator */
    ptytty_consume_output(ptytty);
    /* Toggle between window input and pty output being done */
    if (iwbp > irbp) {
	if (!ptytty->waiting_for_pty_output) {
	    /* Wait for output to complete on pty */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ptytty)),
					  ptytty_pty_output_pending, pty);
	    ptytty->waiting_for_pty_output = 1;
	}
    } else {
	if (ptytty->waiting_for_pty_output) {
	    /* Don't wait for output to complete on pty any more */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ptytty)), 
	    	NOTIFY_FUNC_NULL, pty);
	    ptytty->waiting_for_pty_output = 0;
	}
    }
    /* Set pty input pending */
    if (owbp == orbp) {
	if (!ptytty->waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)(LINT_CAST(ptytty)), 
	    	ptytty_pty_input_pending, pty);
	    ptytty->waiting_for_pty_input = 1;
	}
    } else {
	if (ptytty->waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)(LINT_CAST(ptytty)), NOTIFY_FUNC_NULL, pty);
	    ptytty->waiting_for_pty_input = 0;
	}
    }
}

/*
 * Send program output.
 */
static
ptytty_consume_output(ptytty)
    register Ptytty *ptytty;
{
    int                 cc;

    if (owbp > orbp) {
	cc = ptytty->output(ptytty, orbp, owbp - orbp);
	orbp += cc;
	if (orbp == owbp)
	    orbp = owbp = obuf;
    }
}

/*
 * Add the string to the input queue. 
 */
ptytty_input(ptytty, addr, len)
    Ptytty             *ptytty;
    char               *addr;
    int                 len;
{
    if (iwbp + len >= iebp)
	    return (0);		   /* won't fit */
    bcopy(addr, iwbp, len);
    iwbp += len;
    (void)ptytty_pty_output_pending(ptytty, ptytty->pty);
    ptytty_reset_conditions(ptytty);
    return (len);
}

