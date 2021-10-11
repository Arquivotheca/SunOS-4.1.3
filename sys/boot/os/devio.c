#ifndef lint
static	char sccsid[] = "@(#)devio.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Device interface code for standalone I/O system.
 *
 * Most simply indirect thru the table to call the "right" routine.
 */

#include <mon/sunromvec.h>
#include <stand/saio.h>
#include "param.h"
#include <sys/dir.h>
#include <sys/time.h>
#include "boot/vnode.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include "boot/iob.h"
#ifdef OPENPROMS
#include "boot/conf.h"
#endif OPENPROMS

#ifdef DUMP_DEBUG
static	int	dump_debug = 10;
#endif DUMP_DEBUG


/*
 * Strategy -- handles I/O in large blocks for drivers.
 * (This routine is private to this file.)
 *
 * If a devread() or devwrite() is attempted which is larger
 * than the max I/O size for this device, break it up into a
 * series of max-sized operations.
 *
 * Devices which do not declare a max size get the whole thing.
 */
static int
strategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	register char *ma;
	register unsigned cc;
	register daddr_t bn;
	register int errs;
	register unsigned maxsize;
	register struct devinfo *dp;
#ifdef	 DUMP_DEBUG
	dprint(dump_debug,  6, "strategy(sip 0x%x rw 0x%x)\n",
		sip, rw);
#endif	 /* DUMP_DEBUG */

	if (rw == READ && (sip->si_flgs & F_EOF)) {
		sip->si_flgs &= ~F_EOF;

		return (0);
	}
	ma = sip->si_ma;			/* Save for later */
	bn = sip->si_bn;
	cc = sip->si_cc;

	dp = sip->si_boottab->b_devinfo;
	if (dp && (maxsize = dp->d_maxiobytes) != 0 &&
		((maxsize & (DEV_BSIZE-1)) == 0))
		;
	else
		maxsize = 0x7FFFFFFF;
	errs = 0;
	while (sip->si_cc > 0) {
		if (sip->si_cc > maxsize)
			sip->si_cc = maxsize;
		errs = (*sip->si_boottab->b_strategy)(sip, rw);
		/* short read is expected for 1/2" tape */
		if (errs <= 0)		/* error or EOF */
			break;
		sip->si_ma += errs;
		sip->si_bn += errs / DEV_BSIZE;
		sip->si_cc = cc - (sip->si_ma - ma);
	}
	if (errs == 0)
		sip->si_flgs |= F_EOF;
	if (errs >= 0)
		errs = sip->si_ma - ma;		/* Add the part we did before */
	if (errs == 0)
		sip->si_flgs &= ~F_EOF;
	sip->si_ma = ma;			/* Restore */
	sip->si_bn = bn;
	sip->si_cc = cc;
	return (errs);
}

int
devread(sip)
	struct saioreq *sip;
{

	return (strategy(sip, READ));
}

int
devwrite(sip)
	struct saioreq *sip;
{

	return (strategy(sip, WRITE));
}

int
devopen(sip)
	register struct saioreq *sip;
{
	register struct devinfo *dp;
	register char *a;
	register int result;

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 10,
		"devopen(sip 0x%x)\n",
		sip);
	dump_saioreq(sip);
#endif	/* DUMP_DEBUG */

	sip->si_flgs &= ~F_EOF;
#ifdef OPENPROMS
	if (prom_getversion() > 0)
		sip->si_devaddr = sip->si_dmaaddr = (char *)0;
	else
#endif
	sip->si_devaddr = sip->si_devdata = sip->si_dmaaddr = (char *)0;

	dp = sip->si_boottab->b_devinfo;
	if (dp) {
		/* Map controller number into controller address */
		if (sip->si_ctlr < dp->d_stdcount) {
			sip->si_ctlr = (int)((dp->d_stdaddrs)[sip->si_ctlr]);
		}
		/* Map in device itself */
		if (dp->d_devbytes) {
			a = devalloc(dp->d_devtype, (char *)sip->si_ctlr,
				dp->d_devbytes);
			if (!a)
				goto bad;
			sip->si_devaddr = a;
		}
		if (dp->d_dmabytes) {
			a = resalloc(RES_DMAMEM, dp->d_dmabytes);
			if (!a)
				goto bad;
			sip->si_dmaaddr = a;
		}
		if (dp->d_localbytes) {
			a = resalloc(RES_MAINMEM, dp->d_localbytes);
			if (!a)
				goto bad;
			sip->si_devdata = a;
		}
	}

	result = (sip->si_boottab->b_open)(sip);
	if (result != -1)
		return (result);
bad:
	return (-1);		/* Indicate failure */
}

devclose(sip)
	register struct saioreq *sip;
{

	sip->si_flgs &= ~F_EOF;
	if (sip->si_boottab->b_close)
		(sip->si_boottab->b_close)(sip);
}

