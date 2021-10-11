#ident	"@(#)mode.h 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1990, 1991 by Sun Microsystems Inc.
 */

#ifndef	_scsi_impl_mode_h
#define	_scsi_impl_mode_h

/*
 * Defines and Structures for SCSI Mode Sense/Select data
 *
 * Implementation Specific variations
 */

/*
 * Variations to Direct Access device pages
 */

/*
 * Page 1: CCS error recovery page is a little different than SCSI-2
 */

#define	PAGELENGTH_DAD_MODE_ERR_RECOV_CCS	0x06

struct mode_err_recov_ccs {
	struct	mode_page mode_page;	/* common mode page header */
	u_char		awre	: 1,	/* auto write realloc enabled */
			arre	: 1,	/* auto read realloc enabled */
			tb	: 1,	/* transfer block */
			rc	: 1,	/* read continuous */
			eec	: 1,	/* enable early correction */
			per	: 1,	/* post error */
			dte	: 1,	/* disable transfer on error */
			dcr	: 1;	/* disable correction */
	u_char	retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	recovery_time_limit;
};


/*
 * Page 2:  CCS Disconnect/Reconnect Page
 */

struct mode_disco_reco_ccs {
	struct	mode_page mode_page;	/* common mode page header */
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */
	u_short	bus_inactivity_limit;	/* how much bus quiet time for BSY- */
	u_short	disconect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_char	reserved[2];
};


/*
 * Page 3: CCS Direct Access Device Format Parameters
 *
 * The 0x8 bit in the Drive Type byte is used in CCS
 * as an INHIBIT SAVE bit. This bit is not in SCSI-2.
 */

#define	_reserved_ins	ins


/*
 * Page 0x4 - CCS Rigid Disk Drive Geometry Parameters
 */

struct mode_geometry_ccs {
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
	u_char	reserved[3];
};



/*
 * Page 0x38 - This is the CCS Cache Page
 */

#define	DAD_MODE_CACHE_CCS		0x38

struct mode_cache_ccs {
	struct	mode_page mode_page;	/* common mode page header */
	u_char	mode;			/* Cache control and size */
	u_char	threshold;		/* Prefetch threshold */
	u_char	max_prefetch;		/* Max. prefetch */
	u_char	max_multiplier;		/* Max. prefetch multiplier */
	u_char	min_prefetch;		/* Min. prefetch */
	u_char	min_multiplier;		/* Min. prefetch multiplier */
	u_char	rsvd2[8];
};




/*
 * Emulex MD21 Unique Mode Select/Sense structure.
 * This is apparently not used, although the MD21
 * documentation refers to it.
 *
 * The medium_type in the mode header must be 0x80
 * to indicate a vendor unique format. There is then
 * a standard block descriptor page, which must be
 * zeros (although the block descriptor length is set
 * appropriately in the mode header).
 *
 * After this stuff, comes the vendor unique ESDI
 * format parameters for the MD21.
 *
 * Notes:
 *
 *	1) The logical number of sectors/track should be the
 *	number of physical sectors/track less the number spare
 *	sectors/track.
 *
 *	2) The logical number of cylinders should be the
 *	number of physical cylinders less three (3) reserved
 *	for use by the drive, and less any alternate cylinders
 *	allocated.
 *
 *	3) head skew- see MD21 manual.
 */

struct emulex_format_params {
	u_char	alt_cyl;	/* number of alternate cylinders */
	u_char	nheads	: 4,	/* number of heads */
		ssz	: 1,	/* sector size. 1 == 256 bps, 0 == 512 bps */
		sst	: 2,	/* spare sectors per track */
			: 1;
	u_char	nsect;		/* logical sectors/track */
	u_char	ncyl_hi;	/* logical number of cylinders, msb */
	u_char	ncyl_lo;	/* logical number of cylinders, lsb */
	u_char	head_skew;	/* head skew */
	u_char	reserved[3];
};



#endif	/* !_scsi_impl_mode_h */
