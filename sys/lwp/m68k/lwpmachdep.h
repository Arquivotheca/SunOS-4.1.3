/* @(#) lwpmachdep.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef __LWPMACHDEP_H__
#define __LWPMACHDEP_H__

#ifndef _TIME_
#include <sys/time.h>
#endif _TIME_

#define	MC_GENERALREGS	12	/* d2-d7, a2-a7 */
#define	MC_TEMPREGS	6	/* d0, d1, a0, a1, pc, ps */

typedef struct machstate_t {
	int mc_generals[MC_GENERALREGS]; /* d2-d7, a2-a7 */
	int mc_temps[MC_TEMPREGS]; /* d0, d1, a0, a1, pc, ps */
} machstate_t;

#endif __LWPMACHDEP_H__
