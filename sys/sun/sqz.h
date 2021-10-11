
/*	@(#)sqz.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sun_sqz_h
#define	_sun_sqz_h

#include <sys/ioccom.h>

struct sqz_status {
	int count;
	int total;
};

#define	SQZSET	_IOW(q, 1, int)
#define	SQZGET	_IOR(q, 2, struct sqz_status)

#endif /*!_sun_sqz_h*/
