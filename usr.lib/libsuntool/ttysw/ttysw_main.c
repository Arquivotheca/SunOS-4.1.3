#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttysw_main.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Very active terminal emulator subwindow pty code.
 */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sgtty.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sundev/kbd.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_lock.h>
#include <suntool/icon.h>
#include <suntool/ttysw.h>
#include <suntool/tool.h>
	/* tool.h must be before any indirect include of textsw.h */
#include <suntool/ttysw_impl.h>
#include <suntool/ttytlsw_impl.h>
#include <suntool/selection_svc.h>

/* #ifndef CMDSW */
extern  struct  pixwin *csr_pixwin; /* For pw_batch calls */

/* #endif CMDSW */

extern int       errno;
extern void	 ttysw_lighten_cursor();
extern void	 ttysetselection();
void 		 pw_batch();
extern Textsw_index	textsw_insert();

/* shorthand */
#define	iwbp	ttysw->ttysw_ibuf.cb_wbp
#define	irbp	ttysw->ttysw_ibuf.cb_rbp
#define	iebp	ttysw->ttysw_ibuf.cb_ebp
#define	ibuf	ttysw->ttysw_ibuf.cb_buf
#define	owbp	ttysw->ttysw_obuf.cb_wbp
#define	orbp	ttysw->ttysw_obuf.cb_rbp
#define	oebp	ttysw->ttysw_obuf.cb_ebp
#define	obuf	ttysw->ttysw_obuf.cb_buf

/* #ifdef CMDSW */
#ifdef notdef

/*

The basic strategy for building a line-oriented command subwindow (terminal
emulator subwindow) on top of the text subwindow is as follows.

The idle state has no user input still to be processed, and no outstanding
active processes at the other end of the pty (except the shell).

When the user starts creating input events, they are passed through to the
textsw unless they fall in the class of "activating events" (which right now
consists of \n, \r, \033, ^C and ^Z).  In addition, the end of the textsw's
backing store is recorded when the first event is created.

When an activating event is entered, all of the characters in the textsw
from the recorded former end to the current end of the backing store are
added to the list of characters to be sent to the pty.  In addition, the
current end is set to be the insertion place for response from the pty.

If the user has started to enter a command, then in order to avoid messes on
the display, the first response from the pty will be suffixed with a \n
(unless it ends in a \n), and the pty will be marked as "owing" a \n.

In the meantime, if the user continues to create input events, they are
appended at the end of the textsw, after the response from the pty.  When
an activating event is entered, all of the markers, etc. are updated as
described above.

The most general situation is:
	Old stuff in the log file
	^User editing here
	More old stuff
	Completed commands enqueued for the pty
	Pty inserting here^
	(Prompt)Partial user command
*/

#endif

extern Cmdsw *cmdsw;

extern Textsw
ttysw_to_textsw(ttysw)
    struct ttysubwindow *ttysw;
{
	return((Textsw)ttysw->ttysw_hist);
}

/* #endif CMDSW */


/*
 * Main pty processing. 
 */
 
/*
 * Write window input to pty
 */
ttysw_pty_output(ttysw, pty)
    register struct ttysubwindow *ttysw;
    int                 pty;
{
    register int        cc;

    if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	if (cmdsw->pty_eot > -1) {
	    char *eot_bp = ibuf + cmdsw->pty_eot;
	    char nullc = '\0';
	    
	    /* write everything up to pty_eot */
	    if (eot_bp > irbp) {
		cc = write(pty, irbp, eot_bp - irbp);
		if (cc > 0) {
		    irbp += cc;
		} else if (cc < 0) {
		    perror("TTYSW pty write failure");
		}
	    }
	    /* write the eot */
	    if (irbp == eot_bp) {
		cc = write(pty, &nullc, 0);
		if (cc < 0) {
		    perror("TTYSW pty write failure");
		} else {
		    cmdsw->pty_eot = -1;
		}
	    }
	}   /* cmdsw->pty_eot > -1 */
	
	/* only write the rest of the buffer if it doesn't have an eot in it */
	if (cmdsw->pty_eot > -1)
	    return;
    }

    if (iwbp > irbp) {
	cc = write(pty, irbp, iwbp - irbp);
	if (cc > 0) {
	    irbp += cc;
	    if (irbp == iwbp)
		irbp = iwbp = ibuf;
	} else if (cc < 0) {
	    perror("TTYSW pty write failure");
	}
    }
}

