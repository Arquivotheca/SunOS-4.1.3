#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)tool_select.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * Handle input multiplexing among subwindows and tool.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>
#include <sunwindow/notify.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <sunwindow/cms.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/icon.h>
#include <suntool/tool.h>
#include <suntool/tool_impl.h>
#include <suntool/wmgr.h>

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
	(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

extern	int errno;

/*
 * Select for interesting events.
 */
tool_select(tool, waitprocessesdie)
	struct	tool *tool;
	int	waitprocessesdie;
{
	struct	timeval *_tool_setupselect(), *timer, timerdelta;
	struct	toolsw *sw;
	fd_set	ibitsmask, obitsmask, ebitsmask,
		ibits, obits, ebits;
	int	nfds;
	union	wait status;
	int 	maxfds = GETDTABLESIZE();

	tool->tl_flags &= ~TOOL_DONE;
	do {
		/*
		 * Do this setup in the loop in case change sw's.
		 */
		FD_ZERO(&ibitsmask); FD_ZERO(&obitsmask); FD_ZERO(&ebitsmask);
		timer = (struct timeval *)0;
		timerdelta.tv_sec = timerdelta.tv_usec = 0;
		for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
			timer = _tool_setupselect(&sw->ts_io, sw->ts_windowfd,
			    &ibitsmask, &obitsmask, &ebitsmask, timer);
		}
		timer = _tool_setupselect(&tool->tl_io, tool->tl_windowfd,
		    &ibitsmask, &obitsmask, &ebitsmask, timer);
		ibits = ibitsmask;
		obits = obitsmask;
		ebits = ebitsmask;
		do {
			/*
			 * Gatherup any dead children processes
			 * Note: Should validate sw list here.
			 */
			if ((tool->tl_flags&TOOL_SIGCHLD) && waitprocessesdie) {
				tool->tl_flags &= ~TOOL_SIGCHLD;
				while (wait3(&status, WNOHANG, (struct rusage *)0) > 0) {
					waitprocessesdie--;
					if (waitprocessesdie<=0) {
						return;
					}
				}
			}
			/*
			 * See if recieved a sigwinch
			 */
			if (tool->tl_flags&TOOL_SIGWINCHPENDING &&
			    tool->tl_io.tio_handlesigwinch)
				tool->tl_io.tio_handlesigwinch(tool);
		} while ((tool->tl_flags&TOOL_SIGWINCHPENDING) ||
		    (tool->tl_flags&TOOL_SIGCHLD));
		if (!ntfy_fd_anyset(&ibits) && !ntfy_fd_anyset(&obits) && 
		    !ntfy_fd_anyset(&ebits) && timer == 0) {
			/*
			 * Wait for interrupt.
			 */
			pause();
			continue;
		} else {
			/*
			 * Select for input.
			 */
			nfds = select(
			    maxfds, &ibits, &obits, &ebits, timer);
			if (nfds==-1) {
				if (errno == EINTR)
					/*
					 * Go around again so that signals can
					 * be handled.  ibits may be non-zero
					 * but should be ignored in this case
					 * and they will be selected again.
					 */
					continue;
				else {
					perror(tool->tl_name);
					break;
				}
			}
			if (timer && (nfds == 0))
				/*
				 * Timed out.
				 */
				timerdelta = *timer;
			else
				/*
				 * Didn't time out, assume happened immediately.
				 */
				timerclear(&timerdelta);
		}
		/*
		 * Distribute input notifications.
		 */
		/*
		 * Note: Should serialize input so get correct ordering.
		 * Do this by reading from all the windows that have input
		 * and sending off the oldest event.  Subwindows then
		 * have the option of doing their own reads (kind of a
		 * backdoor method) as long as there is something to read.
		 * Unfortunately, this doesn't solve the general probelm
		 * of input serialization in a multiprocess environment.
		 */
		(void)_tool_dispathselect((caddr_t)(LINT_CAST(tool)), &tool->tl_io,
		    tool->tl_windowfd, ibits, obits, ebits, timerdelta);
		for (sw = tool->tl_sw;sw;sw = sw->ts_next) {
			(void)_tool_dispathselect(sw->ts_data, &sw->ts_io,
			    sw->ts_windowfd, ibits, obits, ebits, timerdelta);
		}
	} while (~tool->tl_flags & TOOL_DONE);
	return;
}

