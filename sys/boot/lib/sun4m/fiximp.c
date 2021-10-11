#ifndef lint
static char sccsid[] = "@(#)fiximp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/mmu.h>
#include <mon/idprom.h>
*/
#include <sys/types.h>
#include <machine/param.h>
#include <machine/clock.h>
#include <mon/sunromvec.h>
#include <sun/openprom.h>

/*
 * macro definitions for routines that form the OBP interface
 */
#define	NEXT			prom_nextnode
#define	CHILD			prom_childnode
#define	GETPROP			prom_getprop
#define	GETPROPLEN		prom_getproplen

/*
 * The following variables are machine-dependent, and are used by boot
 * and kadb. They may also be used by other standalones. Modify with caution!
 */

/*
 * This variable is used in usec_delay().  See the chart in lib/sparc/misc.s.
 * Srt0.s sets this value based on the actual runtime environment encountered.
 * It's critical that the value be no SMALLER than required, e.g. the
 * DELAY macro guarantees a MINIMUM delay, not a maximum.
 */
#define	NWINDOWS_DEFAULT	8
#define	VAC_DEFAULT		0	/* there is *no* vac */
#define	VACSIZE_DEFAULT		2048
#define	VACLINESIZE_DEFAULT	32
#define	SEGMASK_DEFAULT		0x1ff

int Cpudelay = 0;
int nwindows = NWINDOWS_DEFAULT;
int vac = VAC_DEFAULT;
int vac_size = VACSIZE_DEFAULT;
int vac_linesize = VACLINESIZE_DEFAULT;
int segmask = SEGMASK_DEFAULT;

/*
 * The next group of variables and routines handle the
 * Open Boot Prom devinfo or property information.
 */
int debug_prop = 0;		/* Turn on to enable debugging message */

#define	CLROUT(a, l)			\
{					\
	register int c = l;		\
	register char *p = (char *)a;	\
	while (c-- > 0)			\
		*p++ = 0;		\
}

#define	CLRBUF(a)	CLROUT(a, sizeof a)

static void searchtree(), findprop();

fiximp()
{

	searchtree(NEXT(0));	/* Get the root node*/
}

static void
searchtree(curnode)
	register int curnode;
{
#define	MAXNAME 32
	u_char	 tmp_name[MAXNAME];

	for (curnode = CHILD(curnode); curnode; curnode = NEXT(curnode)) {
		CLRBUF(tmp_name);
		if ((void)getprop(curnode, "name", tmp_name) != -1) {
			if (!strcmp(tmp_name, "processor")) {
				findprop(curnode);
				return;
			}
		}
	}

	/* Can not find the "processor" node, use default value */
}

static void
findprop(curnode)
	register int curnode;
{
	(void)getprop(curnode, "ncaches", &vac);
	(void)getprop(curnode, "mmu-nctx", &nwindows);
	(void)getprop(0, "cache-nlines", &vac_size);
	(void)getprop(0, "cache-linesize", &vac_linesize);
	if (debug_prop) {
		printf("nwindows %d\n", nwindows);
		printf("vac %d\n", vac);
		printf("cache-nlines %d\n", vac_size);
		printf("cache_linesize %d\n", vac_linesize);
	}
	if (vac)
		setcpudelay();
}

/*
 * set delay constant for usec_delay()
 * NOTE: we use the fact that the per-
 * processor clocks are available and
 * mapped properly at "utimers".
 */
static
setcpudelay()
{
	register struct count14 *utimers = (struct count14 *)0;
	register unsigned r;    /* timer resolution, ~ns */
	register unsigned e;    /* delay time, us */
	register unsigned es;   /* delay time, ~ns */
	register unsigned t;    /* for time measurement */
	int		nreg, space, nrng;
	addr_t		paddr;
	u_int		tomap, base_addr;
	int		nodeid;
#define	MAXRANGE	8
	struct dev_reg  dev_reg[MAXRANGE];
	struct dev_reg  dev_rng[MAXRANGE];
	extern caddr_t prom_map();

#ifdef noneed
	nrng = getnode(NEXT(0), "obio", dev_rng, &nodeid);
	if (nrng) {
		space = (int)dev_rng[0].reg_bustype;
		base_addr = (u_int)dev_rng[0].reg_addr;
	} else {
		space = 0;
		base_addr = 0;
	}

	if (nodeid) {
		nreg = getreginfo(nodeid, "counter", dev_reg);
		if (nreg) {
			paddr = base_addr + dev_reg[0].reg_addr;
			tomap = dev_reg[0].reg_size;
			(int)utimers = (int)prom_map(COUNTER_ADDR,
				space, paddr, tomap);
		}
	}
#endif noneed

	if ((int)utimers == 0 || (int)utimers == -1)
		utimers = (struct count14 *)COUNTER_ADDR;
	r = 512;		/* worst supported timer resolution */
	es = r * 100;		/* target delay in ~ns */
	e = ((es+1023) >> 10);	/* request delay in us, round up */
	es = e << 10;		/* adjusted target delay in ~ns */
	Cpudelay = 1;		/* initial guess */
	DELAY(1);		/* first time may be weird */
	do {
		Cpudelay <<= 1; /* double until big enough */
		t = utimers->timer_lsw;
		DELAY(e);
		t = utimers->timer_lsw - t;
	} while (t < es);
	Cpudelay = (Cpudelay * es + t) / t;
	if (Cpudelay < 0)
		Cpudelay = 0;
}

#define	MAXSYSNAME	32
int
getnode(curnode, namep, rngp, nodeidp)
	int	curnode;
	char	*namep;
	struct dev_reg  *rngp;
	int	*nodeidp;
{
	register int nrng = 0;
	u_char	tmp_name[MAXSYSNAME];

	for (curnode = CHILD(curnode); curnode; curnode = NEXT(curnode)) {
		CLRBUF(tmp_name);
		if (GETPROP(curnode, "name", tmp_name) != 1)
			if (strcmp(tmp_name, namep))
				continue;
			else {
				nrng = GETPROPLEN(curnode, "range") /
					sizeof (struct dev_reg);
				if (nrng > 0)
					GETPROP(curnode, "range", rngp);
				else
					rngp = (struct dev_reg *)0;
				*nodeidp = curnode;
				return (nrng);
			}
	}
	return (0);
}
int
getreginfo(curnode, namep, dev_reg)
	int	curnode;
	char	*namep;
	struct	dev_reg	*dev_reg;
{
	register	int	nreg = 0;
	u_char	tmp_name[MAXSYSNAME];

	for (curnode = CHILD(curnode); curnode; curnode = NEXT(curnode)) {
		CLRBUF(tmp_name);
		if (GETPROP(curnode, "name", tmp_name) != 1)
			if (strcmp(tmp_name, namep))
				continue;
		nreg = GETPROPLEN(curnode, "reg") / sizeof (struct dev_reg);
		if (nreg > 0)
			GETPROP(curnode, "reg", dev_reg);
		else
			dev_reg = (struct dev_reg *)0;
		break;
	}
	return (nreg);
}
