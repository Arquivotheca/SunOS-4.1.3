/* @(#)schedule.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

#ifndef _lwp_schedule_h
#define _lwp_schedule_h

extern struct lwp_t *__Curproc;	/* Currently running lwp */
extern struct lwp_t Nullproc;	/* null proc*/
extern qheader_t *__RunQ;	/* Queues (1/prio) of runnable lwps */
extern int __MaxPrio;		/* Size of RunQ (0..MaxPrio) */
extern void __schedule();

#endif /*!_lwp_schedule_h*/