void
ttysw_process_STI(ttysw, cp, cc)
    register struct ttysubwindow *ttysw;
    register char		 *cp;
    register int		  cc;
{
    register short		  post_id;
    register Textsw		  textsw = (Textsw) ttysw->ttysw_hist;
    Textsw_index		  pty_index;
    Textsw_index		  cmd_start;

#ifdef DEBUG
    fprintf(stderr, "STI ");
    fwrite(cp, 1, cc, stderr);
    fprintf(stderr, "\n");
#endif DEBUG
    if (!textsw) {
	return;
    }
    /* Assume app wants STI text echoed at cursor position */
    if (cmdsw->cooked_echo) {
	pty_index = textsw_find_mark(textsw, cmdsw->pty_mark);
	if (cmdsw->cmd_started) {
	    cmd_start = textsw_find_mark(textsw, cmdsw->user_mark);
	} else {
	    cmd_start = (Textsw_index) window_get(textsw, TEXTSW_LENGTH);
	}
	if (cmd_start > pty_index) {
	    if (cmdsw->append_only_log) {
		textsw_remove_mark(textsw, cmdsw->read_only_mark);
	    }
	    (void) textsw_delete(textsw, pty_index, cmd_start);
	    if (cmdsw->append_only_log) {
		cmdsw->read_only_mark =
		  textsw_add_mark(textsw, pty_index, TEXTSW_MARK_READ_ONLY);
	    }
	    cmdsw->pty_owes_newline = 0;
	}
    }
    /* Pretend STI text came in from textsw window fd */
    while (cc > 0) {
	post_id = (short) (*cp);
	win_post_id(textsw, post_id, NOTIFY_SAFE);
	cp++;
	cc--;
    }
    /* flush caches */
    (void) window_get(textsw, TEXTSW_LENGTH);
}

/*
 * Read pty's input (which is output from program)
 */
ttysw_pty_input(ttysw, pty)
    register struct ttysubwindow *ttysw;
    int                 pty;
{
    static struct iovec	iov[2];
    register int        cc;
    char		ucntl;
    register unsigned	int_ucntl;

    /* use readv so can read packet header into separate char? */
    iov[0].iov_base = &ucntl;
    iov[0].iov_len  = 1;
    iov[1].iov_base = owbp;
    iov[1].iov_len  = oebp - owbp;
    cc = readv(pty, iov, 2);
    if (cc < 0 && errno == EWOULDBLOCK)
	cc = 0;
    else if (cc <= 0)
	cc = -1;
    if (cc > 0) {
	int_ucntl = (unsigned)ucntl;
	if (int_ucntl != 0 && ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	    unsigned	tiocsti = TIOCSTI;
	    if (int_ucntl == (tiocsti & 0xff)) {
		ttysw_process_STI(ttysw, owbp, cc-1);
	    }
	    (void) ioctl(ttysw->ttysw_tty, TIOCGETC, &cmdsw->tchars);
	    (void) ioctl(ttysw->ttysw_tty, TIOCGLTC, &cmdsw->ltchars);
	    (void) ttysw_getp(ttysw);
	} else {
	    owbp += cc-1;
	}
    }
}

/*
 * Send program output to terminal emulator.
 */
ttysw_consume_output(ttysw)
    register struct ttysubwindow *ttysw;
{
    int                 cc;

    while (owbp > orbp && !ttysw->ttysw_frozen) {
	if (!ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	    if (ttysw->ttysw_primary.sel_made)  {
		ttysel_deselect(&ttysw->ttysw_primary, SELN_PRIMARY);
	    }
	    if (ttysw->ttysw_secondary.sel_made)  {
		ttysel_deselect(&ttysw->ttysw_secondary, SELN_SECONDARY);
	    }
	    pw_batch(csr_pixwin, PW_ALL);
	}
	cc = ttysw_output((Ttysubwindow)(LINT_CAST(ttysw)), orbp, owbp - orbp);
	if (!ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	    pw_batch(csr_pixwin, PW_NONE);
	}
	orbp += cc;
	if (orbp == owbp)
	    orbp = owbp = obuf;
    }
}

/*
 * Add the string to the input queue. 
 */
