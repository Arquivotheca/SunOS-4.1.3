#ifndef lint
static	char sccsid[] = "@(#)setwarp.c 1.1 92/07/30 SMI"; /* from UCB 4.2 05/27/83 */
#endif

# include	"trek.h"
# include	"getpar.h"

/*
**  SET WARP FACTOR
**
**	The warp factor is set for future move commands.  It is
**	checked for consistancy.
*/

setwarp()
{
	double	warpfac;

	warpfac = getfltpar("Warp factor");
	if (warpfac < 0.0)
		return;
	if (warpfac < 1.0)
		return (printf("Minimum warp speed is 1.0\n"));
	if (warpfac > 10.0)
		return (printf("Maximum speed is warp 10.0\n"));
	if (warpfac > 6.0)
		printf("Damage to warp engines may occur above warp 6.0\n");
	Ship.warp = warpfac;
	Ship.warp2 = Ship.warp * warpfac;
	Ship.warp3 = Ship.warp2 * warpfac;
}
