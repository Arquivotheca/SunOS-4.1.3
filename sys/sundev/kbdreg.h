/*	@(#)kbdreg.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Keyboard implementation private definitions.
 */

#ifndef _sundev_kbdreg_h
#define _sundev_kbdreg_h

struct keyboardstate {
	u_char	k_id;
	u_char	k_idstate;
	u_char	k_state;
	u_char	k_rptkey;
	u_int	k_buckybits;
	u_int	k_shiftmask;
	struct	keyboard *k_curkeyboard;
	u_int	k_togglemask;	/* Toggle shifts state */
};

/*
 * States of keyboard ID recognizer
 */
#define	KID_NONE	0		/* startup */
#define	KID_IDLE	1		/* saw IDLE code */
#define	KID_LKSUN2	2		/* probably Sun-2 */
#define	KID_OK		3		/* locked on ID */

#endif /*!_sundev_kbdreg_h*/
