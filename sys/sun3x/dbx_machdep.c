#ifndef lint
static	char sccsid[] = "@(#)dbx_machdep.c 1.1 92/07/30 SMI";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>

#include <sun3x/buserr.h>
#include <sun3x/clock.h>
#include <sun3x/eccreg.h>
#include <sun3x/mmu.h>
#include <sun3x/cpu.h>
#include <sun3x/frame.h>
#include <sun3x/memerr.h>
