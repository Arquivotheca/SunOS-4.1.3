#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)ttysw_init.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Ttysw initialization, destruction and error procedures
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/syscall.h>

#include <sgtty.h>
#include <signal.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>
#include <ctype.h>
/* #include <fcntl.h> */

#include <sunwindow/sun.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_input.h>
#include <sunwindow/defaults.h>
#include <suntool/scrollbar.h>
#include <suntool/ttysw.h>
#include <suntool/ttysw_impl.h>
#include <suntool/charscreen.h>

#ifdef KEYMAP_DEBUG
#include "../../libsunwindow/win/win_keymap.h"
#else
#include <sunwindow/win_keymap.h>
#endif


#define TTYSLOT_NOTFOUND(n)		((n)<=0)	/* BSD returns 0; S5 returns -1 */

#ifdef WITH_3X_LIBC
/* 3.x - 4.0 libc transition code; old (pre-4.0) code must define the symbol */
#define jcsetpgrp(p)	syscall(SYS_setpgrp,0,(p))
#endif

extern	char *	mktemp();
extern	char *	strncpy();
extern	char *	strcpy();
extern	long	lseek();
Textsw_status	textsw_set();
char		*textsw_checkpoint_undo();

#ifndef TIOCUCNTL
#define	TIOCUCNTL	_IOW(t, 102, int)	/* pty: set/clr usr cntl mode */
#endif	TIOCUCNTL
#ifndef TIOCTCNTL
#define TIOCTCNTL       _IOW(t, 32, int)        /* pty: set/clr intercept ioctl mode */
#endif	TIOCTCNTL

static Cmdsw	 cmdsw_tg;
Cmdsw		*cmdsw = &cmdsw_tg;


char 	*getenv();

extern Menu		ttysw_walkmenu();
extern	int	ttysel_use_seln_service;

struct ttysubwindow  *_ttysw;	/* kludge, for routines not fully converted */
int                   wfd;	/* ditto */
Textsw		      ttysw_cmdsw;	/* ditto, see ttysw.h */

struct ttysw_createoptions {
    int		  becomeconsole; /* be the console */
    char	**argv;		 /* args to be used in exec */
    char	 *args[4];	 /* scratch array if need to build argv */
};

/*
 * The tty subwindow cursor.
 */
static	short ttysw_data[16] = {
	0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xF000,
	0xD800, 0x9800, 0x0C00, 0x0C00, 0x0600, 0x0600, 0x0300, 0x0300
};
static	mpr_static(ttysw_mpr, 16, 16, 1, ttysw_data);
struct	cursor ttysw_cursor = {0, 0, PIX_SRC|PIX_DST, &ttysw_mpr};

static	Defaults_pairs	bold_style[] = {
	"None",			TTYSW_BOLD_NONE,
	"Offset_X",		TTYSW_BOLD_OFFSET_X,
	"Offset_Y",		TTYSW_BOLD_OFFSET_Y,
	"Offset_X_and_Y",	TTYSW_BOLD_OFFSET_X | TTYSW_BOLD_OFFSET_Y,
	"Offset_XY",		TTYSW_BOLD_OFFSET_XY,
	"Offset_X_and_XY",	TTYSW_BOLD_OFFSET_X | TTYSW_BOLD_OFFSET_XY,
	"Offset_Y_and_XY",	TTYSW_BOLD_OFFSET_Y | TTYSW_BOLD_OFFSET_XY,
	"Offset_X_and_Y_and_XY",TTYSW_BOLD_OFFSET_X |
				TTYSW_BOLD_OFFSET_Y |
				TTYSW_BOLD_OFFSET_XY,
	"Invert",		TTYSW_BOLD_INVERT,
	NULL,			-1
};

static	Defaults_pairs	inverse_and_underline_mode[] = {
	"Enabled",		TTYSW_ENABLE,
	"Disabled",		TTYSW_DISABLE,
	"Same_as_bold",		TTYSW_SAME_AS_BOLD,
	NULL,			-1
};


typedef enum {
	IF_AUTO_SCROLL		= 0,
	ALWAYS			= 1,
	INSERT_SAME_AS_TEXT	= 2
}	insert_makes_visible_flags;

