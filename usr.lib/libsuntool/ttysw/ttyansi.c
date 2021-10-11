#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttyansi.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1985, 1986, 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>
#undef CTRL
#include <suntool/ttyansi.h>

#include <suntool/selection_attributes.h>

extern Cmdsw	*cmdsw;
extern char	*sprintf();
extern void	pos();
extern void	win_sync();
extern char	*getenv();
char *strncpy();
char *textsw_checkpoint_undo();
Textsw_index	textsw_replace_bytes(), textsw_erase();
Textsw_status	textsw_set();

#ifdef DEBUG
#define ERROR_RETURN(val)	abort();	/* val */
#else
#define ERROR_RETURN(val)	return(val);
#endif DEBUG

extern	int	sv_journal;
extern	char	*shell_prompt;

/* Logical state of window */
int 	curscol;	/* cursor column */
int 	cursrow;	/* cursor row */
extern	int tty_cursor;
extern  int scroll_disabled_from_menu; 

			/* 0 -> NOCURSOR, 1 -> UNDERCURSOR, 2 -> BLOCKCURSOR */
static	int state;	/* ALPHA, SKIPPING, etc, possibly w/ |ESC */
static	int saved_state;
static	int prefix;	/* prefix to arg */
static	int scrlins = 1;/* How many lines to scroll when you have to */
static	int fillfunc;	/* 0 -> reverse video */
static	char strtype;	/* type of ansi string sequence */

/* dimensions of window */
int top;	/* top row of window (0 for now) */
int bottom;	/* bottom row of window */
int left;	/* left column of window (0 for now) */
int right;	/* right column of window */

/*
 * Interpret a string of characters of length <len>.  Stash and restore
 * the cursor indicator.
 *
 * Note that characters with the high bit set will not be recognized.
 * This is good, for it reserves them for ASCII-8 X3.64 implementation.
 * It just means all sources of chars which might come here must mask
 * parity if necessary.
 *
 */

/* #ifdef CMDSW */
static unsigned char *
from_pty_to_textsw(textsw, cp, buf)
	register Textsw		 textsw;
	register unsigned char	*buf;
	register unsigned char	*cp;
{
	int	 status = 0;
	int	 allow_enable = 1;
	register Textsw_index	insert, cmd_start;
	
	if (cp == buf) {
	    return (buf);
	}
	*cp = '\0';
	/* Set up - remove marks, save positions, etc. */
	if (cmdsw->append_only_log) {
	    /* Remove read_only_mark to allow insert */
	    textsw_remove_mark(textsw, cmdsw->read_only_mark);
	}
	
	/* Save start of user command */
	if (cmdsw->cmd_started) {
	    if ((cmd_start = textsw_find_mark(textsw, cmdsw->user_mark)) ==
	        TEXTSW_INFINITY)
	        ERROR_RETURN(0);
	    textsw_remove_mark(textsw, cmdsw->user_mark);
	    cmdsw->user_mark =
		textsw_add_mark(textsw, cmd_start+1,
				TEXTSW_MARK_MOVE_AT_INSERT);
	} else {
	    cmd_start = (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH);
	}

	/* Translate and edit in the pty input */
	ttysw_doing_pty_insert(textsw, cmdsw, TRUE);

	status =
	  send_input_to_textsw(textsw, buf, (long) (cp - buf), cmd_start);

	ttysw_doing_pty_insert(textsw, cmdsw, FALSE);
	    
	/* Restore user_mark, if cmd_started */
	if (cmdsw->cmd_started) {
	    insert = textsw_find_mark(textsw, cmdsw->user_mark);
	    textsw_remove_mark(textsw, cmdsw->user_mark);
	    if (insert == TEXTSW_INFINITY)
	        insert = cmd_start;
	    else
	        insert--;
	    cmdsw->user_mark =
		textsw_add_mark(textsw, insert, TEXTSW_MARK_DEFAULTS);
	    if (cmdsw->append_only_log) {
		cmdsw->read_only_mark =
		    textsw_add_mark(textsw,
		        cmdsw->cooked_echo ? insert : TEXTSW_INFINITY-1,
			TEXTSW_MARK_READ_ONLY);
	    }
	} else {
	    cmdsw->next_undo_point =
	           (caddr_t)textsw_checkpoint_undo(textsw,
	               (caddr_t)TEXTSW_INFINITY);
	    if (cmdsw->append_only_log) {
		insert = (Textsw_index)textsw_get(textsw, TEXTSW_LENGTH);
		cmdsw->read_only_mark =
		    textsw_add_mark(textsw,
		        cmdsw->cooked_echo ? insert : TEXTSW_INFINITY-1,
			TEXTSW_MARK_READ_ONLY);

		if ((sv_journal) && (svj_strcmp(buf) == 0))
		    win_sync(WIN_SYNC_TEXT,-1);
	    }
	}

	/* return (status ? (char *) 0 : buf); */

	if (status) {
	    cmdsw->enable_scroll_stay_on = TRUE;
	    return(NULL);
	} else 
	    return(buf);
}

