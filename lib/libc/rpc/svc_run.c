#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)svc_run.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/syslog.h>

void
svc_run()
{
	fd_set readfds;
	int dtbsize = _rpc_dtablesize();
	extern int errno;

	for (;;) {
		readfds = svc_fdset;
		switch (select(dtbsize, &readfds, (fd_set *)0,
			(fd_set *)0, (struct timeval *)0)) {
		case -1:
			/*
			 * We ignore all other errors except EBADF.  For all
			 * other errors, we just continue with the assumption
			 * that it was set by the signal handlers (or any
			 * other outside event) and not caused by select().
			 */
			if (errno != EBADF) {
				continue;
			}
			(void) syslog(LOG_ERR, "svc_run: - select failed: %m");
			return;
		case 0:
			continue;
		default:
			svc_getreqset(&readfds);
		}
	}
}