static Defaults_pairs insert_makes_visible_pairs[] = {
	"If_auto_scroll",	(int)IF_AUTO_SCROLL,
	"Always",		(int)ALWAYS,
	"Same_as_for_text",	(int)INSERT_SAME_AS_TEXT,
	NULL, 			(int)INSERT_SAME_AS_TEXT
};

typedef enum {
	DO_NOT_USE_FONT		= 0,
	DO_USE_FONT		= 1,
	USE_FONT_SAME_AS_TEXT	= 2
}	control_chars_use_font_flags;

static Defaults_pairs control_chars_use_font_pairs[] = {
	"False",		(int)DO_NOT_USE_FONT,
	"True",			(int)DO_USE_FONT,
	"Same_as_for_text",	(int)USE_FONT_SAME_AS_TEXT,
	NULL, 			(int)USE_FONT_SAME_AS_TEXT
};

typedef enum {
	DO_NOT_AUTO_INDENT		= 0,
	DO_AUTO_INDENT			= 1,
	AUTO_INDENT_SAME_AS_TEXT	= 2
}	auto_indent_flags;

static Defaults_pairs auto_indent_pairs[] = {
	"False",		(int)DO_NOT_AUTO_INDENT,
	"True",			(int)DO_AUTO_INDENT,
	"Same_as_for_text",	(int)AUTO_INDENT_SAME_AS_TEXT,
	NULL, 			(int)AUTO_INDENT_SAME_AS_TEXT
};

ttysw_lookup_boldstyle(str)
    char *str;
{
    int		bstyle;
    if (str && isdigit(*str)) {
        bstyle = atoi(str);
        if (bstyle < TTYSW_BOLD_NONE || bstyle > TTYSW_BOLD_MAX)
            bstyle = -1;
        return bstyle;
    } else {
	return defaults_lookup(str, bold_style);
    }
}

ttysw_print_bold_options()
{
    Defaults_pairs *pbold;
    
    (void) fprintf(stderr, "Options for boldface are %d to %d or:\n",
        TTYSW_BOLD_NONE, TTYSW_BOLD_MAX);
    for (pbold = bold_style; pbold->name; pbold++) {
        (void) fprintf(stderr, "%s\n", pbold->name);
    }
}

/*
 * Ttysw initialization.
 */
