#ifndef lint
static  char sccsid[] = "@(#)fd_conf.c 1.1 92/07/30 SMI";
#endif

/*
 * @(#)fd_conf.c 1.2 89/03/27 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 */
#include "fd.h"

#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dkio.h>
#include <sundev/fdreg.h>

int nfd	    = NFD ;

#if	NFD > 0
struct  fdstate fd[NFD];        /* Floppy drive state */
struct  unit fdunits[NFD];      /* Unit information */
struct  mb_device *fddinfo[NFD];
#endif	NFD

#if NFDC > 0
struct  mb_ctlr *fdcinfo[NFDC];
#endif NFDC
