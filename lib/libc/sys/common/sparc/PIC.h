!
!       @(#)PIC.h 1.1 92/07/30 SMI
!
#ifdef PIC 
#define PIC_SETUP(r) \
	or	%g0,%o7,%g1; \
1: \
	call	2f; \
	nop; \
2: \
	sethi	%hi(__GLOBAL_OFFSET_TABLE_ - (1b-.)), %r; \
	or	%r, %lo(__GLOBAL_OFFSET_TABLE_ - (1b-.)),%r; \
	add	%r, %o7, %r; \
	or	%g0,%g1,%o7
#else 
#define PIC_SETUP()
#endif 