caddr_t
ttysw_init(windowfd)
    int                   windowfd;
{
    struct ttysubwindow  *ttysw;
    extern                ttysw_eventstd();
/* #ifdef CMDSW */
    Textsw		  textsw;
    int			  tmpfile_fd;
    char		  tmpfile_name[100];
    Textsw_index	  length;
/* #endif CMDSW */

    ttysw = (struct ttysubwindow *) LINT_CAST(calloc(1, sizeof (struct ttysubwindow)));
    if (ttysw == 0)
	return ((caddr_t) NULL);
    ttysw->ttysw_wfd = wfd = windowfd;
    /* ttysw only, begin */
    ttysw->ttysw_eventop = ttysw_eventstd;
    (void) ttysw_setboldstyle(
	defaults_lookup(
	    (char *)defaults_get_string("/Tty/Bold_style", "Invert", (int *)0),
	    bold_style));
    (void) ttysw_set_inverse_mode(
	defaults_lookup(
	    (char *)defaults_get_string("/Tty/Inverse_mode", "Enabled", (int *)0),
	    inverse_and_underline_mode));
    (void) ttysw_set_underline_mode(
	defaults_lookup(
	    (char *)defaults_get_string("/Tty/Underline_mode", "Enabled", (int *)0),
	    inverse_and_underline_mode));        
    /* ttysw only, end */

    ttysw->ttysw_ibuf.cb_rbp = ttysw->ttysw_ibuf.cb_buf;
    ttysw->ttysw_ibuf.cb_wbp = ttysw->ttysw_ibuf.cb_buf;
    ttysw->ttysw_ibuf.cb_ebp =
	&ttysw->ttysw_ibuf.cb_buf[sizeof (ttysw->ttysw_ibuf.cb_buf)];
    ttysw->ttysw_obuf.cb_rbp = ttysw->ttysw_obuf.cb_buf;
    ttysw->ttysw_obuf.cb_wbp = ttysw->ttysw_obuf.cb_buf;
    ttysw->ttysw_obuf.cb_ebp =
	&ttysw->ttysw_obuf.cb_buf[sizeof (ttysw->ttysw_obuf.cb_buf)];
    ttysw->ttysw_kmtp = ttysw->ttysw_kmt;
    ttysw->ttysw_helpdata = "sunview:ttysw";
    if (_ttysw == (struct ttysubwindow *) NULL)
	_ttysw = ttysw;
    (void)ttysw_readrc(ttysw);

    if (ttysw_cmdsw) {
	extern int		  ttysw_textsw_changed();
	unsigned		  status;
	int			  ttymargin = 0;

	/* Cannot use ttysw_setopt because not all the way set up yet. */
	ttysw->ttysw_opt |= 1 << TTYOPT_TEXT;
	ttysw->ttysw_hist = (FILE *)LINT_CAST(ttysw_cmdsw);
	textsw = (Textsw)ttysw->ttysw_hist;
	(void) strcpy(tmpfile_name, "/tmp/tty.txt.XXXXXX");
	(void) mktemp(tmpfile_name);
	if ((tmpfile_fd = open(tmpfile_name, O_CREAT|O_RDWR|O_EXCL, 0600)) < 0)
	    return ((caddr_t)NULL);
	(void) close(tmpfile_fd);
	(void)textsw_set((Textsw) ttysw->ttysw_hist,
		     TEXTSW_STATUS, &status,
		     TEXTSW_CLIENT_DATA, ttysw,
		     TEXTSW_NOTIFY_LEVEL,
			    TEXTSW_NOTIFY_STANDARD|
			    TEXTSW_NOTIFY_EDIT|
			    TEXTSW_NOTIFY_DESTROY_VIEW|
			    TEXTSW_NOTIFY_SPLIT_VIEW,
		     TEXTSW_NOTIFY_PROC, ttysw_textsw_changed,
		     0);
	if (status != 0)
	    return ((caddr_t)NULL);
	cmdsw->next_undo_point =
	       (caddr_t)textsw_checkpoint_undo(textsw,
	           (caddr_t)TEXTSW_INFINITY);
	
	cmdsw->erase_line =
	    (char)textsw_get(textsw, TEXTSW_EDIT_BACK_LINE);
	cmdsw->erase_word =
	    (char)textsw_get(textsw, TEXTSW_EDIT_BACK_WORD);
	cmdsw->erase_char =
	    (char)textsw_get(textsw, TEXTSW_EDIT_BACK_CHAR);
	cmdsw->pty_eot = -1;
	cmdsw->cooked_echo = TRUE;
	cmdsw->ttysw_resized = FALSE;
	cmdsw->enable_scroll_stay_on = FALSE;
	
	ttymargin = (int)scrollbar_get(
	    textsw_get(textsw, TEXTSW_SCROLLBAR), SCROLL_THICKNESS);
	ttymargin += (int)textsw_get(textsw, TEXTSW_LEFT_MARGIN);
	ttymargin += (int)textsw_get(textsw, TEXTSW_RIGHT_MARGIN);
	(void)ttysw_setleftmargin(ttymargin);
	
	/* windowfd = wfd = ttysw_wfd is fd of tty subwindow.  Save it. */

	(void)ttysw_mapsetim(ttysw);
	
	/* stash textsw window fd in ttysw_wfd. */
	ttysw->ttysw_wfd = (int)(LINT_CAST(
		window_get((Window)(LINT_CAST(textsw)), WIN_FD)));
    }
    /*  Now windowfd is the tty subwindow fd  */

    if (ttyinit((Ttysubwindow)(LINT_CAST(ttysw))) == -1) {
	free((char *) ttysw);
	_ttysw = (struct ttysubwindow *) NULL;
	perror("ttysw_init");
	return ((caddr_t) NULL);
    }
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	char		*def_str;
	
	(void)textsw_set(textsw, TEXTSW_FILE, tmpfile_name, 0);
	(void)textsw_set(textsw, TEXTSW_TEMP_FILENAME, tmpfile_name, 0);
	length = (int)textsw_get(textsw, TEXTSW_LENGTH);
	cmdsw->cmd_started = cmdsw->pty_owes_newline = 0;
	cmdsw->user_mark =
	    textsw_add_mark(textsw, length, TEXTSW_MARK_DEFAULTS);
	cmdsw->pty_mark =
	    textsw_add_mark(textsw, length, TEXTSW_MARK_DEFAULTS);
	cmdsw->append_only_log =
	    (int)defaults_get_boolean("/Tty/Append_only_log", (Bool)TRUE, (int *)NULL);
	if (cmdsw->append_only_log)
	    /* Note that read_only_mark is not TEXTSW_MOVE_AT_INSERT.
	     * Thus, as soon as it quits being moved by pty inserts,
	     * it will equal the user_mark.
	     */
	    cmdsw->read_only_mark =
		textsw_add_mark(textsw,
		    cmdsw->cooked_echo ? length : TEXTSW_INFINITY-1,
		    TEXTSW_MARK_READ_ONLY);

	def_str = defaults_get_string("/Tty/Auto_indent",
				     "False", (int *)NULL);
	switch(defaults_lookup(def_str, auto_indent_pairs)) {
	case DO_NOT_AUTO_INDENT:
	    (void)textsw_set(textsw,
		TEXTSW_AUTO_INDENT, 0,
		0);
	    break;
	case DO_AUTO_INDENT:
	    (void)textsw_set(textsw,
		TEXTSW_AUTO_INDENT, TRUE,
		0);
	    break;
	/* default: do nothing */
	}

	def_str = defaults_get_string("/Tty/Control_chars_use_font",
				     "Same_as_for_text", (int *)NULL);
	switch(defaults_lookup(def_str, control_chars_use_font_pairs)) {
	case DO_NOT_USE_FONT:
	    (void)textsw_set(textsw,
		TEXTSW_CONTROL_CHARS_USE_FONT, 0,
		0);
	    break;
	case DO_USE_FONT:
	    (void)textsw_set(textsw,
		TEXTSW_CONTROL_CHARS_USE_FONT, TRUE,
		0);
	    break;
	/* default: do nothing */
	}

	def_str = defaults_get_string("/Tty/Insert_makes_caret_visible",
				     (char *) NULL, (int *)NULL);
	switch(defaults_lookup(def_str, insert_makes_visible_pairs)) {
	case IF_AUTO_SCROLL:
	    (void)textsw_set(textsw,
		TEXTSW_INSERT_MAKES_VISIBLE, TEXTSW_IF_AUTO_SCROLL,
		0);
	    break;
	case ALWAYS:
	    (void)textsw_set(textsw,
		TEXTSW_INSERT_MAKES_VISIBLE, TEXTSW_ALWAYS,
		0);
	    break;
	/* default: do nothing */
	}

	/* Must come *after* setting append_only_log to get string
	 * correct in the append_only_log toggle item.
	 */
	(void)ttysw_set_menu(textsw);
    }
    (void)ansiinit(ttysw);
    if (imageinit(windowfd) == 0) {
	free((char *) ttysw);
	_ttysw = (struct ttysubwindow *) NULL;
	return ((caddr_t) NULL);
    }
    (void)win_setcursor(windowfd, &ttysw_cursor);
    /* ttysw only, begin */
    /* initialize selection service code */
    (void)ttysw_setopt((caddr_t)ttysw, TTYOPT_SELSVC, ttysel_use_seln_service);
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_SELSVC)) {
	    ttysel_init_client(ttysw);
    }

    /* setup the walking menu */
    ttysw->ttysw_menu = ttysw_walkmenu(ttysw);
    /* ttysw only, end */
    return ((caddr_t) ttysw);
}

