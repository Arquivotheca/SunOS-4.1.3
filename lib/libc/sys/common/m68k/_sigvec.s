/*
 * @(#)_sigvec.s 1.1 92/07/30 SMI
 */

#include "SYS.h"

	.text
	.globl	__sigvec
	.align 2
err:
	CERROR(a0)
__sigvec:
	pea	SYS_sigvec
	trap	#0
	jcs	err
	rts
	
