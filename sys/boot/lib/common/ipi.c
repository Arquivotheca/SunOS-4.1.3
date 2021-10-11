/*
 * @(#)ipi.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * ip.c - Boot PROM Driver for IPI Disk controller.
 */
#include <sys/types.h>
#include <stand/saio.h>
#include <sys/buf.h>
#include <sys/dkbad.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/ipdev.h>
#include <sundev/ipvar.h>

#define	DEV_BSIZE	512

#define LABEL
/* #define SUPERFAST */				/* For very high speed boot */
/* #define DEBUG     */				/* For debug purposes	    */

#ifdef	SUPERFAST
#define	BIGSIZE		(256)			/* size in blocks (512) is ok with DVMA */
int	gotcyl = -1;
#endif


struct ipidma {
	struct	ipi3pkt	    	ipi3pkt;
	u_long		        pad_to_align1;
#ifndef	SUPERFAST
	char		        buff_addr[8196];		    /* R/W data */
#else
	char		        buff_addr[BIGSIZE * 512];	    /* R/W data */
#endif
	u_long		        pad_to_align3;
};
    
#define	NSTD	4
u_long  ipistd[NSTD]  = { 0x1080000, 0x1080400, 0x1080800, 0x1080c00
		 	};

struct devinfo ipinfo = {
	sizeof (struct ipidevice),	/* size of register I/O space */ 
	sizeof (struct ipidma),		/* size of DMA memory */
	1024,				/* size of parameter block */
	NSTD,			        /* # of standard addresses */
	ipistd,			        /* vector of standard addresses */
	MAP_VME32A32D,		 	/* which map space */
	DEV_BSIZE * 16			/* transfer size 8k */
};

/* 
 * external functions that are called by the driver interfaces.
 */

extern int	xxboot(), xxprobe();
extern int	nullsys();
int		ipiopen(), ipistrategy(), ipiready();
extern int	await();


/*
 * boottab structure that is used in the boot routine.
 */

struct boottab ipidriver = {
	"id",	xxprobe, xxboot, ipiopen, nullsys, ipistrategy,
	"id: PANTHER (ipi) controller", &ipinfo,
}; 


#define ADDR_DVMA(x)	(u_long)((u_long)x & ~0xfff00000)
#define	ALIGN(x, i)	(u_long)(((u_long)x + (u_long)i) & ~(u_long)(i - 1))
#define HOWMANY(x, y)   ((((u_int)(x))+(((u_int)(y))-1))/((u_int)(y)))


#define CTLR(sip)	( ( (sip->si_ctlr) >> 8) & 0xf) / 4  /* No kidding */
#define UNIT(sip)	(sip->si_unit & 07)

#define TIMEOUT		0xfffff			/* timeout location */
#define LOW		0
#define HIGH		1

/*
 * Description: opens the IPI disk controller
 * Synopsis:	status = ipiopen(sip)
 *		status	:(int) 0 = opened
 *			      -1 = error
 *		sip	:(int *) point to saioreq structure
 * Routines:	bzero	
 */
ipiopen(sip)
	register struct saioreq	    *sip;
{
	register struct ipidevice    *ipaddr;
	caddr_t			    xfer_ma;
	struct	dk_label	     *dkptr;
	int			     opcode = IP_READ;
	int			     k;
	int			     i = 0;


	ipaddr = (struct ipidevice *)sip->si_devaddr;

#ifndef BOOTBLOCK
	/* Verify that ipi is really present */
	if (peekl((char *)&ipaddr->dev_csr) == -1)
		return (-1);
#endif	!BOOTBLOCK

	/* Wait for CSR_RESET bit to go LOW for 1000000 loops
	 * after which barf "IPI reset"
	*/
	if (await (&ipaddr->dev_csr, CSR_RESET, LOW, 70000000, "reset")) {
	    return(-1);			/* waited too long */
	}

	k = ipaddr->dev_resp;
#ifdef lint
	k = k;
#endif	
	while ( i++ < 100000) {
	    if (ipiready(sip)) break;
	}

	if (!isspinning(ipiready, sip)) {
		return(-1);
	}

#ifdef LABEL
	dkptr = (struct dk_label *)sip->si_devdata;

	xfer_ma	= (caddr_t)(((struct ipidma *)(sip->si_dmaaddr))->buff_addr);
	xfer_ma = (caddr_t)ALIGN(xfer_ma, 4);

	ipicmd (opcode|(RWMOD <<8), IP_SYNC, CTLR(sip), UNIT(sip), 0,
		2, (caddr_t)xfer_ma,		sip);

	bcopy(xfer_ma, dkptr, 2 * DEV_BSIZE);

	if ( dkptr->dkl_magic !=  0xDABE){
		printf("Bad label\n");
		return(-1);
	}

#endif

	return(0);
}

