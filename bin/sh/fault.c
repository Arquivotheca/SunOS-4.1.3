/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)fault.c 1.1 92/07/30 SMI"; /* from S5R3 1.8 */
#endif

/*
 * UNIX shell
 */

#include	"defs.h"

extern void	done();

static void	fault();

char	*trapcom[MAXTRAP];
BOOL	trapjmp;
BOOL	trapflg[MAXTRAP] =
{
	0,
	0,	/* hangup */
	0,	/* interrupt */
	0,	/* quit */
	0,	/* illegal instr */
	0,	/* trace trap */
	0,	/* IOT */
	0,	/* EMT */
	0,	/* float pt. exp */
	0,	/* kill */
	0, 	/* bus error */
	0,	/* memory faults */
	0,	/* bad sys call */
	0,	/* bad pipe call */
	0,	/* alarm */
	0, 	/* software termination */
	0,	/* urgent condition on I/O channel */
	0,	/* stop not from tty */
	0,	/* stop from tty */
	0,	/* continue */
	0,	/* status change of child */
	0,	/* stop from background tty read */
	0,	/* stop from background tty write */
	0,	/* I/O possible */
	0,	/* exceeded CPU time limit */
	0,	/* exceeded file size limit */
	0,	/* virtual time alarm */
	0,	/* profiling time alarm */
	0,	/* window changed */
	0,	/* resource lost */
	0,	/* user-defined signal 1 */
	0,	/* user-defined signal 2 */
};

void 	(*(sigval[MAXTRAP]))() = 
{
	SIG_DFL,
	done,		/* hangup */
	fault,		/* interrupt */
	fault,		/* quit */
	done,		/* illegal instr */
	done,		/* trace trap */
	done,		/* IOT */
	done,		/* EMT */
	done,		/* float pt. exp */
	SIG_DFL,	/* kill */
	done,		/* bus error */
	done,		/* memory faults */
	done,		/* bad sys call */
	done,		/* bad pipe call */
	fault,		/* alarm */
	fault,		/* software termination */
	SIG_DFL,	/* urgent condition on I/O channel */
	done,		/* stop not from tty */
	done,		/* stop from tty */
	done,		/* continue */
	SIG_DFL,	/* status change of child */
	done,		/* stop from background tty read */
	done,		/* stop from background tty write */
	SIG_DFL,	/* I/O possible */
	done,		/* exceeded CPU time limit */
	done,		/* exceeded file size limit */
	done,		/* virtual time alarm */
	done,		/* profiling time alarm */
	SIG_DFL,	/* window changed */
	done,		/* resource lost */
	done,		/* user-defined signal 1 */
	done,		/* user-defined signal 2 */
};

/* ========	fault handling routines	   ======== */


static void
fault(sig)
register int	sig;
{
	register int	flag;

	signal(sig, fault);
	if (sig == SIGALRM)
	{
		if (flags & waiting)
			done();
	}
	else
	{
		flag = (trapcom[sig] ? TRAPSET : SIGSET);
		trapnote |= flag;
		trapflg[sig] |= flag;
		if (sig == SIGINT)
		{
			wasintr++;
			if (trapjmp)
			{
				trapjmp = 0;
				longjmp(INTbuf, 1);
			}
		}
	}
}

stdsigs()
{
	setsig(SIGHUP);
	setsig(SIGINT);
	ignsig(SIGQUIT);
	setsig(SIGILL);
	setsig(SIGTRAP);
	setsig(SIGIOT);
	setsig(SIGEMT);
	setsig(SIGFPE);
	setsig(SIGBUS);
	setsig(SIGSYS);
	setsig(SIGPIPE);
	setsig(SIGALRM);
	setsig(SIGTERM);
	setsig(SIGUSR1);
	setsig(SIGUSR2);
}

ignsig(n)
{
	register int	s, i;

	if ((s = (signal(i = n, SIG_IGN) == SIG_IGN)) == 0)
	{
		trapflg[i] |= SIGMOD;
	}
	return(s);
}

getsig(n)
{
	register int	i;

	if (trapflg[i = n] & SIGMOD || ignsig(i) == 0)
		signal(i, fault);
}


setsig(n)
{
	register int	i;

	if (ignsig(i = n) == 0)
		signal(i, sigval[i]);
}

oldsigs()
{
	register int	i;
	register char	*t;

	i = MAXTRAP;
	while (i--)
	{
		t = trapcom[i];
		if (t == 0 || *t)
			clrsig(i);
		trapflg[i] = 0;
	}
	trapnote = 0;
}

clrsig(i)
int	i;
{
	free(trapcom[i]);
	trapcom[i] = 0;
	if (trapflg[i] & SIGMOD)
	{
		trapflg[i] &= ~SIGMOD;
		signal(i, sigval[i]);
	}
}

/*
 * check for traps
 */
chktrap()
{
	register int	i = MAXTRAP;
	register char	*t;

	trapnote &= ~TRAPSET;
	while (--i)
	{
		if (trapflg[i] & TRAPSET)
		{
			trapflg[i] &= ~TRAPSET;
			if (t = trapcom[i])
			{
				int	savxit = exitval;

				execexp(t, 0);
				exitval = savxit;
				exitset();
			}
		}
	}
}
