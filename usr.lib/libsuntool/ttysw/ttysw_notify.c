#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)ttysw_notify.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1988 by Sun Microsystems, Inc.
 */

/*
 * Notifier related routines for the ttysw.
 */
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_notify.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/ttysw.h>
#include <suntool/window.h>
#include <suntool/tool_struct.h>
	/* tool.h must be before any indirect include of textsw.h */
#include <suntool/ttysw_impl.h>

/* #ifdef CMDSW */
extern void	  ttysw_display_capslock();
extern void	  ttynullselection();
extern caddr_t	  textsw_get();
extern Textsw_status	textsw_set();
extern void		textsw_display();


extern Cmdsw	*cmdsw;
extern struct ttysubwindow *_ttysw;

/* #else */
#include <suntool/charimage.h>
#include <suntool/charscreen.h>
#undef length
#define ITIMER_NULL   ((struct itimerval *)0)

#define	TTYSW_USEC_DELAY 100000
	/* Duplicate of what's in ttysw_tio.c */

Notify_value		ttysw_itimer_expired();
/* #endif CMDSW */

/* #ifndef CMDSW */
static        Notify_value    ttysw_win_input_pending();
/* #endif CMDSW */
Notify_value		ttysw_text_input_pending();
static	Notify_value	ttysw_pty_output_pending();
Notify_value  		ttysw_pty_input_pending();
Notify_value		ttysw_prioritizer();
void			ttysw_sendsig();

/* 
 * These three procedures are no longer needed because the 
 * pty driver bug that causes the ttysw to lock up is fixed.
 * void			ttysw_add_pty_timer();
 * void			ttysw_remove_pty_timer();
 * Notify_value		ttysw_pty_timer_expired();
*/


void			ttysw_sigwinch();

/* shorthand - Duplicate of what's in ttysw_main.c */

#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp
#define	ibuf	ttysw->ttysw_ibuf.cb_buf
#define	owbp	ttysw->ttysw_obuf.cb_wbp
#define	orbp	ttysw->ttysw_obuf.cb_rbp

int	ttysw_waiting_for_pty_input;	/* Accelerator to avoid excessive
					   notifier activity */
static	ttysw_waiting_for_pty_output;	/* Accelerator to avoid excessive
					   notifier activity */
Notify_func ttysw_cached_pri;		/* Default prioritizer */

extern Notify_value	ttysw_text_destroy();	/* Destroy func for cmdsw */
extern Notify_value	ttysw_text_event();	/* Event func for cmdsw */
extern int		ttysw_scan_for_completed_commands();
extern void		ttysw_move_mark();

/* Only extern for use by tty_window_object() in tty.c */
ttysw_interpose(ttysw)
    Ttysw                *ttysw;
{
    (void) notify_interpose_input_func((Notify_client)ttysw,
	    ttysw_win_input_pending,
	    (int)LINT_CAST(window_get((Window)(LINT_CAST(ttysw)), WIN_FD)));
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	Textsw		  textsw = (Textsw) ttysw->ttysw_hist;

	(void)notify_interpose_event_func((Notify_client)textsw, ttysw_text_event, NOTIFY_SAFE);
	(void)notify_interpose_destroy_func((Notify_client)textsw, ttysw_text_destroy);
	(void) notify_interpose_input_func((Notify_client)textsw,
	    ttysw_text_input_pending,
	    (int)LINT_CAST(window_get((Window)(LINT_CAST(ttysw->ttysw_hist)), 
	    	WIN_FD)));
    }
    (void) notify_set_input_func((Notify_client)ttysw, ttysw_pty_input_pending,
				 ttysw->ttysw_pty);
    ttysw_waiting_for_pty_input = 1;
    ttysw_cached_pri = notify_set_prioritizer_func((Notify_client)ttysw,
						   ttysw_prioritizer);
}

