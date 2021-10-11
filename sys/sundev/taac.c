/*
 * Copyright (c) 1988-89, Sun Microsystems Inc., Raleigh, North Carolina
 */

/* @(#)taac.c	1.1 92/07/30 */

#ifndef lint
static char sccs_id[]="@(#)taac.c	1.1 92/07/30 Copyr 1989 Sun Micro";
static char *ncaaid = "NCAA TAAC 1.1 opts = lock/pixrects/profiler";
#endif

/*
 * TAAC-1 driver
 *
 * Rev	Date		Author	Description
 * 1.0	?? Dec 1986	???	Original
 * ?.?	???????????	???	Much time passes and changes are made.
 * 2.1	27 Feb 1989	gfo	add probe bypass for Sun3x/80 (Hydra)
 *			gfo	remove conditional compile for Pegasus
 *				(use hard-code CPUid value of Pegasus machine)
 * 2.2	14 Nov 1989	gfo	"bump" rev level for install into SunOS 4.1
 * 2.3	29 Nov 1989	gfo	Remove dependencies to <taac1/taio.h>
 */

#include "taac.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <sys/vmmac.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/kmem_alloc.h>

#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/pte.h>

#include <sundev/mbvar.h>
#include <sun/fbio.h>

/*
 * The following defines were extracted from <mon/idprom.h> of SunOS
 *	4.0.3 for Sun3x.  Since this driver must support a number of cpu
 *	architectures and OS versions, not all of which use <mon/idprom.h>
 *	and some not fully defining all architecture ids in <mon/idprom.h>,
 *	it is best for now to include the needed defines here.  These
 *	numbers do not change!
 */

#define	IDM_SUN3_M25		0x12	/* Sun-3/50 */
#define	IDM_SUN3_F		0x17	/* Sun-3/60 */
#define	IDM_SUN3X_PEGASUS	0x41	/* Sun-3x/470/480 */
#define	IDM_SUN3X_HYDRA		0x42	/* Sun-3x/80 */
#ifndef PGT_VME_D32
#define	PGT_VME_D32		0xC000000	/* == VME_D32<<26 == 0x3<<26 */
#endif


/*
 * This is a real crock but OS3 does make depend with egrep '^#include' which
 * does not understand that these includes are def'ed out. If your compiling
 * for OS3 delete the following 5 include files bracketed by 'ifdef _MAP_NEW.'
 *
 * 4.X include files
 */

#ifdef _MAP_NEW

/*
 *	These defines are extracted from /usr/include/taac1/taio.h and
 *	have been included here to avoid releasing "taio.h" and all its
 *	nested include files to the kernel.  All these numbers are fixed!
 */
#define	TA_PAGE0	0x0000
#define	TA_PAGE1	0x2000
#define	TA_PAGE2	0x4000
#define	TA_PAGE3	0x6000
#define	TA_PAGEBITS	0x6000
#define	TA_TWODIM	0x1000
#define	TIOCTEST	_IOW(t, 1, int)
#define	TIOCPROFON	_IOW(t, 2, int)
#define	TIOCPROFOFF	_IOW(t, 3, caddr_t)
#define	TIOCDEBUG	_IOW(t, 4, int)

#include <vm/seg.h>
#include <machine/seg_kmem.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memreg.h>

#else

/*
 * 3.X include files
 */

/*
 *	These defines are extracted from /usr/include/taac1/taio.h and
 *	have been included here to avoid releasing "taio.h" and all its
 *	nested include files to the kernel.  All these numbers are fixed!
 */
#define	TA_PAGE0	0x0000
#define	TA_PAGE1	0x2000
#define	TA_PAGE2	0x4000
#define	TA_PAGE3	0x6000
#define	TA_PAGEBITS	0x6000
#define	TA_TWODIM	0x1000
#define	TIOCTEST	_IOW(t, 1, int)
#define	TIOCPROFON	_IOW(t, 2, int)
#define	TIOCPROFOFF	_IOW(t, 3, caddr_t)
#define	TIOCDEBUG	_IOW(t, 4, int)

