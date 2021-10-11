#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)wmgr_menu.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Window mgr menu handling.
 */

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <strings.h>
#include <vfork.h>
#include <sunwindow/defaults.h>
#include <suntool/alert.h>
#include <suntool/frame.h>
#include <suntool/tool_hs.h>

extern	int (*win_errorhandler())();
extern  void wmgr_changestate(), wmgr_changerect(),wmgr_changelevelonly(),
		wmgr_refreshwindow();
extern	int errno;

static	int wmgr_error();
static	int (*wmgr_errorcached)();

void
wmgr_open(toolfd, rootfd)
	int	toolfd, rootfd;
{
	void wmgr_set_tool_level();
	
	(void)win_lockdata(toolfd);
	wmgr_changestate(toolfd, rootfd, FALSE);
	wmgr_set_tool_level(rootfd, toolfd);
	(void)win_unlockdata(toolfd);
}

void
wmgr_close(toolfd, rootfd)
	int	toolfd, rootfd;
{
	void wmgr_set_icon_level();
	
	(void)win_lockdata(toolfd);
	wmgr_changestate(toolfd, rootfd, TRUE);
	wmgr_set_icon_level(rootfd, toolfd);
	(void)win_unlockdata(toolfd);
}

void
wmgr_move(toolfd)
	int	toolfd;
{
	struct	inputevent event;

	wmgr_changerect(toolfd, toolfd, &event, TRUE, FALSE);
}

void
wmgr_stretch(toolfd)
	int	toolfd;
{
	struct	inputevent event;

	wmgr_changerect(toolfd, toolfd, &event, FALSE, FALSE);
}

void
wmgr_top(toolfd, rootfd)
	int	toolfd, rootfd;
{
	wmgr_changelevelonly(toolfd, rootfd, TRUE);
}

void
wmgr_bottom(toolfd, rootfd)
	int	toolfd, rootfd;
{	
	wmgr_changelevelonly(toolfd, rootfd, FALSE);
}

#define	ARGS_MAX	100

wmgr_forktool(programname, otherargs, rectnormal, recticon, iconic)
	char	*programname, *otherargs;
	struct	rect *rectnormal, *recticon;
	int	iconic;
{
	int	pid, prid;
	char	*args[ARGS_MAX];
 	char	*otherargs_copy;
 	extern	char *calloc();

	(void)we_setinitdata(rectnormal, recticon, iconic);
 	/*
 	 * Copy otherargs because using vfork and don't want to modify
 	 * otherargs that is passed in.
 	 */
 	if (otherargs) {
 		if ((otherargs_copy = calloc(1, (unsigned) (LINT_CAST(
			strlen(otherargs)+1)))) == NULL) {
 			perror("calloc");
 			return(-1);
 		}
 		(void)strcpy(otherargs_copy, otherargs);
 	} else
 		otherargs_copy = NULL;
 	pid = vfork();

	if (pid < 0) {
		perror("fork");
		return(-1);
	}
 	if (pid) {
 		if (otherargs)
 			free(otherargs_copy);
		return(pid);
	}
	prid = getpid();
	setpgrp(prid, prid);
	/*
	 * Could nice(2) here so that window manager has higher priority
	 * but this also has the affect of making some of the deamons higher
	 * priority.  This can be a problem because when they startup they
	 * preempt the user.
	 */
	/*
	 * Separate otherargs into args
	 */
	(void) constructargs(args, programname, otherargs_copy, ARGS_MAX);
	execvp(programname, args);
	perror(programname);
	_exit(1);
	/*NOTREACHED*/
}

int
constructargs(args, programname, otherargs, maxargcount)
	char	*args[], *programname, *otherargs;
	int	maxargcount;
{
#define	terminatearg() {*cpt = NULL;needargstart = 1;}
#define	STRINGQUOTE	'"'
	int	argindex = 0, needargstart = 1, quotedstring = 0;
	register char *cpt;

	args[argindex++] = programname;
	for (cpt = otherargs;(cpt != 0) && (*cpt != NULL);cpt++) {
		if (quotedstring) {
			if (*cpt == STRINGQUOTE) {
				terminatearg();
				quotedstring = 0;
			} else {/* Accept char in arg */}
		} else if (isspace(*cpt)) {
			terminatearg();
		} else {
			if (needargstart && (argindex < maxargcount)) {
				args[argindex++] = cpt;
				needargstart = 0;
			}
			if (*cpt == STRINGQUOTE) {
				/*
				 * Advance cpt in current arg
				 */
				args[argindex-1] = cpt+1;
				quotedstring = 1;
			}
		}
	}
	args[argindex] = '\0';
	return(argindex);
}

/*
 * Error handling
 */
static
wmgr_error(errnum, winopnum)
	int	errnum, winopnum;
{
	switch (errnum) {
	case 0:
		return;
	case -1:
		/*
		 * Probably an ioctl err (could check winopnum)
		 */
		switch (errno) {
		case ENOENT:
		case ENXIO:
		case EBADF:
		case ENODEV:
			break;
			/*
			 * Tool must have gone away
			 * Note: Should do a longjmp to abort the operation
			 */
		default:
			wmgr_errorcached(errnum, winopnum);
			break;
		}
		return;
	default:
		(void)fprintf(stderr, "Window mgr operation %d produced error %d\n",
		    winopnum, errnum);
	}
}
