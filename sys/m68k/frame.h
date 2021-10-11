/*	@(#)frame.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Definition of the sun stack frame
 */

#ifndef _m68k_frame_h
#define _m68k_frame_h

struct frame {
	struct frame	*fr_savfp;	/* saved frame pointer */
	int	fr_savpc;		/* saved program counter */
	int	fr_arg[1];		/* array of arguments */
};

#endif /*!_m68k_frame_h*/
