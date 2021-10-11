#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)jcsetpgrp.c 1.1 92/07/30 SMI";
#endif

#include <sys/syscall.h>

/*
 * POSIX call to set job control process group of current process.
 * Use 4BSD "setpgrp" call, but don't call "setpgrp" since that may refer
 * to SVID "setpgrp" call in System V environment.
 */
int
jcsetpgrp(pgrp)
	int pgrp;
{
	return (syscall(SYS_setpgrp, 0, pgrp));
}
