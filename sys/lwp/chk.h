/* @(#) chk.h 1.1 92/07/30 Copyr 1987 Sun Micro */

#ifndef _lwp_check_h
#define _lwp_check_h

#ifdef mc68000
#include <lwp/m68k_chk.h>
#endif mc68000

#ifdef vax
#include <lwp/vax_chk.h>
#endif vax

#ifdef sparc
#include <lwp/sparc_chk.h>
#endif sparc

#endif /*!_lwp_check_h*/
