/* @(#) stk.h 1.1 92/07/30 Copyr 1987 Sun Micro */

#ifndef _lwp_stackdep_h
#define _lwp_stackdep_h

#ifdef mc68000
#include <lwp/m68k_stackdep.h>
#endif mc68000

#ifdef vax
#include <lwp/vax_stackdep.h>
#endif vax

#ifdef sparc
#include <lwp/sparc_stackdep.h>
#endif sparc

#define	STKTOP(s) (STACKTOP((s), sizeof(s) / sizeof(stkalign_t)))
extern stkalign_t *lwp_newstk();

#endif /*!_lwp_stackdep_h*/
