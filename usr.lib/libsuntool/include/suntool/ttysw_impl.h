#ifndef SUNTOOL_TTYSW_IMPL
#define SUNTOOL_TTYSW_IMPL	1

/*      @(#)ttysw_impl.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * A tty subwindow is a subwindow type that is used to provide a
 * terminal emulation for teletype based programs.
 */

#include <sunwindow/notify.h>
#include <suntool/walkmenu.h>
#include <suntool/help.h>

#ifndef LINT_CAST
#ifdef lint
#define LINT_CAST(arg)  (arg ? 0 : 0)
#else
#define LINT_CAST(arg)  arg
#endif
#endif  LINT_CAST

/*
 * These definitions of the standard user-interface function keys
 * are used in ttysw_mapkeys.c and ttysw_main.c, at a higher precedence
 * than any user-defined key mappings
 */

#define KEY_STOP    ACTION_STOP
#define KEY_AGAIN   ACTION_AGAIN
#define KEY_PROPS   ACTION_PROPS
#define KEY_UNDO    ACTION_UNDO
#define KEY_FRONT   ACTION_FRONT
#define KEY_PUT	    ACTION_COPY
#define KEY_CLOSE   ACTION_CLOSE
#define KEY_GET	    ACTION_PASTE
#define KEY_FIND    ACTION_FIND_FORWARD
#define KEY_DELETE  ACTION_CUT
#define KEY_HELP    ACTION_HELP

#define KEY_CAPS    ACTION_CAPS_LOCK

/*
 * These are the data structures internal to the tty subwindow
 * implementation.  They are considered private to the implementation.
 */

struct cbuf {
    char               *cb_rbp;	   /* read pointer */
    char               *cb_wbp;	   /* write pointer */
    char               *cb_ebp;	   /* end of buffer */
    char                cb_buf[8192];
};

struct keymaptab {
    int                 kmt_key;
    int                 kmt_output;
    char               *kmt_to;
};

struct textselpos {
    int                 tsp_row;
    int                 tsp_col;
};

struct ttyselection {
    char                sel_level; /* see below */
    char                sel_anchor;/* -1 = left, 0 = none, 1 = right */
    struct textselpos   sel_begin; /* beginning of selection */
    struct textselpos   sel_end;   /* end of selection */
    struct timeval      sel_time;  /* time selection was made */
    int                 sel_made:1;  /* a selection has been made */
    int                 sel_null:1;  /* the selection is null */
};

/* selection levels */
#define	SEL_CHAR	0
#define	SEL_WORD	1
#define	SEL_LINE	2
#define	SEL_PARA	3
#define	SEL_MAX		3

extern struct ttyselection	null_ttyselection;

typedef struct ttysubwindow {
    /* common */
    int                 ttysw_opt;		/* mask of options from ttysw.h */
#define	TTYOPT_HISTORY	2		/* XXX Put in ttysw.h when supported */
    struct cbuf         ttysw_ibuf;		/* input buffer */
    struct cbuf         ttysw_obuf;		/* output buffer */
    int                 ttysw_wfd;		/* window file descriptor */
    /* pty and subprocess */
    int                 ttysw_pty;		/* pty file descriptor */
    int                 ttysw_tty;		/* tty file descriptor */
    int                 ttysw_ttyslot;		/* ttyslot in utmp for tty */
    /* page mode */
    int                 ttysw_frozen;		/* output is frozen */
    int                 ttysw_lpp;		/* page mode: lines per page */
    /* Caps Lock */
    int                 ttysw_capslocked;
#define TTYSW_CAPSLOCKED	0x01	/* capslocked on mask bit */
#define TTYSW_CAPSSAWESC	0x02	/* saw escape while caps locked */
    /* history */
    FILE               *ttysw_hist;		/* history file */
    /* selection */
    int                 ttysw_butdown;		/* which button is down */
    struct ttyselection	ttysw_caret;
    struct ttyselection	ttysw_primary;
    struct ttyselection	ttysw_secondary;
    struct ttyselection	ttysw_shelf;
    caddr_t             ttysw_seln_client;
    /* client data */
    caddr_t             ttysw_client;		/* private data of client */
    /* help data */
    caddr_t		ttysw_helpdata;
    /* replaceable ops (return TTY_OK or TTY_DONE) */
    int                 (*ttysw_escapeop) ();	/* handle escape sequences */
    int                 (*ttysw_stringop) ();	/* handle accumulated string */
    int                 (*ttysw_eventop) ();	/* handle input event */
    /* kbd translation */
    struct keymaptab    ttysw_kmt[3 * 16 + 2];	/* Key map list */
    struct keymaptab   *ttysw_kmtp;		/* Ptr into ttysw_kmt next empty slot */
    /* walking menu */
    Menu		ttysw_menu;
    /* subprocess */
    int                 ttysw_pidchild;		/* pid of the child */
    unsigned		ttysw_flags;
}                   Ttysw;

