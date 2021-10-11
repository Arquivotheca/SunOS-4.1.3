#ifndef lint
static	char sccsid[] = "@(#)mti_conf.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 *
 * Configuration dependent variables
 */

#include "mti.h"
#include <sys/param.h>
#include <sys/termios.h>
#include <sys/buf.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/clist.h>

#include <sundev/mbvar.h>
#include <sundev/mtivar.h>

#define	NMTILINE	(NMTI*16)

int	nmti = NMTILINE;

struct	mb_device *mtiinfo[NMTI];

struct	mtiline mtiline[NMTILINE];

struct	mti_softc mti_softc[NMTI];
