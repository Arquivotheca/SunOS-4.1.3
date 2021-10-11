#ifndef ECBOOT
#ifndef lint
static char sccsid[] = "@(#)if_ec.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <stand/saio.h>
#include <stand/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <sunif/if_ecreg.h>
#include <mon/sunromvec.h>

#ifdef OPENPROMS
#define	millitime() prom_gettime()
#else
#define	millitime() (*romp->v_nmiclock)
#endif !OPENPROMS

int ecxmit(), ecpoll(), ecreset(), myetheraddr(), ecstats();

unsigned long ecstd[] = { 0xE0000, 0xE2000 };
#define	NSTD	2

struct ec_softc {
	char	es_proto[PROTOSCRATCH];	/* for higher protocols */
	struct ecdevice	*es_ec;
	struct	saioreq	*es_sip;
};

struct devinfo ecinfo = {
	sizeof (struct ecdevice),	/* size */
	0,				/* DMA */
	sizeof (struct ec_softc),	/* software */
	NSTD,
	ecstd,
	MAP_MBMEM,
	0,				/* transfer size handled by ND */
};

int xxprobe(), xxboot(), ecopen(), nullsys(), etherstrategy();
int nullsys();

struct boottab ecdriver = {
	"ec", xxprobe, xxboot, ecopen, nullsys,
	etherstrategy, "ec: 3COM Ethernet", &ecinfo,
};

struct saif ecif = {
	ecxmit,
	ecpoll,
	ecreset,
	myetheraddr,
	ecstats
};

#define	CSRSET(v) ec->ec_csr = (ec->ec_csr & EC_INTPA) | (v)
#define	CSRCLR(v) ec->ec_csr = ec->ec_csr & (EC_INTPA & ~(v))

ecopen(sip)
	struct saioreq *sip;
{
	register struct ec_softc *es;

	es = (struct ec_softc *)sip->si_devdata;
	es->es_ec = (struct ecdevice *)sip->si_devaddr;
	es->es_sip = sip;
	sip->si_sif = &ecif;
	ecreset(es);
	return (etheropen(sip));
}


ecreset(es)
	register struct ec_softc *es;
{
	register struct ecdevice *ec = es->es_ec;

	ec->ec_csr = EC_RESET;
	DELAY(20);
	myetheraddr(&ec->ec_aram);
	CSRSET(EC_AMSW);
	CSRSET(EC_PA | EC_ABSW | EC_BBSW);
}

ecxmit(es, buf, count)
	struct ec_softc *es;
	caddr_t buf;
	int count;
{
	register struct ecdevice *ec = es->es_ec;
	caddr_t cp;
	int time = millitime() + 500;	/* .5 seconds */
	short mask = -1, back;

	cp = (caddr_t)&ec->ec_abuf[-count];
	bcopy(buf, cp, count);
	*(short *)(ec->ec_tbuf) = cp - (caddr_t)ec->ec_tbuf;
	CSRSET(EC_TBSW);
	for (;;) {
		if (millitime() > time) {
			ecreset(es);
			return (-1);
		}
		if (ec->ec_csr & EC_JAM) {
			mask <<= 1;
			if (mask == 0) {
				ecreset(es);
				return (-1);
			}
			back = -(millitime() & ~mask);
			if (back == 0)
				back = -(0x5555 & ~mask);
			ec->ec_back = back;
			CSRSET(EC_JAM);
		}
		if ((ec->ec_csr & EC_TBSW) == 0)
			return (0);
	}
}

ecpoll(es, buf)
	struct ec_softc *es;
	char *buf;
{
	register struct ecdevice *ec = es->es_ec;
	short len, *sp;
	int xbsw = 0;

	if ((ec->ec_csr & EC_ABSW) == 0) {
		sp = (short *)ec->ec_abuf;
		xbsw = EC_ABSW;
	} else if ((ec->ec_csr & EC_BBSW) == 0) {
		sp = (short *)ec->ec_bbuf;
		xbsw = EC_BBSW;
	} else
		return (0);
	len = *sp++ & EC_DOFF;
	bcopy((char *)sp, buf, len);
	CSRSET(xbsw);
	return (len);
}

ecstats()
{
	/* XXX implement sometime */
}