/*
 * A version of textsw_replace_bytes that allows you to
 * trivially check the error code.
 */
static int
local_replace_bytes(textsw, pty_insert, last_plus_one, buf, buf_len)
	Textsw	 	 textsw;
	Textsw_index	 pty_insert;
	Textsw_index	 last_plus_one;
	register char	*buf;
	register long	 buf_len;
{
	int		delta = 0;
	int		status = 0;
	Textsw_mark	tmp_mark;
	
	tmp_mark = textsw_add_mark(textsw,
				  pty_insert,
				  TEXTSW_MARK_MOVE_AT_INSERT);
	delta = textsw_replace_bytes(textsw, pty_insert, last_plus_one,
			     buf, buf_len);
	if (!delta
	&& (textsw_find_mark(textsw, tmp_mark) == pty_insert)) {
	    status = 1;
	}
	textsw_remove_mark(textsw, tmp_mark);
	return (status);
}

/* Caller must be inserting text from pty and is responsible for unsetting
 *   the user_mark and read_only_mark BEFORE calling, and AFTER call for
 *   resetting them.
 */
static int
send_input_to_textsw(textsw, buf, buf_len, end_transcript)
	Textsw	 	textsw;
	register char	*buf;
	register long	 buf_len;
	Textsw_index	 end_transcript;
{
	Textsw_index	pty_insert = textsw_find_mark(textsw, cmdsw->pty_mark);
	Textsw_index	insert = (Textsw_index)textsw_get(textsw,
					TEXTSW_INSERTION_POINT);
	Textsw_index	last_plus_one;
	Textsw_index	add_newline = 0;
	Textsw_index	expanded_size;
#define BUFSIZE 200
	char		expand_buf[BUFSIZE];
	Textsw_mark	owe_newline_mark;
	int		status = 0;

	textsw_remove_mark(textsw, cmdsw->pty_mark);
	last_plus_one = end_transcript;
	if (cmdsw->pty_owes_newline)
	    last_plus_one--;
	if (buf_len < (last_plus_one - pty_insert))
	    last_plus_one = pty_insert + buf_len;
	/* replace from pty_insert to last_plus_one with buf */
	if (cmdsw->pty_owes_newline) {
	    /* try to pay the newline back */
	    if (buf[buf_len-1] == '\n' && last_plus_one == end_transcript) {
		cmdsw->pty_owes_newline = 0;
		if (--buf_len == (long) 0) {
		    return (status);
		}
	    }
	} else {
	    if ((cmdsw->cmd_started != 0) && (buf[buf_len-1] != '\n')) {
		add_newline = 1;
		owe_newline_mark = textsw_add_mark(textsw,
			end_transcript,
			TEXTSW_MARK_MOVE_AT_INSERT);
	    }
	}
	/* in case of tabs or control chars, expand chars to be replaced */
	expanded_size = last_plus_one - pty_insert;
	switch (textsw_expand(
		textsw, pty_insert, last_plus_one, expand_buf, BUFSIZE,
		(int *)(LINT_CAST(&expanded_size)))) {
          case TEXTSW_EXPAND_OK:
            break;
          case TEXTSW_EXPAND_FULL_BUF:
          case TEXTSW_EXPAND_OTHER_ERROR:
          default:
            expanded_size = last_plus_one - pty_insert;
            break;
	}
	if (expanded_size > buf_len) {
	    (void)strncpy(buf + buf_len, expand_buf + buf_len, 
	                  (int)expanded_size - buf_len);
	    buf_len = expanded_size;
	}
	if ((status = local_replace_bytes(textsw, pty_insert, last_plus_one,
					  buf, buf_len))) {
	    add_newline = 0;
	    buf_len = 0;
	}
	cmdsw->pty_mark = textsw_add_mark(textsw,
				  pty_insert + buf_len,
				  TEXTSW_MARK_DEFAULTS);
	if (add_newline != 0) {
	    add_newline = textsw_find_mark(textsw, owe_newline_mark);
	    textsw_remove_mark(textsw, owe_newline_mark);
	    cmdsw->pty_owes_newline =
	        textsw_replace_bytes(textsw, add_newline, add_newline,
	        "\n", (long int) 1);
	    if (!cmdsw->pty_owes_newline) {
	        status = 1;
	    }
	    add_newline = 1;
	}
	if (status)
	    return (status);
	/*
	 * BUG ALERT!
	 * If !append_only_log, and caret is in
	 * text that is being replaced, you lose.
	 */
	if (cmdsw->cooked_echo && insert >= end_transcript) {
	    /* if text before insertion point grew, move insertion point */
	    if (buf_len + add_newline > last_plus_one - pty_insert) {
		insert += buf_len + add_newline -
			  (int) (last_plus_one - pty_insert);
		(void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	    }
	} else if (!cmdsw->cooked_echo
	&& insert == pty_insert) {
	    insert += buf_len;
	    (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, insert, 0);
	}
	return (status);
#undef BUFSIZE
}
/* #endif CMDSW */

ansiinit(ttysw)
	struct ttysubwindow *ttysw;
{
	int ansi_escape();
	int ansi_string();
	char	windowname[WIN_NAMESIZE];

	/*
	 * Need to we_setmywindow in case tty processes want to find out
	 * which window running in.
	 */
	(void)win_fdtoname(ttysw->ttysw_wfd, windowname);
	(void)we_setmywindow(windowname);
	/*
	 * Setup gfx window environment value for gfx processes.
	 * Can be reset if a more appropriate window is available.
	 */
	(void)we_setgfxwindow(windowname);
	
	ttysw->ttysw_stringop = ansi_string;
	ttysw->ttysw_escapeop = ansi_escape;
}

/* ARGSUSED */
ansi_string(data, type, c)
	caddr_t data;
	char type, c;
{
	return(TTY_OK);
}

/* #ifndef CMDSW */
/* compatibility kludge */
gfxstring(addr, len)
	unsigned char *addr;
	int len;
{
	extern struct ttysubwindow *_ttysw;

	(void) ttysw_output((Ttysubwindow)(LINT_CAST(_ttysw)), addr, len);
}

void
ttysw_save_state() {
    saved_state = state;
    state = S_ALPHA;
}

void
ttysw_restore_state() {
    state = saved_state;
}

/* #endif CMDSW */

static int
erase_chars(textsw, pty_insert, end_span)
    Textsw		textsw;
    Textsw_index	pty_insert;
    Textsw_index	end_span;
{
    int		status = 0;

    if (pty_insert < 0)
	pty_insert = 0;
    if (end_span <= pty_insert)
	return (status);
    if (cmdsw->append_only_log) {
	/* Remove read_only_mark to allow insert */
	textsw_remove_mark(textsw, cmdsw->read_only_mark);
    }
    ttysw_doing_pty_insert(textsw, cmdsw, TRUE);

    status = textsw_erase(textsw, pty_insert, end_span) ? 0 : 1;

    ttysw_doing_pty_insert(textsw, cmdsw, FALSE);
    if (cmdsw->append_only_log) {
	int	cmd_start;
	if (cmdsw->cmd_started) {
	    cmd_start = textsw_find_mark(textsw, cmdsw->user_mark);
	} else {
	    cmd_start = (int)textsw_get(textsw, TEXTSW_LENGTH);
	}
	cmdsw->read_only_mark =
	    textsw_add_mark(textsw,
	        (Textsw_index) (cmdsw->cooked_echo ?
	        		cmd_start : TEXTSW_INFINITY-1),
	        TEXTSW_MARK_READ_ONLY);
    }
    return (status);
}

static int
replace_chars(textsw, start_span, end_span, buf, buflen)
    Textsw		 textsw;
    Textsw_index	 start_span;
    Textsw_index	 end_span;
    char		*buf;
    long int		 buflen;
{
    int		status = 0;
    if (start_span < 0)
	start_span = 0;
    if (end_span < start_span)
	end_span = start_span;
    if (cmdsw->append_only_log) {
	/* Remove read_only_mark to allow insert */
	textsw_remove_mark(textsw, cmdsw->read_only_mark);
    }
    ttysw_doing_pty_insert(textsw, cmdsw, TRUE);

    status = local_replace_bytes(textsw, start_span, end_span, buf, buflen);

    ttysw_doing_pty_insert(textsw, cmdsw, FALSE);
    if (cmdsw->append_only_log) {
	int	cmd_start;
	if (cmdsw->cmd_started) {
	    cmd_start = textsw_find_mark(textsw, cmdsw->user_mark);
	} else {
	    cmd_start = (int)textsw_get(textsw, TEXTSW_LENGTH);
	}
	cmdsw->read_only_mark =
	    textsw_add_mark(textsw,
	        (Textsw_index) (cmdsw->cooked_echo ?
	        		cmd_start : TEXTSW_INFINITY-1),
	        TEXTSW_MARK_READ_ONLY);
    }
    return (status);
}

static void
adjust_insertion_point(textsw, pty_index, new_pty_index)
    Textsw	textsw;
    int		pty_index,
    		new_pty_index;
{
    /* in ![cooked, echo], pty_mark = insert */
    if (!cmdsw->cooked_echo
    &&  (int)textsw_get(textsw, TEXTSW_INSERTION_POINT)
    ==  pty_index) {
        if (cmdsw->append_only_log) {
            /* Remove read_only_mark to allow insert */
            textsw_remove_mark(textsw, cmdsw->read_only_mark);
        }
        (void)textsw_set(textsw, TEXTSW_INSERTION_POINT, new_pty_index, 0);
        if (cmdsw->append_only_log) {
            cmdsw->read_only_mark =
              textsw_add_mark(textsw, TEXTSW_INFINITY-1,
              			TEXTSW_MARK_READ_ONLY);
        }
    }
}

static int
do_backspace(textsw, addr)
    Textsw	 textsw;
    char	*addr;
{
    Textsw_index	pty_index;
    Textsw_index	pty_end;
    Textsw_index	textsw_start_of_display_line();
    int			increment = 0;
    Textsw_index	expanded_size = 1;
#define BUFSIZE 10
    char		buf[BUFSIZE];

    pty_end = cmdsw->cmd_started ?
    	      textsw_find_mark(textsw, cmdsw->user_mark) :
    	      (int)textsw_get(textsw, TEXTSW_LENGTH);
    pty_index = textsw_find_mark(textsw, cmdsw->pty_mark);
    if (pty_index > textsw_start_of_display_line(textsw, pty_index)) {
        switch(textsw_expand(
            textsw, pty_index-1, pty_index, buf, BUFSIZE, 
	    (int *)(LINT_CAST(&expanded_size)))) {
          case TEXTSW_EXPAND_OK:
            break;
          case TEXTSW_EXPAND_FULL_BUF:
          case TEXTSW_EXPAND_OTHER_ERROR:
          default:
            buf[0] = ' ';
            expanded_size = 1;
            break;
        }
        textsw_remove_mark(textsw, cmdsw->pty_mark);
        if (expanded_size != 1) {
	    if (replace_chars(textsw, pty_index - 1, pty_index,
	        buf, expanded_size)) {
	        increment = -1;
	    }
	    pty_index += expanded_size - 1;
	    pty_end += expanded_size - 1;
        }
        cmdsw->pty_mark = textsw_add_mark(textsw,
	  pty_index - 1, TEXTSW_MARK_DEFAULTS);
	if (increment < 0)
	    return (increment);
	adjust_insertion_point(textsw, (int) pty_index, (int) pty_index - 1);
	/*
	 * if at the end of transcript, interpret ' ' as
	 * delete a character.
	 */
	if (pty_end == pty_index
	&&  strncmp(addr+1, " ", 2) == 0) {
	    if (erase_chars(textsw, pty_index-1, pty_index)) {
		increment = -1;
	    } else {
		increment = 2;
	    }
	}
    }
    return(increment);
#undef BUFSIZE
}

static
get_end_of_line(textsw)
    Textsw	 textsw;
{
    int		pty_index;
    int		pty_end;
    int		pattern_start;
    int		pattern_end;
    char	newline = '\n';

    pty_end = cmdsw->cmd_started ?
	textsw_find_mark(textsw, cmdsw->user_mark) :
	(int)textsw_get(textsw, TEXTSW_LENGTH);
    pty_index = textsw_find_mark(textsw, cmdsw->pty_mark);
    pattern_start = pty_index;
    if (pty_index == pty_end - cmdsw->pty_owes_newline
    ||  textsw_find_bytes(textsw,
	(long *)(LINT_CAST(&pattern_start)), 
	(long *)(LINT_CAST(&pattern_end)), &newline, 1, 0) ==  -1
    ||  pattern_start >= pty_end - cmdsw->pty_owes_newline
    ||  pattern_start < pty_index) {
	pattern_start = pty_end - cmdsw->pty_owes_newline;
    }
    return (pattern_start);
}

/*
 *  By definition, the pty_mark is on the last line of the transcript.
 *  Therefore, must insert a newline at pty_end, plus enough spaces
 *  to line up with old column.
 */
static int
do_linefeed(textsw)
    Textsw	textsw;
{
    int			pty_index;
    int			pty_end;
    Textsw_index	line_start;
    Textsw_index	textsw_start_of_display_line();
    char		newline = '\n';
#define BUFSIZE 2048
    char		buf[2048];
    char		*cp = buf;
    int			column;
    int			i;

    pty_end = cmdsw->cmd_started ?
	textsw_find_mark(textsw, cmdsw->user_mark) :
	(int)textsw_get(textsw, TEXTSW_LENGTH);
    pty_index = textsw_find_mark(textsw, cmdsw->pty_mark);
    line_start = textsw_start_of_display_line(textsw, pty_index);
    column = min(BUFSIZE - 3, (pty_index - line_start));

    textsw_remove_mark(textsw, cmdsw->pty_mark);
    cmdsw->pty_mark = textsw_add_mark(textsw,
        (Textsw_index) (pty_end - cmdsw->pty_owes_newline),
        TEXTSW_MARK_DEFAULTS);
    adjust_insertion_point(textsw,
        pty_index, pty_end - cmdsw->pty_owes_newline);

    *cp++ = newline;
    for (i = 0; i < column; i++) {
        *cp++ = ' ';
    }
    return (from_pty_to_textsw(textsw, cp, buf) ? 0 : 1);
#undef BUFSIZE
}

/*  This is a static instead of a return code, for backward
 *  compatibility reasons.
 */
static int handle_escape_status = 0;

ttysw_output(ttysw_client, addr, len0)
	Ttysubwindow ttysw_client;
	register unsigned char *addr;
	int len0;
{
    static	 int av[10];	/* args in ESCBRKT sequences.	*/
				/* -1 => defaulted		*/
    static	 int ac;	/* number of args in av		*/
#define BUFSIZE 1024
    Ttysw	*ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    Textsw	 textsw = (Textsw)ttysw->ttysw_hist;
    unsigned char	 buf[BUFSIZE];
    unsigned char	*cp = buf;
    int		 len = 0;
    int         upper_context;

    addr[len0] = '\0';
    if (!ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT))
	(void)removeCursor();
    for (; len < len0 && !ttysw->ttysw_frozen; len++, addr++) {
	if (state & S_ESC) {
	    switch (*addr) {
	    case NUL:
	    case DEL:
		/* all ignored */
		continue;
	    case '[':	/* Begin X3.64 escape code sequence */
		    ac = 0;
		    prefix = 0;
		    av[0] = -1;
		    state = S_ESCBRKT;
		    continue;

	    case 'P':	/* ANSI Device Control String */
	    case ']':	/* ANSI Operating System Command */
	    case '^':	/* ANSI Privacy Message */
	    case '_':	/* ANSI Application Program Command */
		    state = S_STRING;
		    strtype = *addr;
		    continue;

	    case '?':		/* simulate DEL char for systems
				    /* that can't xmit it. */
		*addr = DEL;
		state &= ~S_ESC;
		break;

	    case '\\':	/* ANSI string terminator */
		    if (state == (S_STRING|S_ESC)) {
			    ttysw_handlestring(ttysw, strtype, 0);
			    state = S_ALPHA;
			    continue;
		    }
		    /* FALL THROUGH */

	    default:
		state &= ~S_ESC;
		continue;
	    }
	}
	switch (state) {
	case S_ESCBRKT:
	    if (prefix == 0 && *addr >= '<' && *addr <= '?')
		    prefix = *addr;
	    else if (*addr >= '0' && *addr <= '9') {
		    if (av[ac] == -1)
			    av[ac] = 0;
		    av[ac] = ((short)av[ac])*10 + *addr - '0';
			    /* short for inline muls */
	    } else if (*addr == ';') {
		    av[ac] |= prefix << 24;
		    ac++;
		    av[ac] = -1;
		    prefix = 0;
	    } else {
		    /* XXX - should only terminate on valid end char */
		    av[ac] |= prefix << 24;
		    ac++;
		    switch (ttysw_handleescape(ttysw, *addr, ac, av)) {
		    case TTY_OK:
			    /* bug 1028029 
			    state = S_SKIPPING;
			    break;
			    */
		    case TTY_DONE:
			    state = S_ALPHA;
			    break;
		    default: {}
		    }
		    if (handle_escape_status) {
		        handle_escape_status = 0;
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    ac = 0;
		    prefix = 0;
	    }
	    break;

	case S_SKIPPING:
	    /* Waiting for char from cols 4-7 to end esc string */
	    if (*addr < '@')
		    break;
	    state = S_ALPHA;
	    break;

	case S_STRING:
	    if (!iscntrl(*addr))
		    ttysw_handlestring(ttysw, strtype, *addr);
	    else if (*addr == CTRL([))
		    state |= S_ESC;
	    break;

	case S_ALPHA:
	default:
	    if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
		state = S_ALPHA;
		switch (*addr) {
		case CTRL([):		/* Escape */
		    state |= S_ESC;
		    /* spit out what we have so far */
		    cp = from_pty_to_textsw(textsw, cp, buf);
		    if (!cp) {
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    break;
		case CTRL(G): {
		    extern struct pixwin *csr_pixwin;
		    struct pixwin *tmp_pixwin = csr_pixwin;
		    csr_pixwin = (struct pixwin *)
		        window_get(textsw, WIN_PIXWIN);
		    (void)blinkscreen();
		    csr_pixwin = tmp_pixwin;
		    break;
		}
		case NUL:	/* ignored */
		case DEL:	/* ignored */
		    break;
		case '\f': {	/* formfeed */
		    Textsw	view, textsw_first(), textsw_next();
		    int		pty_mark_shows;
		    int		pty_index =
		    		  textsw_find_mark(textsw, cmdsw->pty_mark);
		    *cp++ = '\n';
		    for (view = textsw_first(textsw);
			 view;
			 view = textsw_next(view))
		    {
			/*
			 * If pty_mark is showing,
			 * or if TEXTSW_INSERT_MAKES_VISIBLE == TEXTSW_ALWAYS
			 */
			pty_mark_shows = !textsw_does_index_not_show(view,
				(long)pty_index, (int *)0);
			if (pty_mark_shows
			|| (Textsw_enum)textsw_get(view,
				TEXTSW_INSERT_MAKES_VISIBLE)
			==	TEXTSW_ALWAYS /* != NEVER ??? */) {
			    /* spit out what we have so far */
			    cp = from_pty_to_textsw(view, cp, buf);
			    if (!cp) {
			        (void)ttysw_setopt(
			            (caddr_t)ttysw, TTYOPT_TEXT, 0);
			        return (0);
			    }
			    pty_index =
				textsw_find_mark(textsw, cmdsw->pty_mark);
                            /* we set the upper context to 0 for the clear */
                            /* command, then set it back to original value */
                            upper_context = 
                                (int) window_get(view, TEXTSW_UPPER_CONTEXT);
                            window_set (view, TEXTSW_UPPER_CONTEXT, 0, 0);
			    (void)textsw_set(
				view, TEXTSW_FIRST, pty_index, 0);
                            window_set (view, TEXTSW_UPPER_CONTEXT,
                                        upper_context, 0);
			}
		    }
		    if (cp >= &buf[sizeof(buf) - 1]) {
			/* spit out what we have so far */
			cp = from_pty_to_textsw(textsw, cp, buf);
			if (!cp) {
			    (void)ttysw_setopt(
				(caddr_t)ttysw, TTYOPT_TEXT, 0);
			    return (0);
			}
		    }
		    break;
		}
		case '\b': {	/* backspace */
		    register int	increment;
		    
		    /* preprocess buf */
		    if (cp > buf && *(cp-1) != '\t' && *(cp-1) != '\n') {
			while (*addr == '\b' && !iscntrl(*(addr+1))
			       && *(addr+1) != ' ') {
			    *(cp-1) = *(++addr);
			    addr++;
			    len += 2;
			}
		    }
		    if (*addr != '\b') {
			addr--;
			len--;
			break;
		    }
		    /* back up pty mark */
		    cp = from_pty_to_textsw(textsw, cp, buf);
		    if (!cp) {
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    if ((increment = do_backspace(textsw, addr)) > 0) {
		        addr += increment;
		        len += increment;
		    } else if (increment < 0) {
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    break;
		}
		case '\r': {
		    int			pty_index;
		    char		newline = '\n';
		    Textsw_index	line_start;
		    Textsw_index	textsw_start_of_display_line();

		    switch (*(addr+1)) {
		      case '\r':
		        /*
		         * compress multiple returns.
		         */
		        break;
		      case '\n':
		        /*
		         * if we're at the end,
		         *   increment to the newline and goto print_char,
		         * else process return normally.
		         */
		        pty_index = textsw_find_mark(textsw, cmdsw->pty_mark);
		        if ((cp-buf) >=
		            (get_end_of_line(textsw) - pty_index)) {
		            addr++;
		            len++;
		            goto print_char;
		        }
		      default: {

			/* spit out what we have so far */
			cp = from_pty_to_textsw(textsw, cp, buf);
			if (!cp) {
			    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
			    return (0);
			}
			pty_index =
			    textsw_find_mark(textsw, cmdsw->pty_mark);
			line_start =
			    textsw_start_of_display_line(textsw, pty_index);
			textsw_remove_mark(textsw, cmdsw->pty_mark);
			cmdsw->pty_mark = textsw_add_mark(textsw,
			      line_start,
			      TEXTSW_MARK_DEFAULTS);
			adjust_insertion_point(
			    textsw, pty_index, (int)line_start);
		      }
		    } /* else textsw displays \n as \r\n */
		    break;
		}
		case '\n': {	/* linefeed */
		    cp = from_pty_to_textsw(textsw, cp, buf);
		    if (!cp) {
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    if (do_linefeed(textsw)) {
		        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
		        return (0);
		    }
		    break;
		}
		case CTRL(K):	/* explicitly NOT HANDLED       */
		case '\t':	/* let textsw handle tab        */
print_char:
		default:
		    if (!(!iscntrl(*addr))
		    && *addr != '\t'
		    && *addr != '\n'
		    ||  ttysw->ttysw_frozen)
			break;
		    while ((!iscntrl(*addr)
		    || *addr == '\t' ||  *addr == '\n')
		    && len < len0 && !ttysw->ttysw_frozen) {
			*cp++ = *addr++;
			len++;
			if (cp == &buf[sizeof(buf) - 1]) {
			    /* spit out what we have so far */
			    cp = from_pty_to_textsw(textsw, cp, buf);
			    if (!cp) {
			        (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
			        return (0);
			    }
			}
		    }
		    len--; addr--;
		    break;
		}   /* switch (*addr) */
	    } else {   /* if (! TTYOPT_TEXT) */
		switch (*addr) {
		case CTRL(G):
			(void)blinkscreen();
			break;
		case '\b':
			pos(curscol-1, cursrow);
			break;
		case '\t':
			pos((curscol&-8)+8, cursrow);
			break;
		case '\n':	/* linefeed */
			if (ansi_lf(ttysw, addr, (len0-len)-1) == 0)
				goto ret;
			break;
		case CTRL(K):
			pos(curscol, cursrow-1); /* 4014 */
			break;
		case '\f':
			if ((ttysw->ttysw_opt&(1<<TTYOPT_PAGEMODE)) &&
			    ttysw->ttysw_lpp > 1) {
				if (ttysw_freeze(ttysw, 1))
					goto ret;
			}
#ifdef TTYHIST
			if (ttysw->ttysw_opt&(1<<TTYOPT_HISTORY))
				ttyhist_write(ttysw, bottom);
#endif
			(void)ttysw_clear(ttysw);
		case '\r':
			/* pos(0,cursrow); */
			curscol = 0;
			break;
		case CTRL([):
			state |= S_ESC;
			break;
		case DEL:	/* ignored */
			break;
    
		default:
			if (!iscntrl(*addr)) {
				int n;
    
				n = ansi_char(ttysw, addr, (len0-len));
				addr += n;
				len += n;
			}
		}
	    }   /* if (TTYOPT_TEXT) */
	}   /* switch (state) */
    }   /* for (; *addr; addr++) */
ret:
    if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
	cp = from_pty_to_textsw(textsw, cp, buf);
	if (!cp) {
	    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_TEXT, 0);
	    return (0);
	}
    } else {
	(void)drawCursor(cursrow, curscol);
    }
    return(len);
}

/* #ifndef CMDSW */

void
ttysw_lighten_cursor()
{
	(void)removeCursor();
	tty_cursor |= LIGHTCURSOR;
	(void)restoreCursor();
}

ttysw_restore_cursor()
{
	(void)removeCursor();
	tty_cursor &= ~LIGHTCURSOR;
	(void)restoreCursor();
}

static int
ansi_lf(ttysw, addr, len)
	register struct ttysubwindow *ttysw;
	char *addr;
	register int len;
{
	register int lfs = scrlins;
	extern int delaypainting;

	if (ttysw->ttysw_lpp >= bottom) {
		if (ttysw_freeze(ttysw, 1))
			return (0);
	}
	if (cursrow < bottom-1) {
		/* pos(curscol, cursrow+1); */
		cursrow++;
		if (ttysw->ttysw_opt&(1<<TTYOPT_PAGEMODE))
			ttysw->ttysw_lpp++;
		if (!scrlins) /* ...clear line */
			(void)deleteChar(left, right, cursrow);
	} else {
		if (delaypainting)
			(void)pdisplayscreen(1);
		if (!scrlins) {  /* Just wrap to top of screen and clr line */
			pos(curscol, 0);
			(void)deleteChar(left, right, cursrow);
		} else {
			if (lfs == 1) {
			    /* Find pending LF's and do them all now */
			    register char *cp;
			    register int left_end;

			    for (cp = addr+1, left_end = len; left_end--; cp++) {
				if (*cp == '\n')
				    lfs++;
				else if (*cp == '\r' || *cp >= ' ')
				    continue;
				else if (*cp > '\n')
				    break;
			    }
			}
			if (lfs + ttysw->ttysw_lpp > bottom)
				lfs = bottom - ttysw->ttysw_lpp;
#ifdef TTYHIST
			if (ttysw->ttysw_opt&(1<<TTYOPT_HISTORY))
				ttyhist_write(ttysw, lfs);
#endif
			(void)cim_scroll(lfs);
			if (ttysw->ttysw_opt&(1<<TTYOPT_PAGEMODE))
				ttysw->ttysw_lpp++;
			if (lfs != 1) /* avoid upsetting <dcok> for nothing */
				pos(curscol, cursrow+1-lfs);
		}
	}
	return (lfs);
}

static int
ansi_char(ttysw, addr, olen)
	struct ttysubwindow *ttysw;
	register unsigned char *addr;
	int olen;
{
	register int len = olen;
	char buf[300];
	register char *cp = &buf[0];
	int curscolstart = curscol;

	for (;;) {
		*cp++ = *addr;
		/* Update cursor position.  Inline for speed. */
		if (curscol < right-1)
			curscol++;
		else {
			/* Wrap to col 1 then pretend LF seen */
			*cp = '\0';
			(void)writePartialLine(buf, curscolstart);
			curscol = 0;
			(void) ansi_lf(ttysw, addr, len);
			return (olen - len);
		}
		if (len > 0) {
			if (!iscntrl(*(addr+1))
			&& cp < &buf[sizeof (buf) - 1]) {
				len--;
				addr++;
				continue;
			} else
				break; /* out of for loop */
		} else
			break;  /* out of for loop */
	}
	*cp = '\0';
	(void)writePartialLine(buf, curscolstart);
	return (olen - len);
}
/* #endif CMDSW */

static
ansi_escape(ttysw, c, ac, av)
	struct ttysubwindow *ttysw;
	register char c;
	register int ac, *av;
{
	register int av0, i, found = TRUE;
	Textsw	textsw;

	if ((av0 = av[0]) <= 0)
		av0 = 1;
	if (!ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) switch (c) {
	case '@':
		(void)insertChar(curscol, curscol+av0, cursrow);
		break;
	case 'A':
		pos(curscol, cursrow-av0);
		break;
	case 'B':
		pos(curscol, cursrow+av0);
		break;
	case 'C':
		pos(curscol+av0, cursrow);
		break;
	case 'D':
		pos(curscol-av0, cursrow);
		break;
	case 'E':
		pos(left, cursrow+av0);
		break;
	case 'f':
	case 'H':
		if (av[1] <= 0)
			av[1] = 1;
		pos(av[1]-1, av0-1);
		av[1] = 1;
		break;
	case 'L':	
		(void)insert_lines(cursrow, av0);
		break;
	case 'M':
		(void)delete_lines(cursrow, av0);
		break;
	case 'P':
		(void)deleteChar(curscol, curscol+av0, cursrow);
		break;
	case 'm':
		for (i = 0; i < ac; i++) {
			switch (av[i]) {
			case 0:
				clear_mode();
				break;
			case 1:
				bold_mode();
				break;
			case 4:
				underscore_mode();
				break;
			case 7:
				inverse_mode();
				break;
			case 2:
			case 3:
			case 5:
			case 6:	
			case 8:
			case 9: {
				int	ttysw_getboldstyle();	
				int	boldstyle = ttysw_getboldstyle();
			
				if (boldstyle & TTYSW_BOLD_NONE)
					inverse_mode();
				else
					bold_mode();
				break;
				}			
			default:
				clear_mode();
				break;
			}
		}
		break;
	case 'p':
		if (!fillfunc) {
			(void)screencomp();
			fillfunc = 1-fillfunc;
		}
		break;
	case 'q':
		if (fillfunc) {
			(void)screencomp();
			fillfunc = 1-fillfunc;
		}
		break;
	case 'r':
		scrlins = av0;
		break;
	case 's':
		scrlins = 1;
		(void)clear_mode();
#ifdef TTYHIST
		ttyhist_flush(ttysw);
#endif
		break;
	default:
		found = FALSE;
		break;
	} else {
	    found = FALSE;
	}
	if (!found) switch(c) {
	case 'J':
		if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
		    textsw = (Textsw)ttysw->ttysw_hist;
		    if (erase_chars(textsw,
		        textsw_find_mark(textsw, cmdsw->pty_mark),
		        cmdsw->cmd_started ?
			    textsw_find_mark(textsw, cmdsw->user_mark)
			      - (Textsw_index) cmdsw->pty_owes_newline :
			    (Textsw_index)
			      textsw_get(textsw, TEXTSW_LENGTH)))
		    {
		        handle_escape_status = 1;
		    }
		} else {
		    (void)delete_lines(cursrow+1, bottom-(cursrow+1));
		    (void)deleteChar(curscol, right, cursrow);
		}
		break;
	case 'K':	/* clear to end of line */
		if (ttysw_getopt((caddr_t) ttysw, TTYOPT_TEXT)) {
		    textsw = (Textsw)ttysw->ttysw_hist;
		    if (erase_chars(textsw,
		        textsw_find_mark(textsw, cmdsw->pty_mark),
		        (Textsw_index) get_end_of_line(textsw)))
		    {
		        handle_escape_status = 1;
		    }
		} else {
		    (void)deleteChar(curscol, right, cursrow);
		}
		break;
	case 'h': {	/* set mode */
		int	turn_on;
		
		for (i = 0; i < ac; i++) {
			if (av[i] > 0 &&
			    (av[i] & 0xff000000) == ('>' << 24)) {
			        turn_on = (((av[i] & 0x00ffffff) == TTYOPT_TEXT) ?
			                  !scroll_disabled_from_menu : TRUE);
			        if (!turn_on && scroll_disabled_from_menu)
			            cmdsw->enable_scroll_stay_on = TRUE;
				(void)ttysw_setopt((caddr_t) ttysw,
				    av[i] & 0x00ffffff, turn_on);
			}	    
		    }	  
		}		    
		break;

	case 'k':	/* report mode */
		for (i = 0; i < ac; i++)
			if (av[i] > 0 &&
			    (av[i] & 0xff000000) == ('>' << 24)) {
				char buf[16];

				(void) sprintf(buf, "\33[>%d%c",
				    av[i] & 0x00ffffff,
				    ttysw_getopt((caddr_t) ttysw,
				    av[i] & 0x00ffffff) ? 'h' : 'l');
				(void) ttysw_input((caddr_t) ttysw,
				    buf, strlen(buf));
			}
		break;

	case 'l':	/* reset mode */
		for (i = 0; i < ac; i++)
			if (av[i] > 0 &&
			    (av[i] & 0xff000000) == ('>' << 24))
				(void)ttysw_setopt((caddr_t) ttysw,
				    av[i] & 0x00ffffff, 0);
		break;

	default:	/* X3.64 says ignore if we don't know */
		return(TTY_OK);
	}
	return(TTY_DONE);
}

/* #ifndef CMDSW */
extern void
pos(col, row)
	register int col, row;
{

	if (col >= right)
		col = right - 1;
	if (col < left)
		col = left;
	if (row >= bottom)
		row = bottom - 1;
	if (row < top)
		row = top;
	cursrow = row;
	curscol = col;
	(void)vpos(row, col);
}
/* #endif CMDSW */

/* ARGSUSED */
ttysw_clear(ttysw)
      Ttysw *ttysw;
{
      pos(left, top);
      (void)cim_clear(top, bottom);
}
