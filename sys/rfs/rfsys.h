/*	@(#)rfsys.h 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:sys/rfsys.h	10.4" */

#ifndef _rfs_rfsys_h
#define _rfs_rfsys_h

#ifdef KERNEL
extern	char	Domain[];
#endif

/* 
 *  opcodes for rfsys system calls
 *  Note: These are also defined in /usr/src/lib/libc/sys/common/rfssys.c
 *  for unbundling reasons, and the definitions must match.
 */
#define RF_FUMOUNT	1	/* forced unmount */
#define RF_SENDUMSG	2	/* send buffer to remote user-level */
#define RF_GETUMSG	3	/* wait for buffer from remote user-level */
#define RF_LASTUMSG	4	/* wakeup from GETUMSG */
#define RF_SETDNAME	5	/* set domain name */
#define RF_GETDNAME	6	/* get domain name */
#define RF_SETIDMAP	7	/* Load a uid/gid map */
#define RF_FWFD		8 	/* Stuff a TLI circuit into the kernel */
#define RF_VFLAG	9	/* Handle verification option */
#define RF_DISCONN	10	/* return value for link down */
#define RF_VERSION	11 	/* Handle version information */
#define RF_RUNSTATE	12	/* See if RFS is running */
#define RF_ADVFS	13	/* Advertise a resource */
#define RF_UNADVFS	14 	/* Unadvertise a resource */
#define RF_RFSTART	15	/* Start up RFS */
#define RF_RFSTOP	16 	/* Stop RFS */


/* defines for VFLAG option	*/
#define V_CLEAR 0
#define V_SET	1
#define V_GET	2

/* defines for the VERSION option	*/
#define VER_CHECK	1
#define VER_GET		2

#endif /*!_rfs_rfsys_h*/
