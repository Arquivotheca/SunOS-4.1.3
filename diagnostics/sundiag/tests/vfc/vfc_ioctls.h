/********** @(#)vfc_ioctls.h 1.1 7/30/92 ************/
/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ifndef _VIDEOPICS_VFC_IOCTLS_H
#define	_VIDEOPICS_VFC_IOCTLS_H

#ident	"@(#)vfc_ioctls.h	1.4	90/12/12 SMI"

#include <sys/ioctl.h>

	/* IOCTLs */
#define VFCGCTRL	_IOR(j, 0, int)	/* get ctrl. attributes */
#define VFCSCTRL	_IOW(j, 1, int)	/* set ctrl. attr. */
#define VFCGVID		_IOR(j, 2, int)	/* get video decoder attr. */
#define VFCSVID		_IOW(j, 3, int)	/* set video decoder attr. */
#define VFCHUE		_IOW(j, 4, int)	/* change hue to that indicated */
#define VFCPORTCHG	_IOW(j, 5, int)	/* change i/p port to that indicated */
#define VFCRDINFO	_IOW(j, 6, int) /* info necessary for reading */


	/* Options for the VFCSCTRL */
#define MEMPRST		0x1	/* reset FIFO ptr. */
#define CAPTRCMD	0x2	/* issue capture cmd. */
#define DIAGMODE	0x3	/* set board into diag mode */
#define NORMMODE	0x4	/* set board into normal mode */

	/* Options for the VFCSVID */
#define STD_NTSC	0x1	/* switch to NTSC std. */
#define STD_PAL		0x2	/* switch to PAL std. */
#define COLOR_ON	0x3	/* force color ON */
#define MONO		0x4	/* force color OFF */
#define ON			0x1
#define OFF			0x0

	/* Values returned by VFCGVID */

#define NO_LOCK	        1
#define NTSC_COLOR	2
#define NTSC_NOCOLOR    3
#define PAL_COLOR	4
#define PAL_NOCOLOR	5

	/* Options for setting Field number */
#define ODD_FIELD	0x1
#define EVEN_FIELD	0x0
#define ACTIVE_ONLY     0x2
#define NON_ACTIVE	0x0

#endif /* !_VIDEOPICS_VFC_IOCTLS_H */
