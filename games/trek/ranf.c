#ifndef lint
static	char sccsid[] = "@(#)ranf.c 1.1 92/07/30 SMI"; /* from UCB 4.2 05/27/83 */
#endif

# include	<stdio.h>

ranf(max)
int	max;
{
	register int	t;

	if (max <= 0)
		return (0);
	t = rand() >> 5;
	return (t % max);
}


double franf()
{
	double		t;
	t = rand() & 077777;
	return (t / 32767.0);
}