int
ttysw_add_FNDELAY(fd)
    int                   fd;
{
    int                   fdflags;

    if ((fdflags = fcntl(fd, F_GETFL, 0)) == -1)
	return (-1);
    fdflags |= FNDELAY;
    if (fcntl(fd, F_SETFL, fdflags) == -1)
	return (-1);
    return (0);
}

/*
 * Fork the ttysw's driving tty process. 
 */
/* ARGSUSED */
int
ttysw_fork(ttysw0, argv, inputmask, outputmask, exceptmask)
    caddr_t               ttysw0;
    char                **argv;
    int                  *inputmask, *outputmask, *exceptmask;
{
    struct ttysubwindow  *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);

    *inputmask |= (1 << ttysw->ttysw_pty) | (1 << ttysw->ttysw_wfd);
    return (ttysw_fork_it(ttysw0, argv));
}

/* ARGSUSED */
int
ttysw_fork_it(ttysw0, argv)
    caddr_t               ttysw0;
    char                **argv;
{
    struct ttysubwindow  *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);
    struct ttysw_createoptions opts;
    int                   pidchild;
    unsigned              ttysw_error_sleep = 2;
    struct sigvec 	  vec, ovec;
    register int 	  i;

    ttysw->ttysw_pidchild = fork();
    if (ttysw->ttysw_pidchild < 0)
	return (-1);
    if (ttysw->ttysw_pidchild) {

	if (((ttysw->ttysw_wfd == wfd) ? FALSE :
	    ttysw_add_FNDELAY(wfd))
	||  ttysw_add_FNDELAY(ttysw->ttysw_wfd)
	||  ttysw_add_FNDELAY(ttysw->ttysw_pty)) {
	    perror("fcntl");
	    return (-1);
	}

	/* (void) signal(SIGTSTP, SIG_IGN); */
	vec.sv_handler = SIG_IGN;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(SIGTSTP, &vec, 0);

#ifdef DEBUG
	usleep(3 << 20);
#endif DEBUG
	return (ttysw->ttysw_pidchild);
    }

    /* (void) signal(SIGWINCH, SIG_DFL);/* we don't care about SIGWINCH's */
    vec.sv_handler = SIG_DFL; 
    vec.sv_mask = vec.sv_onstack = 0; 
    sigvec(SIGWINCH, &vec, 0);

    /*
     * Change process group of child process (me at this point in code) so its
     * signal stuff doesn't affect the terminal emulator. 
     */
    pidchild = getpid();

    vec.sv_handler = SIG_IGN;
    vec.sv_mask = vec.sv_onstack = 0;
    sigvec(SIGTTOU, &vec, &ovec);
    {
	/* int (*presigval) () = signal(SIGTTOU, SIG_IGN); */

	if ((ioctl(ttysw->ttysw_tty, TIOCSPGRP, &pidchild)) == -1) {
	    perror("ttysw_fork_it-TIOCSPGRP");
	    usleep(ttysw_error_sleep << 20);
	}
	(void) jcsetpgrp(pidchild);

	/* (void) signal(SIGTTOU, presigval); */
	sigvec(SIGTTOU, &ovec, 0);
    }
    /*
     * Set up file descriptors 
     */
    (void) close(ttysw->ttysw_wfd);
    (void) close(ttysw->ttysw_pty);
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
        int   tmp_window_fd;
        
        if ((tmp_window_fd = win_get_fd((Notify_client) ttysw->ttysw_hist))
        != ttysw->ttysw_wfd)
            (void) close(tmp_window_fd);
        if ((tmp_window_fd = win_get_fd((Notify_client) ttysw))
        != ttysw->ttysw_wfd)
            (void) close(tmp_window_fd);
    }
    (void) dup2(ttysw->ttysw_tty, 0);
    (void) dup2(ttysw->ttysw_tty, 1);
    (void) dup2(ttysw->ttysw_tty, 2);
    (void) close(ttysw->ttysw_tty);