caddr_t
ttysw_create(tool, name, width, height)
    struct tool          *tool;
    char                 *name;
    short                 width, height;
{
    struct toolsw        *toolsw;
    extern struct pixwin *csr_pixwin;
    Bool retained;
    Ttysw                *ttysw;

    /* Create an empty subwindow */
    if ((toolsw = tool_createsubwindow(tool, name, width, height)) ==
	(struct toolsw *) 0)
	return (NULL);
    /* Make the empty subwindow into a tty subwindow */
    if ((toolsw->ts_data = ttysw_init(toolsw->ts_windowfd)) == NULL)
	return (NULL);
    ((Ttysw *)LINT_CAST(toolsw->ts_data))->ttysw_flags |= TTYSW_FL_USING_NOTIFIER;
    ttysw = (Ttysw *)LINT_CAST(toolsw->ts_data);
    /* Register with window manager */
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	toolsw->ts_data = (caddr_t)ttysw->ttysw_hist;
	(void)textsw_set((Textsw)ttysw->ttysw_hist, TEXTSW_TOOL, tool, 0);
	(void)ttysw_interpose(ttysw);
    }
    /* begin ttysw only */
    retained = defaults_get_boolean("/Tty/Retained",
	(Bool)FALSE, (int *)NULL);
    if (win_register((char *)ttysw, csr_pixwin, ttysw_event,
		     ttysw_destroy, (unsigned)((retained)? PW_RETAIN: 0)))
	return (NULL);
    /* Draw cursor on the screen and retained portion */
    (void)drawCursor(0, 0);
    /* end ttysw only */
    return ((caddr_t)ttysw);
}

int
ttysw_start(ttysw_client, argv)
    Ttysubwindow        ttysw_client;
    char                **argv;
{
    int                   pid;
    Ttysw		  *ttysw;
#ifdef DEBUG          
    Notify_func		  func;	/* for debugging */
#endif DEBUG                                                 

    ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    if ((pid = ttysw_fork_it((caddr_t)ttysw, argv)) != -1) {
	/* Wait for child process to die */
	(void) notify_set_wait3_func((Notify_client)(LINT_CAST(ttysw)), 
		notify_default_wait3, pid);
#ifdef DEBUG          
      func = notify_get_output_func(ttysw, ttysw->ttysw_pty);
      func = notify_get_input_func(ttysw, ttysw->ttysw_pty);
      func = notify_get_prioritizer_func(ttysw);
      if (pid == -1) notify_dump(ttysw, 0, 2);
#endif DEBUG                                                 
    }
    return (pid);
}

Notify_value
ttysw_destroy(ttysw, status)
    Ttysw                *ttysw;
    Destroy_status        status;
{
    if (status != DESTROY_CHECKING) {
    	/*
    	 * Pty timer is no longer needed because the
	 * pty driver bug that causes the ttysw to lock up is fixed.
    	 * ttysw_remove_pty_timer(ttysw);
    	 */
    	
        (void) notify_set_itimer_func((Notify_client)(LINT_CAST(ttysw)), 
		ttysw_itimer_expired,
	    ITIMER_REAL, (struct itimerval *) 0, ITIMER_NULL);
	    
	/* Sending both signal is to cover all bases  */ 
	/* This is the workaround for the poison pty bug in the kernel */   
	/* ttysw_sendsig(ttysw, (Textsw) 0, SIGTERM);
           ttysw_sendsig(ttysw, (Textsw) 0, SIGHUP);
        */	
        
	(void)win_unregister((char *)(LINT_CAST(ttysw)));
	if (ttysw->ttysw_hist) {
	    window_set(ttysw->ttysw_hist, TEXTSW_CLIENT_DATA, 0, 0);
	}
	(void)ttysw_done((caddr_t)ttysw);
	return (NOTIFY_DONE);
    }
    return (NOTIFY_IGNORED);
}

void
ttysw_sigwinch(ttysw)
    Ttysw               *ttysw;
{
    int                  pgrp;
    int                  sig = SIGWINCH;

    /*
     * 2.0 tty based programs relied on getting SIGWINCHes at times other
     * then when the size changed.  Thus, for compatibility, we also do
     * that here.  However, I wish that I could get away with only sending
     * SIGWINCHes on resize.
     */
    /* Notify process group that terminal has changed. */
    if (ioctl(ttysw->ttysw_tty, TIOCGPGRP, &pgrp) == -1) {
	perror("ttysw_sigwinch, can't get tty process group");
	return;
    }
    /*
     * Only killpg when pgrp is not tool's.  This is the case of haven't
     * completed ttysw_fork yet (or even tried to do it yet). 
     */
    if (getpgrp(0) != pgrp)
	/*
	 * killpg could return -1 with errno == ESRCH but this is OK. 
	 */
	/* (void) killpg(pgrp, SIGWINCH); */
	ioctl(ttysw->ttysw_pty, TIOCSIGNAL, &sig);
    return;
}

