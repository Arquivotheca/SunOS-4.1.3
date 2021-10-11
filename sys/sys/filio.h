/*	@(#)filio.h 1.1 92/07/30 SMI; from UCB ioctl.h 7.1 6/4/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * General file ioctl definitions.
 */

#ifndef _sys_filio_h
#define	_sys_filio_h

#include <sys/ioccom.h>

#define	FIOCLEX		_IO(f, 1)		/* set exclusive use on fd */
#define	FIONCLEX	_IO(f, 2)		/* remove exclusive use */

/* another local */
#define	FIONREAD	_IOR(f, 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW(f, 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW(f, 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW(f, 124, int)	/* set owner */
#define	FIOGETOWN	_IOR(f, 123, int)	/* get owner */

/* file system locking */
#define	FIOLFS		_IO(f, 64)		/* file system lock */
#define	FIOLFSS		_IO(f, 65)		/* file system lock status */
#define	FIOFFS		_IO(f, 66)		/* file system flush */

/* short term backup */
#define	FIOAI		_IO(f, 67)		/* allocation information */
#define	FIODUTIMES	_IO(f, 68)		/* delay update access time */
#define	FIODIO		_IO(f, 69)		/* delay write all data */
#define	FIODIOS		_IO(f, 70)		/* status of FIODIO */

#endif /*!_sys_filio_h*/
