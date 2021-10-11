#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)stty.c 1.1 92/07/30 SMI"; /* from UCB 4.2 83/07/04 */
#endif

/*
 * Writearound to old stty system call.
 */

#include <sgtty.h>

stty(fd, ap)
	struct sgttyb *ap;
{

	return(ioctl(fd, TIOCSETP, ap));
}
