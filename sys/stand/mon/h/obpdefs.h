/*
 * @(#)obpdefs.h 1.1 92/07/30 SMI
 * Copyright (c) 1990 Sun Microsystems, Inc.
 */

/*
 * This file is intended as standalone inclusion by non-prom library
 * functions that need it.  They should not include sunromvec.h nor
 * openprom.h.
 */

/*
 *
 * OBP Versions:
 *	V0:	Original SS1
 *	V1:	NONE (There is no V1.)
 *	V2:	Calvin OBP with compatibility with V0
 *	V3:	All new OBP, no compatability with previous versions.
 *
 *	Thus, V0 and V2 are currently only on sun4c's
 *	V3 are for newer architectures but since they *may* be backported
 *	to sun4c's they need place-holders for sun4c only functions.
 *
 */

#ifndef _mon_opbdefs_h
#define	_mon_opbdefs_h

#ifndef __sys_types_h
#include <sys/types.h>
#endif __sys_types_h

#define	OPENPROMS
#define	OBP_MAGIC		((u_int)0x10010407)

#define	OBP_V0_ROMVEC_VERSION	0
#define	OBP_V2_ROMVEC_VERSION	2
#define	OBP_V3_ROMVEC_VERSION	3

#define	OBP_PLUGIN_VERSION	2

typedef unsigned int ihandle_t;
typedef unsigned int phandle_t;
typedef unsigned int dnode_t;

/*
 * Device type matching
 */

#define	OBP_NONODE	((dnode_t)0)
#define	OBP_BADNODE	((dnode_t)-1)

/*
 * Property Defines
 */

#define	OBP_NAME		"name"
#define	OBP_MAC_ADDR		"mac-address"
#define	OBP_STDINPATH		"stdin-path"
#define	OBP_STDOUTPATH		"stdout-path"
#define	OBP_IDPROM		"idprom"

#define	OBP_DEVICETYPE		"device_type"
#define	OBP_DISPLAY		"display"
#define	OBP_NETWORK		"network"
#define	OBP_BYTE		"byte"
#define	OBP_BLOCK		"block"
#define	OBP_SERIAL		"serial"
#define	OBP_HIERARCHICAL	"hierarchical"
#define OBP_CPU			"cpu"

/*
 * Size of the boot command buffer
 */

#define	OBP_BOOTBUFSIZE		128

/*
 * Longest (in bytes) device path name
 */

#define	OBP_MAXDEVNAME		128

#define	ROMVEC_BLKSIZE		512	/* XXX */

/*
 *  OBP Module mailbox messages for MP's
 *
 *	00..7F	: power-on self test
 *
 *	80..8F	: active in boot prom (at the "ok" prompt)
 *
 *	90..EF  : idle in boot prom
 *
 *	F0	: active in application
 *
 *	F1..FA	: reserved for future use
 *
 *	FB	: One processor exited to the PROM via op_exit(),
 *		  call to prom_stopcpu() requested.
 *
 *	FC	: One processor entered the PROM via op_enter(),
 *		  call to prom_idlecpu() requested.
 *
 *	FD	: One processor hit a BREAKPOINT,
 *		  call to prom_idlecpu() requested.
 *
 *	FE	: One processor got a WATCHDOG RESET
 *		  call to prom_stopcpu() requested.
 *
 *	FF	: This processor not available.
 *
 */

#define	OBP_MB_IDLE_LOW		((unsigned char)(0x90))
#define	OBP_MB_IDLE_HIGH	((unsigned char)(0xef))

#define	OBP_MB_IS_IDLE(s)	(((s) >= OBP_MB_IDLE_LOW) && \
				    ((s) <= OBP_MB_IDLE_HIGH))

#define	OBP_MB_ACTIVE		((unsigned char)(0xf0))
#define	OBP_MB_EXIT_STOP	((unsigned char)(0xfb))
#define	OBP_MB_ENTER_IDLE	((unsigned char)(0xfc))
#define	OBP_MB_BRKPT_IDLE	((unsigned char)(0xfd))
#define	OBP_MB_WATCHDOG_STOP	((unsigned char)(0xfe))


/*
 * V0 only Memory layout stuff
 */
struct memlist {
	struct memlist	*next;		/* link to next list element */
	u_int		address;	/* starting address of memory segment */
	u_int		size;		/* size of same */
};

extern struct sunromvec *romp;		/* as per the above discussion */

/*
 * XXX - This is not, strictly speaking, correct: we should never
 * depend upon "magic addresses". However, the MONSTART/MONEND
 * addresses are wired into too many standalones, and they are
 * not likely to change in any event.
 */
#ifdef sun4c
#define	MONSTART	(0xffd00000)
#define	MONEND		(0xfff00000)
#endif

#endif _mon_opbdefs_h
