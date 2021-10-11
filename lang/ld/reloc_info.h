/*      @(#)reloc_info.h 1.1 92/07/30 SMI; 	*/

/* <a.out.h> must be included before this file. */

/* This is so the common linker code can refer to either the 68k or
 * SPARC relcation structure via the same name.
 */

#undef relocation_info

#if (TARGET==SUN2) || (TARGET==SUN3)
#define relocation_info reloc_info_68k
#define	MAX_GOT_SIZE	16384		/* XXX Should this be here? */
#endif

#if (TARGET==SUN4)
#define relocation_info reloc_info_sparc
#define	MAX_GOT_SIZE	2048		/* XXX Should this be here? */
#endif