ttysw_input(ttysw0, addr, len)
    caddr_t             ttysw0;
    char               *addr;
    int                 len;
{
    struct ttysubwindow *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);

    if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	register Textsw	textsw = (Textsw)ttysw->ttysw_hist;
    
	(void)textsw_insert(textsw, addr, (long int)len);
    } else {
	if (iwbp + len >= iebp)
	    return (0);		   /* won't fit */
	bcopy(addr, iwbp, len);
	iwbp += len;
	ttysw->ttysw_lpp = 0;	   /* reset page mode counter */
	if (ttysw->ttysw_frozen)
	    (void) ttysw_freeze(ttysw, 0);
	if ( !(ttysw->ttysw_flags & TTYSW_FL_IN_PRIORITIZER)
	&&    (ttysw->ttysw_flags & TTYSW_FL_USING_NOTIFIER)) {
	    (void)ttysw_reset_conditions(ttysw);
	}
    }
    return (len);
}

int
ttysw_append_to_ibuf(ttysw, p, len)
struct ttysubwindow *ttysw;
char *p;
int len;
{
    int is_cmdtool;

    /*
     * Since command line editing is not supported in remote
     * cmdtools, report-generating tty escape sequences (e.g., 
     * ESC[11t) must be handled separately.
     */

    is_cmdtool = ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT); 

    if (is_cmdtool && (!cmdsw->cooked_echo)) {    /* remote cmdtool */
	 if (iwbp + len >= iebp)
             return (0);    /* (won't fit) */
         bcopy(p, iwbp, len);             
         iwbp += len;
    }
    else { 
         if (is_cmdtool && (cmdsw->cooked_echo))  /* local cmdtool */
	     cmdsw->pty_owes_newline = TRUE;      /* avoid extra newline */

	 ttysw_input(ttysw, p, len);              /* shelltool, local cmdtool */
    }
    return(len);
}

/* #ifndef CMDSW */
ttysw_handle_itimer(ttysw)
    register struct ttysubwindow *ttysw;
{
    if (ttysw->ttysw_primary.sel_made)  {
	ttysel_deselect(&ttysw->ttysw_primary, SELN_PRIMARY);
    }
    if (ttysw->ttysw_secondary.sel_made)  {
	ttysel_deselect(&ttysw->ttysw_secondary, SELN_SECONDARY);
    }
    pw_batch(csr_pixwin, PW_ALL);
    (void)pdisplayscreen(0);
    pw_batch(csr_pixwin, PW_NONE);
}

/*
 *  handle standard events.
 *  Note that KBD_USE, KBD_DONE cheat and assume that the
 *  ttysw_client of the ttysw is a ttytoolsubwindow.
 */
int
ttysw_eventstd(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
    struct tool *tool;

    tool = NULL;
    if (ttysw->ttysw_client)
	tool = ((struct ttytoolsubwindow *)LINT_CAST(
	       ttysw->ttysw_client))->tool;
    switch (event_id(ie)) {
      case KBD_REQUEST: {
	extern Seln_rank ttysel_mode();

	/* Refuse kbd focus if in secondary selection mode */
	if (ttysel_mode(ttysw) == SELN_SECONDARY)
	    (void)win_refuse_kbd_focus(ttysw->ttysw_wfd);
	return TTY_DONE;
	}
      case KBD_USE:
        (void)ttysw_restore_cursor();
        if (tool)
	    (void)tool_kbd_use(tool, (char *)(LINT_CAST(ttysw)));
	return TTY_DONE;
      case KBD_DONE:
        ttysw_lighten_cursor();
        if (tool)
	    (void)tool_kbd_done(tool, (char *)(LINT_CAST(ttysw)));
	return TTY_DONE;
      case MS_LEFT:
	return ttysw_process_point(ttysw, ie);
      case MS_MIDDLE:
	return ttysw_process_adjust(ttysw, ie);
      case MS_RIGHT:
	return ttysw_process_menu(ttysw, ie);
      case LOC_WINEXIT:
	return ttysw_process_exit(ttysw, ie);
      case LOC_MOVEWHILEBUTDOWN:
	return ttysw_process_motion(ttysw, ie);
      default:
	return ttysw_process_keyboard(ttysw, ie);
    }
}

static int
ttysw_process_point(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
    register int        wfd = ttysw->ttysw_wfd;
    struct inputmask    im;

    if (win_inputposevent(ie)) {
	ttysw->ttysw_butdown = MS_LEFT;
	ttysel_make(ttysw, ie, 1);
	(void)win_get_pick_mask(wfd, &im);
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_unsetinputcodebit(&im, LOC_WINEXIT);
	(void)win_set_pick_mask(wfd, &im);
    } else {
	if (ttysw->ttysw_butdown == MS_LEFT) {
		(void)ttysel_adjust(ttysw, ie, 0);
		ttysetselection(ttysw);
		(void)win_get_pick_mask(wfd, &im);
		win_unsetinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
		win_setinputcodebit(&im, LOC_WINEXIT);
		(void)win_set_pick_mask(wfd, &im);
	}
	ttysw->ttysw_butdown = 0;
    }
    return TTY_DONE;
}

