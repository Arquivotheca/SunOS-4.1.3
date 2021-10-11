#ident	"@(#)sense.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988, 1989, 1990 by Sun Microsystems, Inc.
 */
#ifndef	_scsi_generic_sense_h
#define	_scsi_generic_sense_h


/*
 * (Old) standard (Non-Extended) SCSI Sense.
 *
 * For Error Classe 0-6. This is all
 * Vendor Unique sense information.
 */

struct scsi_sense {
	u_char	ns_valid	: 1,	/* Logical Block Address is valid */
		ns_class	: 3,	/* Error class */
		ns_code		: 4;	/* Vendor Uniqe error code */
	u_char	ns_vu		: 3,	/* Vendor Unique value */
		ns_lba_hi	: 5;	/* High Logical Block Address */
	u_char	ns_lba_mid;		/* Middle Logical Block Address */
	u_char	ns_lba_lo;		/* Low part of Logical Block Address */
};

/*
 * SCSI Extended Sense structure
 *
 * For Error Class 7, the Extended Sense Structure is applicable.
 *
 * Since we are moving to SCSI-2 Compliance, the following structure
 * will be the one typically used.
 */

#define CLASS_EXTENDED_SENSE 0x7     /* indicates extended sense */

struct scsi_extended_sense {
	u_char	es_valid	: 1,	/* sense data is valid */
		es_class	: 3,	/* Error Class- fixed at 0x7 */
				: 3,	/* reserved (SCSI-2) */
		es_deferred	: 1;	/* this is a deferred error (SCSI-2) */

	u_char	es_segnum;		/* segment number: for COPY cmd only */

	u_char	es_filmk	: 1,	/* File Mark Detected */
		es_eom		: 1,	/* End of Media */
		es_ili		: 1,	/* Incorrect Length Indicator */
				: 1,	/* reserved */
		es_key		: 4;	/* Sense key (see below) */

	u_char	es_info_1;		/* information byte 1 */
	u_char	es_info_2;		/* information byte 2 */
	u_char	es_info_3;		/* information byte 3 */
	u_char	es_info_4;		/* information byte 4 */
	u_char	es_add_len;		/* number of additional bytes */
	u_char	es_cmd_info[4];		/* command specific information */
	u_char	es_code;		/* Additional Sense Code */
	u_char	es_qual_code;		/* Additional Sense Code Qualifier */
	u_char	es_fru_code;		/* Field Replaceable Unit Code */
	u_char	es_skey_specific[3];	/* Sense Key Specific information */

	/*
	 * Additional bytes may be defined in each implementation.
	 * The actual amount of space allocated for Sense Information
	 * is also implementation dependent.
	 *
	 * Modulo that, the declaration of an array two bytes in size
	 * nicely rounds this entire structure to a size of 20 bytes.
	 */

	u_char	es_add_info[2];		/* additional information */
};

/*
 * Define a macro to test the msb of the Sense Key
 * Specific data, which functions a VALID bit.
 */

#define	SKS_VALID(sep)	((sep)->es_skey_specific[0] & 0x80)

/* 
 * Sense Key values for Extended Sense.
 */

#define KEY_NO_SENSE		0x00
#define KEY_RECOVERABLE_ERROR	0x01
#define KEY_NOT_READY		0x02
#define KEY_MEDIUM_ERROR	0x03
#define KEY_HARDWARE_ERROR	0x04
#define KEY_ILLEGAL_REQUEST	0x05
#define KEY_UNIT_ATTENTION	0x06
#define KEY_WRITE_PROTECT	0x07
#define KEY_BLANK_CHECK		0x08
#define KEY_VENDOR_UNIQUE	0x09
#define KEY_COPY_ABORTED	0x0A
#define KEY_ABORTED_COMMAND	0x0B	/* often a parity error */
#define KEY_EQUAL		0x0C
#define KEY_VOLUME_OVERFLOW	0x0D
#define KEY_MISCOMPARE		0x0E
#define KEY_RESERVED		0x0F

/*
 * Each implementation will have specific mappings to what
 * Sense Information means
 */

#include <scsi/impl/sense.h>

#endif	/* !_scsi_generic_sense_h */
