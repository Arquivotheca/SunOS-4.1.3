#ident "@(#)sdisk.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*
 * fakeprom dummy disk
 */
/*
 * Dummy driver for sparc simulated disks.
 *	uses systems calls via simulator to accomplish
 *	base level open, seek, read, and write operations
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/file.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <vm/page.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>

#include <stand/saio.h>

int errno;
extern u_int debug;

struct sdisk {
	int	fdes;
	int	flag;
} sdparts[] = {
	{ -1, 0, },
	{ -1, 0, },
	{ -1, 0, },
	{ -1, 0, },
};

#define	NPARTS (sizeof (sdparts) / sizeof (sdparts[0]))

/*
 *
 */
extern int sdopen(), sdclose(), sdstrategy();
struct boottab sd_boottab = {
	"sd", NULL, NULL,
	 	sdopen, sdclose, sdstrategy, "sd", NULL
};

/* saio request buffer used by read, write blocks */
struct saioreq saio;

/*
 * sdclose:
 */
int
sdclose(sip)
struct saioreq *sip;
{
	int part = sip->si_boff;
	int unit = sip->si_unit;
	int ctlr = sip->si_ctlr;

	if (part >= NPARTS || unit > 0 || ctlr > 0)
		return (-1);

	sdparts[part].fdes = -1;
	return (0);
}

/*
 * sdopen:
 */
int
sdopen(sip)
struct saioreq *sip;
{
	int part = sip->si_boff;
	int unit = sip->si_unit;
	int ctlr = sip->si_ctlr;

	if (debug)
		printf("sdopen: %x\n", part);

	if (part >= NPARTS || unit > 0 || ctlr > 0)
		return (-1);

	if (sdparts[part].fdes != -1)
		return (0);	/* second time through */

	sdparts[part].fdes = s_open(part, O_RDWR);

	if (sdparts[part].fdes != -1)
		return (0);	/* open succeeded */

	if (debug)
		printf("sdopen: failed\n");
	return (-1);
}



/*
 * sdstrategy:
 *   read/write from simulated disk
 *
 * returns:
 *   -1 on error
 *   bytes transfered on success
 */
int
sdstrategy(sip, rw)
struct saioreq *sip;
u_int rw;
{
	register daddr_t bn;		/* block number */
	register int part;		/* partition number */
	register int fdes;		/* file descriptor */
	register int cnt;		/* completed transfer count */
	register int addr;	/* might be virtual, might be physical ... */
	u_int vaddr, paddr;
	struct page *pp;
	union ptpe *ptpe;
	int offset, bn_offset;
	int tcount, count;
	int rcount = 0;

	part = sip->si_boff;
	if (part >= NPARTS) {
		printf("sdstrategy: bad partition %d\n", part);
		return (-1);
	}

	fdes = sdparts[part].fdes;

/* For now assume the IOMMU is disabled - this may or not be true but
	the MMU is certainly enabled and so we must translate, for now don't
   	even do this, just run the disk.
 */


	bn = sip->si_bn;
	vaddr = (u_int)sip->si_ma; /* caller buffer address */
	tcount = sip->si_cc/DEV_BSIZE; /* blocks */
	bn_offset = 0;

	if (debug)
		printf("sdstrategy: tcount=%x bn=%d\n", tcount, bn);

	while (1) {

		count = (1 << DEV_BSHIFT);

		if (debug)
			printf("sdstrategy: count=%x blk=%d addr=0x%x\n",
				count, bn+bn_offset, vaddr);
		if (s_lseek(fdes, (bn+bn_offset) << DEV_BSHIFT, L_SET) == -1) {
			printf("can't seek\n\r");
			return (-1);
		} else if (rw == READ) {
			if (((cnt=s_read(fdes, vaddr, count)) == -1))
			{
				printf("sdstrategy: read error\n\r");
				return (-1);
			}
		} else {
			if (((cnt=s_write(fdes, vaddr, count)) == -1))
			{
				printf("sdstrategy: write error\n\r");
				return (-1);
			}
		}

		tcount -= 1;
		bn_offset += 1;

		vaddr += (1 << DEV_BSHIFT);
		rcount += count;

		if (tcount <= 0)
			break;
	}
	return (rcount);
}
