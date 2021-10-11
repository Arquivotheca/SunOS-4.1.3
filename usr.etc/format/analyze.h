
/*      @(#)analyze.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains definitions related to surface analysis.
 */
#ifndef	_ANALYZE_
#define	_ANALYZE_

/*
 * These are variables referenced by the analysis routines.  They
 * are declared in analyze.c.
 */
extern	int scan_entire;
extern	daddr_t scan_lower, scan_upper;
extern	int scan_correct, scan_stop, scan_loop, scan_passes;
extern	int scan_random, scan_size, scan_auto;
extern  int scan_restore_defects, scan_restore_label;
extern  int scan_patterns[], purge_patterns[], alpha_pattern;

/*
 * These variables hold summary info for the end of analysis.  They
 * are declared in analyze.c.
 */
extern	daddr_t scan_cur_block;
extern	int scan_blocks_fixed;

/*
 * This variable is used to tell whether the most recent surface
 * analysis error was caused by a media defect or some other problem.
 * It is declared in analyze.c.
 */
extern	int media_error;

/*
 * These defines are flags for the surface analysis types.
 */
#define	SCAN_VALID		0x01		/* read data off disk */
#define	SCAN_PATTERN		0x02		/* write and read pattern */
#define	SCAN_COMPARE		0x04		/* manually check pattern */
#define	SCAN_WRITE		0x08		/* write data to disk */
#define SCAN_PURGE              0x10            /* purge data on disk */
#define SCAN_PURGE_READ_PASS    0x20            /* read/compare pass */
#define SCAN_PURGE_ALPHA_PASS   0x40            /* alpha pattern pass */


/*
 * Miscellaneous defines.
 */
#define	BUF_SECTS		126		/* size of the buffers */
/*
 * Number of passes for purge command.  It is kept here to allow
 * it to be used in menu_analyze.c also
 * This feature is added at the request of Sun Fed.
 */
#define NPPATTERNS      4               /* number of purge patterns */

#endif	!_ANALYZE_