/*
 * Set flag indicated SIGCHLD was recieved.
 */
tool_sigchld(tool)
struct	tool *tool;
{
	tool->tl_flags |= TOOL_SIGCHLD;
}

/*
 * Set flag indicated SIGWINCH was recieved.
 */
tool_sigwinch(tool)
	struct	tool *tool;
{
	tool->tl_flags |= TOOL_SIGWINCHPENDING;
}

struct	timeval *
_tool_setupselect(tio, windowfd, ibitsmask, obitsmask, ebitsmask, timer)
	register struct	toolio *tio;
	int	windowfd;
	fd_set *ibitsmask, *obitsmask, *ebitsmask;
	struct	timeval *timer;
{
	if (tio->tio_selected) {
		ntfy_fd_cpy_or(ibitsmask, &tio->tio_inputmask);
		ntfy_fd_cpy_or(obitsmask, &tio->tio_outputmask);
		ntfy_fd_cpy_or(ebitsmask, &tio->tio_exceptmask);
		/*
		 * If no fds in masks specified then always
		 * select for input on the window.
		 * Otherwise, assume that window wants
		 * complete select control.
		 */
		/* if (!(tio->tio_inputmask | tio->tio_outputmask |
		    tio->tio_exceptmask) && tio->tio_selected)
		*/
		if (!ntfy_fd_anyset(&tio->tio_inputmask) &&
		    !ntfy_fd_anyset(&tio->tio_outputmask) &&
		    !ntfy_fd_anyset(&tio->tio_exceptmask) && tio->tio_selected)
			FD_SET(windowfd, ibitsmask);	
		/*
		 * Find shortest timeout
		 */
		if (!timer && tio->tio_timer)
			return(tio->tio_timer);
		if (timer && tio->tio_timer &&
		   (tio->tio_timer->tv_sec < timer->tv_sec ||
		   (tio->tio_timer->tv_sec == timer->tv_sec &&
		    tio->tio_timer->tv_usec < timer->tv_usec)))
			return(tio->tio_timer);
	}
	return(timer);
}

struct	timeval
timersubtract(time, timedelta)
	struct	timeval time, timedelta;
{
	if ((time.tv_sec < timedelta.tv_sec) ||
	    (time.tv_sec == timedelta.tv_sec &&
	     time.tv_usec < timedelta.tv_usec)) {
		timerclear(&time);
	} else {
		if (time.tv_usec < timedelta.tv_usec) {
			time.tv_usec += 1000000;
			time.tv_sec--;
		}
		time.tv_usec -= timedelta.tv_usec;
		time.tv_sec -= timedelta.tv_sec;
	}
	return(time);
}

_tool_dispathselect(data, tio, windowfd, ibits, obits, ebits, timedelta)
	caddr_t	data;
	register struct	toolio *tio;
	int	windowfd;
	fd_set  ibits, obits, ebits;
	struct	timeval timedelta;
{
	if (tio->tio_timer)
		*tio->tio_timer = timersubtract(*tio->tio_timer, timedelta);
	/*
	    if (ibits & (1<<windowfd) || ibits & tio->tio_inputmask ||
	    obits & tio->tio_outputmask || ebits & tio->tio_exceptmask || 
	*/
	if (FD_ISSET(windowfd, &ibits) ||
	    ntfy_fd_cmp_and(&ibits, &tio->tio_inputmask) ||
	    ntfy_fd_cmp_and(&obits, &tio->tio_outputmask) ||
	    ntfy_fd_cmp_and(&ebits, &tio->tio_exceptmask) ||
	    (tio->tio_timer && tio->tio_timer->tv_sec == 0 &&
	     tio->tio_timer->tv_usec == 0)) {
		tio->tio_inputmask = ibits;
		tio->tio_outputmask = obits;
		tio->tio_exceptmask = ebits;
		tio->tio_selected(data, &tio->tio_inputmask,
		    &tio->tio_outputmask, &tio->tio_exceptmask,
		    &tio->tio_timer);
	}
}