ipiready(sip)
	struct	saioreq		*sip;
{
	int			    status;

	status  = ipicmd (IP_REPORT_STAT|(RWMOD <<8), IP_SYNC, CTLR(sip), UNIT(sip),
		0, 2, (caddr_t)0,  sip);

	return(status);
	
}


/*
 * Description:	Executes read or write on the IPI controller
 * Synopsis:	status = ipistrategy(sip, rw)
 *		status	:(int)	= 0, failure
 *				= 1, success (count==0)
 *				= character count read, success
 *		sip	:(int *) pointer to saio stucture
 *		rw	:(int)	 read/write command code (overwritten)
 * Routines:	ipicmd 
 */

int
ipistrategy(sip, rw)
	struct saioreq *sip;
	register int rw;
{
	register int opcode = (rw == WRITE) ? IP_WRITE : IP_READ;
	register int blkno, count, boff, part;
	struct	 dk_label   *dkptr;
	caddr_t		ma, xfer_ma;

#ifdef	SUPERFAST
	int		savblkno, savcount, cylno;
#endif


#ifdef LABEL
	dkptr = (struct dk_label *)sip->si_devdata;
	part  = sip->si_boff;
	boff  = dkptr->dkl_map[part].dkl_cylno
		* dkptr->dkl_nsect * dkptr->dkl_nhead;
#endif

	blkno = sip->si_bn + boff;
	count = sip->si_cc / DEV_BSIZE;

	if ((blkno + count) >= (boff + dkptr->dkl_map[part].dkl_nblk))
		return(0);

	ma    = sip->si_ma;

	xfer_ma	= (caddr_t)(((struct ipidma *)(sip->si_dmaaddr))->buff_addr);
	xfer_ma = (caddr_t)ALIGN(xfer_ma, 4);

	if (opcode == IP_WRITE)
		bcopy(ma, xfer_ma, count*DEV_BSIZE);

#ifdef	SUPERFAST				    /* get the cyl */
	savblkno = blkno;
	savcount = count;

	cylno = blkno / BIGSIZE ;

	count = BIGSIZE;
	blkno = cylno * BIGSIZE;

	if (cylno != gotcyl) {
#endif

	ipicmd (opcode|(RWMOD <<8), IP_SYNC, CTLR(sip), UNIT(sip), blkno,
		count, (caddr_t)xfer_ma,  sip);

#ifdef SUPERFAST
	}

	blkno	= savblkno;
	count = savcount;

	gotcyl	= cylno;
	xfer_ma	+= (blkno % BIGSIZE)*512;

#endif

	if (opcode == IP_READ)
		bcopy(xfer_ma, ma, count*DEV_BSIZE);

	return(count * DEV_BSIZE);
}


/*
 * Description:	Internal interface to the IPI controller command set
 *
 * Synopsis:	status = ipicmd(opcode, mode, ctlr, unit, blkno, count, bufaddr, sip)
 *		status	:(int)	= -1, severe unretryable failure
 *				= 0, failure
 *				= 1, success (count==0)
 *				= character count read, success
 *		opcode	:(int) command code
 *              mode    : IP_SYNC
 *		sip	:(int *) pointer to saio stucture
 *		blkno	:(int) block number
 *		count	:(int) number of sectors
 * Routines:	bzero
 */

int
ipicmd(opcode, mode, ctlr, unit, blkno, count, bufaddr, sip)
u_long	opcode, mode;
int	ctlr, unit, blkno, count;
caddr_t	bufaddr;
struct	saioreq	*sip;
{
	struct  ipi3pkt *pktptr;
	struct  ippcb	pcb;
	struct	ippcb *ppcb = &pcb;
	

	/* pcb and DVMA is already available */

	bzero(ppcb, sizeof(struct ippcb));

