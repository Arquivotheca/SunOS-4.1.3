| 	map.s 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
|
| /*
|  * Memory Mapping and Paging on the Sun 3
|  */
| 
| /*
|  * Mapping registers are accessed thru the  movs  instruction in the Sun-3.
|  *
|  * The following subroutines accept any address in the mappable range
|  * (256 megs).  They access the map for the current context register.  They
|  * assume that currently we are running in supervisor state.
|  */ 
| 
/* 
 * These constants are fixed by the hardware, 
 * so don't feel too bad that I redefined them here
 */
#define	FC_MAP		3		/* space for MMU ops */
#define	NUMPMEGS	256		/* number of Pmegs */
#define	PMAPOFF		0x10000000	/* offset for page map */
#define	SMAPOFF		0x20000000	/* offset for segment map */
#define	MAPADDRMASK	0x0FFFE000	/* translatable bits */
#define	PME_INVALID	0		/* invalid page */

	.text

	.globl	_setpgmap
_setpgmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#PMAPOFF,d0	| Offset to page maps
	movl	d0,a0
	lea	FC_MAP,a1	| Get function code in a reg
| The following code can be used to verify that the only page
| table entries written to the Invalid Pmeg are invalid pages, since the 
| consequences of writing valid entries here are somewhat spectacular.
	movc	sfc,d0		| Save source function code
	movc	a1,sfc		| Set source function code, too.
	addl	#SMAPOFF-PMAPOFF,a0	| Offset to segment map
	movsb	a0@,d1		| Get segment map entry number
	subl	#SMAPOFF-PMAPOFF,a0	| Offset back to page map
	movc	d0,sfc		| Restore source function code
| If debug code is removed, next line must remain!
	movl	sp@(8),d0	| Get page map entry to write
| More debug code
#if NUMPMEGS != 256
	andb	#NUMPMEGS-1,d1	| Top bit ignored in 128-pmeg version
#endif
	cmpb	#NUMPMEGS-1,d1	| Are we writing in the Invalid Pmeg?
	jne	1$		| Nope, go on.
	cmpl	#PME_INVALID,d0	| Writing the Invalid Page Map Entry?
	jeq	1$		| Yes, it's ok.
	bras	.-1		| Fault out, this is nasty thing to do.
1$:
| End of debugging code.
	movc	dfc,d1		| Save dest function code
	movc	a1,dfc		| Set destination function code
	movsl	d0,a0@		| Write page map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done

|
| Read the page map entry for the given address v
| and return it in a form suitable for software use.
|
| long
| getpgmap(v)
| caddr_t v;
	.globl	_getpgmap
_getpgmap:
	movl	sp@(4),d0		| get access address
	andl	#MAPADDRMASK,d0		| clear extraneous bits
	orl	#PMAPOFF,d0		| set to page map base offset
	lea	FC_MAP,a1		| Get function code in a reg
	movc	sfc,d1			| Save source function code
	movc	a1,sfc			| Set source function code, too.
	movl	d0,a0
	movsl	a0@,d0			| read page map entry
					| no mods needed to make pte from pme
	movc	d1,sfc			| restore function code
	rts				| done

	.globl	_setsegmap
_setsegmap:
	movl	sp@(4),d0	| Get access address
	andl	#MAPADDRMASK,d0	| Mask out irrelevant bits
	addl	#SMAPOFF,d0	| Bump to segment map offset
	movl	d0,a0
	movl	sp@(8),d0	| Get seg map entry to write
	movc	dfc,d1		| Save dest function code
	lea	FC_MAP,a1	| Get function code in a reg
	movc	a1,dfc		| Set destination function code
	movsb	d0,a0@		| Write segment map entry
	movc	d1,dfc		| Restore dest function code
	rts			| done
