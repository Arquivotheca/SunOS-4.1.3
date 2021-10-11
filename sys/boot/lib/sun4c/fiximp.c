#ifndef lint
static	char sccsid[] = "@(#)fiximp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <machine/asm_linkage.h>
#include <machine/param.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/trap.h>
#include <machine/mmu.h>
#include <mon/idprom.h>

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
#define	CPUDELAY_DEFAULT	11	/* worst case value (33 Mhz) */
#define	NWINDOWS_DEFAULT	8
#define	NPMGRPS_DEFAULT		NPMGRPS_60
#define	VAC_DEFAULT		0	/* there is *no* vac */
#define	VACSIZE_DEFAULT		VAC_SIZE_60
#define	VACLINESIZE_DEFAULT	VAC_LINESIZE_60
#define	SEGMASK_DEFAULT		0x1ff
#define	MIPS_DEFAULT		CPU_MAXMIPS_25MHZ

int Cpudelay = CPUDELAY_DEFAULT;
int nwindows = NWINDOWS_DEFAULT;
u_int npmgrps = NPMGRPS_DEFAULT;
int vac = VAC_DEFAULT;
int vac_size = VACSIZE_DEFAULT;
int vac_linesize = VACLINESIZE_DEFAULT;
int segmask = SEGMASK_DEFAULT;
int mips_off = MIPS_DEFAULT;	/* approx. MIPS with cache off/on */
int mips_on = MIPS_DEFAULT;

/*
 * The next group of variables and routines handle the
 * Open Boot Prom devinfo or property information.
 */
int debug_prop = 0;		/* Turn on to enable debugging message */

void
fiximp()
{
	int freq;

	/*
	 * Trapped in a world we never made! Determine the physical
	 * constants which govern this universe.
	 */
	(void)getprop(0, "mmu-npmg", &npmgrps);
	(void)getprop(0, "vac-size", &vac_size);
	(void)getprop(0, "vac-linesize", &vac_linesize);
	if (getprop(0, "mips-on", &mips_on) != sizeof (int)) {
		/*
		 * This prom lacks the "mips-on/off" property. Use
		 * clock frequency for a good approximation.
		 */
		if (getprop(0, "clock-frequency", &freq) == sizeof (int) &&
		    freq > 15000000 && freq < 100000000) {
			mips_on = freq / 1000000;
			mips_off = 3;
		} else {
			/*
			 * Oh, well. Make an assumption.
			 */
			mips_off = CPU_MAXMIPS_25MHZ;
			mips_on = CPU_MAXMIPS_25MHZ;
		}
	} else {
		(void)getprop(0, "mips-off", &mips_off);
	}
	if (debug_prop) {
		printf("nwindows %d\n", nwindows);
		printf("npmgrps %d\n", npmgrps);
		printf("vac_size %d\n", vac_size);
		printf("vac_linesize %d\n", vac_linesize);
		printf("mips_off %d\n", mips_off);
		printf("mips_on %d\n", mips_on);
	}
	if (vac) {
		setdelay(mips_on);
	} else {
		setdelay(mips_off);
	}
}

/*
 * set delay constant for usec_delay()
 * delay ~= (usecs * (Cpudelay * 2 + 3) + 8) / mips
 *  => Cpudelay = ceil((mips - 3) / 2)
 */
static
setdelay(mips)
	int mips;
{
	if (mips > 3)
		Cpudelay = (mips - 2) >> 1;
}
