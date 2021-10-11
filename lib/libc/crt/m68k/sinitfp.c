#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)sinitfp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include "fpcrttypes.h"
#define SKY "/dev/sky"

int _sky_fd;
extern int errno;
extern caddr_t mmap();

sinitfp_()
{
	/* 
	 * map sky ffp into user address space
	 */
	register short *stcreg;
	int saveerrno = errno;

	if (fp_state_skyffp != fp_unknown) {
		if (fp_state_skyffp == fp_enabled) {
			fp_switch = fp_skyffp;
			return (1);
		}
		return (0);
	}

	_sky_fd = open(SKY, 2);
	if (_sky_fd < 0) {
		errno = saveerrno;
		fp_state_skyffp = fp_absent;
		return (0);
	}

	errno = 0;
	_skybase = mmap((caddr_t)0, getpagesize(), PROT_READ|PROT_WRITE,
	    MAP_SHARED, _sky_fd, 0);
	if (errno) {
		close(_sky_fd);
		_skybase = 0;
		errno = saveerrno;
		fp_state_skyffp = fp_absent;
		return (0);
	}

	_skybase += 4;		/* point at the data port */

	errno = saveerrno;
	fp_state_skyffp = fp_enabled;
	fp_switch = fp_skyffp;

	return (1);
}

int
getskyfd()
{

	return (_sky_fd);
}
