#ifndef lint
#ifdef SunB1
#ident			"@(#)scsi_block_size.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)scsi_block_size.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/mtio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "install.h"
#include "media.h"

/***************************************************************************
**
**	Function:	(int)	scsi_block_size()
**
**	Description:	gets the block size for a scsi tape drive
**
**      Return Value:   the block size for the tape drive
**
****************************************************************************
*/

int
scsi_block_size(tape_device)
	register char *tape_device;
{
	register int bs, fd;
	struct mtget tape;
	char device[24];

	(void) sprintf(device,"/dev/nr%s",tape_device);
	if ((fd = open(device,O_RDONLY)) == -1)
		return (BS_DEFAULT);

	/* ask the tape device what type they are;
	 * set the block size accordingly
	 * else use default block size
	 */
	if (ioctl(fd, MTIOCGET, &tape) == 0) {
		bs = tape.mt_bf;
	} else
		bs = BS_DEFAULT;

	(void) close(fd);
	return (bs);
}