/* #ifdef CMDSW */
void
ttysw_sendsig(ttysw, textsw, sig)
    Ttysw                *ttysw;
    Textsw                textsw;
    int			  sig;
{
    int control_pg;
    
    /* Send the signal to the process group of the controlling tty */
    if (ioctl(ttysw->ttysw_pty, TIOCGPGRP, &control_pg) >= 0) {
	/*
	 *  Flush our buffers of completed and partial commands.
	 *  Be sure to do this BEFORE killpg, or we'll flush
	 *  the prompt coming back from the shell after the
	 *  process dies.
	 */
	(void)ttysw_flush_input((caddr_t)ttysw);
	
	if (textsw) 
	    ttysw_move_mark(textsw, &cmdsw->pty_mark,
		  (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH),
		  TEXTSW_MARK_DEFAULTS);
	cmdsw->cmd_started = 0;
	cmdsw->pty_owes_newline = 0;
        /* (void) killpg(control_pg, sig); */
        ioctl(ttysw->ttysw_pty, TIOCSIGNAL, &sig);
    } else
        perror("ioctl");
}

/* #endif CMDSW */

/* ARGSUSED */
Notify_value
ttysw_event(ttysw, event, arg, type)
    Ttysw                *ttysw;
    Event                *event;
    Notify_arg     	  arg;
    Notify_event_type     type;
{
    switch (event_id(event)) {
      case WIN_REPAINT:
	if (ttysw->ttysw_hist && cmdsw->cmd_started) {
	    (void)ttysw_scan_for_completed_commands(ttysw, -1, 0);
	}
	(void)ttysw_display((Ttysubwindow)(LINT_CAST(ttysw)));
	return (NOTIFY_DONE);
      case WIN_RESIZE:
	(void)ttysw_resize(ttysw);
	return (NOTIFY_DONE);
      default:
	if (ttysw_eventstd(ttysw, event) == TTY_DONE)
	    return (NOTIFY_DONE);
	else
	    return (NOTIFY_IGNORED);
    }
}

ttysw_display(ttysw_client)
    Ttysubwindow         ttysw_client;
{
    Ttysw	*ttysw;
    
    ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	textsw_display((Textsw)ttysw->ttysw_hist);
    } else {
	extern struct pixwin *csr_pixwin;
	extern                wfd;
    
	(void)saveCursor();
	(void)prepair(wfd, &csr_pixwin->pw_clipdata->pwcd_clipping);
	/*
	 * If just hilite the selection part that is damaged then the other
	 * non-damaged selection parts should still be visible, thus creating
	 * the entire selection image. 
	 */
	ttysel_hilite(ttysw);
	(void)restoreCursor();
    }
    ttysw_sigwinch(ttysw);
}

static Notify_value
ttysw_pty_output_pending(ttysw, pty)
    Ttysw                *ttysw;
    int                   pty;
{    
    (void)ttysw_pty_output(ttysw, pty);
    return (NOTIFY_DONE);
}

Notify_value
ttysw_pty_input_pending(ttysw, pty)
    Ttysw                *ttysw;
    int                   pty;
{
    (void)ttysw_pty_input(ttysw, pty);
    return (NOTIFY_DONE);
}

/* ARGSUSED */
Notify_value
ttysw_itimer_expired(ttysw, which)
    Ttysw                *ttysw;
    int                   which;
{
    (void)ttysw_handle_itimer(ttysw);
    return (NOTIFY_DONE);
}

static Notify_value
ttysw_win_input_pending(ttysw, win_fd)
    Ttysw                *ttysw;
    int                   win_fd;
{
    /*
     * Differs from readwin in that don't loop around input_readevent
     * until get a EWOULDBLOCK. 
     */
    if (iwbp >= iebp)
	return (NOTIFY_IGNORED);
    return(notify_next_input_func((Notify_client)ttysw, win_fd));
}

/* Pkg_private */ Notify_value
ttysw_text_input_pending(textsw, win_fd)
    Textsw	 textsw;
    int		 win_fd;
{
    Ttysw	*ttysw = (Ttysw *)
		    LINT_CAST(textsw_get(textsw, TEXTSW_CLIENT_DATA));
    /*
     * Differs from readwin in that don't loop around input_readevent
     * until get a EWOULDBLOCK. 
     */
    if (iwbp >= iebp)
	return (NOTIFY_IGNORED);
    return(notify_next_input_func((Notify_client)textsw, win_fd));
}

/*
 * Conditionally set conditions
 */
