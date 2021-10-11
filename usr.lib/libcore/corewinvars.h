/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
/*	@(#)corewinvars.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

	int _core_maskhaschanged;		/* using new input mask */
	int _core_curshaschanged;		/* using new cursor */
	struct cursor _core_oldcursor;		/* saved old cursor */
	struct inputmask _core_oldim;		/* saved old input mask */
	struct sigvec _core_sigwin, _core_sigint;	/* sigwinch, sigint */
