/*
 * @(#)bootparam.h 1.1 92/07/30 SMI
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Constants for stand-alone I/O (bootstrap) code.
 */

#ifndef _mon_bootparam_h
#define _mon_bootparam_h

#ifdef sun4
#define	BBSIZE   (64*512) /* Boot block size.               */
#ifdef CACHE
#define LOADADDR 0x20000  /* Load address of boot programs. */
#else CACHE
#define LOADADDR 0x4000   /* Load address of boot programs. */
#endif CACHE
#else sun4
#define BBSIZE   8192     /* Boot block size.               */
#define LOADADDR 0x4000   /* Load address of boot programs. */
#endif sun4

#define	DEV_BSIZE 512

#endif /*!_mon_bootparam_h*/
