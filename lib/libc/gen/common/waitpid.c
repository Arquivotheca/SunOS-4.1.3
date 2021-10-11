#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)waitpid.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif


#include <sys/types.h>
#include <sys/wait.h>

/*
 * posix meanings for pid
 *	-1	any process
 *	> 0	that particular process
 *	0	any process in my pgrp
 *	< -1	any process in pgrp |pid|
 *
 * wait4 meanings for pid
 *	0	any process
 *	> 0	that process
 *	< 0	any process in that process group
 */
pid_t waitpid(pid, stat_loc, options)
    pid_t pid;
    int *stat_loc;
    int options;
{
    if (pid == -1)
	pid = 0;
    else if (!pid)
	pid = -getpgrp(0);
    return wait4(pid, stat_loc, options, (struct rusage*)0);
}
