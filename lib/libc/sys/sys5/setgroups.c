/*
 * POSIX.1 compatible setgroups() routine
 * This is needed while gid_t is not the same size as int (or whatever the
 * syscall is using at the time).
 */

#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)setgroups.c 1.1 92/07/30 SMI";
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syscall.h>

setgroups(ngroups, grouplist)
int	ngroups;
gid_t	grouplist[];
{
	int	glist[NGROUPS];	/* setgroups() syscall expects ints */
	register int	i;	/* loop control */

	if (ngroups > NGROUPS) {
		errno = EINVAL;
		return (-1);
	}
	for (i = 0; i < ngroups; i++)
		glist[i] = (int)grouplist[i];
	return (syscall(SYS_setgroups, ngroups, glist));
}