/*    i = getdtablesize();
    while (--i > 2)
       (void) close(i);
*/
    if (*argv == (char *) NULL || strcmp("-c", *argv) == 0) {
	/* Process arg list */
	int                   argc;

	for (argc = 0; argv[argc]; argc++);
	ttysw_parseargs(&opts, &argc, argv);
	argv = opts.argv;
    }
    execvp(*argv, argv);
    perror(*argv);
    /* Wait a few seconds so that message can be read on ttysw */
    usleep(ttysw_error_sleep << 20);
    exit(1);
    /* NOTREACHED */
}

/*
 * Create options calls 
 */
ttysw_parseargs(opts, argcptr, argv)
    struct ttysw_createoptions *opts;
    int                  *argcptr;
    char                **argv;
{
    char                 *shell;
    int                   argc = *argcptr;
    char                **args = argv;

    bzero((caddr_t) opts, sizeof (*opts));
    /* Get options */
    while (argc > 0) {
	if (strcmp(argv[0], "-C") == 0 ||
	    strcmp(argv[0], "CONSOLE") == 0) {
	    opts->becomeconsole = 1;
	    (void)cmdline_scrunch(argcptr, argv);
	} else
	    argv++;
	argc--;
    }
    argv = args;
    opts->argv = opts->args;
    /* Determine what shell to run. */
    shell = getenv("SHELL");
    if (!shell || !*shell)
	shell = "/bin/sh";
    opts->args[0] = shell;
    /* Setup remainder of arguments */
    if (*argv == (char *) NULL) {
	opts->args[1] = (char *) NULL;
#ifndef someday			       /* this should go away */
    } else if (strcmp("-c", *argv) == 0) {
	/*
	 * The '-c' flag tells the shell to run following arg 
	 */
	opts->args[1] = *argv;
	(void)cmdline_scrunch(argcptr, argv);
	opts->args[2] = *(argv);
	(void)cmdline_scrunch(argcptr, argv);
	opts->args[3] = (char *) NULL;
#endif
    } else {
	/*
	 * Run program not under shell 
	 */
	opts->argv = argv;
    }
    return;
}