static int
ttysw_process_adjust(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
    register int        wfd = ttysw->ttysw_wfd;
    struct inputmask    im;


    if (win_inputposevent(ie)) {
	ttysw->ttysw_butdown = MS_MIDDLE;
	(void)ttysel_adjust(ttysw, ie, 1);
	(void)win_get_pick_mask(wfd, &im);
	win_setinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
	win_unsetinputcodebit(&im, LOC_WINEXIT);
	(void)win_set_pick_mask(wfd, &im);
    } else {
	if (ttysw->ttysw_butdown == MS_MIDDLE) {
		(void)ttysel_adjust(ttysw, ie, 0);
		ttysetselection(ttysw);
		(void)win_get_pick_mask(wfd, &im);
		win_unsetinputcodebit(&im, LOC_MOVEWHILEBUTDOWN);
		win_setinputcodebit(&im, LOC_WINEXIT);
		(void)win_set_pick_mask(wfd, &im);
	}
	ttysw->ttysw_butdown = 0;
    }
    return TTY_DONE;
}

static int
ttysw_process_motion(ttysw, ie)
    register struct ttysubwindow *ttysw;
    register struct inputevent *ie;
{
#ifdef wipethrough
    (void)ttysel_adjust(ttysw, ie, 0);
#else
    if (ttysw->ttysw_butdown == MS_LEFT)
	ttysel_move(ttysw, ie);
    else
	(void)ttysel_adjust(ttysw, ie, 0);
#endif
    return TTY_DONE;
}

/* ARGSUSED */
static int
ttysw_process_exit(ttysw, ie)
    struct ttysubwindow *ttysw;
    struct inputevent  *ie;
{
    register int        wfd = ttysw->ttysw_wfd;
    struct inputmask    im;

    (void)win_get_pick_mask(wfd, &im);
    win_unsetinputcodebit(&im, LOC_WINEXIT);
    (void)win_set_pick_mask(wfd, &im);
    return TTY_DONE;
}

static int
ttysw_process_keyboard(ttysw, ie)
    struct ttysubwindow *ttysw;
    struct inputevent  *ie;

{
    register int  action = event_action(ie);
    register int  unmapped_key = event_id(ie);

    if ((action >= EUC_FIRST && action <= EUC_LAST) && (win_inputposevent(ie))) {
	char c = (char) action;
	/*
	 * State machine for handling logical caps lock, ``F1'' key.
	 * Capitalize characters except when an ESC goes by.  Then go
	 * into a state where characters are passed uncapitalized until
	 * an alphabetic character is passed.  We presume that all ESC
	 * sequences end with an alphabetic character.
	 *
	 * Used to solve the function key problem where the final `z' is
	 * is being capitalized. (Bug id: 1005033)
	 */
	if (ttysw->ttysw_capslocked & TTYSW_CAPSLOCKED) {
	    if (ttysw->ttysw_capslocked & TTYSW_CAPSSAWESC) {
		if (isalpha(c))
		    ttysw->ttysw_capslocked &= ~TTYSW_CAPSSAWESC;

	    } else {
		if (islower(c))
		    c = toupper(c);
		else if (c == '\033')
		    ttysw->ttysw_capslocked |= TTYSW_CAPSSAWESC;

	    }
	}
	(void) ttysw_input((caddr_t) ttysw, &c, 1);
#ifdef TTY_ACQUIRE_CARET
	if (!ttysw->ttysw_caret.sel_made)  {
	    ttysel_acquire(ttysw, SELN_CARET);
	}
#endif
	return TTY_DONE;

    /* 
     * else if this is a meta-c, meta-v, meta-f, meta-x, or some
     * permutation with shift or cntl, pass
     * it through to ttysw_input without going through ttysw_domap.
     * bugid 1026817
    */
    } else if ((unmapped_key == 227) || (unmapped_key == 246) || (unmapped_key == 230) || (unmapped_key == 248) || (unmapped_key == 198)|| (unmapped_key == 216)|| (unmapped_key == 152)|| (unmapped_key == 195)|| (unmapped_key == 131)|| (unmapped_key == 214)|| (unmapped_key == 150)) {    
	if (event_is_down(ie)) {
	    char c = (char) unmapped_key;
	    (void) ttysw_input((caddr_t) ttysw, &c, 1);
	}
    } else if (action > META_LAST)  {
	return ttysw_domap(ttysw, ie);
    }
    return TTY_OK;
}
/* #endif CMDSW */

