#ifndef lint
static	char sccsid[] = "@(#)dbx_machdep.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>

#include <sun3/buserr.h>
#include <sun3/clock.h>
#include <sun3/eccreg.h>
#include <sun3/mmu.h>
#include <sun3/cpu.h>
#include <sun3/frame.h>
#include <sun3/memerr.h>
