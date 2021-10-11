#ident	"@(#)sd_conf.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */
#include "sd.h"
#if	NSD > 0
#include <scsi/scsi.h>
#include <scsi/targets/sddef.h>

/*
 *
 * Global SCSI disk data definitions
 *
 */

int sd_error_level	=	SDERR_RETRYABLE;
int sd_io_time		=	SD_IO_TIME;
int sd_fmt_time		=	SD_FMT_TIME;
int sd_retry_count	=	SD_RETRY_COUNT;


/*
 *
 * Drive Type Table
 *
 * The only use for this table is to note which *special*
 * drives have problems that we should watch out for, else
 * generic CCS should suffice.
 *
 */


struct sd_drivetype sdspecial[] = {
#ifdef	ADAPTEC
/* Adaptec ACB4000 */
/*
 * This controller doesn't respond to the Inquiry command,
 * so the inquiry is artificially generated (by us).
 */
{ CTYPE_ACB4000,		SD_NOPARITY,	"ADAPTEC" },
#endif	ADAPTEC



/* Emulex MD21 */
{ CTYPE_MD21,			0,		"EMULEX  MD21/S2" }

};
int sdnspecial = (sizeof sdspecial / (sizeof sdspecial[0]));

#endif	/* NSD > 0 */
