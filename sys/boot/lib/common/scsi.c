/*
 * @(#)scsi.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 *
 * Contains SCSI probe routine to probe for host adapter.
 */

/*#define SCSI_DEBUG	1	/* Allow compiling of debug code */
#define REL4			/* Enable release 4.00 mods */

#ifdef	REL4
#include <sys/types.h>
#include <mon/sunromvec.h>
#include <stand/saio.h>
#include <stand/scsi.h>
#include <mon/idprom.h>
#else	REL4
#include "../h/types.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#include "saio.h"
#include "scsi.h"
#endif	REL4

					/* external routines */
#ifdef	sun2
extern	scopen();
#endif	sun2

#ifdef	sun3
extern	siopen(), scopen(), se_open();
#endif	sun2

#ifdef	sun3x
extern	siopen(), scopen(), se_open(), smopen();
#endif	sun3x

#ifdef	sun4
extern	swopen(), smopen();
#endif	sun4


#ifdef SCSI_DEBUG
int scsi_debug = SCSI_DEBUG;		/* 0 = normal operation
					 * 1 = extended error info only
					 * 2 = debug and error info
					 * 3 = all status info
					 */
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (scsi_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (scsi_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (scsi_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (scsi_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (scsi_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (scsi_debug) printf(str,arg1,arg2)

#else SCSI_DEBUG
int scsi_debug = 0;
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SCSI_DEBUG

/* 
 * scsi reset flag
 */
int sc_reset = 0;

/* 
 * scsi CDB op code to command length decode table.
 */
u_char sc_cdb_size[] = {
	CDB_GROUP0,		/* Group 0, 6 byte cdb */
	CDB_GROUP1,		/* Group 1, 10 byte cdb */
	CDB_GROUP2,		/* Group 2, ? byte cdb */
	CDB_GROUP3,		/* Group 3, ? byte cdb */
	CDB_GROUP4,		/* Group 4, ? byte cdb */
	CDB_GROUP5,		/* Group 5, ? byte cdb */
	CDB_GROUP6,		/* Group 6, ? byte cdb */
	CDB_GROUP7		/* Group 7, ? byte cdb */
};


/*
 * Description:	Probe for SCSI host adaptor interface. 
 *
 * 		SI vme scsi host adaptor occupies 2K bytes in the vme address
 * 		space.  SC vme scsi host adaptor occupies 4K bytes in the vme
 * 		address space.  This difference is used to determine which host
 *		adapter is connected.
 *
 *		The call to si_open will determine whether the system has a
 *		on-board si host adapter, or not.  Similarly, scopen will
 *		determine whether the system has a multibus or VME bus.
 *
 * Synopsis:	status = scsi_probe(sip)
 *		status	:(int) 0 one of the host-adapter found
 *			      -1 neither host-adapter found
 *		h_sip	:(int *) pointer to host adapter structure
 * Routines:	scopen, siopen
 */
int
scsi_probe(h_sip)
	register struct host_saioreq	*h_sip;
{
#ifdef	sun2
	DPRINTF("scsi_probe: sc\n");
	/* For Sun-2, there is only the SCSI-2 (sc). */
	if (scopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using sc\n");
		return (0);
	}
#endif	sun2

	/*
	 * For Sun-3 and Sun-4, there are several choices:
	 * VME SCSI-3, onboard Sun-3 SCSI-3, onborad Sun-4 SCSI-3,
	 * and VME SCSI-2.  Si takes care of all the SCSI-3 permutations.
	 * Sc takes care of SCSI-2.
	 */
#ifdef	sun4
	DPRINTF("scsi_probe: sw\n");
	/* Check for Sun-4/110 SCSI */
	if (swopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using sw\n");
		return (0);
	}
	/* last test for Emulex ESP controller */
	/* Check for Sun-4 STINGRAY SCSI */
	DPRINTF("scsi_probe: sm\n");
	if (smopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using sm\n");
		return (0);
	}
#endif	sun4

#if	defined(sun3) || defined(sun3x) || defined(sun4)
	/* Check for Sun-3 SCSI-3 */
	DPRINTF("scsi_probe: si\n");
	if (siopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using si\n");
		return (0);
	}

	/* Check for Sun-3E SCSI */
	DPRINTF("scsi_probe: se\n");
	if (se_open(h_sip) == 0) {
		EPRINTF("scsi_probe: using se\n");
		return (0);
	}

	/* Check for Sun-2/3 SCSI-2 */
	DPRINTF("scsi_probe: sc\n");
	if (scopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using sc\n");
		return (0);
	}
#ifdef	sun3x
	/* last test for Emulex ESP controller */
	/* Check for Sun-3x HYDRA SCSI */
	DPRINTF("scsi_probe: sm\n");
	if (smopen(h_sip) == 0) {
		EPRINTF("scsi_probe: using sm\n");
		return (0);
	}
#endif	sun3x

#endif	sun3 || sun3x || sun4

#ifdef	lint
	scsi_debug = scsi_debug;
#endif	lint

	/* No host adapter found */
	return (-1);
}
