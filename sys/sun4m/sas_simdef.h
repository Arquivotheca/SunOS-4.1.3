/*
 * @(#)sas_simdef.h 1.1 88/08/10
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

/*
 * Definitions for the SAS simulated disk.
 *
 * tuned for running in
 *	lisbon:/usr/src/sys/sun4x/sastools
 */

#define SIMD_RDEV	"sasfs/root"
#define SIMD_RSIZE	16192
#define SIMD_SDEV	"sasfs/swap"
#define SIMD_SSIZE	32768
#define SIMD_UDEV       "sasfs/usr"
#define SIMD_USIZE      294800
