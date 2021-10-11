/* @(#)mmap.s 1.1 92/07/30 SMI */

/*
 * Interface to mmap introduced in 4.0.  Incorporates flag telling
 * system to use 4.0 interface to mmap.
 */

#include "SYS.h"
#include <sys/mman.h>

#define	FLAGS	sp@(16)

ENTRY(mmap)
	orl	#_MAP_NEW,FLAGS
	pea	SYS_mmap
	trap	#0
	jcs	err
	RET

err:	CERROR(a0)