#include "/usr/include/pixrect/pixrect.h"
#include "/usr/include/pixrect/mem_rop_impl_util.h"
#include "/usr/include/pixrect/memreg.h"
#endif

#define	TAAC_PROBE_SIZE	(sizeof (long))
#define	TAAC_TOTAL_SIZE	(32*1024*1024)		/* 32 Mb */

#define	TAAC_SMR_MASK	0xffffff		/* valid bits in SMR */
#define	TAAC_SMR_RESET	0			/* reset code */
#define	TAAC_SMR_INIT	1			/* initialization code */

#define	TA_ISIZE 0x4000

#if NWIN > 0
/* SunWindows specific stuff */
struct pixrectops taac_ops = {
	taac_rop,
	taac_putcolormap,
	taac_putattributes
};
#endif (NWIN > 0)

static int pagevals[] = { TA_PAGE0, 0, 0, 0, TA_PAGE1, 0, TA_PAGE2, TA_PAGE3 };

struct taac_softc {
	int sc_debug;		/* printf on/off		*/
	unsigned sc_flag;	/* the usual status		*/
				/*    locking			*/
	int sc_uid;		/* current uid for locking	*/
	int sc_count;		/* reference count for uid	*/
				/*    profiling			*/
	int sc_offset;		/* addr of data we want		*/
	int sc_page;		/* smr page value		*/
	long sc_smrkmp;		/* kernel map for smr		*/
	long sc_kmp;		/* kernel map for data		*/
	unsigned short *sc_pbuf; /* profile trace buffe	r	*/
	int sc_pbufsize;	/* size of the trace buffer	*/
	unsigned *sc_smrp;	/* pointer to smr		*/
	unsigned *sc_datap;	/* pointer to data page		*/
	struct proc *sc_procp;	/* traceing proc		*/
	u_short *sc_pcdata;	/* trace data			*/
				/* 	pixrects		*/
	caddr_t image;		/* pointer to frame buffer	*/
	int w, h;		/* resolution			*/
	int depth;		/* bit planes available		*/
	int size;		/* frame buffer size (bytes)	*/
};

struct taac_softc taac_softc[NTAAC];

/* values for sc_flag
*/
#define	TA_PROFON 0x1

int taacdebug = 0;
int taacprobe();

struct mb_device *taacinfo[NTAAC];
struct mb_driver taacdriver = {
	taacprobe, 0, 0, 0, 0, 0,
	TAAC_PROBE_SIZE, "taac", taacinfo, 0, 0, 0, 0
};

/*ARGSUSED*/
taacprobe(reg, unit)
caddr_t reg; int unit;
{
	struct taac_softc *sc = &taac_softc[unit];
	register long *smrp;
	long smr;
	long OK;

	/* make sure we have a VME bus! */
	switch (cpu) {
	case IDM_SUN3_M25:
	case IDM_SUN3_F:
	case IDM_SUN3X_HYDRA:
	    return (0);
	}

	/*
	 * init softc
	 */
	sc->sc_uid = -1;
	sc->sc_count = 0;
	sc->sc_smrkmp = -1;
	sc->sc_kmp = -1;
	sc->sc_smrp = NULL;
	sc->sc_datap = NULL;

	sc->h = 2048;
	sc->w = 1024;
	sc->depth = 8;
	sc->size = sc->h * sc->w * sc->depth / sizeof (char);

	/*
	 * Reset the TAAC by writing 0 to the SMR.
	 * XXX There should be a more sophisticated test here to
	 * determine that this is REALLY the TAAC.
	 */
	smrp = (long *) reg;
	OK =  pokel(smrp, (long) TAAC_SMR_RESET) ||
		peekl(smrp, &smr) ||
		(smr & TAAC_SMR_MASK) != 0 ||
		(*smrp = TAAC_SMR_INIT,
			(*smrp & TAAC_SMR_MASK) != TAAC_SMR_INIT) ?
		0 : TAAC_PROBE_SIZE;

	return (OK);
}

