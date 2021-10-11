#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)chroot.c 1.1 92/07/30 SMI";
#endif

/*
 * XPG2 VSX2.502 does not want a NULL pointer to fail. This
 * replaces the default libc version
 */
#include <sys/syscall.h>
#include <sys/errno.h>

extern int      syscall();
extern int	errno;

chroot(d)
    char           *d;
{
	if (d == 0) {
		errno = EFAULT; 
		return -1;
	}
	return syscall(SYS_chroot, d);
}
