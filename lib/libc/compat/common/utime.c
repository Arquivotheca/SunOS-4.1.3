#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)utime.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/05/31 */
#endif

#include <sys/types.h>
#include <sys/time.h>

extern long time();

/*
 * Backwards compatible utime.
 *
 * The System V system call allows any user with write permission
 * on a file to set the accessed and modified times to the current
 * time; they specify this by passing a null pointer to "utime".
 * This is done to simulate reading one byte from a file and
 * overwriting that byte with itself, which is the technique used
 * by older versions of the "touch" command.  The advantage of this
 * hack in the system call is that it works correctly even if the file
 * is zero-length.
 *
 * The BSD system call never allowed a null pointer so there should
 * be no compatibility problem there.
 */
utime(name, otv)
	char *name;
	time_t otv[2];
{
	struct timeval tv[2];

	if (otv == 0) {
		return (utimes(name, (struct timeval *)0));
	} else {
		tv[0].tv_sec = (long)otv[0];
		tv[0].tv_usec = 0;
		tv[1].tv_sec = (long)otv[1];
		tv[1].tv_usec = 0;
	}
	return (utimes(name, tv));
}
