
#ifndef lint
static char sccsid[] = "@(#)unpack_hostid.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <mon/idprom.h>
#include "install.h"

extern long gethostid();

#define IE "ie0"
#define LE "le0"
#define EC "ec0"

/*
 * Architecture types
 */
#define SUN2			"sun2.sun2"
#define	SUN3			"sun3.sun3"
#define	SUN3X			"sun3.sun3x"
#define	SUN4			"sun4.sun4"
#define SUN386			"sun386.sun386"

/*
 * this static struct contains everything we need to know about
 * a machine that we can determine or infer from the gethostid() call.
 */
static struct machine_info {
	unsigned long mi_idm;	/* IDM_WHATEVER (from idprom.h) >> 24 */
	char *mi_arch;		/* architecture, e.g., sun2, sun3 ... */
	char *mi_mach;		/* machine type, e.g., sun3_carrera */
	char *mi_ether;		/* ethernet i/f for this machine, e.g., ie0 */
} machinfo[] = {
	IDM_SUN2_VME,		SUN2,	"sun2_vme",		IE,
	IDM_SUN2_MULTI,		SUN2,	"sun2_multi",		IE,
	IDM_SUN3_CARRERA,	SUN3,	"sun3_1x0",		IE,
	IDM_SUN3_SIRIUS,	SUN3,	"sun3_2x0",		IE,
	IDM_SUN3_PRISM,		SUN3,	"sun3_110",		IE,
	IDM_SUN4,		SUN4,	"sun4",			IE,
	IDM_SUN4_COBRA,		SUN4,	"sun4_110",		IE,
#if 0
	IDM_SUN386,		SUN386,	"sun386",		IE,
#endif
	IDM_SUN3_F,		SUN3,	"sun3_60",		LE,
	IDM_SUN3_M25,		SUN3,	"sun3_50",		LE,
#if 0
	IDM_SUN3_E,		SUN3,	"sun3_e",		"??",
	IDM_SUN4_STINGRAY,	SUN4,	"sun4_stingray",	"??",
	IDM_SUN4_SUNRAY,	SUN4,	"sun4_sunray",		"??",
	IDM_SUN3X_PEGASUS,	SUN3X,	"sun3x_pegasus",	"??",
	IDM_SUN4C_CAMPUS1,	SUN4C,	"sun4c_campus1",	"??",
#endif
	0, 			0, 	0, 			0
};


/*
 * return ptr to the appropriate member of the machinfo struct above
 */
static struct machine_info *
get_machinfo()
{
	unsigned long idm;
	struct machine_info *mi;

	idm = (unsigned long)gethostid() >> 24;

	for (mi = machinfo; mi->mi_idm != 0; mi++) {
		if (mi->mi_idm == idm) {
			return(mi);
		}
	}

	/*
	 * if we get here, return a ptr to all NULLs, which lets the calling
	 * routine return NULL to its caller
	 */
	return(mi);
}


/*
 * return a string (e.g., "sun3") describing architecture based
 * on the results of the "hostid" system call; returns NULL if there's
 * no appropriate entry.
 */

char *
archtype()
{
	return((get_machinfo())->mi_arch);
}

/*
 * return a pointer to a string describing the ethernet interface
 * (e.g., "ie0", "le0") based on results of gethostid; returns NULL if there's
 * no appropriate entry.
 */
char *
ethertype()
{
	return((get_machinfo())->mi_ether);
}

/*
 * return a pointer to a string describing the machine type
 * (e.g., "sun3_50", "sun4") based on results of gethostid;
 * returns NULL if there's no appropriate entry.
 */
char *
machtype()
{
	return((get_machinfo())->mi_mach);
}
