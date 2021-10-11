#ifndef lint
static	char sccsid[] = "@(#)options.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Options special file
 */

#include <sun/optionsio.h>	/*???*/

/**
#include <sys/param.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/vm.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <vm/seg.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/eeprom.h>
#include <machine/seg_kmem.h>
**/

optionsioctl(dev, com, data, flag);
	dev_t dev;
	int com;
	caddr_t data;
	int flag;
{

	return (0);
}
