/*
 * @(#)param.h 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * We redefine these things here to prevent header file madness
 */

#define	NBBY	8		/* bits per byte */

#define	MAXBSIZE	8192	/* max FS block size */
#define	MAXFRAG		8	/* max frags per block */

#define	DEV_BSIZE	512
#define	SECSIZE		512
#define DEV_BSHIFT	9

/*
 * Size of protocol work area allocated by Ethernet drivers
 * Must be multiple of pagesize for all machines
 */
#define	PROTOSCRATCH	8192
