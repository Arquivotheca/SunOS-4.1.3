#ifndef lint
static	char sccsid[] = "@(#)dkbad.c 1.1 92/07/30";
#endif

/*LINTLIBRARY*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#ifndef NOBADSECT
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/dkbad.h>

/*
 * Search the bad sector table looking for
 * the specified sector.  Return index if found.
 * Return -1 if not found.
 */
isbad(bt, cyl, trk, sec)
	register struct dkbad *bt;
{
	register int i;
	register long blk, bblk;

	blk = ((long)cyl << 16) + (trk << 8) + sec;
	for (i = 0; i < NDKBAD; i++) {
		bblk = ((long)bt->bt_bad[i].bt_cyl << 16) +
		    bt->bt_bad[i].bt_trksec;
		if (blk == bblk)
			return (i);
		if (bblk < 0)
			break;
	}
	return (-1);
}
#endif
