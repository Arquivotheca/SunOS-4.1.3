	.data
	.asciz	"@(#)underflow.S 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "DEFS.h"

RTENTRY(_gradual_underflow_)
	fpmove	fpamode,d0
	andl	#0xe,d0		| Clear garbage bits and fast mode.
	fpmove	d0,fpamode
	RET

RTENTRY(_abrupt_underflow_)
	fpmove	fpamode,d0
	andl	#0xf,d0		| Clear garbage bits.
	bset	#0,d0
	fpmove	d0,fpamode
	RET
