/* @(#)romvec.h 1.1 92/07/30 SMI */

#ifndef	_sun4m_romvec_h
#define	_sun4m_romvec_h

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Sun4m specific romvec routines/variables
 * The following code is dropped directly into the sunromvec struct,
 * thus its strange appearance
 */
/* 
 * NOTE: these were stolen from sun3x version sunromvec.h, we may
 *	 not need all of them here.
 */
int	**v_lomemptaddr;    /* address of low memory ptes      */
int	**v_monptaddr;      /* address of debug/mon ptes       */
int	**v_dvmaptaddr;     /* address of dvma ptes            */
int	**v_monptphysaddr;  /* Physical Addr of the KADB PTE's */
int	**v_shadowpteaddr;  /* addr of shadow cp of DVMA pte's */
#endif	_sun4m_romvec_h
