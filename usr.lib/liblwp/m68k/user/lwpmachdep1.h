/* @(#)lwpmachdep1.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this source code without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS PROGRAM CONTAINS SOURCE CODE COPYRIGHTED BY SUN MICROSYSTEMS, INC.
 * AND IS LICENSED TO SUNSOFT, INC., A SUBSIDIARY OF SUN MICROSYSTEMS, INC.
 * SUN MICROSYSTEMS, INC., MAKES NO REPRESENTATIONS ABOUT THE SUITABLITY
 * OF SUCH SOURCE CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT
 * EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  SUN MICROSYSTEMS, INC. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO SUCH SOURCE CODE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN
 * NO EVENT SHALL SUN MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT,
 * INCIDENTAL, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM USE OF SUCH SOURCE CODE, REGARDLESS OF THE THEORY OF LIABILITY.
 * 
 * This source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction, 
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS
 * SOURCE CODE OR ANY PART THEREOF.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California 94043
 */
#ifndef _lwp_m68k_machdep_h
#define _lwp_m68k_machdep_h

#ifndef _lwp_wait
#define _lwp_wait
#include <sys/wait.h>
#endif _lwp_wait

#include <sys/time.h>
#include <sys/resource.h>

/* info from SIGCHLD event */
typedef struct sigchldev_t {
	eventinfo_t sigchld_event;      /* general event info, MUST BE FIRST */
	int sigchld_pid;                /* pid of process that changed */
	union wait sigchld_status;      /* status from wait3 */
	struct rusage sigchld_rusage;   /* rusage from wait3 */
} sigchldev_t;

#define	MC_GENERALREGS	12	/* d2-d7, a2-a7 */
#define	MC_TEMPREGS	6	/* d0, d1, a0, a1, pc, ps */

typedef struct machstate_t {
	int mc_generals[MC_GENERALREGS]; /* d2-d7, a2-a7 */
	int mc_temps[MC_TEMPREGS]; /* d0, d1, a0, a1, pc, ps */
} machstate_t;

#endif /*!_lwp_m68k_machdep_h*/
