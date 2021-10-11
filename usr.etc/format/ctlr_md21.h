/* @(#)ctlr_md21.h	1.1  7/30/92 Copyright 1987 Sun Microsystems, Inc. */
/*
 * Controller specific definitions for Emulex MD21 (actually, any CCS ctlr).
 */

/* Page Codes */
#define ERR_RECOVERY_PARMS	1
#define DISCO_RECO_PARMS	2
#define FORMAT_PARMS		3
#define GEOMETRY_PARMS		4
#define CERTIFICATION_PARMS	6

/*
 * Mode select header
 */
struct ccs_modesel_head {
	u_char	reserve1;
	u_char	medium;			/* medium type */
	u_char 	reserve2;
	u_char	block_desc_length;	/* length of block descriptor */
	u_int	density		: 8;	/* density code */
	u_int	number_blocks	:24;
	u_int	reserve3	: 8;
	u_int	block_length	:24;
}; 

/*
 * Page Header
 */
struct page_header {
	u_char	reserved_1	: 1;
	u_char	reserved_2	: 1;
	u_char	page_code	: 6;	/* define page function */
	u_char	page_length;		/* length of current page */
};

/*
 * Page One - Error Recovery Parameters 
 */
struct ccs_err_recovery {
	struct	page_header page_header;
	u_char	awre		: 1;	/* auto write realloc enabled */
	u_char	arre		: 1;	/* auto read realloc enabled */
	u_char	tb		: 1;	/* transfer block */
	u_char 	rc		: 1;	/* read continuous */
	u_char	eec		: 1;	/* enable early correction */
	u_char	per		: 1;	/* post error */
	u_char	dte		: 1;	/* disable transfer on error */
	u_char	dcr		: 1;	/* disable correction */
	u_char	retry_count;
	u_char	correction_span;
	u_char	head_offset_count;
	u_char	strobe_offset_count;
	u_char	recovery_time_limit;
};

/*
 * Page Two - Disconnect / Reconnect Control Parameters
 */
struct ccs_disco_reco {
	struct	page_header page_header;
	u_char	buffer_full_ratio;	/* write, how full before reconnect? */
	u_char	buffer_empty_ratio;	/* read, how full before reconnect? */

	u_short	bus_inactivity_limit;	/* how much bus time for busy */
	u_short	disconnect_time_limit;	/* min to remain disconnected */
	u_short	connect_time_limit;	/* min to remain connected */
	u_short	reserved;
};

/*
 * Page Three - Direct Access Device Format Parameters
 */
struct ccs_format {
	struct	page_header page_header;
	u_short	tracks_per_zone;	/*  Handling of Defects Fields */
	u_short	alt_sect_zone;
	u_short alt_tracks_zone;
	u_short	alt_tracks_vol;
	u_short	sect_track;		/* Track Format Field */
	u_short data_sect;		/* Sector Format Fields */
	u_short	interleave;
	u_short	track_skew_factor;
	u_short	cyl_skew_factor;
	u_char	ssec		: 1;	/* Drive Type Field */
	u_char	hsec		: 1;
	u_char	rmb		: 1;
	u_char	surf		: 1;
	u_char	ins		: 1;
	u_char	reserved_1	: 3;
	u_char	reserved_2;
	u_char	reserved_3;
	u_char	reserved_4;
};

/*
 * Page Four - Rigid Disk Drive Geometry Parameters 
 */
struct ccs_geometry {
	struct	page_header page_header;
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
	u_char	reserved_1;
	u_char	reserved_2;
	u_char	reserved_3;
};
	
struct ccs_mode_select_parms {
	long	edl_len;
	long	reserved;
	long	bsize;
	unsigned fmt_code :8;
	unsigned ncyl     :16;
	unsigned nhead    :8;
	short	rwc_cyl;
	short	wprc_cyl;
	char	ls_pos;
	char	sporc;
};
