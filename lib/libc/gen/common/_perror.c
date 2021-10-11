#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)_perror.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/06/30 */
#endif
/*
 * Print the error indicated
 * in the cerror cell.
 */
#include <sys/types.h>
#include <sys/uio.h>

extern	int errno;
extern	int sys_nerr;
extern	char *sys_errlist[];
extern	int strlen();
extern	int writev();

void
_perror(s)
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
	    (unsigned)errno < sys_nerr ? sys_errlist[errno] : "Unknown error";
	v->iov_len = strlen(v->iov_base);
	v++;
	v->iov_base = "\n";
	v->iov_len = 1;
	writev(2, iov, (v - iov) + 1);
}
