/*	@(#)vcmd.h 1.1 92/07/30 SMI; from UCB 4.4 83/04/05	*/

#ifndef _sys_vcmd_h
#define _sys_vcmd_h

#include <sys/ioctl.h>

#define	VPRINT		0100
#define	VPLOT		0200
#define	VPRINTPLOT	0400
#define	VPC_TERMCOM	0040
#define	VPC_FFCOM	0020
#define	VPC_EOTCOM	0010
#define	VPC_CLRCOM	0004
#define	VPC_RESET	0002

#define	VGETSTATE	_IOR(v, 0, int)
#define	VSETSTATE	_IOW(v, 1, int)

#endif /*!_sys_vcmd_h*/
