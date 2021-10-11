/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.sem.c 1.1 92/07/30 SMI; from UCB 5.4 5/13/86";
#endif

#include "sh.h"
#include "sh.proc.h"
#include <sys/ioctl.h>
#include "sh.tconst.h"
#ifdef TRACE
# include <stdio.h>
#endif

/*
 * C shell
 */

/*VARARGS 1*/
execute(t, wanttty, pipein, pipeout)
	register struct command *t;
	int wanttty, *pipein, *pipeout;
{
	bool forked = 0;
	struct biltins *bifunc;
	int pid = 0;
	int pv[2];
#ifdef TRACE
	tprintf("TRACE- execute()\n");
#endif

	if (t == 0)
		return;
	if ((t->t_dflg & FAND) && wanttty > 0)
		wanttty = 0;
	switch (t->t_dtyp) {

	case TCOM:
		if (t->t_dcom[0][0] == (tchar)S_TOPBIT[0])
			(void) strcpy_(t->t_dcom[0], t->t_dcom[0] + 1);
		if ((t->t_dflg & FREDO) == 0)
			Dfix(t);		/* $ " ' \ */
		if (t->t_dcom[0] == 0)
			return;
		/* fall into... */

	case TPAR:
		if (t->t_dflg & FPOU)
			mypipe(pipeout);
		/*
		 * Must do << early so parent will know
		 * where input pointer should be.
		 * If noexec then this is all we do.
		 */
		if (t->t_dflg & FHERE) {
			(void) close(0);
			heredoc(t->t_dlef);
			if (noexec)
				(void) close(0);
		}
		if (noexec)
			break;

		set(S_status, S_0);

		/*
		 * This mess is the necessary kludge to handle the prefix
		 * builtins: nice, nohup, time.  These commands can also
		 * be used by themselves, and this is not handled here.
		 * This will also work when loops are parsed.
		 */
		while (t->t_dtyp == TCOM)
			if (eq(t->t_dcom[0], S_nice /*"nice"*/))
				if (t->t_dcom[1])
					/*if (any(t->t_dcom[1][0], "+-"))*/
					if (t->t_dcom[1][0] == '+' ||
					    t->t_dcom[1][0] == '-')
						if (t->t_dcom[2]) {
							setname(S_nice /*"nice"*/);
							t->t_nice = getn(t->t_dcom[1]);
							lshift(t->t_dcom, 2);
							t->t_dflg |= FNICE;
						} else
							break;
					else {
						t->t_nice = 4;
						lshift(t->t_dcom, 1);
						t->t_dflg |= FNICE;
					}
				else
					break;
			else if (eq(t->t_dcom[0], S_nohup /*"nohup"*/))
				if (t->t_dcom[1]) {
					t->t_dflg |= FNOHUP;
					lshift(t->t_dcom, 1);
				} else
					break;
			else if (eq(t->t_dcom[0], S_time /*"time"*/))
				if (t->t_dcom[1]) {
					t->t_dflg |= FTIME;
					lshift(t->t_dcom, 1);
				} else
					break;
			else
				break;
		/*
		 * Check if we have a builtin function and remember which one.
		 */
		bifunc = t->t_dtyp == TCOM ? isbfunc(t) : (struct biltins *) 0;

		/*
		 * We fork only if we are timed, or are not the end of
		 * a parenthesized list and not a simple builtin function.
		 * Simple meaning one that is not pipedout, niced, nohupped,
		 * or &'d.
		 * It would be nice(?) to not fork in some of these cases.
		 */
		if (((t->t_dflg & FTIME) || (t->t_dflg & FPAR) == 0 &&
		     (!bifunc || t->t_dflg & (FPOU|FAND|FNICE|FNOHUP))))
#ifdef VFORK
		    if (t->t_dtyp == TPAR || t->t_dflg&(FREDO|FAND) || bifunc)
#endif
			{ forked++; pid = pfork(t, wanttty); }
#ifdef VFORK
		    else {
			void vffree();
			struct sv {
				int mask, child, setintr, haderr, didfds;
				int SHIN, SHOUT, SHDIAG, OLDSTD, tpgrp;
				struct sigvec sigv;
			} sv;

			/* 
			 * Prepare for the vfork by saving everything
			 * that the child corrupts before it exec's.
			 * Note that in some signal implementations
			 * which keep the signal info in user space
			 * (e.g. Sun's) it will also be necessary to
 			 * save and restore the current sigvec's for
			 * the signals the child touches before it
			 * exec's.
			 */
			sv.mask = sigblock(sigmask(SIGCHLD));
			sv.child = child; sv.setintr = setintr;
			sv.haderr = haderr; sv.didfds = didfds;
			sv.SHIN = SHIN; sv.SHOUT = SHOUT;
			sv.SHDIAG = SHDIAG; sv.OLDSTD = OLDSTD;
			sv.tpgrp = tpgrp;
			Vsav = Vdp = 0; Vav = 0;
			(void) sigvec(SIGINT, (struct sigvec *)0, &sv.sigv);
			pid = vfork();
			if (pid < 0) {
				(void) sigsetmask(sv.mask);
				error("Vfork failed");
			}
			forked++;
			if (pid) {	/* parent */
				child = sv.child; setintr = sv.setintr;
				haderr = sv.haderr; didfds = sv.didfds;
				SHIN = sv.SHIN;
				SHOUT = sv.SHOUT; SHDIAG = sv.SHDIAG;
				OLDSTD = sv.OLDSTD; tpgrp = sv.tpgrp;
				xfree(Vsav); Vsav = 0;
				xfree(Vdp); Vdp = 0;
				xfree( (tchar *)Vav); Vav = 0;
				/* this is from pfork() */
				palloc(pid, t);
				/*
				 * Restore SIGINT handler.
				 */
				(void) sigvec(SIGINT, &sv.sigv, (struct sigvec *)0);
				(void) sigsetmask(sv.mask);
			} else {	/* child */
				/* this is from pfork() */
				int pgrp;
				bool ignint = 0;

				if (setintr)
					ignint =
					    (tpgrp == -1 && (t->t_dflg&FINT))
					    || gointr 
						&& eq(gointr, S_MINUS/*"-"*/);
				pgrp = pcurrjob ? pcurrjob->p_jobid : getpid();
				child++;
				if (setintr) {
					setintr = 0;
#ifdef notdef
					(void) signal(SIGCHLD, SIG_DFL);
#endif
					(void) signal(SIGINT, ignint ?
						SIG_IGN : vffree);
					(void) signal(SIGQUIT, ignint ?
						SIG_IGN : SIG_DFL);
					if (wanttty >= 0) {
						(void) signal(SIGTSTP, SIG_DFL);
						(void) signal(SIGTTIN, SIG_DFL);
						(void) signal(SIGTTOU, SIG_DFL);
					}
					(void) signal(SIGTERM, parterm);
				} else if (tpgrp == -1 && (t->t_dflg&FINT)) {
					(void) signal(SIGINT, SIG_IGN);
					(void) signal(SIGQUIT, SIG_IGN);
				}
				if (wanttty > 0)
					(void) ioctl(FSHTTY, TIOCSPGRP,
						 (tchar *)&pgrp);
				if (wanttty >= 0 && tpgrp >= 0)
					(void) setpgrp(0, pgrp);
				if (tpgrp > 0)
					tpgrp = 0;
				if (t->t_dflg & FNOHUP)
					(void) signal(SIGHUP, SIG_IGN);
				if (t->t_dflg & FNICE)
					(void) setpriority(PRIO_PROCESS,
						0, t->t_nice);
			}

		    }
#endif
		if (pid != 0) {
			/*
			 * It would be better if we could wait for the
			 * whole job when we knew the last process
			 * had been started.  Pwait, in fact, does
			 * wait for the whole job anyway, but this test
			 * doesn't really express our intentions.
			 */
			if (didfds==0 && t->t_dflg&FPIN) {
				(void) close(pipein[0]);
				(void) close(pipein[1]);
			}
			if ((t->t_dflg & (FPOU|FAND)) == 0)
				pwait();
			break;
		}

		doio(t, pipein, pipeout);

		if (t->t_dflg & FPOU) {
			(void) close(pipeout[0]);
			(void) close(pipeout[1]);
		}
#ifdef TRACE
		{
			extern	FILE * trace;

			if (trace != NULL)	/* -T 	trace switch on */
			  {
				trace_init();
			  }
		}
#endif
		/*
		 * Perform a builtin function.
		 * If we are not forked, arrange for possible stopping
		 */
		if (bifunc) {
			func(t, bifunc);
			if (forked)
				exitstat();
			break;
		}
		if (t->t_dtyp != TPAR) {
			doexec(t);
			/*NOTREACHED*/
		}
		/*
		 * For () commands must put new 0,1,2 in FSH* and recurse
		 */
		OLDSTD = dcopy(0, FOLDSTD);
		SHOUT = dcopy(1, FSHOUT);
		SHDIAG = dcopy(2, FSHDIAG);
		(void) close(SHIN);
		SHIN = -1;
		didfds = 0;
		wanttty = -1;
		t->t_dspr->t_dflg |= t->t_dflg & FINT;
		execute(t->t_dspr, wanttty);
		exitstat();

	case TFIL:
		t->t_dcar->t_dflg |= FPOU |
		    (t->t_dflg & (FPIN|FAND|FDIAG|FINT));
		execute(t->t_dcar, wanttty, pipein, pv);
		t->t_dcdr->t_dflg |= FPIN |
		    (t->t_dflg & (FPOU|FAND|FPAR|FINT));
		if (wanttty > 0)
			wanttty = 0;		/* got tty already */
		execute(t->t_dcdr, wanttty, pv, pipeout);
		break;

	case TLST:
		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			/*
			 * In strange case of A&B make a new job after A
			 */
			if (t->t_dcar->t_dflg&FAND && t->t_dcdr &&
			    (t->t_dcdr->t_dflg&FAND) == 0)
				pendjob();
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;

	case TOR:
	case TAND:
		if (t->t_dcar) {
			t->t_dcar->t_dflg |= t->t_dflg & FINT;
			execute(t->t_dcar, wanttty);
			if ((getn(value(S_status/*"status"*/)) == 0) != (t->t_dtyp == TAND))
				return;
		}
		if (t->t_dcdr) {
			t->t_dcdr->t_dflg |= t->t_dflg & (FPAR|FINT);
			execute(t->t_dcdr, wanttty);
		}
		break;
	}
	/*
	 * Fall through for all breaks from switch
	 *
	 * If there will be no more executions of this
	 * command, flush all file descriptors.
	 * Places that turn on the FREDO bit are responsible
	 * for doing donefds after the last re-execution
	 */
	if (didfds && !(t->t_dflg & FREDO))
		donefds();
}

