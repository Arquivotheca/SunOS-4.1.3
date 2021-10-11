#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)win_screenadj.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include <sunwindow/rect.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_struct.h>

/*
 * Returns -1 if problem opening *rootname's.
 */
win_screenadj(base, neighbors, updateneighbors)
	register struct	screen *base, neighbors[SCR_POSITIONS];
	int	updateneighbors;
{
	int	numneighbors[SCR_POSITIONS], numbase, numtmp[SCR_POSITIONS];
	int	basefd, neighborfd;
	register int i, j;

	/*
	 * Initialize numneighbors
	 */
	for (i = 0; i < SCR_POSITIONS; ++i)
		numneighbors[i] = (neighbors[i].scr_rootname[0] == '\0')?
		    WIN_NULLLINK:
		    win_nametonumber(neighbors[i].scr_rootname);
	/*
	 * Set neighbors of base.
	 */
	numbase = win_nametonumber(base->scr_rootname);
	if ((basefd = open(base->scr_rootname, O_RDONLY, 0)) < 0)
		return(-1);
	(void)win_setscreenpositions(basefd, numneighbors);
	(void)close(basefd);
	/*
	 * Set base as neighbor of neighbors.
	 */
	if (updateneighbors) {
		for (i = 0; i < SCR_POSITIONS; ++i) {
			if (numneighbors[i] == WIN_NULLLINK)
				continue;
			if ((neighborfd = open(neighbors[i].scr_rootname,
			    O_RDONLY, 0)) < 0)
				return(-1);
			for (j = 0; j < SCR_POSITIONS; ++j)
				numtmp[j] = WIN_NULLLINK;
			switch (i) {
			case SCR_NORTH:
				numtmp[SCR_SOUTH] = numbase;
				break;
			case SCR_SOUTH:
				numtmp[SCR_NORTH] = numbase;
				break;
			case SCR_EAST:
				numtmp[SCR_WEST] = numbase;
				break;
			case SCR_WEST:
				numtmp[SCR_EAST] = numbase;
				break;
			}
			(void)win_setscreenpositions(neighborfd, numtmp);
			(void)close(neighborfd);
		}
	}
	return(0);
}

