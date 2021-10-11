
/*	@(#)swtab.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:swtab.c	1.3" */

#include <sys/mount.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/pn.h>

/* these are the indicies into sw_tab.
   note that the orders must match the opcodes */

pntab_t sw_tab[NUMSWENT] = {	"RFV", RF_RF,
				"NSV", RF_NS,
				"RSP", RF_AK 
			   };

/* these are the indicies into du_tab.
   note that the orders must match the opcodes */

pntab_t du_tab[NUMDUENT] = {	"MNT", MNT
			   };
