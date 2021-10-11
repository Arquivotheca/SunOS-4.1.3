#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)gfxsw.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Overview: Implement the abstraction defined in gfxsw.h
 */

#include <sys/types.h>
#include <sys/file.h>
#include <signal.h>
#include <stdio.h>
#include <strings.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_util.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_environ.h>
#include <sunwindow/win_screen.h>
#include <suntool/tool.h>
#include <suntool/gfxsw.h>
#include <suntool/gfxswimpl.h>
#include "sunwindow/sv_malloc.h"

int	gfxsw_selected(), gfxsw_handlesigwinch();
void	gfxsw_catchsigwinch(), gfxsw_catchsigtstp(), gfxsw_catchsigcont();

extern	int errno;
static	struct	gfxsubwindow *gfxtakeover;

struct	gfxsubwindow *
gfxsw_init(windowfd, argv)
	int	windowfd;
	char	**argv;
{
	struct	gfxsubwindow *gfx;
	struct	gfx_takeover *gt = (struct gfx_takeover *) 0;
	char	*error_msg = NULL;
	int	retained = 0, oldsigmask;
	struct	screen screen;
	int	parentfd = -1;

	gfx = (struct gfxsubwindow *) (LINT_CAST(
	    sv_calloc(1, sizeof (struct gfxsubwindow))));
	/*
	 * Prevent getting a sigwinch until ready for it.
	 */
	oldsigmask = sigblock(1 << (SIGWINCH-1));
	if (windowfd)
		/*
		 * Not taking over window.
		 */
		gfx->gfx_windowfd = windowfd;
	else {
		/*
		 * Taking over another window.
		 */
		if (gfxtakeover) {
		    error_msg = "Can't take over two windows in same process";
		    errno = 0;
		    goto MsgErrorReturn;
		}
		gt = (struct gfx_takeover *) (LINT_CAST(
		    sv_calloc(1, sizeof (struct gfx_takeover))));
		gt->gt_rootfd = -1;
		/*
		 * Get framebuffer device name that may have been passed in.
		 * (none will mean default fb).
		 */
		(void)win_initscreenfromargv(&screen, argv);
		if (screen.scr_fbname[0]) {
			/*
			 * Try to create new screen to run on.
			 */
			if (gfxsw_newscreen(gt, &screen)) {
				(void)fprintf(stderr,
				    "couldn't create a new screen on %s\n",
				    screen.scr_fbname);
				goto PErrorReturn;
			}
		} else {
			/*
			 * Determine what window to run in.
			 */
			if (we_getgfxwindow(gt->gt_parentname) &&
			    we_getmywindow(gt->gt_parentname) &&
			    we_getparentwindow(gt->gt_parentname) &&
			    gfxsw_newscreen(gt, &screen)) {
				(void)fprintf(stderr,
					"gfxsw: no %s or %s in environment\n",
					"WINDOW_GFX", "WINDOW_ME");
				error_msg = "   Cannot find a frame buffer";
				errno = 0;
				goto MsgErrorReturn;
			}
		}
		/*
		 * Open gt->gt_parentname window
		 */
		if ((parentfd = open(gt->gt_parentname, O_RDONLY, 0)) < 0) {
			(void)fprintf(stderr, "%s would not open\n",
			    gt->gt_parentname);
			goto PErrorReturn;
		}
		/*
		 * Create new window
		 */
		if ((gfx->gfx_windowfd = win_getnewwindow()) < 0) {
			error_msg = "couldn't get a new gfxsw window";
			goto MsgErrorReturn;
		}
		/*
		 * Setup tool struct for passage to tool_select in gfxsw_select
		 * This is kind of a hack.  The tool_select stuff should be
		 * repackaged to accomodate "outside" clients like this.
		 */
		gt->gt_tool = (struct tool *) (LINT_CAST(
			sv_calloc(1, sizeof (struct tool))));
		gt->gt_tool->tl_windowfd = gfx->gfx_windowfd;
		gt->gt_tool->tl_io.tio_handlesigwinch = gfxsw_handlesigwinch;
		gt->gt_tool->tl_io.tio_selected = gfxsw_selected;
	}
	if (argv) {
		char	**args;
		int	nextisreps = 0;

		/*
		 * Get number of repetitions and retained flag from argv
		 */
		gfx->gfx_reps = 200000;
		for (args = argv;*args;args++) {
			if (nextisreps)
				(void) sscanf(*args, "%hd", &gfx->gfx_reps);
			if (strcmp(*args, "-r") == 0)
				retained = 1;
			if (strcmp(*args, "-n") == 0)
				nextisreps = 1;
		}
	}
	if (gt) {
		/*
		 * Set up for SIGWINCH signal.
		 */
		(void)signal_ifdfl(SIGWINCH, gfxsw_catchsigwinch);
		/*
		 * Install as blanket window.
		 */
		if (win_insertblanket(gfx->gfx_windowfd, parentfd)) {
			error_msg = "couldn't insert blanket window";
			goto MsgErrorReturn;
		}
		(void)close(parentfd);
		/*
		 * Set up for job control signals.
		 * (Doing after insert as blanket because change blanket).
		 */
		(void)signal_ifdfl(SIGTSTP, gfxsw_catchsigtstp);
		(void)signal_ifdfl(SIGCONT, gfxsw_catchsigcont);
		/*
		 * Set global to notices this situation as takeover one.
		 */
		gfxtakeover = gfx;
		gfx->gfx_takeoverdata = (caddr_t) gt;
	}
	(void)win_getsize(gfx->gfx_windowfd, &gfx->gfx_rect);
	/*
	 * Setup pixwins
	 */
#ifdef	PRE_IBIS
	if ((gfx->gfx_pixwin = pw_open(gfx->gfx_windowfd)) ==
#else
	if ((gfx->gfx_pixwin = pw_open_monochrome(gfx->gfx_windowfd)) ==
#endif
	    (struct pixwin *)0)
		goto ErrorReturn;
	if (retained)
		(void)gfxsw_getretained(gfx);
	/*
	 * Now ready for SIGWINCH.
	 */
	(void)sigsetmask(oldsigmask);
	return(gfx);

MsgErrorReturn:
	if (error_msg != NULL)
	    (void)fprintf(stderr, "%s\n", error_msg);
PErrorReturn:
	if (errno != 0)
	    perror("gfxsw");
ErrorReturn:
	if (parentfd != -1)
		(void)close(parentfd);
	(void)gfxsw_done(gfx);
	(void)sigsetmask(oldsigmask);
	return(0);
}

gfxsw_newscreen(gt, screen)
	struct	gfx_takeover *gt;
	struct	screen *screen;
{
	/*
	 * The default kbd and ms should be NONE.
	 */
	if (screen->scr_msname[0] == '\0')
		(void)strncpy(screen->scr_msname, "NONE", SCR_NAMESIZE);
	if (screen->scr_kbdname[0] == '\0')
		(void)strncpy(screen->scr_kbdname, "NONE", SCR_NAMESIZE);
	/*
	 * Create root window
	 */
	if ((gt->gt_rootfd = win_screennew(screen)) == -1) {
		perror("gfxsw");
		return(-1);
	}
	(void)win_screenget(gt->gt_rootfd, screen);
	/*
	 * Set up root's name in environment
	 */
	(void)win_fdtoname(gt->gt_rootfd, gt->gt_parentname);
	return(0);
}

gfxsw_handlesigwinch(gfx)
	struct	gfxsubwindow *gfx;
{
	struct	gfx_takeover *gt = (struct gfx_takeover *) 0;

	if (gfxtakeover) {
		/*
		 * In case using this is called with tool handle for gfx
		 */
		gfx = gfxtakeover;
		gt = gfxgettakeover(gfx);
		gt->gt_tool->tl_flags &= ~TOOL_SIGWINCHPENDING;
	}
	/*
	 * Change retained pixrect if size changed.
	 */
	if (gfxisretained(gfx) &&
	    ((gfx->gfx_rect.r_width !=
		gfx->gfx_pixwin->pw_prretained->pr_width) ||
	    (gfx->gfx_rect.r_height !=
		gfx->gfx_pixwin->pw_prretained->pr_height))) {
		/*
		 * Note: Could copy old to new here but sometimes blows up
		 * with an out of swap space condition.  Releasing the old
		 * first reduces the likelyhood of this situation.
		 */
		(void)gfxsw_getretained(gfx);
	}
	gfx->gfx_flags &= ~GFX_DAMAGED;
	/*
	 * Toss damaged area if not retained.  Fix if retained.
	 */
	(void)pw_damaged(gfx->gfx_pixwin);
	if (gfxisretained(gfx))
		(void)pw_repairretained(gfx->gfx_pixwin);
	(void)pw_donedamaged(gfx->gfx_pixwin);
	if (gt && gt->gt_selected && (gfx->gfx_flags & GFX_RESTART)) {
		/*
		 * Give chance to gfxsw_selected to look at GFX_RESTART
		 * so that screen gets fixed before next event actually
		 * happens.
		 */
		FD_ZERO(&gt->gt_tool->tl_io.tio_inputmask);
		FD_ZERO(&gt->gt_tool->tl_io.tio_outputmask);
		FD_ZERO(&gt->gt_tool->tl_io.tio_exceptmask);
		(void)gfxsw_selected(gt->gt_tool, &gt->gt_tool->tl_io.tio_inputmask,
		    &gt->gt_tool->tl_io.tio_outputmask,
		    &gt->gt_tool->tl_io.tio_exceptmask,
		    &gt->gt_tool->tl_io.tio_timer);
	}
}

gfxsw_done(gfx)
	struct	gfxsubwindow *gfx;
{

	if (gfx) {
		struct	gfx_takeover *gt = gfxgettakeover(gfx);

		/*
		 * Reset state of window
		 */
		if (gt) {
			gfxtakeover = (struct gfxsubwindow *) 0;
			/*
			 * Gt only non-zero if gt portion of gfxsw_init was
			 * successful.
			 */
			(void)resetsignal_todfl(SIGTSTP, gfxsw_catchsigtstp);
			(void)resetsignal_todfl(SIGCONT, gfxsw_catchsigcont);
			(void)win_removeblanket(gfx->gfx_windowfd);
			(void)resetsignal_todfl(SIGWINCH, gfxsw_catchsigwinch);
			(void)close(gfx->gfx_windowfd);
			free((char *)(LINT_CAST(gt->gt_tool)));
			if (gfxmadescreen(gt)) (void)close(gt->gt_rootfd);
			free((char *)(LINT_CAST(gt)));
		}
		if (gfx->gfx_pixwin)
			(void)pw_close(gfx->gfx_pixwin);
		free((char *)(LINT_CAST(gfx)));
	}
	return;
}

struct	toolsw *
gfxsw_createtoolsubwindow(tool, name, width, height, argv)
	struct	tool *tool;
	char	*name;
	short	width, height;
	char	**argv;
{
	struct	toolsw *toolsw;

	/*
	 * Create subwindow
	 * Note: Should be in another module so doesn't drag in tools code.
	 */
	toolsw = tool_createsubwindow(tool, name, width, height);
	/*
	 * Setup empty subwindow
	 */
	if ((toolsw->ts_data = (caddr_t) gfxsw_init(toolsw->ts_windowfd, argv))
	    == (caddr_t)0)
		return((struct  toolsw *)0);
	toolsw->ts_io.tio_handlesigwinch = gfxsw_handlesigwinch;
	toolsw->ts_destroy = gfxsw_done;
	return(toolsw);
}

gfxsw_getretained(gfx)
	struct	gfxsubwindow *gfx;
{
	struct	colormapseg cms;

	(void)pw_getcmsdata(gfx->gfx_pixwin, &cms, (int *)0);
	if (gfx->gfx_pixwin->pw_prretained) {
		(void)pr_destroy(gfx->gfx_pixwin->pw_prretained);
		gfx->gfx_pixwin->pw_prretained = (struct pixrect *)0;
	}
	gfx->gfx_pixwin->pw_prretained = mem_create(
	    gfx->gfx_rect.r_width, gfx->gfx_rect.r_height,
	    (cms.cms_size == 2 && gfx->gfx_pixwin->pw_pixrect->pr_depth < 32)
		? 1 : gfx->gfx_pixwin->pw_pixrect->pr_depth);
	if (!gfxisretained(gfx))
		(void)fprintf(stderr, "graphics program: can't open retained win\n");
}

gfxsw_interpretesigwinch(gfx)
	struct	gfxsubwindow *gfx;
{
	struct	rect rect;
	struct	gfx_takeover *gt = gfxgettakeover(gfx);

	/*
	 * Always restart on size change.
	 */
	(void)win_getsize(gfx->gfx_windowfd, &rect);
	if (!rect_equal(&rect, &gfx->gfx_rect)) {
		gfx->gfx_flags |= GFX_RESTART;
		gfx->gfx_rect = rect;
	}
	/*
	 * Tell demo to restart after first signal (from win_setowner),
	 * if not retained.
	 */
	if (gt) {
		if (!gfxisretained(gfx))
			gfx->gfx_flags |= GFX_RESTART;
		(void)tool_sigwinch(gt->gt_tool);
	} else
		if (!gfxisretained(gfx))
			gfx->gfx_flags |= GFX_RESTART;
	gfx->gfx_flags |= GFX_DAMAGED;
}

gfxsw_select(gfx, selected, ibits, obits, ebits, timer)
	struct	gfxsubwindow *gfx;
	int	(*selected)();
	fd_set  ibits, obits, ebits;
	struct	timeval *timer;
{
	struct	gfx_takeover *gt = gfxgettakeover(gfx);

	gt->gt_selected = selected;
	gt->gt_tool->tl_io.tio_inputmask = ibits;
	gt->gt_tool->tl_io.tio_outputmask = obits;
	gt->gt_tool->tl_io.tio_exceptmask = ebits;
	gt->gt_tool->tl_io.tio_timer = timer;
	(void)tool_select(gt->gt_tool, 0);
	gt->gt_selected = 0;
}

/*ARGSUSED*/
gfxsw_selected(tool, ibitsptr, obitsptr, ebitsptr, timer)
	struct	tool *tool;
	fd_set	*ibitsptr, *obitsptr, *ebitsptr;
	struct	timeval **timer;
{
	struct	gfx_takeover *gt = gfxgettakeover(gfxtakeover);

	if (gt->gt_selected)
		gt->gt_selected(gfxtakeover, ibitsptr, obitsptr, ebitsptr, timer);
}

gfxsw_selectdone(gfx)
	struct	gfxsubwindow *gfx;
{
	struct	gfx_takeover *gt = gfxgettakeover(gfx);

	(void)tool_done(gt->gt_tool);
}

void
gfxsw_catchsigwinch()
{
	if (gfxtakeover == (struct gfxsubwindow *)0)
		return;
	(void)gfxsw_interpretesigwinch(gfxtakeover);
}

void
gfxsw_catchsigtstp()
{
	struct	gfx_takeover *gt;

	if (gfxtakeover == (struct gfxsubwindow *)0)
		return;
	gt = gfxgettakeover(gfxtakeover);
	/*
	 * Note: Haven't handled case of getting a TSTOP when outside suntools.
	 * For now, do nothing.  Could set kbd and ms to NONE and then restore
	 * old values on CONT.
	 */
	if (gfxmadescreen(gt))
		return;
	(void)win_removeblanket(gfxtakeover->gfx_windowfd);
	/*
	 * Reset pixwin while stopped.
	 * Note: Assumes client is using pixwin interface, even when
	 * display is locked, so that pixwin mechanism will notice reset
	 * on the next access.  Note, also, that a race condition exists.
	 * If a display access is already in progress then it will complete
	 * after a SIGCONT is sent.
	 */
	(void)pw_reset(gfxtakeover->gfx_pixwin);
	/*
	 * Need to stop self in tracks when get SIGTSTP.
	 */
	(void)kill(getpid(), SIGSTOP);
}

void
gfxsw_catchsigcont()
{
	struct	gfx_takeover *gt;
	int	parentfd;

	if (gfxtakeover == (struct gfxsubwindow *)0)
		return;
	gt = gfxgettakeover(gfxtakeover);
	/*
	 * Open gt->gt_parentname window
	 */
	if ((parentfd = open(gt->gt_parentname, O_RDONLY, 0)) < 0) {
		(void)fprintf(stderr, "%s would not open\n", gt->gt_parentname);
		return;
	}
	if (win_insertblanket(gfxtakeover->gfx_windowfd, parentfd)) {
		(void)fprintf(stderr, "couldn't insert blanket window\n");
		perror("gfxsw");
	}
	(void)close(parentfd);
}

signal_ifdfl(sig, catchernew)
	int	sig;
	void	(*catchernew)();
{
	void	(*catcherold)();
	struct sigvec vec, ovec;

/*        if ((catcherold = signal(sig, catchernew)) != SIG_DFL)
 *              (void) signal(sig, catcherold);
 */
	vec.sv_handler = catchernew;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(sig, &vec, &ovec);
	if (ovec.sv_handler != SIG_DFL)
		sigvec(sig, &ovec, 0);

}

resetsignal_todfl(sig, catchermine)
	int	sig;
	void	(*catchermine)();
{
	void	(*catcherold)();
	struct sigvec vec, ovec;

/*	if ((catcherold = signal(sig, SIG_DFL)) != catchermine)
 *		(void) signal(sig, catcherold);
 */
	vec.sv_handler = SIG_DFL;
	vec.sv_mask = vec.sv_onstack = 0;
	sigvec(sig, &vec, &ovec);
	if (ovec.sv_handler != catchermine)
		sigvec(sig, &ovec, 0);
}