#ifdef VFORK
void
vffree()
{
	register tchar **v;

#ifdef TRACE
	tprintf("TRACE- vffree()\n");
#endif
	if (v = gargv)
		gargv = 0, xfree( (tchar *)v);
	if (v = pargv)
		pargv = 0, xfree( (tchar *)v);
	_exit(1);
}
#endif

/*
 * Perform io redirection.
 * We may or maynot be forked here.
 */
doio(t, pipein, pipeout)
	register struct command *t;
	int *pipein, *pipeout;
{
	register tchar *cp, *dp;
	register int flags = t->t_dflg;

#ifdef TRACE
	tprintf("TRACE- doio()\n");
#endif
	if (didfds || (flags & FREDO))
		return;
	if ((flags & FHERE) == 0) {	/* FHERE already done */
		(void) close(0);
		if (cp = t->t_dlef) {
			dp = Dfix1(cp);
			cp = globone(dp);
			xfree(dp);
			xfree(cp);
			if (open_(cp, 0) < 0)
				Perror(cp);
		} else if (flags & FPIN) {
			(void) dup(pipein[0]);
			(void) close(pipein[0]);
			(void) close(pipein[1]);
		} else if ((flags & FINT) && tpgrp == -1) {
			(void) close(0);
			(void) open("/dev/null", 0);
		} else
			(void) dup(OLDSTD);
	}
	(void) close(1);
	if (cp = t->t_drit) {
		dp = Dfix1(cp);
		cp = globone(dp);
		xfree(dp);
		xfree(cp);
		if ((flags & FCAT) && open_(cp, 1) >= 0)
			(void) lseek(1, (off_t)0, 2);
		else {
			if (!(flags & FANY) && adrof(S_noclobber/*"noclobber"*/)) {
				if (flags & FCAT)
					Perror(cp);
				chkclob(cp);
			}
			if (creat_(cp, 0666) < 0)
				Perror(cp);
		}
	} else if (flags & FPOU)
		(void) dup(pipeout[1]);
	else
		(void) dup(SHOUT);

	(void) close(2);
	if (flags & FDIAG)
		(void) dup(1);
	else
		(void) dup(SHDIAG);
	didfds = 1;
}

mypipe(pv)
	register int *pv;
{

#ifdef TRACE
	tprintf("TRACE- mypipe()\n");
#endif
	if (pipe(pv) < 0)
		goto oops;
	pv[0] = dmove(pv[0], -1);
	pv[1] = dmove(pv[1], -1);
	if (pv[0] >= 0 && pv[1] >= 0)
		return;
oops:
	error("Can't make pipe");
}

chkclob(cp)
	register tchar *cp;
{
	struct stat stb;
	unsigned short	type;

#ifdef TRACE
	tprintf("TRACE- chkclob()\n");
#endif
	if (stat_(cp, &stb) < 0)
		return;
	type = stb.st_mode & S_IFMT;
	if (type == S_IFCHR || type == S_IFIFO)
		return;
	error("%t: File exists", cp);
}
