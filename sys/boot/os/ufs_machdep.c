#ifndef lint
static        char sccsid[] = "@(#)ufs_machdep.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */



#include <machine/pte.h>

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vm.h>

/*
 * Machine dependent handling of the buffer cache.
 */

/*
 * Expand or contract the actual memory allocated to a buffer.
 * If no memory is available, release buffer and take error exit
 */
/*ARGSUSED*/
allocbuf(tp, size)
	register struct buf *tp;
	int size;
{
#ifdef lint
	tp = tp;
	size = size;
#endif lint
	return (1);
}

/*
 * Release space associated with a buffer.
 */
bfree(bp)
	struct buf *bp;
{

	bp->b_bcount = 0;
}
