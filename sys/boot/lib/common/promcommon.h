/*
 * @(#)promcommon.h 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef	_boot_lib_promcommon_h
#define _boot_lib_promcommon_h


/*
 * Common header file for prom support library
 * Used so that the kernel and other stand-alones (eg boot)
 * don't have to directly reference the prom (of which there
 * are now several completely different variants).
 */

#include <sys/types.h>
#include <mon/sunromvec.h>
#include <sun/openprom.h>
#include <sun/autoconf.h>

/*
 * Undefine DPRINTF/VPRINTF to remove compiled debug code.
 * For run time, see prom_init for prom_debug variable definition.
 *
 * #define	DPRINTF		if (promlib_debug < 2); else prom_printf
 * #define	VPRINTF		if (promlib_debug < 1); else prom_printf
 */
#if	defined(DPRINTF) || defined (VPRINTF)
extern int	promlib_debug;
#endif

extern int obp_romvec_version;

#endif	_boot_lib_promcommon_h
