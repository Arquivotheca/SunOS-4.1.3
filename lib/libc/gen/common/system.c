#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)system.c 1.1 92/07/30 SMI"; /* from S5R2 1.2 */
#endif

/*LINTLIBRARY*/
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

extern int execl();

int
system(s)
char	*s;
{
	int	status;
	pid_t	pid, w;
	register void (*istat)(), (*qstat)();

	if ((pid = vfork()) == 0) {
		(void) execl("/bin/sh", "sh", "-c", s, (char *)0);
		_exit(127);
	}
	if (pid == -1) {
		return (-1);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	w = waitpid(pid, &status, 0);
	(void) signal(SIGINT, istat);
	(void) signal(SIGQUIT, qstat);
	return ((w == -1) ? -1: status);
}
