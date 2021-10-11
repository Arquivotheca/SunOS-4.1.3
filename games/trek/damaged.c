#ifndef lint
static	char sccsid[] = "@(#)damaged.c 1.1 92/07/30 SMI"; /* from UCB 4.1 03/23/83 */
#endif

# include	"trek.h"

/*  DAMAGED -- check for device damaged
**
**	This is a boolean function which returns non-zero if the
**	specified device is broken.  It does this by checking the
**	event list for a "device fix" action on that device.
*/

damaged(dev)
int	dev;
{
	register int		d;
	register struct event	*e;
	register int		i;

	d = dev;

	for (i = 0; i < MAXEVENTS; i++)
	{
		e = &Event[i];
		if (e->evcode != E_FIXDV)
			continue;
		if (e->systemname == d)
			return (1);
	}

	/* device fix not in event list -- device must not be broken */
	return (0);
}
