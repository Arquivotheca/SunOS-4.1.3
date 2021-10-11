#ifndef lint
static char sccsid[] = "@(#)scsi_type.c 1.1 92/07/30 SMI";
#endif lint

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "install.h"

extern	char *sprintf();

int
scsi_tapetype(tape_device)
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
