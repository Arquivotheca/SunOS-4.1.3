/*
 * @(#)atc.s 1.1 92/07/30 SMI
 */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <machine/asm_linkage.h>

	ENTRY(atc_flush)
	pflusha
	rts

