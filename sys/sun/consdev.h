/*	@(#)consdev.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#ifndef _sun_consdev_h
#define	_sun_consdev_h

dev_t	consdev;	/* user retargettable console */
struct vnode *consvp;	/* pointer to vnode for that device */
dev_t	rconsdev;	/* real console - 0 if not retargetted */
struct vnode *rconsvp;	/* pointer to vnode for that device */
dev_t	mousedev;	/* default mouse device */
dev_t	kbddev;		/* default keyboard device */
int	kbddevopen;	/* keyboard open flag */
dev_t	fbdev;		/* default framebuffer device */

#endif /*!_sun_consdev_h*/
