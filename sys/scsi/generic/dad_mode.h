#ident	"@(#)dad_mode.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1989, 1990, 1991 by Sun Microsystems Inc.
 */

#ifndef	_scsi_generic_dad_mode_h
#define	_scsi_generic_dad_mode_h

/*
 * Structures and defines for DIRECT ACCESS mode sense/select operations
 */

/*
 * Direct Access Device mode heade device specific byte definitions.
 *
 * On MODE SELECT operations, the effect of the state of the WP bit is unknown,
 * else reflects the Write-Protect status of the device.
 *
 * On MODE SELECT operations, the the DPOFUA bit is reserved and must
 * be zero, else on MODE SENSE operations it reflects whether or not
 * the device has a cache.
 */

#define	MODE_DAD_WP	0x80
#define	MODE_DAD_DPOFUA	0x10

/*
 * Direct Access Device Medium Types
 */

#define	DAD_MTYP_DFLT	0x0 /* default (currently mounted) type */

#define	DAD_MTYP_FLXSS	0x1 /* flexible disk, single side, unspec. media */
#define	DAD_MTYP_FLXDS	0x2 /* flexible disk, double side, unspec. media */

#define	DAD_MTYP_FLX_8SSSD 0x05	/* 8", single side, single density, 48tpi */
#define	DAD_MTYP_FLX_8DSSD 0x06	/* 8", double side, single density, 48tpi */
#define	DAD_MTYP_FLX_8SSDD 0x09	/* 8", single side, double density, 48tpi */
#define	DAD_MTYP_FLX_8DSDD 0x0A	/* 8", double side, double density, 48tpi */
#define	DAD_MTYP_FLX_5SSLD 0x0D	/* 5.25", single side, single density, 48tpi */
#define	DAD_MTYP_FLX_5DSMD1 0x12	/*
					 * 5.25", double side,
					 * medium density, 48tpi
					 */
#define	DAD_MTYP_FLX_5DSMD2 0x16	/*
					 * 5.25", double side,
					 * medium density, 96tpi
					 */
#define	DAD_MTYP_FLX_5DSQD 0x12	/* 5.25", double side, quad density, 96tpi */
#define	DAD_MTYP_FLX_3DSLD 0x12	/* 3.5", double side, low density, 135tpi */

/* I am not going to bother noting direct access tape device codes */


/*
 * Direct Access device Mode Sense/Mode Select Defined pages
 */

#define	DAD_MODE_ERR_RECOV	0x01
#define	DAD_MODE_FORMAT		0x03
#define	DAD_MODE_GEOMETRY	0x04
#define	DAD_MODE_FLEXDISK	0x05
#define	DAD_MODE_VRFY_ERR_RECOV	0x07
#define	DAD_MODE_CACHE		0x08
#define	DAD_MODE_MEDIA_TYPES	0x0B
#define	DAD_MODE_NOTCHPART	0x0C

/*
 * Page 0x1 - Error Recovery Parameters
 *
 * Note:	This structure is incompatible with previous SCSI
 *		implementations. See <scsi/impl/mode.h> for an
 *		alternative form of this structure. They can be
 *		distinguished by the length of data returned
 *		from a MODE SENSE command.
 */

#define	PAGELENGTH_DAD_MODE_ERR_RECOV	0x0A

struct mode_err_recov {
	struct	mode_page mode_page;	/* common mode page header */
	u_char		awre	: 1,	/* auto write realloc enabled */
			arre	: 1,	/* auto read realloc enabled */
			tb	: 1,	/* transfer block */
			rc	: 1,	/* read continuous */
			eec	: 1,	/* enable early correction */
			per	: 1,	/* post error */
			dte	: 1,	/* disable transfer on error */
			dcr	: 1;	/* disable correction */
	u_char	read_retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	reserved;
	u_char	write_retry_count;
	u_char	reserved_2;
	u_short	recovery_time_limit;
};

/*
 * Page 0x3 - Direct Access Device Format Parameters
 */

struct mode_format {
	struct	mode_page mode_page;	/* common mode page header */
	u_short	tracks_per_zone;	/* Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_bytes_sect;	/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew;
	u_short	cylinder_skew;
	u_char		ssec	: 1,	/* Drive Type Field */
			hsec	: 1,
			rmb	: 1,
			surf	: 1,
		_reserved_ins	: 1,	/* see <scsi/impl/mode.h> */
				: 3;
	u_char	reserved[3];
};

/*
 * Page 0x4 - Rigid Disk Drive Geometry Parameters
 */

struct mode_geometry {
	struct	mode_page mode_page;	/* common mode page header */
	u_char	cyl_ub;			/* number of cylinders */
	u_char	cyl_mb;
	u_char	cyl_lb;
	u_char	heads;			/* number of heads */
	u_char	precomp_cyl_ub;		/* cylinder to start precomp */
	u_char	precomp_cyl_mb;
	u_char	precomp_cyl_lb;
	u_char	current_cyl_ub;		/* cyl to start reduced current */
	u_char	current_cyl_mb;
	u_char	current_cyl_lb;
	u_short	step_rate;		/* drive step rate */
	u_char	landing_cyl_ub;		/* landing zone cylinder */
	u_char	landing_cyl_mb;
	u_char	landing_cyl_lb;
	u_char			: 6,
			rpl	: 2;	/* rotational position locking */
	u_char	rotational_offset;	/* rotational offset */
	u_char	reserved;
	u_short	rpm;			/* rotations per minute */
	u_char	reserved2[2];
};

#define	RPL_SPINDLE_SLAVE		1
#define	RPL_SPINDLE_MASTER		2
#define	RPL_SPINDLE_MASTER_CONTROL	3

/*
 * Page 0x5 - Flexible Disk Parameters
 */

/* XXX To Be Supplied XXX */

/*
 * Page 0x7 - Verify Error Recovery Page
 */

/* XXX To Be Supplied XXX */

/*
 * Page 0x8 - Caching Page
 *
 * Most current targets use the CCS cache parameter page (0x38)-
 * see <scsi/impl/mode.h>
 */

struct mode_cache {
	struct	mode_page mode_page;	/* common mode page header */
	u_char			: 5,
			wce	: 1,	/* Write Cache Enable */
			mf	: 1,	/* Multiplication Factor */
			rcd	: 1;	/* Read Cache Disable */
	u_char	read_reten_pri	: 4,	/* Demand Read Retention Priority */
		write_reten_pri	: 4;	/* Write Retention Priority */
	u_short	dis_prefetch_len;	/* Disable prefetch xfer length */
	u_short	min_prefetch;		/* minimum prefetch length */
	u_short	max_prefetch;		/* maximum prefetch length */
	u_short	prefetch_ceiling;	/* max prefetch ceiling */
};

/*
 * Page 0xB - Media Types Supported Page
 */

/* XXX To Be Supplied XXX */

/*
 * Page 0xC - Notch and Partition Page
 */

/* XXX To Be Supplied XXX */

#endif	/* !_scsi_generic_dad_mode_h */