/*
 * If the device is an ethernet interface (hard to tell, isn't it?),
 * reset it. This restarts transmitters after they've been shut off.
 */
devreopen(sip)
	register struct saioreq *sip;
{
	sip->si_flgs &= ~F_EOF;
	if (sip->si_boottab && sip->si_boottab->b_open)
		(sip->si_boottab->b_open)(sip);
}

dump_iob(f)
	struct	iob	*f;
{

	printf("iob: 0x%x\n", f);
	dump_saioreq(&(f->i_si));

}

dump_saioreq(s)
	struct	saioreq	*s;
{

	printf("saioreq: si_flgs 0x%x\n",
		s->si_flgs);
	dump_boottab(s->si_boottab);
	if (s->si_devdata)
#ifdef OPENPROMS
		if (prom_getversion() > 0)
			dump_devdata(s->si_devdata);
		else
#endif OPENPROMS
		printf("saioreq: si_devdata '%s'\n", s->si_devdata);
	else
		printf("saioreq: si_devdata '  '\n");
	printf("saioreq: si_ctlr 0x%x si_unit 0x%x si_boff 0x%x\n",
		s->si_ctlr, s->si_unit, s->si_boff);
	printf("saioreq: si_cyloff 0x%x si_offset 0x%x si_bn 0x%x si_ma 0x%x ",
		s->si_cyloff, s->si_offset, s->si_bn, s->si_ma);
	printf("si_cc 0x%x\n", s->si_cc);
	if (s->si_sif)
		dump_saif(s->si_sif);
	else
		printf("saif:\n");
	printf("saioreq: si_devaddr 0x%x si_dmaaddr 0x%x\n",
		s->si_devaddr, s->si_dmaaddr);

}

dump_saif(s)
	struct  saif *s;
{
	printf("saif: sif_xmit 0x%x sif_poll 0x%x sif_reset 0x%x\n",
		s->sif_xmit, s->sif_poll, s->sif_reset);
}

dump_bootparam(bp)
	struct bootparam *bp;
{
	printf("bootparam: bp_argv[0] 0x%x bp_argv[1] 0x%x bp_argv[2] 0x%x\n",
		bp->bp_argv[0], bp->bp_argv[1], bp->bp_argv[2]);
	printf("bootparam: bp_argv[3] 0x%x bp_argv[4] 0x%x bp_argv[5] 0x%x\n",
		bp->bp_argv[3], bp->bp_argv[4], bp->bp_argv[5]);
	printf("bootparam: bp_argv[6] 0x%x bp_argv[7] 0x%x\n",
		bp->bp_argv[6], bp->bp_argv[7]);
	printf("bootparam: bp_strings %s\n", bp->bp_strings);
	printf("\tbp_dev %c%c bp_ctlr 0x%x bp_unit 0x%x bp_part 0x%x\n",
		bp->bp_dev[0], bp->bp_dev[1], bp->bp_ctlr, bp->bp_unit,
		bp->bp_part);
	printf("bootparam: bp_name %s\n", bp->bp_name);
	dump_boottab(bp->bp_boottab);
}

dump_boottab(b)
	struct	boottab *b;
{

	printf("boottab: dev %c%c b_probe 0x%x b_boot 0x%x b_open 0x%x\n",
		b->b_dev[0], b->b_dev[1], b->b_probe, b->b_boot, b->b_open);
	printf("boottab: b_close 0x%x b_strategy 0x%x\n",
		b->b_close, b->b_strategy);
	if (b->b_desc)
		printf("boottab: b_desc '%s'\n", b->b_desc);
	else
		printf("boottab: b_desc '  '\n");
	if (b->b_devinfo)
		dump_devinfo(b->b_devinfo);
	else
		printf("devinfo:\n");
}

dump_devinfo(d)
	struct	devinfo *d;
{

	printf("devinfo: d_devbytes 0x%x d_dmabytes 0x%x d_localbytes 0x%x\n",
		d->d_devbytes, d->d_dmabytes, d->d_localbytes);
	dump_stdaddrs((unsigned long *)d->d_stdaddrs, (int)d->d_stdcount);
	printf("devinfo: d_devtype 0x%x d_maxiobytes 0x%x\n",
		d->d_devtype, d->d_maxiobytes);
}

dump_stdaddrs(vec, count)
	unsigned long   *vec;
	int	count;
{

	printf("stdaddrs: ");
	for (; count > 0; count--, vec++)
		printf("d_stdaddr[%d] 0x%x ",
			count, *vec);
	printf ("\n");
}

#ifdef OPENPROMS
dump_devdata(dp)
	struct binfo *dp;
{
	printf("devdata: ihandle 0x%x name '%s'\n", dp->ihandle, dp->name);
}
#endif
