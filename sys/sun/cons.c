#ifndef lint
static  char sccsid[] = "@(#)cons.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Indirect console driver for Sun.
 *
 * Redirects all I/O to the device that is the real console (a pseudo-tty,
 * if the console is redirected; a "console device" made out of the primary
 * frame buffer and keyboard, if a workstation; or a serial port, otherwise).
 *
 * Also contains routines for low-level input and output to the console;
 * just indirects to the PROM monitor code (does I/O to the primary keyboard/
 * frame buffer or the serial port).
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/ucred.h>

#include <sun/consdev.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <mon/keyboard.h>

void	resetcons(/*void*/);

/*
 * We must open the real console device, rather than the current console
 * device; otherwise, the opens and closes on those devices may not be
 * properly nested.
 */
/*ARGSUSED*/
cnopen(dev, flag)
	dev_t dev;
	int flag;
{

	return (stropen(&rconsvp, flag, 0));
	/* will not do cloning */
}

/*
 * We must close the real console device, rather than the current console
 * device; otherwise, the opens and closes on those devices may not be
 * properly nested.
 */
/*ARGSUSED*/
cnclose(dev, flag)
	dev_t dev;
{

	/*
	 * Only call the close routine if no open references through
	 * any [s, v]node exist.
	 */
	if (stillopen(rconsdev, VCHR))
		return (0);
	strclose(rconsvp, flag);
	return (u.u_error);
}

/*ARGSUSED*/
cnread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	strread(consvp, uio);
	return (u.u_error);
}

/*ARGSUSED*/
cnwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	strwrite(consvp, uio);
	return (u.u_error);
}

/*ARGSUSED*/
cnioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{

	if (cmd == TIOCCONS ||		/* reset consdev to real console */
	    cmd == _O_TIOCCONS) {	/* XXX */
		resetcons();
		return (0);
	}
	strioctl(consvp, cmd, data, flag);
	return (u.u_error);
}

/*ARGSUSED*/
cnselect(dev, flag)
	dev_t dev;
	int flag;
{

	return (strselect(consvp, flag));
}

/*
 * After verifying permissions, redirect console I/O to a specified device; a
 * vnode for that device must already be known to "specfs".  Return 0 on
 * success, errno value on failure.
 */
int
setcons(dev, uid, gid)
	dev_t	dev;
	u_short	uid;	/* as in struct iocblk */
	u_short	gid;	/* as in struct iocblk */
{
	extern struct vnode *slookup();	/* XXX - put in an include file? */
	struct ucred *cred = crget();
	struct vnode *fsconsvp;
	register int error = 0;

	cred->cr_uid = uid;
	cred->cr_gid = gid;
	/*
	 * XXX:	Wired in knowledge of major/minor values for /dev/console.
	 *	Good only for a patch.
	 */
	fsconsvp = slookup(VCHR, makedev(0, 0));
	error = VOP_ACCESS(fsconsvp, VREAD, cred);
	crfree(cred);
	if (error != 0)
		return (error);

	consdev = dev;
	if ((consvp = slookup(VCHR, consdev)) == NULL)
		panic("setcons: vnode for console missing");

	return (error);
}

/*
 * Undo any redirection of console I/O.
 */
void
resetcons()
{

	/*
	 * Release and reset "consvp" if and only if the
	 * console is re-directed.
	 */
	if (consvp != rconsvp) {
		VN_RELE(consvp);
		consdev = rconsdev;
		consvp = rconsvp;
	}
}
