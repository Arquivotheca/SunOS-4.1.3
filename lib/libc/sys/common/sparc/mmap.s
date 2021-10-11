/* @(#)mmap.s 1.1 92/07/30 SMI */

/*
 * Interface to mmap introduced in 4.0.  Incorporates flag telling
 * system to use 4.0 interface to mmap.
 */

#include "SYS.h"
#include <sys/mman.h>

#define	FLAGS	%o3

ENTRY(mmap)
	sethi	%hi(_MAP_NEW), %g1
	or	%g1, FLAGS, FLAGS
	mov	SYS_mmap, %g1
	t	0
	CERROR(o5)
	RET