/* Values for ttysw_flags */
#define TTYSW_FL_USING_NOTIFIER		0x000001
#define TTYSW_FL_IN_PRIORITIZER		0x000002

#define TTYSW_NULL      ((Ttysw *)0)

/*
 * Possible returns codes from replaceable ops. 
 */
#define	TTY_OK		(0)	   /* args should be handled as normal */
#define	TTY_DONE	(1)	   /* args have been fully handled */

#define	ttysw_handleevent(ttysw, ie) \
	(*(ttysw)->ttysw_eventop)((ttysw), (ie))
#define	ttysw_handleescape(ttysw, c, ac, av) \
	(*(ttysw)->ttysw_escapeop)((ttysw), (c), (ac), (av))
#define	ttysw_handlestring(ttysw, strtype, c) \
	(*(ttysw)->ttysw_stringop)((ttysw), (strtype), (c))

/*	extern routines	*/

void	ttysel_init_client(),
	ttysel_destroy(),
	ttysel_acquire(),
	ttysel_make(),
	ttysel_move(),
	ttysel_deselect(),
	ttysel_hilite(),
	ttyhiliteselection(),
	ttysel_nullselection(),
	ttysel_setselection(),
	ttysel_getselection();

extern Notify_value	ttysw_event();		/* in file ttysw_notify.c */
extern Notify_value	ttysw_destroy();	/* in file ttysw_notify.c */
extern caddr_t          ttysw_init();           /* in file ttysw_init.c */

/* #ifdef CMDSW */

#include <sgtty.h>
#include <suntool/textsw.h>

typedef struct cmdsw {
	int		cmd_started;	  /* Actually Boolean: 0 or !0 */
	int		append_only_log;  /* Actually Boolean: 0 or !0 */
	Textsw_mark	user_mark;
	Textsw_mark	pty_mark;
	Textsw_mark	read_only_mark;   /* Valid iff append_only_log */
	int		pty_owes_newline; /* Actually Boolean: 0 or !0 */
	int		pty_eot;	  /* Actually Boolean: 0 or !0 */
	int		doing_pty_insert; /* Actually Boolean: 0 or !0 */
	caddr_t		next_undo_point;
	char		erase_line;
	char		erase_word;
	char		erase_char;
	int             cooked_echo;      /* Actually Boolean: 0 or !0 */
	int		history_limit;	  /* save while in !cooked_echo*/
	int		ttysw_resized;	  /* Actually Boolean: 0 or !0 */
	int		literal_next;	  /* Actually Boolean: 0 or !0 */
        int		enable_scroll_stay_on;    /* Actually Boolean: 0 or !0 */
                /* Keep track of terminal characteristics */
        struct sgttyb   sgttyb;
        struct tchars   tchars;
        struct ltchars  ltchars;
} Cmdsw;

/* #endif CMDSW */

#ifdef	cplus
/*
 * C Library routines specifically related to private ttysw subwindow
 * functions.  ttysw_output and ttysw_input return the number of characters
 * accepted/processed (usually equal to len). 
 */
int 
ttysw_output(struct ttysubwindow * ttysw, char *addr, int len);

/* Interpret string in terminal emulator. */
int 
ttysw_input(struct ttysubwindow * ttysw, char *addr, int len);

/* Add string to the input queue. */
#endif	cplus
#endif	SUNTOOL_TTYSW_IMPL