/*
 * Pty size setter.  Called from ttyimage imagealloc. 
 */
ttynewsize(cols, lines)
    int                 cols, lines;

{
    struct ttysize      ts;
    extern struct ttysubwindow *_ttysw;

    ts.ts_lines = lines;
    ts.ts_cols = cols;
    /* XXX - ttysw should be passed */
    if ((ioctl(_ttysw->ttysw_tty, TIOCSSIZE, &ts)) == -1)
	perror("ttysw-TIOCSSIZE");
}


/*
 * A stop sign cursor, for when output is stopped. 
 */
static short        stop_data[16] = {
	       0x07C0, 0x0FE0, 0x1FF0, 0x1FF0, 0x1FF0, 0x1FF0, 0x1FF0, 0x0FE0,
		0x07C0, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0FE0
};
static 
mpr_static(stop_mpr, 16, 16, 1, stop_data);
static struct cursor stop_cursor = {7, 5, PIX_SRC | PIX_DST, &stop_mpr};

/*
 * Freeze tty output. 
 */
ttysw_freeze(ttysw, on)
    struct ttysubwindow *ttysw;
    int                 on;
{
    extern struct cursor ttysw_cursor;

    if (!ttysw->ttysw_frozen && on) {
	struct sgttyb       sgttyb;

	(void) ioctl(ttysw->ttysw_tty, TIOCGETP, (char *) &sgttyb);
	if ((sgttyb.sg_flags & (RAW | CBREAK)) == 0) {
	    (void)win_setcursor(ttysw->ttysw_wfd, &stop_cursor);
	    ttysw->ttysw_frozen = 1;
	} else
	    ttysw->ttysw_lpp = 0;
    } else if (ttysw->ttysw_frozen && !on) {
	(void)win_setcursor(ttysw->ttysw_wfd, &ttysw_cursor);
	ttysw->ttysw_frozen = 0;
	ttysw->ttysw_lpp = 0;
    }
    return (ttysw->ttysw_frozen);
}

/*
 * Set (or reset) the specified option number. 
 */
ttysw_setopt(ttysw0, opt, on)
    caddr_t             ttysw0;
    int                 opt, on;
{
    struct ttysubwindow *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);
    int			 result = 0;

    switch (opt) {
      case TTYOPT_HISTORY:	   /* tty history */
#ifdef TTYHIST
	if (on)
	    ttyhist_on(ttysw);
	else
	    ttyhist_off(ttysw);
#endif
	break;
      case TTYOPT_TEXT:	   	  /* cmdsw */
	if (on)
	    result = ttysw_be_cmdsw(ttysw);
	else
	    result = ttysw_be_ttysw(ttysw);
#ifdef DEBUG
	break;
      case 5:	   	  	  /* HACK */
	result = ttysw_getp(ttysw);
#endif DEBUG
    }
    if (result != -1)
	if (on)
	    ttysw->ttysw_opt |= 1 << opt;
	else
	    ttysw->ttysw_opt &= ~(1 << opt);
}

ttysw_getopt(ttysw0, opt)
    caddr_t             ttysw0;
    int                 opt;
{
    struct ttysubwindow *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);

    return ((ttysw->ttysw_opt & (1 << opt)) != 0);
}

ttysw_flush_input(ttysw0)
    caddr_t             ttysw0;
{
    struct ttysubwindow *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);
    struct sigvec vec, ovec;
    /* int                 (*presigval) () = signal(SIGTTOU, SIG_IGN); */

    vec.sv_handler = SIG_IGN;
    vec.sv_mask = vec.sv_onstack = 0;
    (void) sigvec(SIGTTOU, &vec, &ovec);

    /* Flush tty input buffer */
    if (ioctl(ttysw->ttysw_tty, TIOCFLUSH, 0))
	perror("TIOCFLUSH");

    /* (void) signal(SIGTTOU, presigval); */
    (void) sigvec(SIGTTOU, &ovec, (struct sigvec *)0);

    /* Flush ttysw input pending buffer */
    irbp = iwbp = ibuf;
}