/*
 * Make ttysw the console. 
 */
ttysw_becomeconsole(ttysw0)
    caddr_t               ttysw0;
{
    struct ttysubwindow  *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);

    if ((ioctl(ttysw->ttysw_tty, TIOCCONS, 0)) == -1)
	perror("ttysw-TIOCCONS");
}

/*
 * Ttysw cleanup. 
 */
ttysw_done(ttysw0)
    caddr_t               ttysw0;
{
    struct ttysubwindow  *ttysw = (struct ttysubwindow *) LINT_CAST(ttysw0);

    if (ttysw->ttysw_ttyslot)
	(void) updateutmp("", ttysw->ttysw_ttyslot, ttysw->ttysw_tty);
    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	(void)winCleanup();
	ttysel_destroy(ttysw);
    }

    if (ttysw->ttysw_menu)
	menu_destroy(ttysw->ttysw_menu);
	
    if (ttysw->ttysw_pty)
        close(ttysw->ttysw_pty);
    if (ttysw->ttysw_tty)    
        close(ttysw->ttysw_tty);
        
    free((char *) ttysw);
}

/*
 * Set TERM and TERMCAP environment variables.
 */
static void
ttysw_setenv(ttysw)
    Ttysw		  *ttysw;
{
    char		  *termcap;
    static char		   *cmd_termcap = "sun-cmd:te=\\E[>4h:ti=\\E[>4l:tc=sun:";
    static char		   *cmd_term = "sun-cmd";

    if (!ttysw->ttysw_hist)
	return;
    setenv("TERM", cmd_term);
    termcap = getenv("TERMCAP");
    if (termcap && *termcap == '/')
	return;
    setenv("TERMCAP", cmd_termcap);
}

/*
 * Do tty/pty setup 
 */
int
ttyinit(ttysw_client)
    Ttysubwindow  ttysw_client;
{
    int                   tt, tmpfd, ptynum = 0;
    int                   pty = 0, tty = 0;
    int                   on = 1;
    char                  linebuf[20], *line = &linebuf[0];
    char		  *ptyp = "pqrstuvwxyzPQRST";
    struct stat           stb;
    Ttysw		  *ttysw;

    ttysw = (Ttysw *)(LINT_CAST(ttysw_client));
    ttysw_setenv(ttysw);
    /*
     * find unopened pty 
     */
needpty:
    while (*ptyp) {
	(void) strcpy(line, "/dev/ptyXX");
	line[strlen("/dev/pty")] = *ptyp;
	line[strlen("/dev/ptyp")] = '0';
	if (stat(line, &stb) < 0)
	    break;
	while(ptynum < 16) {
	    line[strlen("/dev/ptyp")] = "0123456789abcdef"[ptynum];
	    pty = open(line, 2);
	    if (pty > 0)
		goto gotpty;
	    ptynum++;
	}
	ptyp++;
	ptynum = 0;
    }
    (void) fprintf(stderr, "All pty's in use\n");
    return (-1);
    /* NOTREACHED */
gotpty:
    line[strlen("/dev/")] = 't';
    tt = open("/dev/tty", 2);
    if (tt > 0) {
	(void) ioctl(tt, TIOCNOTTY, 0);
	(void) close(tt);
    }
    tty = open(line, 2);
    if (tty < 0) {
	ptynum++;
	(void) close(pty);
	goto needpty;
    }
#define	WE_TTYPARMS	"WINDOW_TTYPARMS"
    if (ttysw_restoreparms(tty))
	(void)unsetenv(WE_TTYPARMS);

    /*
     * Copy stdin.  Set stdin to tty so ttyslot in updateutmp will think this
     * is the control terminal.  Restore state. Note: ttyslot should have
     * companion ttyslotf(fd). 
     */
    tmpfd = dup(0);
    (void) close(0);
    (void) dup(tty);
    ttysw->ttysw_ttyslot = updateutmp((char *) 0, 0, tty);
    (void) close(0);
    (void) dup(tmpfd);
    (void) close(tmpfd);
    ttysw->ttysw_tty = tty;
    ttysw->ttysw_pty = pty;
    (void)ttysw_mapsetim(ttysw);	/* in cmdsw, set input masks for textsw */

    if (ttysw_getopt((caddr_t)ttysw, TTYOPT_TEXT)) {
	(void) ioctl(ttysw->ttysw_tty, TIOCGETP, &cmdsw->sgttyb);
	(void) ioctl(ttysw->ttysw_tty, TIOCGETC, &cmdsw->tchars);
	(void) ioctl(ttysw->ttysw_tty, TIOCGLTC, &cmdsw->ltchars);
	(void) ioctl(ttysw->ttysw_pty, TIOCREMOTE, &on);
    }
    if (ioctl(ttysw->ttysw_pty, TIOCTCNTL,  &on) == -1) {
	perror("TTYSW - setting TIOCTCNTL to 1 failed");
    }

    return (0);
}

