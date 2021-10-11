#ident	"@(#)commands.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_generic_commands_h
#define	_scsi_generic_commands_h

/*
 * Standard SCSI Command Definitions
 *
 * Macros to determine known command sizes
 */

#define	CDB_GROUPID(cmd)	((cmd >> 5) & 0x7)
#define	CDB_GROUPID_0	0
#define	CDB_GROUPID_1	1
#define	CDB_GROUPID_5	5

#define	CDB_GROUP0	6	/*  6-byte cdb's */
#define	CDB_GROUP1	10	/* 10-byte cdb's */
#define	CDB_GROUP2	0	/* reserved */
#define	CDB_GROUP3	0	/* reserved */
#define	CDB_GROUP4	0	/* reserved */
#define	CDB_GROUP5	12	/* 12-byte cdb's */
#define	CDB_GROUP6	0	/* reserved */
#define	CDB_GROUP7	0	/* reserved */

/*
 * Generic Command Definitions
 */

/*
 * Group 0 Commands
 */

#define	SCMD_GROUP0		0x00

/*
 * Group 0 commands, All Devices
 */

#define	SCMD_TEST_UNIT_READY	0x00
#define	SCMD_REQUEST_SENSE	0x03
#define	SCMD_INQUIRY		0x12
#define	SCMD_COPY		0x18
#define	SCMD_GDIAG		0x1C	/* receive diagnostic results */
#define	SCMD_SDIAG		0x1D	/* send diagnostic results */

/*
 * Group 0 commands, Direct Access Devices
 */

/*	SCMD_TEST_UNIT_READY	0x00	*/
#define	SCMD_REZERO_UNIT	0x01
/*	SCMD_REQUEST_SENSE	0x03	*/
#define	SCMD_FORMAT		0x04
#define	SCMD_REASSIGN_BLOCK	0x07
#define	SCMD_READ		0x08
#define	SCMD_WRITE		0x0a
#define	SCMD_SEEK		0x0b
/*	SCMD_INQUIRY		0x12	*/
#define	SCMD_MODE_SELECT	0x15
#define	SCMD_RESERVE		0x16
#define	SCMD_RELEASE		0x17
/*	SCMD_COPY		0x18	*/
#define	SCMD_MODE_SENSE		0x1a
#define	SCMD_START_STOP		0x1b
/*	SCMD_GDIAG		0x1C	*/
/*	SCMD_SDIAG		0x1D	*/
#define	SCMD_DOORLOCK		0x1E	/* Prevent/Allow Medium Removal */

/*
 * Group 0 commands, Sequential Access Devices
 */

/*	SCMD_TEST_UNIT_READY	0x00	*/
#define	SCMD_REWIND		0x01	/* Note similarity to SCMD_REZERO */
/*	SCMD_REQUEST_SENSE	0x03	*/
#define	SCMD_READ_BLKLIM	0x05
/*	SCMD_READ		0x08	*/
/*	SCMD_WRITE		0x0a	*/
#define	SCMD_TRK_SEL		0x0b	/* Note similarity to SCMD_SEEK */
#define	SCMD_READ_REVERSE	0x0f
#define	SCMD_WRITE_FILE_MARK	0x10
#define	SCMD_SPACE		0x11
/*	SCMD_INQUIRY		0x12	*/
#define	SCMD_VERIFY_G0		0x13
#define	SCMD_RECOVER_BUF	0x14
/*	SCMD_MODE_SELECT	0x15	*/
/*	SCMD_RESERVE		0x16	*/
/*	SCMD_RELEASE		0x17	*/
/*	SCMD_COPY		0x18	*/
#define	SCMD_ERASE		0x19
/*	SCMD_MODE_SENSE		0x1a	*/
#define	SCMD_LOAD		0x1b	/* Note similarity to SCMD_START_STOP */
/*	SCMD_GDIAG		0x1c	*/
/*	SCMD_SDIAG		0x1d	*/
/*	SCMD_DOORLOCK		0x1e	*/


/*
 * Group 0 commands, Printer Devices
 */

/*	SCMD_TEST_UNIT_READY	0x00	*/
/*	SCMD_REQUEST_SENSE	0x03	*/
/*	SCMD_FORMAT		0x04	*/
#define	SCMD_PRINT		0x0a	/* Note similarity to SCMD_WRITE */
#define	SCMD_SLEW_PRINT		0x0b	/* ? similar to SCMD_SEEK ? */
#define	SCMD_FLUSH_PRINT_BUF	0x10	/* ? similar to SCMD_WRITE_FILE_MARK */
/*	SCMD_INQUIRY		0x12	*/
/*	SCMD_RECOVER_BUF	0x14	*/
/*	SCMD_MODE_SELECT	0x15	*/
/*	SCMD_RESERVE		0x16	*/
/*	SCMD_RELEASE		0x17	*/
/*	SCMD_COPY		0x18	*/
/*	SCMD_MODE_SENSE		0x1a	*/
#define	SCMD_STOP_PRINT		0x1b	/* Note similarity to SCMD_START_STOP */
/*	SCMD_GDIAG		0x1c	*/
/*	SCMD_SDIAG		0x1d	*/

/*
 * Group 0 commands, Processor Devices
 */