int taaclockenable = 1;  /* patch to disable locking */

/*ARGSUSED*/
taacopen(dev, flag)
dev_t dev; int flag;
{
	struct taac_softc *sc = &taac_softc[minor(dev)];
	register struct mb_device *md;

	if (minor(dev) >= NTAAC ||
		(md = taacinfo[minor(dev)]) == 0 ||
				md->md_alive == 0)
	    return (ENXIO);

	if (taaclockenable){
	    if (sc->sc_count > 0){
		if (u.u_uid != sc->sc_uid && u.u_uid != 0){
		    return (EBUSY);
		} else {
		    sc->sc_count++;
		}
	    } else {
		sc->sc_uid = u.u_uid;
		sc->sc_count++;
	    }
	}

	sc->sc_smrp = (unsigned *)taacinfo[minor(dev)]->md_addr;

	if (taacdebug)
	    printf("taac0 (open): u: %d c: %d\n", sc->sc_uid, sc->sc_count);

	return (0);
}

/*ARGSUSED*/
taacclose(dev, flag)
{
	struct taac_softc *sc = &taac_softc[minor(dev)];

	if (taacdebug)
	    printf("taac0 (close): u: %d c: %d\n", sc->sc_uid, sc->sc_count);

	if (taaclockenable){
	    sc->sc_count = 0;
	    sc->sc_uid = -1;
	}

	if (sc->sc_flag & TA_PROFON){
	    sc->sc_flag &= ~(TA_PROFON);

	if (sc->sc_kmp)
	    rmfree(kernelmap, (long)1, (u_long)sc->sc_kmp);
	sc->sc_kmp = NULL;

	if (sc->sc_pcdata)
	    kmem_free((caddr_t)sc->sc_pcdata, (u_int)TA_ISIZE * sizeof (short));
	sc->sc_pcdata = NULL;
	}

	return (0);
}

#if defined(_MAP_NEW)

u_int hat_getkpfnum();

#endif

/*ARGSUSED*/
taacmmap(dev, off, prot)
dev_t dev; off_t off; int prot;
{
	int addr;

	if ((u_int) off >= TAAC_TOTAL_SIZE)
		return (-1);

#if defined(_MAP_NEW)
	addr = (int) hat_getkpfnum((addr_t)taacinfo[minor(dev)]->md_addr)
								+ btop(off);
#else
	addr = (getkpgmap(taacinfo[minor(dev)]->md_addr) & PG_PFNUM)
								+ btop(off);
#endif

	return (addr);
}

