/*	@(#)alloca.h	1.1	92/07/30	SMI	*/

#ifndef _alloca_h
#define _alloca_h

#if defined(sparc)
# define alloca(x) __builtin_alloca(x)
#endif

#endif /*!_alloca_h*/