/*	SCMD_TEST_UNIT_READY	0x00	*/
/*	SCMD_REQUEST_SENSE	0x03	*/
#define	SCMD_RECEIVE		0x08	/* Note similarity to SCMD_READ */
#define	SCMD_SEND		0x0a	/* Note similarity to SCMD_WRITE */
/*	SCMD_INQUIRY		0x12	*/
/*	SCMD_COPY		0x18	*/
/*	SCMD_MODE_SENSE		0x1a	*/
/*	SCMD_GDIAG		0x1c	*/
/*	SCMD_SDIAG		0x1d	*/

/*
 * Group 0 commands, WORM devices
 */

/*	SCMD_TEST_UNIT_READY	0x00	*/
/*	SCMD_REZERO_UNIT	0x01	*/
/*	SCMD_REQUEST_SENSE	0x03	*/
/*	SCMD_REASSIGN_BLOCK	0x07	*/
/*	SCMD_READ		0x08	*/
/*	SCMD_WRITE		0x0a	*/
/*	SCMD_SEEK		0x0b	*/
/*	SCMD_INQUIRY		0x12	*/
/*	SCMD_MODE_SELECT	0x15	*/
/*	SCMD_RESERVE		0x16	*/
/*	SCMD_RELEASE		0x17	*/
/*	SCMD_COPY		0x18	*/
/*	SCMD_MODE_SENSE		0x1a	*/
/*	SCMD_START_STOP		0x1b	*/
/*	SCMD_GDIAG		0x1C	*/
/*	SCMD_SDIAG		0x1D	*/
/*	SCMD_DOORLOCK		0x1E	*/

/*
 * Group 0 commands, RODAT devices
 */

/*	SCMD_TEST_UNIT_READY	0x00	*/
/*	SCMD_REZERO_UNIT	0x01	*/
/*	SCMD_REQUEST_SENSE	0x03	*/
/*	SCMD_REASSIGN_BLOCK	0x07	*/
/*	SCMD_READ		0x08	*/
/*	SCMD_SEEK		0x0b	*/
/*	SCMD_INQUIRY		0x12	*/
/*	SCMD_MODE_SELECT	0x15	*/
/*	SCMD_RESERVE		0x16	*/
/*	SCMD_RELEASE		0x17	*/
/*	SCMD_COPY		0x18	*/
/*	SCMD_MODE_SENSE		0x1a	*/
/*	SCMD_START_STOP		0x1b	*/
/*	SCMD_GDIAG		0x1C	*/
/*	SCMD_SDIAG		0x1D	*/
/*	SCMD_DOORLOCK		0x1E	*/

/*
 * Group 1 Commands
 */

#define	SCMD_GROUP1		0x20

/*
 * Group 1 Commands, all devices
 */

#define	SCMD_COMPARE		0x39
#define	SCMD_COPY_VERIFY	0x3A

/*
 * Group 1 Commands, direct access devices
 */

#define	SCMD_READ_CAPACITY	0x25
#define	SCMD_READ_G1		0x28	/* Note that only the group changed */
#define	SCMD_WRITE_G1		0x2a	/* Note that only the group changed */
#define	SCMD_SEEK_G1		0x2b	/* Note that only the group changed */
#define	SCMD_WRITE_VERIFY	0x2e
#define	SCMD_VERIFY		0x2f
#define	SCMD_SEARCH_HIGH	0x30
#define	SCMD_SEARCH_EQUAL	0x31
#define	SCMD_SEARCH_LOW		0x32
#define	SCMD_SET_LIMITS		0x33
#define	SCMD_READ_DEFECT_LIST	0x37
#define	SCMD_WRITE_BUFFER	0x3B
#define	SCMD_READ_BUFFER	0x3c

/*
 * Group 1 Commands, sequential access devices
 */

/* (None Defined) */

/*
 * Group 1 Commands, printer devices
 */

/* (None Defined) */

/*
 * Group 1 Commands, processor devices
 */

/* (None Defined) */

/*
 * Group 1 Commanads, WORM devices
 */

/*	SCMD_READ_CAPACITY	0x25	*/
/*	SCMD_READ_G1		0x28	*/
/*	SCMD_WRITE_G1		0x2a	*/
/*	SCMD_SEEK_G1		0x2b	*/
/*	SCMD_WRITE_VERIFY	0x2e	*/
/*	SCMD_VERIFY		0x2f	*/
/*	SCMD_SEARCH_HIGH	0x30	*/
/*	SCMD_SEARCH_EQUAL	0x31	*/
/*	SCMD_SEARCH_LOW		0x32	*/
/*	SCMD_SET_LIMITS		0x33	*/

/*
 * Group 1 Commanads, RODAT devices
 */

/*	SCMD_READ_CAPACITY	0x25	*/
/*	SCMD_READ_G1		0x28	*/
/*	SCMD_SEEK_G1		0x2b	*/
/*	SCMD_VERIFY		0x2f	*/
/*	SCMD_SEARCH_HIGH	0x30	*/
/*	SCMD_SEARCH_EQUAL	0x31	*/
/*	SCMD_SEARCH_LOW		0x32	*/
/*	SCMD_SET_LIMITS		0x33	*/


/*
 * Below are inclusions of files describing various command structures
 * of interest								*
 */

#include	<scsi/generic/inquiry.h>
#include	<scsi/generic/sense.h>

/*
 * Private Vendor Unique Commands - Each implementation provides this.
 */

#include	<scsi/impl/commands.h>

#endif	_scsi_generic_commands_h
