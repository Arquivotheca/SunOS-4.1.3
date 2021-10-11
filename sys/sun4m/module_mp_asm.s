/*
 *	@(#)module_mp_asm.s 1.1 92/07/30 SMI
 *	Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * "mp" layer for module interface. slides into the same
 * hooks as a normal module interface, replacing the service
 * routines for the specific module with linkages that will
 * force crosscalls.
 *
 * This file is included only when
 *	options	MODULE_MP
 * is included in the kernel configuration file,
 * and will fail to compile if MULTIPROCESSOR is
 * not defined.
 */

#include <sys/param.h>
#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/devaddr.h>
#include <percpu_def.h>
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * REVEC: indirect call via xc_sync.
 */
#define	REVEC(name)				\
	sethi	%hi(_s_/**/name), %o5		; \
	b	_xc_sync			; \
	ld	[%o5+%lo(_s_/**/name)], %o5

/*
 * CONDREVEC: if first parm is inside VA_PERCPUME
 * region, branch to indirect service; otherwise,
 * do an indirect call via xc_sync.
 */
#define	CONDREVEC(name)				\
	srl	%o0, 24, %o5			; \
	cmp	%o5, VA_PERCPUME>>24		; \
	sethi	%hi(_s_/**/name), %o5		; \
	bne	_xc_sync			; \
	ld	[%o5+%lo(_s_/**/name)], %o5	; \
	jmp	%o5				; \
	nop

	ALTENTRY(mp_mmu_flushall)
	REVEC(mmu_flushall)

	ALTENTRY(mp_mmu_flushctx)
	REVEC(mmu_flushctx)

	ALTENTRY(mp_mmu_flushrgn)
	CONDREVEC(mmu_flushrgn)

	ALTENTRY(mp_mmu_flushseg)
	CONDREVEC(mmu_flushseg)

	ALTENTRY(mp_mmu_flushpage)
	CONDREVEC(mmu_flushpage)

	ALTENTRY(mp_mmu_flushpagectx)
	CONDREVEC(mmu_flushpagectx)

	ALTENTRY(mp_vac_flushall)
	REVEC(vac_flushall)

	ALTENTRY(mp_vac_usrflush)
	REVEC(vac_usrflush)

	ALTENTRY(mp_vac_ctxflush)
	REVEC(vac_ctxflush)

	ALTENTRY(mp_vac_rgnflush)
	CONDREVEC(vac_rgnflush)

	ALTENTRY(mp_vac_segflush)
	CONDREVEC(vac_segflush)

	ALTENTRY(mp_vac_pageflush)
	CONDREVEC(vac_pageflush)

	ALTENTRY(mp_vac_pagectxflush)
	CONDREVEC(vac_pagectxflush)

	ALTENTRY(mp_vac_flush)
	CONDREVEC(vac_flush)
