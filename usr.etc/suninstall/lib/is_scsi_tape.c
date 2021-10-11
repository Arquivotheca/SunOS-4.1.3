#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)is_scsi_tape.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)is_scsi_tape.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <strings.h>

/***************************************************************************
**
**	Function:	(int)	is_scsi_tape()
**
**	Description:	tells if the named device is a scsi tape device
**
**      Return Value:  1  : if device is scsi tape device
**		       0  : if device is not scsi tape device
**
****************************************************************************
*/
int
is_scsi_tape(device)
	char	*device;	/* the device to test */
{
	if (strncmp(device, "st", 2) == 0)
		return(1);
	else
		return(0);
}

