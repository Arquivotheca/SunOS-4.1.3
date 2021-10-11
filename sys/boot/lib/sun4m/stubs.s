/*
 *	.seg	"data"
 *	.asciz	"@(#)stubs.s	1.1 7/30/92 SMI"
 *	Copyright (c) 1988,1990 by Sun Microsystems, Inc.
 */

	.global	_nullsys, _xxboot, _xxprobe, _ttboot
_nullsys:
_xxprobe:
_xxboot:
_ttboot:
	retl
	mov	-1, %o0
