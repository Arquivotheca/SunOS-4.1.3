/* @(#)ctlr_acb4000.h	1.1  7/30/92 Copyright 1987 Sun Microsystems, Inc. */
/*
 * Controller specific definitions for Adaptec ACB4000.
 */


struct acb4000_mode_select_parms {
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
