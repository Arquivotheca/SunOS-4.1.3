/*	@(#)lasertool.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * Header file with some useful defines used throughout
 * the code
 */
#define FWD		0	/* Play disk forwards */
#define REV		1 	/* Play disk in reverse */

#define POLL_PRINT	10	/* Continue to print frame #, etc */
#define POLL_SEARCH	20	/* Stop print info and look for "poll_ch" */
#define POLL_OFF	30	/* Don't do anything when poll is called */