/*ARGSUSED*/
taacioctl(dev, cmd, data, flag)
dev_t dev; caddr_t data;
{
	struct taac_softc *sc = &(taac_softc[minor(dev)]);
	struct taac_softc *softc = &(taac_softc[minor(dev)]);
	int i, pageval, error = 0;

	switch (cmd){
	case TIOCTEST:
		if (taacdebug)
			printf("taac0: ioctl test %x\n", *((int *)data));
		break;

	case TIOCPROFON:
		if (taacdebug)
			printf("taac0: ioctl prof on %x\n", *((int *)data));

		if (sc->sc_flag & TA_PROFON)
			return (EBUSY);

		if ((sc->sc_pcdata =
		    (u_short *)kmem_alloc((u_int)TA_ISIZE * sizeof (short)))
		    == NULL)
			return (ENOMEM); /* can't happen anyway, he said */

		sc->sc_offset = *((int *)data) & 0x3fffff;
		sc->sc_page = pagevals[*((int *)data) >> 27];

		if ((sc->sc_kmp = rmalloc(kernelmap, (long)1)) == 0)
			panic("taac: out of kernelmap for devices");

		sc->sc_datap = (unsigned *)kmxtob(sc->sc_kmp);

#ifdef _MAP_NEW
		pageval = hat_getkpfnum(taacinfo[0]->md_addr)
		    + btop(sc->sc_offset + 0x1000000);
		/*
		 *  Pegasus use MC68030 memory management unit which does not
		 *	use page table type bits.
		 */
		if ((cpu != IDM_SUN3X_PEGASUS))
			pageval |= PGT_VME_D32;

		segkmem_mapin(&kseg, (addr_t)sc->sc_datap, MMU_PAGESIZE,
		    (u_int)PROT_READ | PROT_WRITE, (u_int)pageval, 0);
#else
		pageval = PGT_VME_D32 |
		    ((getkpgmap(taacinfo[minor(dev)]->md_addr) & PG_PFNUM) +
		    btop(0x1000000 + sc->sc_offset));
		sc->sc_offset &= 0x1fff;
		mapin(&Usrptmap[sc->sc_kmp], btop(sc->sc_datap),
		    pageval, 1, PG_V|PG_KW);
#endif

		sc->sc_flag |= TA_PROFON;
		sc->sc_procp = u.u_procp;

		for (i=0; i<TA_ISIZE; i++)
			(sc->sc_pcdata)[i] = 0;

		taactimeout(sc);
		break;

	case TIOCPROFOFF:
		if (taacdebug)
			printf("taac0: ioctl prof off %x\n", *((int *)data));

		/*
		 * TODO: get our interupt priority up
		 */
		sc->sc_flag &= ~(TA_PROFON);

		if (sc->sc_kmp)
			rmfree(kernelmap, (long)1, (u_long)(sc->sc_kmp));
		sc->sc_kmp = NULL;

		if (copyout((caddr_t)(sc->sc_pcdata), *((caddr_t *)data),
		    TA_ISIZE * sizeof (short)))
		error = EFAULT;

		if (sc->sc_pcdata)
			kmem_free((caddr_t)sc->sc_pcdata,
			    TA_ISIZE * sizeof (short));
		sc->sc_pcdata = NULL;

		break;

	case TIOCDEBUG:
		sc->sc_debug = *((int *)data);
		break;

	case FBIOGTYPE:
		{
			register struct fbtype *fb = (struct fbtype *) data;

			/* fb->fb_type = FBTYPE_SUNTAAC; */
			fb->fb_type = FBTYPE_NOTSUN3;
			fb->fb_height = softc->h;
			fb->fb_width = softc->w;
			fb->fb_depth = softc->depth;
			fb->fb_cmsize = 2 << softc->depth;
			fb->fb_size = softc->size;
		}
		break;

#if NWIN > 0

	case FBIOGPIXRECT:
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		softc->pr.pr_ops = &taac_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;
		softc->pr.pr_depth = softc->depth;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		softc->prd.md_linebytes = mpr_linebytes(softc->w, 1);
		softc->prd.md_image = (short *) softc->image;
		softc->prd.md_offset.x = 0;
		softc->prd.md_offset.y = 0;
		softc->prd.md_primary = 0;
		softc->prd.md_flags = MP_DISPLAY;
		break;

#endif NWIN > 0

	/*
	 * TODO: return hz
	 */
	default:
		return (ENOTTY);
	}

	return (error);
}

taacread()
{

}

taacwrite()
{

}

taactimeout(sc)
struct taac_softc *sc;
{
	unsigned oldsmr;

	if (!sc->sc_flag & TA_PROFON)
		return;

	/*
	s = spl7();
	*/

	oldsmr = *(sc->sc_smrp);
	*(sc->sc_smrp) &= ~(TA_PAGEBITS | TA_TWODIM);
	*(sc->sc_smrp) |= sc->sc_page;

	(sc->sc_pcdata)[(*(sc->sc_datap)) & 0x3fff]++;

	if (taacdebug)
		printf("timeout: %x\n", (*(sc->sc_datap)) & 0x3fff);

	*(sc->sc_smrp) = oldsmr;

	/*
	splx(s);
	*/

	timeout(taactimeout, (caddr_t)sc, 1);

	return;
}
