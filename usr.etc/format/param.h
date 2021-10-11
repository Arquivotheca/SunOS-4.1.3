
/*      @(#)param.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file contains declarations of miscellaneous parameters.
 */
#ifndef	SECSIZE
#define	SECSIZE		DEV_BSIZE
#endif

#define	MAX_CYLS	(32 * 1024 - 1)		/* max legal cylinder count */
#define	MAX_HEADS	(64)			/* max legal head count */
#define	MAX_SECTS	(256)			/* max legal sector count */

#define	MIN_RPM		2000			/* min legal rpm */
#define	AVG_RPM		3600			/* default rpm */
#define	MAX_RPM		6000			/* max legal rpm */

#define	MIN_BPS		512			/* min legal bytes/sector */
#define	AVG_BPS		600			/* default bytes/sector */
#define	MAX_BPS		1000			/* max legal bytes/sector */

#define	INFINITY	0x0100000		/* a big number */

