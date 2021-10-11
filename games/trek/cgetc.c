#ifndef lint
static	char sccsid[] = "@(#)cgetc.c 1.1 92/07/30 SMI"; /* from UCB 1.1 05/27/83 */
#endif

# include	<stdio.h>

char	cgetc(i)
int	i;
{
	return ( getchar() );
}