	pktptr	    = &(((struct ipidma *)(sip->si_dmaaddr))->ipi3pkt);
	pktptr	    = (struct ipi3pkt *)ALIGN(pktptr,  4);

	bzero(pktptr, sizeof(struct ipi3pkt));

	ppcb->pcb_ppkt	= pktptr;
        ppcb->pcb_mode  = mode;

	pktptr->pkt_hdr.hdr.hdr_refno	= 0xBAD;
	pktptr->pkt_hdr.hdr.hdr_opcode	= opcode & 0xff;
	pktptr->pkt_hdr.hdr.hdr_mods	= 1;
	pktptr->pkt_hdr.hdr.hdr_ctlr	= ctlr;
	pktptr->pkt_hdr.hdr.hdr_unit	= unit;

	switch(opcode & 0xff) {
	    case IP_READ:
	    case IP_WRITE:
		pktptr->pkt_hdr.hdr.hdr_pktlen	= sizeof(struct read_write) + 6;
                pktptr->p_cmd.rdwr.xnp.pad	= 0;
                pktptr->p_cmd.rdwr.xnp.id	= XFER_NOTIFY;
                pktptr->p_cmd.rdwr.xnp.len	= 0x5;
                pktptr->p_cmd.rdwr.xnp.bufadr	= (caddr_t)ADDR_DVMA(bufaddr);
                pktptr->p_cmd.rdwr.cxp.pad	= 0;
                pktptr->p_cmd.rdwr.cxp.id	= CMD_EXTENT;
                pktptr->p_cmd.rdwr.cxp.len	= 0x9;
                pktptr->p_cmd.rdwr.cxp.blk_cnt	= count;
                pktptr->p_cmd.rdwr.cxp.lbn	= blkno;
                break;
	    case IP_REPORT_STAT:
		pktptr->pkt_hdr.hdr.hdr_pktlen	= 6;
		break;
	    default:
		break;
	}


	return ( ipigo(ppcb, sip) ? count * DEV_BSIZE : 0 ); 
}
 
int
ipigo(ppcb,  sip)
struct ippcb	    *ppcb;
struct saioreq	    *sip;
{
	register struct ipidevice   *ipaddr;
	int			     k;
	u_long			    *pptr;

	ipaddr = (struct ipidevice *)sip->si_devaddr;

	if (await (&ipaddr->dev_csr, CSR_CRBUSY, LOW, 1000000, "device too busy")) {
	    return(1);			/* waited too long */
	}


	ipaddr->dev_cmdreg   = ADDR_DVMA(ppcb->pcb_ppkt);

	if (await(&ipaddr->dev_csr, CSR_CRBUSY, LOW, 10000000, "device still busy")){
	    return(0);			/* waited too long */
	}
	if (await(&ipaddr->dev_csr, CSR_RRVLID, HIGH, 10000000, "no response")) {
	    return(0);			/* waited too long */
	}


	if (ipaddr->dev_csr & CSR_ERROR) {
		ipaddr->dev_csr |= CSR_RESET;
		printf("Error/doing reset\n");
		k = ipaddr->dev_resp;
		return(0);
	}

	pptr	= (u_long *)&ipaddr->dev_resp_pkt;


#define IP_NOTOPER  0x40000000		    /* goes in include file */
#define	IP_NOTREADY 0x10000000

	k = ipaddr->dev_resp;

#ifdef lint
        k = k;
#endif

	if ((pptr[1] & 0xffff0000) ==  0x3010000 ) {

	    if (pptr[3] & IP_NOTOPER ) {
		return(0);
	    }

	    if (pptr[3] & IP_NOTREADY)    {
		return(0);
	    }
	}


	return(1);

}


await(paddr, val, ord, timeout, s)
long	*paddr;
u_long	val, ord;
int	timeout;
char	*s;
{
    
	int i;

	for (i=0; i < timeout; i++) {

	    if (ord) {
		if ( (*paddr & val)) return(0); else continue;
	    }
	    else {
		if (!(*paddr & val)) return(0); else continue;
	    }
	}

	printf("timeout: ipi: %s\n", s);
	return(1);

}




#ifdef DEBUG

pr_pkt(pptr)
u_long *pptr;
{
	int i;

        printf("pptr %x:: ", pptr);
        for (i=0; i< 12; i++) {
               printf("%x ", *pptr++);
	}
	printf("\n");

}
#endif
