#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)blocks_to_str.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)blocks_to_str.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint


/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		blocks_to_str()
 *
 *	Description:	Convert a block count into a string using the
 *		display_unit selected by the user as a guide.
 */

#include "install.h"

/*
 *	External functions:
 */
extern	char *		sprintf();


char *
blocks_to_str(disk_p, blocks)
	disk_info *	disk_p;
	daddr_t		blocks;
{
	static	char	buf[SMALL_STR];		/* buffer for output */


	switch (disk_p->display_unit) {
	case DU_BLOCKS:
		(void) sprintf(buf, "%ld", blocks);
		break;

	case DU_BYTES:
		(void) sprintf(buf, "%ld", blocks_to_bytes(blocks));
		break;

	case DU_CYLINDERS:
		(void) sprintf(buf, "%ld",
			       blocks_to_cyls(blocks, disk_p->geom_buf));
		break;

	case DU_KBYTES:
		(void) sprintf(buf, "%ld", blocks_to_Kb(blocks));
		break;

	case DU_MBYTES:
		(void) sprintf(buf, "%ld",
			       Real_to_DI_Meg(blocks_to_Mb(blocks)));
		break;
	}

	return(buf);
} /* end blocks_to_str() */
