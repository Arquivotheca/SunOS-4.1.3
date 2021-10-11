#ifndef lint
static	char sccsid[] = "@(#)ip.c	1.1 92/07/30	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stand/saio.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include "ipreg.h"
#include <ufs/fsdir.h>	/* DEV_BSIZE */

#define MAXHEAD		4	/* Max # heads to search for a label */

#define	MAX(a, b)	(a > b ? a : b)

#define IPSTD 4
u_long ipstd[] = { 0x40, 0x44, 0x48, 0x4c, };

struct ipparam {
	int	ip_unit;
	int	ip_boff;
	int	ip_nsect;
	int	ip_ncyl;
	int	ip_nhead;
	int	ip_bhead;		/* Base head #, for CDC Lark support */
	struct ipdevice *ip_addr;
};

/*
 * Layout of our DMA space
 */
struct ipdma {
	struct iopb0	iopb0;
	union {
		char	buffer[DEV_BSIZE];
		struct dk_label	label;
		struct uib	up;
	} u;
};

#define	ipbuffer	(((struct ipdma *)sip->si_dmaaddr)->u.buffer)
#define	iplabel		(&((struct ipdma *)sip->si_dmaaddr)->u.label)
#define	ipup		(&((struct ipdma *)sip->si_dmaaddr)->u.up)


/*
 * What resources we need
 */
struct devinfo ipinfo = {
	sizeof (struct ipdevice),
	sizeof (struct ipdma),
	sizeof (struct ipparam),
	IPSTD,
	ipstd,
	MAP_MBIO,
	DEV_BSIZE,		/* block at a time */
};

/*
 * What facilities we export to the world
 */
int ipprobe(), ipopen(), ipstrategy();
extern int xxboot(), xxprobe();
extern int nullsys();

struct boottab ipdriver = {
	"ip",	xxprobe, xxboot, ipopen, nullsys, ipstrategy,
	"ip: Interphase disk", &ipinfo,
};


#define	CYL(p)	(p * ipp->ip_nsect * ipp->ip_nhead)	/* block # at cylinder location */


/*
 * Open an Interphase disk.
 */
ipopen(sip)
	register struct saioreq *sip;
{
	register struct ipparam *ipp;
	register struct dk_label *label = iplabel;
	u_short ppart;

	ipp = (struct ipparam *)sip->si_devdata;
	ipp->ip_unit = sip->si_unit & 0x03;
	ppart = (sip->si_unit >> 2) & 1;
	ipp->ip_addr = (struct ipdevice *)sip->si_devaddr;

	ipp->ip_nsect = 2;		/* Read label */
	ipp->ip_ncyl  = 2;
	ipp->ip_nhead = MAXHEAD;
	ipp->ip_bhead = 0;		/* No base head yet, til label read */
	ipp->ip_boff  = 0;		/* Don't offset block numbers */

	for (ipp->ip_bhead = 0; ipp->ip_bhead < MAXHEAD; ipp->ip_bhead++) {
		register short *sp, sum;
		short count;

		label->dkl_magic = 0;
		if (ipcmd(IP_READ, sip, 0, label))
			continue;
		if (label->dkl_magic != DKL_MAGIC)
			continue;

		sum = 0;
		count = sizeof (struct dk_label) / sizeof (short);
		sp = (short *)label;
		while (count--) 
			sum ^= *sp++;
		if (sum != 0) {
			printf("Corrupt label on head %d\n", ipp->ip_bhead);
			continue;
		}
		if (ipp->ip_bhead != label->dkl_bhead) {
			printf("Misplaced label on head %d\n", ipp->ip_bhead);
			continue;
		}
		if (ppart != label->dkl_ppart)
			continue;
		goto foundlabel;
	}
	if (ppart)
		printf("For phys part %d, ", ppart);
	printf("No label found.\n");
	return (-1);

foundlabel:
	ipp->ip_nhead = label->dkl_nhead;
	ipp->ip_nsect = label->dkl_nsect;
	ipp->ip_ncyl  = label->dkl_ncyl;
	ipp->ip_bhead = label->dkl_bhead;
	ipp->ip_boff  = label->dkl_map[sip->si_boff].dkl_cylno;
	return (0);
}

