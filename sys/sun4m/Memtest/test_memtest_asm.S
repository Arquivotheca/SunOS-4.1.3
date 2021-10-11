#include <machine/asm_linkage.h>

	/*
	 * doustorem_asmX(addr)
	 *	addr_t addr;
	 *
	 * Create an asynchronous store error immediately followed by a
	 * synchronous error.
	 *
	 * There are 4 routines, to get all 4 possible cache-line
	 * assignments.
	 */
	ENTRY(doustorem_asm0)
	save	%sp, -SA(MINFRAME), %sp
1:	set	1b, %l0		! so we'll store to a cached location
	st	%g0, [%i0]	! this should cause an async trap
	st	%g0, [%l0]	! and this should cause a sync one
	ret
	restore

	ENTRY(doustorem_asm1)
	save	%sp, -SA(MINFRAME), %sp
1:	set	1b, %l0		! so we'll store to a cached location
	st	%g0, [%i0]	! this should cause an async trap
	st	%g0, [%l0]	! and this should cause a sync one
	ret
	restore

	ENTRY(doustorem_asm2)
	save	%sp, -SA(MINFRAME), %sp
1:	set	1b, %l0		! so we'll store to a cached location
	st	%g0, [%i0]	! this should cause an async trap
	st	%g0, [%l0]	! and this should cause a sync one
	ret
	restore

	ENTRY(doustorem_asm3)
	save	%sp, -SA(MINFRAME), %sp
1:	set	1b, %l0		! so we'll store to a cached location
	st	%g0, [%i0]	! this should cause an async trap
	st	%g0, [%l0]	! and this should cause a sync one
	ret
	restore