ttysw_reset_conditions(ttysw)
    register Ttysw       *ttysw;
{
/* #ifndef CMDSW */
    static struct itimerval ttysw_itimerval = {{0, 0}, {0, TTYSW_USEC_DELAY}};
/* #endif */
    static struct itimerval pty_itimerval = {{0, 0}, {0, 10}};
    register int          pty = ttysw->ttysw_pty;

    /* Send program output to terminal emulator */
    (void)ttysw_consume_output(ttysw);
    /* Toggle between window input and pty output being done */
    if (iwbp > irbp
    || (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && cmdsw->pty_eot > -1)) {
	if (!ttysw_waiting_for_pty_output) {
	    /* Wait for output to complete on pty */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)),
					  ttysw_pty_output_pending, pty);
	    ttysw_waiting_for_pty_output = 1;
	    /* 
	     * Pty timer is no longer needed because the
	     * pty driver bug that causes the ttysw to lock up is fixed.
	     * (void)ttysw_add_pty_timer(ttysw, &pty_itimerval);
	     */
	}
    } else {
	if (ttysw_waiting_for_pty_output) {
	    /* Don't wait for output to complete on pty any more */
	    (void) notify_set_output_func((Notify_client)(LINT_CAST(ttysw)), 
	    		NOTIFY_FUNC_NULL, pty);
	    ttysw_waiting_for_pty_output = 0;
	}
    }
    /* Set pty input pending */
    if (owbp == orbp) {
	if (!ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)ttysw,
					 ttysw_pty_input_pending, pty);
	    ttysw_waiting_for_pty_input = 1;
	}
    } else {
	if (ttysw_waiting_for_pty_input) {
	    (void) notify_set_input_func((Notify_client)ttysw, NOTIFY_FUNC_NULL,
					 pty);
	    ttysw_waiting_for_pty_input = 0;
	}
    }
    /*
     * Try to optimize displaying by waiting for image to be completely filled
     * after being cleared (vi(^F ^B) page) before painting. 
     */
    if (!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && delaypainting)
	(void) notify_set_itimer_func((Notify_client)(LINT_CAST(ttysw)), 
			ttysw_itimer_expired,
		       ITIMER_REAL, &ttysw_itimerval, ITIMER_NULL);
}

Notify_value
ttysw_prioritizer(ttysw, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		  nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events)
    register Ttysw       *ttysw;
    fd_set               *ibits_ptr, *obits_ptr, *ebits_ptr;
    int			  nsig, *sigbits_ptr, *event_count_ptr;
    register int         *auto_sigbits_ptr, nfd;
    Notify_event         *events;
{
    register int          bit;
    register int          pty = ttysw->ttysw_pty;


    ttysw->ttysw_flags |= TTYSW_FL_IN_PRIORITIZER;
    if (*auto_sigbits_ptr) {
	/* Send itimers */
	if (*auto_sigbits_ptr & SIG_BIT(SIGALRM)) {
	    (void) notify_itimer((Notify_client)(LINT_CAST(ttysw)), ITIMER_REAL);
	    *auto_sigbits_ptr &= ~SIG_BIT(SIGALRM);
	}
    }
    /* if (*obits_ptr & (bit = FD_BIT(ttysw->ttysw_tty))) { */
    if (FD_ISSET(ttysw->ttysw_tty, obits_ptr)) {
	(void) notify_output((Notify_client)(LINT_CAST(ttysw)), ttysw->ttysw_tty);
	FD_CLR(ttysw->ttysw_tty, obits_ptr);
    }
    /* if (*ibits_ptr & (bit = FD_BIT(ttysw->ttysw_wfd))) { */
    if (FD_ISSET(ttysw->ttysw_tty, ibits_ptr)) {
	(void) notify_input((Notify_client)(LINT_CAST(ttysw)), ttysw->ttysw_wfd);
	FD_CLR(ttysw->ttysw_tty, ibits_ptr);
    }
    /* if (*obits_ptr & (bit = FD_BIT(pty))) { */
    if (FD_ISSET(pty, obits_ptr)) {
	(void) notify_output((Notify_client)(LINT_CAST(ttysw)), pty);
	FD_CLR(pty, obits_ptr);
	
	/* 
	 * Pty timer is no longer needed because the
	 * (void)ttysw_remove_pty_timer(ttysw);
	 */
    }
    /* if (*ibits_ptr & (bit = FD_BIT(pty))) { */
    if (FD_ISSET(pty, ibits_ptr)) {
	(void) notify_input((Notify_client)(LINT_CAST(ttysw)), pty);
	FD_CLR(pty, ibits_ptr);
    }
    (void) ttysw_cached_pri(ttysw, nfd, ibits_ptr, obits_ptr, ebits_ptr,
		nsig, sigbits_ptr, auto_sigbits_ptr, event_count_ptr, events);
    (void)ttysw_reset_conditions(ttysw);
    ttysw->ttysw_flags &= ~TTYSW_FL_IN_PRIORITIZER;

    return (NOTIFY_DONE);
}

