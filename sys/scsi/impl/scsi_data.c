#ident	"%Z%%M% %I% %E% SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */

/*
 * Global SCSI data
 */

#include <scsi/scsi.h>

char *sense_keys[NUM_SENSE_KEYS + NUM_IMPL_SENSE_KEYS] = {
	"No Additional Sense",		/* 0x00 */
	"Soft Error",			/* 0x01 */
	"Not Ready",			/* 0x02 */
	"Media Error",			/* 0x03 */
	"Hardware Error",		/* 0x04 */
	"Illegal Request",		/* 0x05 */
	"Unit Attention",		/* 0x06 */
	"Write Protected",		/* 0x07 */
	"Blank Check",			/* 0x08 */
	"Vendor Unique",		/* 0x09 */
	"Copy Aborted",			/* 0x0a */
	"Aborted Command",		/* 0x0b */
	"Equal Error",			/* 0x0c */
	"Volume Overflow",		/* 0x0d */
	"Miscompare Error",		/* 0x0e */
	"Reserved",			/* 0x0f */
	"fatal",			/* 0x10 */
	"timeout",			/* 0x11 */
	"EOF",				/* 0x12 */
	"EOT",				/* 0x13 */
	"length error",			/* 0x14 */
	"BOT",				/* 0x15 */
	"wrong tape media"		/* 0x16 */
};

char *state_bits = "\20\05STS\04XFER\03CMD\02SEL\01ARB";

/*
 * Common Capability Strings
 * See <scsi/impl/services.h> for
 * definitions of positions in this
 * array
 */

char *scsi_capstrings[] = {
	"dma_max",
	"msg_out",
	"disconnect",
	"synchronous",
	"wide_xfer",
	"parity",
	"initiator-id",
	"untagged-qing",
	"tagged-qing",
	0
};
