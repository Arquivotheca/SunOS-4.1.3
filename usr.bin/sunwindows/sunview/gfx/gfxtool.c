#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)gfxtool.c 1.1 92/07/30";
#endif
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 *  gfxtool - run a process in a tty subwindow with a separate graphic area
 */

#include <stdio.h>
#include <signal.h>
#include <suntool/tool_hs.h>
#include <suntool/ttysw.h>
#include <suntool/ttytlsw.h>
#include <suntool/emptysw.h>

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

extern	char *getenv();
extern	char *strcat();
extern	char *strncat();

static	void sigwinchcatcher(), sigchldcatcher(), sigtermcatcher();

static	struct tool *tool;
static	struct	toolsw *tsw;

static short ic_image[258] = {
#include <images/gfxtool.icon>
};
mpr_static(gfxic_mpr, 64, 64, 1, ic_image);

static	struct icon icon = {64, 64, (struct pixrect *)NULL, 0, 0, 64, 64,
	    &gfxic_mpr, 0, 0, 0, 0, NULL, (struct pixfont *)NULL,
	    ICON_BKGRDCLR};
#ifdef STANDALONE
main(argc, argv)
#else
int gfxtool_main(argc, argv)
#endif STANDALONE
	int argc;
	char **argv;
{
	char	**tool_attrs = NULL;
	int	become_console = 0;
	char	*tool_name = argv[0], *tmp_str;
	static	char *label_default = "gfxtool";
	static	char *label_console = " (CONSOLE): ";
	static	char label[150];
	static	char icon_label[30];
	static  char *sh_argv[2]; /* Let default to {NULL, NULL} */
	struct  toolsw *emptysw;
	char    name[WIN_NAMESIZE];
	struct sigvec vec;

	argv++;
	argc--;
	/*
	 * Pick up command line arguments to modify tool behavior
	 */
	if (tool_parse_all(&argc, argv, &tool_attrs, tool_name) == -1) {
		(void) tool_usage(tool_name);
		EXIT(1);
	}
	/*
	 * Get ttysw related args
	 */
	while (argc > 0 && **argv == '-') {
		switch (argv[0][1]) {
		case 'C':
			become_console = 1;
			break;
		case '?':
			(void) tool_usage(tool_name);
			(void)
			  fprintf(stderr, "To make the console use -C\n");
			EXIT(1);
		default:
			;
		}
		argv++;
		argc--;
	}
	if (argc == 0) {
		argv = sh_argv;
		if ((argv[0] = getenv("SHELL")) == NULL)
			argv[0] = "/bin/sh";
	}
	/*
	 * Set default icon label
	 */
	if (tool_find_attribute(tool_attrs, (int) WIN_LABEL, &tmp_str)) {
		/* Using tool label supplied on command line */
		(void) strncat(icon_label, tmp_str, sizeof(icon_label));
		(void) tool_free_attribute((int) WIN_LABEL, tmp_str);
	} else if (become_console)
		(void) strncat(icon_label, "CONSOLE", sizeof(icon_label));
	else
		/* Use program name that is run under ttysw */
		(void) strncat(icon_label, argv[0], sizeof(icon_label));
	/*
	 * Buildup tool label
	 */
	(void) strcat(label, label_default);
	if (become_console)
		(void) strcat(label, label_console);
	else
		(void) strcat(label, ": ");
	(void) strncat(label, *argv, sizeof(label)-
	    strlen(label_default)-strlen(label_console)-1);
	/*
	 * Create tool window
	 */
	tool = tool_make(
	    WIN_LABEL,		label,
	    WIN_NAME_STRIPE,	1,
	    WIN_BOUNDARY_MGR,	1,
	    WIN_ICON,		&icon,
	    WIN_ICON_LABEL,	icon_label,
	    WIN_ATTR_LIST,	tool_attrs,
	    0);
	if (tool == (struct tool *)NULL)
		EXIT(1);
	tool_free_attribute_list(tool_attrs);
	/*
	 * Create tty tool subwindow
	 */
	tsw = ttytlsw_createtoolsubwindow(tool, "ttysw", TOOL_SWEXTENDTOEDGE,
	    200);
	if (tsw == (struct toolsw *)NULL)
		EXIT(1);
        /* Create empty subwindow for graphics */
        emptysw = esw_createtoolsubwindow(tool, "emptysw",
            TOOL_SWEXTENDTOEDGE, TOOL_SWEXTENDTOEDGE);
        if (emptysw == (struct toolsw *)NULL)
		EXIT(1);

	/* Install tool in tree of windows */
	/* (void) signal(SIGWINCH, sigwinchcatcher); */
	/* (void) signal(SIGCHLD, sigchldcatcher); */
	/* (void) signal(SIGTERM, sigtermcatcher); */

	vec.sv_handler = sigwinchcatcher;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(SIGWINCH, &vec, 0);

	vec.sv_handler = sigchldcatcher;
	sigvec(SIGCHLD, &vec, 0);

	vec.sv_handler = sigtermcatcher;
	sigvec(SIGTERM, &vec, 0);

	(void) tool_install(tool);
	/* Start tty process */
        (void) win_fdtoname(emptysw->ts_windowfd, name);
        (void) we_setgfxwindow(name);
	if (become_console)
		(void) ttysw_becomeconsole(tsw->ts_data);
	if (ttysw_fork(tsw->ts_data, argv, &tsw->ts_io.tio_inputmask,
	    &tsw->ts_io.tio_outputmask, &tsw->ts_io.tio_exceptmask) == -1) {
		perror(tool_name);
		EXIT(1);
	}
	/* Handle input */
	(void) tool_select(tool, 1);
	/* Cleanup */
	(void) tool_destroy(tool);
	EXIT(0);
}

static void
sigchldcatcher()
{
	(void) tool_sigchld(tool);
}

static void
sigwinchcatcher()
{
	(void) tool_sigwinch(tool);
}

static void
sigtermcatcher()
{
	/* Special case: Do ttysw related cleanup (e.g., /etc/utmp) */
	(void) ttysw_done(tsw->ts_data);
	exit(0);
}
