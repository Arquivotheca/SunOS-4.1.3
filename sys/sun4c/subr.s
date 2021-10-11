/*	@(#)subr.s 1.1 92/07/30 SMI	*/

/*
 *	Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * General assembly language routines.
 */

#include <machine/asm_linkage.h>
#include <machine/mmu.h>
#include <machine/enable.h>
#include <machine/eeprom.h>
#include "assym.s"

	.seg	"text"
	.align	4

/*
 * Read the ID prom.
 * This is mapped from IDPROM_ADDR for IDPROMSIZE bytes in the
 * kernel virtual address space.
 * XXX - my first sparc assembly code -- may be messed up!!!!
 */
	ENTRY(getidprom)
	set	IDPROM_ADDR, %o1
	clr	%o2
1:
	ldub	[%o1 + %o2], %o3	! get id prom byte
	add	%o2, 1, %o2		! interlock
	stb	%o3, [%o0]		! put it out
	cmp	%o2, IDPROMSIZE		! done yet?
	bne,a	1b
	add	%o0, 1, %o0		! delay slot
	retl				! leaf routine return
	nop				! delay slot

/*
 * Flush any write buffers between the CPU and the device at address v.
 * This will force any pending stores to complete, and any exceptions
 * they cause to occur before this routine returns.
 *
 * void
 * flush_writebuffers_to(v)
 *	addr_t v;
 *
 *
 * Flush all write buffers in the system.
 *
 * void
 * flush_all_writebuffers();
 *
 * void
 * flush_poke_writebuffers();	/* sun4m compatibility * /
 *
 * We implement this by reading the context register; this will stall
 * until the store buffer(s) are empty, on both 4/60's and 4/70's (and
 * clones).  Note that we ignore v on Sun4c.
 */
	ALTENTRY(flush_all_writebuffers)
	ALTENTRY(flush_poke_writebuffers)
	ENTRY(flush_writebuffers_to)
	set	CONTEXT_REG, %o1
	lduba	[%o1]ASI_CTL, %g0	! read the context register
	nop; nop			! two delays for synchronization
	nop; nop			! two more to strobe the trap address
	nop				! and three to empty the pipleine
	retl				!  so that any generated interrupt
	nop				!  will occur before we return
