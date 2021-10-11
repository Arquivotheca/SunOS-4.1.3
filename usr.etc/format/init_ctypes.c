
#ifndef lint
static  char sccsid[] = "@(#)init_ctypes.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * This file defines the known controller types.  To add a new controller
 * type, simply add a new line to the array and define the necessary
 * ops vector in a 'driver' file.
 */
#include "global.h"

extern	struct ctlr_ops xy450ops, acb4000ops, md21ops, xd7053ops, idops;
extern	struct ctlr_ops scsiops;

/*
 * This array defines the supported controller types
 */
struct	ctlr_type ctlr_types[] = {
	/*
	 * ctype                 name              ops
	 *            flags
	 */
	{ DKC_XY450,		"XY450",	&xy450ops,
		CF_SMD_DEFS | CF_450_TYPES },
	{ DKC_ACB4000,		"ACB4000",	&acb4000ops,
		CF_SCSI | CF_BLABEL | CF_BSK_WPR },
	{ DKC_MD21,		"MD21",		&md21ops,
		CF_SCSI | CF_DEFECTS },
	{ DKC_XD7053,		"XD7053",	&xd7053ops,
		CF_SMD_DEFS },
	{ DKC_SCSI_CCS,		"SCSI",		&scsiops,
		CF_SCSI | CF_EMBEDDED },
	{ DKC_PANTHER,		"ISP-80",	&idops,
		CF_IPI|CF_WLIST },
};

/*
 * This variable is used to count the entries in the array so its
 * size is not hard-wired anywhere.
 */
int	nctypes = (sizeof ctlr_types) / (sizeof ctlr_types[0]);

