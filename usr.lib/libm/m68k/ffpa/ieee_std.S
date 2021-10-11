	.data
	.asciz	"@(#)ieee_std.S 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1988 by Sun Microsystems, Inc.

#include "DEFS.h"
#define FPABASEADDRESS 0xe0000000

RTENTRY(_standard_arithmetic)
	fpmove	fpamode,d0
	andl	#0xe,d0		| Clear garbage bits and fast mode.
	fpmove	d0,fpamode
	RET

RTENTRY(_nonstandard_arithmetic)
	fpmove	fpamode,d0
	andl	#0xf,d0		| Clear garbage bits.
	bset	#0,d0
	fpmove	d0,fpamode
	RET

RTENTRY(_standard_arithmetic_)
	fpmove	fpamode,d0
	andl	#0xe,d0		| Clear garbage bits and fast mode.
	fpmove	d0,fpamode
	RET

RTENTRY(_nonstandard_arithmetic_)
	fpmove	fpamode,d0
	andl	#0xf,d0		| Clear garbage bits.
	bset	#0,d0
	fpmove	d0,fpamode
	RET

RTENTRY(__is_nonstandard)
	fpmove	fpamode,d0	| d0 gets mode bits.
	moveq	#1,d1
	andl	d1,d0		| Clear all but lsb = nonstandard bit.
	RET
