#if  !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)sigvec.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <signal.h>
#include <syscall.h>
#include <errno.h>

void	(*_sigfunc[NSIG])();
extern	void _sigtramp();
extern	int errno;

sigvec(sig, vec, ovec)
	int sig;
	register struct sigvec *vec, *ovec;
{
	struct sigvec avec;
	register void (*osig)(), (*nsig)();
	register int omask, saved_errno;

	if (sig <= 0 || sig >= NSIG) {
		errno = EINVAL;
		return (-1);
	}

	omask = sigblock(sigmask(sig));
	osig = _sigfunc[sig];
	if (_sigvec(sig, vec, ovec) < 0)
		goto error;

	if (vec) {
		avec = *vec;
		vec = &avec;
		if ((nsig = vec->sv_handler) != SIG_DFL && nsig != SIG_IGN) {
			_sigfunc[sig] = nsig;
			vec->sv_handler = _sigtramp;
		}
		if (_sigvec(sig, vec, (struct sigvec *)0) < 0)
			goto error;
	}
	if (ovec && ovec->sv_handler == _sigtramp)
		ovec->sv_handler = osig;
	(void)sigsetmask(omask);
	return (0);

error:
	saved_errno = errno;
	(void)sigsetmask(omask);
	_sigfunc[sig] = osig;
	errno = saved_errno;
	return (-1);
}
