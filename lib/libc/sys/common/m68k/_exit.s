/* @(#)_exit.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

#if vax
	.align	1
#endif
PSEUDO(_exit,exit)
			/* _exit(status) */
