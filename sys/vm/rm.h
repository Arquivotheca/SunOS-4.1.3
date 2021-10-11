/*	@(#)rm.h	1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _vm_rm_h
#define	_vm_rm_h

/*
 * VM - Resource Management.
 */

struct	page *rm_allocpage(/* seg, addr */);
void	rm_outofanon();
void	rm_outofhat();
int	rm_asrss(/* as */);

#endif /*!_vm_rm_h*/
