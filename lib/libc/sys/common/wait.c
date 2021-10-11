#if	!defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)wait.c 1.1 92/07/30 Copyright (C) 1986 Sun Microsystems, Inc";
#endif

#include <sys/wait.h>

pid_t
wait(status)
	union wait *status;
{
	return ((pid_t)wait4(0, status, 0, (struct rusage *)0));
}
