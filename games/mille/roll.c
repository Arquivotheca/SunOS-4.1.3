#ifndef lint
static	char sccsid[] = "@(#)roll.c 1.1 92/07/30 SMI"; /* from UCB 1.1 04/01/82 */
#endif

# include	"mille.h"

/*
 *	This routine rolls ndie nside-sided dice.
 */

roll(ndie, nsides)
reg int	ndie, nsides; {

	reg int			tot;
	extern unsigned int	random();

	tot = 0;
	while (ndie--)
		tot += random() % nsides + 1;
	return tot;
}
