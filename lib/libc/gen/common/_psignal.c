#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)_psignal.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/02/10 */
#endif

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <sys/types.h>
#include <sys/uio.h>
#include <signal.h>

extern	char *sys_siglist[];
extern	int strlen();
extern	int writev();

void
_psignal(sig, s)
	unsigned sig;
	char *s;
{
	struct iovec iov[4];
	register struct iovec *v = iov;

	if (s && *s) {
		v->iov_base = s;
		v->iov_len = strlen(s);
		v++;
		v->iov_base = ": ";
		v->iov_len = 2;
		v++;
	}
	v->iov_base = 
	    (unsigned)sig < NSIG ? sys_siglist[sig] : "Unknown signal";
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = "\n";
	v->iov_len = 1;
	writev(2, iov, (v - iov) + 1);
}
