/*	@(#)vfork.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * this atrocity is necessary on sparc because registers modified
 * by the child get propagated back to the parent via the window
 * save/restore mechanism.
 */

#ifndef _vfork_h
#define _vfork_h

extern int vfork();

#ifdef sparc
#pragma unknown_control_flow(vfork)
#endif

#endif /*!_vfork_h*/
