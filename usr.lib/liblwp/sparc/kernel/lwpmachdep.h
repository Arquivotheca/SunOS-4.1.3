/* @(#) lwpmachdep.h 1.1 92/07/30 */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef __LWPMACHDEP_H__
#define __LWPMACHDEP_H__

#ifndef _TIME_
#include <sys/time.h>
#endif _TIME_

/* registers saved in the context. Locals and %i's are found on the stack. */
typedef struct machstate_t {
	union {
		double sparc_gdregs[4];	
		int sparc_gsregs[8];
	} sparc_globals;	/* g0-g7 */
	union {
		double sparc_odregs[4];
		int sparc_osregs[8];
	} sparc_oregs;	/* o0-o7 */
	int sparc_pc;
	int sparc_npc;
	int sparc_psr;
	int sparc_y;
} machstate_t;

#endif __LWPMACHDEP_H__