/*
 * Make entry in /etc/utmp for ttyfd. Note: this is dubious usage of /etc/utmp
 * but many programs (e.g. sccs) look there when determining who is logged in
 * to the pty. 
 */
int
updateutmp(username, ttyslotuse, ttyfd)
    char                 *username;
    int                   ttyslotuse, ttyfd;
{
    /*
     * Update /etc/utmp 
     */
    struct utmp           utmp;
    struct passwd        *passwdent;
    extern struct passwd *getpwuid();
    int                   f;
    char                 *ttyn;
    extern char          *ttyname(), *rindex();

    if (!username) {
	/*
	 * Get login name 
	 */
	if ((passwdent = getpwuid(getuid())) == (struct passwd *) NULL) {
	    (void) fprintf(stderr, "couldn't find user name\n");
	    return (0);
	}
	username = passwdent->pw_name;
    }
    utmp.ut_name[0] = '\0';	       /* Set incase *username is 0 */
    (void) strncpy(utmp.ut_name, username, sizeof (utmp.ut_name));
    /*
     * Get line (tty) name 
     */
    ttyn = ttyname(ttyfd);
    if (ttyn == (char *) NULL)
	ttyn = "/dev/tty??";
    (void) strncpy(utmp.ut_line, rindex(ttyn, '/') + 1, sizeof (utmp.ut_line));
    /*
     * Set host to be empty 
     */
    (void) strncpy(utmp.ut_host, "", sizeof (utmp.ut_host));
    /*
     * Get start time 
     */
    (void) time((time_t *) (&utmp.ut_time));
    /*
     * Put utmp in /etc/utmp 
     */
    if (ttyslotuse == 0)
		ttyslotuse = ttyslot();
		
    if (TTYSLOT_NOTFOUND(ttyslotuse)) {
		(void) fprintf(stderr,
	  		"Cannot find slot in /etc/ttys for updating /etc/utmp.\n");
		(void) fprintf(stderr, "Commands like \"who\" will not work.\n");
		(void) fprintf(stderr, "Add tty[qrs][0-f] to /etc/ttys file.\n");
		return (0);
    }
    if ((f = open("/etc/utmp", 1)) >= 0) {
		(void) lseek(f, (long) (ttyslotuse * sizeof (utmp)), 0);
		(void) write(f, (char *) &utmp, sizeof (utmp));
		(void) close(f);
    } else {
		(void) fprintf(stderr, "make sure that you can write /etc/utmp!\n");
		return (0);
    }
    return (ttyslotuse);
}

/* Pkg_private */
void
ttysw_tty_restore (fd)
int fd;
{
        struct sigvec         vec, ovec;
        unsigned              ttysw_error_sleep = 2;
        int                   pgrp = getpgrp(getpid());

        vec.sv_handler = SIG_IGN;
        vec.sv_mask = vec.sv_onstack = 0;
        sigvec(SIGTTOU, &vec, &ovec);
        if ((ioctl(fd, TIOCSPGRP, &pgrp)) == -1) {
            perror("ttysw_tty_restore-TIOCSPGRP");
            usleep(ttysw_error_sleep << 20);
        }
        sigvec(SIGTTOU, &ovec, 0);
}
