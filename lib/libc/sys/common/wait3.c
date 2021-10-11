#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)wait3.c 1.1 92/07/30 Copyright (C) 1986 Sun Microsystems, Inc";
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <syscall.h>

wait3(status, options, rusage)
	union wait *status;
	int options;
	struct rusage *rusage;
{
	return (wait4(0, status, options, rusage));
}