ttysw_resize(ttysw)
    Ttysw                *ttysw;
{
    int                   pagemode;

    /*
     * Turn off page mode because send characters through character image
     * manager during resize. 
     */
    pagemode = ttysw_getopt((caddr_t)ttysw, TTYOPT_PAGEMODE);
    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, 0);
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	(void)ttynewsize(textsw_screen_column_count((Textsw)ttysw->ttysw_hist),
		   textsw_screen_line_count((Textsw)ttysw->ttysw_hist));
    } else {
	/* Have character image update self */
	(void)csr_resize(ttysw);
	/* Have screen update any size change parameters */
	(void)cim_resize(ttysw);
	if (ttysw->ttysw_hist) {
	    cmdsw->ttysw_resized++;
	}
    }
    /* Turn page mode back on */
    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_PAGEMODE, pagemode);
}

cim_resize(ttysw)
    Ttysw                *ttysw;
{
    struct rectlist       rl;
    extern struct pixwin *csr_pixwin;
    int                   (*tmp_getclipping) ();
    int                   ttysw_null_getclipping();

    /* Prevent any screen writing by making clipping null */
    rl = rl_null;
    (void)pw_restrict_clipping(csr_pixwin, &rl);
    /* Make sure clipping remains null */
    tmp_getclipping = csr_pixwin->pw_clipops->pwco_getclipping;
    csr_pixwin->pw_clipops->pwco_getclipping = ttysw_null_getclipping;
    /* Redo character image */
    (void)imagerepair(ttysw);
    /* Restore get clipping function */
    csr_pixwin->pw_clipops->pwco_getclipping = tmp_getclipping;
}

csr_resize(ttysw)
    Ttysw                *ttysw;
{
    struct rect           r_new;
    extern                wfd;

    /* Update notion of size */
    (void)win_getsize(wfd, &r_new);
    winwidthp = r_new.r_width;
    winheightp = r_new.r_height;
    /* Don't currently support selections across size changes */
    ttynullselection(ttysw);
}

ttysw_null_getclipping()
{
}

#ifdef notdef
---------------------- Obsoleted code follows -----------------------------
/* ARGSUSED */
Notify_value
ttysw_pty_timer_expired(pty, which)
    Notify_client              pty;
    int				which;
{
/* 
 * Pty timer is no longer needed because the
 * pty driver bug that causes the ttysw to lock up is fixed.
 * When there is more than 256 character stuffed into the ttysw,
 * it will beep at the user.   
 */
    register Ttysw	*ttysw;
/*
**	Added by brentb (8-12-86) to take care of input queue overflow warning
*/
    int			max_fds = GETDTABLESIZE();
    fd_set			writefds;
    static struct timeval	tv = {0,0};
    struct inputevent		event;
    int				nfd;
    
    ttysw = _ttysw; /*global data */
    FD_ZERO(&writefds);
    FD_SET(ttysw->ttysw_pty, &writefds);
    
    if(!ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT) && 
	ttysw_waiting_for_pty_output)
    {
	if((nfd = select(max_fds, NULL, &writefds, NULL, &tv)) < 0)
		perror("TTYSW select");

	if (nfd == 0)
	{
		int	result;
		Event	event;

		result = alert_prompt(
				(Frame)0,
				&event,
				ALERT_MESSAGE_STRINGS,
				    "Too many keystrokes in the input buffer.",
				    0,
				ALERT_BUTTON_YES,	"Flush input buffer",
				ALERT_BUTTON_NO,	"Cancel",
				ALERT_POSITION,		ALERT_SCREEN_CENTERED,
				0);
		if (result == ALERT_YES)
			(void)ttysw_flush_input((caddr_t)ttysw);
	}
    } 
    return(NOTIFY_DONE);
}

void
ttysw_add_pty_timer(ttysw, itimer)
    register Ttysw                *ttysw;
    register struct itimerval	*itimer;
{
/* 
 * Pty timer is no longer needed because the
 * pty driver bug that causes the ttysw to lock up is fixed.
 */   
    if (NOTIFY_FUNC_NULL ==
        notify_set_itimer_func((Notify_client)(LINT_CAST(&ttysw->ttysw_pty)),
				   ttysw_pty_timer_expired, ITIMER_REAL,
				   itimer, ITIMER_NULL))
	    notify_perror("ttysw_add_pty_timer");
}

void
ttysw_remove_pty_timer(ttysw)
    register Ttysw                *ttysw;
{
/* 
 * Pty timer is no longer needed because the
 * pty driver bug that causes the ttysw to lock up is fixed.
 */
    ttysw_add_pty_timer(ttysw, ITIMER_NULL);
} 

#endif