ipstrategy(sip, rw)
	struct saioreq *sip;
	int rw;
{
	register int cmd = (rw == WRITE) ? IP_WRITE : IP_READ;
	register int blk = sip->si_bn;
	register char *ma = sip->si_ma;

	if (ipcmd(cmd, sip, blk, ma))
		return (-1);
	return (sip->si_cc);
}

ipcmd(cmd, sip, bno, buf)
	register int cmd;
	struct saioreq *sip;
	register int bno;
	register char *buf;
{
	register struct ipparam *ipp = (struct ipparam *)sip->si_devdata;
	register int u;
	register struct iopb0 *ip0;
	register struct ipdevice *ipaddr = ipp->ip_addr;
	register char *bp;
	int cylno, sect, status, error, errcnt = 0;

	ip0 = &((struct ipdma *)sip->si_dmaaddr)->iopb0;
	bzero((char *)ip0, IPIOPBSZ);
	bp = ipbuffer;

	cylno = bno / CYL(1);
	cylno += ipp->ip_boff;
	if (cylno > ipp->ip_ncyl)
		return (-1);
	sect = bno % ipp->ip_nsect;

	if (cmd == IP_WRITE && buf != bp)	/* Many just use common buf */
		bcopy(buf, bp, DEV_BSIZE);

retry:
	while (ipaddr->ip_r0 & IP_BUSY)
		DELAY(30);
	ip0->i0_cmd = cmd;
	ip0->i0_status = 0;
	ip0->i0_error = 0;
	u = 1 << (ipp->ip_unit&3);
	ip0->i0_unit_cylhi = (u<<4) | ((cylno>>8)&0xF);
	ip0->i0_cylinder = cylno;
	if (cmd == IP_RESTORE)
		ip0->i0_sector = ipp->ip_nsect;
	else
		ip0->i0_sector = sect;
	ip0->i0_secnt = 1;
	ip0->i0_buf_xmb = (MB_DMA_ADDR(bp)) >> 16;
	ip0->i0_buf_msb = (MB_DMA_ADDR(bp)) >> 8;
	ip0->i0_buf_lsb = (MB_DMA_ADDR(bp));
	ip0->i0_head = ((bno % CYL(1)) / ipp->ip_nsect) + ipp->ip_bhead;
	ip0->i0_ioaddr = (int)ipaddr;
	ip0->i0_burstlen = IP0_BURSTLEN;
	ip0->i0_nxt_xmb = 0;
	ip0->i0_nxt_msb = 0;
	ip0->i0_nxt_lsb = 0;
	ip0->i0_seg_msb = 0;
	ip0->i0_seg_lsb = 0;

	/* point controller at iopb and start it up */

	ipaddr->ip_r1 = ((MB_DMA_ADDR(ip0)) >> 16) | IP_BUS;
	ipaddr->ip_r2 = (MB_DMA_ADDR(ip0)) >> 8;
	ipaddr->ip_r3 = (MB_DMA_ADDR(ip0));
	ipaddr->ip_r0 = IP_GO;

	while ((ip0->i0_status == 0) || (ip0->i0_status == IP_DBUSY))
		DELAY(30);
	status = ip0->i0_status;
	error = ip0->i0_error;

	ipaddr->ip_r0 = IP_CLRINT;
	if (status != IP_OK) {
		printf("ip: error %x\n", error);
		/* Attempt to reset the error condition */
		if (cmd != IP_RESTORE)
			if (ipcmd(IP_RESTORE, ipp, 0, (char *)0))
				return (-1);
		if (++errcnt < 10)
			goto retry;
		return (-1);			/* Error */
	}

	ipaddr->ip_r0 = IP_CLRINT;

	if (cmd == IP_READ && buf != bp)	/* Many just use common buf */
		bcopy(bp, buf, DEV_BSIZE);

	return (0);
}
