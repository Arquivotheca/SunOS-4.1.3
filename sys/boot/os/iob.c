#ifndef lint
static	char sccsid[] = "@(#)iob.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Utility routines to deal with the internal IOB file
 * structures used by boot.
 */

#include <stand/param.h>
#include <stand/saio.h>
#include <sys/dir.h>
#include <sys/time.h>
#include "boot/vnode.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include "boot/iob.h"


/* These are the pools of buffers, iob's, etc. */
extern	char	b[NBUFS][MAXBSIZE];
extern	daddr_t	blknos[NBUFS];
extern	struct	iob	iob[NFILES];

static	int	dump_debug = 10;
static	int	openfirst = 1;

/*
 * Allocate an IOB for a newly opened file.   Simple linear
 * search of IOB table for an unused IOB.    Return the index
 * of an unused table entry if found, else exit.    Initialise
 * the table on the first call to this routine.
 */
int
getiob()
{
	register int fdesc;

#ifdef	DUMP_DEBUG
	dprint (dump_debug, 6,
		"getiob()\n");
#endif

	if (openfirst != 0) {
		openfirst = 0;
		for (fdesc = 0; fdesc < NFILES; fdesc++)
			iob[fdesc].i_flgs = IOB_UNUSED;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)	{
		if (iob[fdesc].i_flgs == IOB_UNUSED)	{
			(&iob[fdesc])->i_flgs |= F_ALLOC;
			return fdesc;
		}
	}

	_stop("No more file slots");
	/*NOTREACHED*/
}

ungetiob(fdesc)
	int fdesc;
{

	if ((fdesc < 0) || (fdesc >= NFILES)) {
		dprint (dump_debug, 10, "ungetiob: bad fdesc 0x%x\n", fdesc);
		return (-1);
	}

	iob[fdesc].i_flgs = IOB_UNUSED;
	return (0);
}
