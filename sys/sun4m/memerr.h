/*	@(#)memerr.h 1.1 92/07/30 SMI	*/

#ifndef	_sun4m_memerr_h
#define	_sun4m_memerr_h

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * All sun4m machines has ECC Memory error correction.
 */

/* Definitions of ECC registers addresses and bit fields */
#include <machine/devaddr.h>
#include <machine/async.h>

#define EFER_VADDR (MEMERR_ADDR)
#define EFDR_VADDR (MEMERR_ADDR+0x18)
#define EFER_EE  0x1          /* Enables ECC Checking */
#define EFER_EI  0x2          /* Enables Interrupt on CEs */
#define EFAR1_PGNUM 0xFFFFF000        /* masked out 12 bit offset */
#define EFDR_GENERATE_MODE      0x400



#endif	/* !_sun4m_memerr_h */
