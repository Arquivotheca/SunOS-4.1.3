/*	@(#)tty_tty.c 1.1 92/07/30 SMI; from UCB 4.14 82/12/05	*/

/*
 * Indirect driver for controlling tty, which is assumed to be a streams
 * device.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/stream.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/session.h>

/*ARGSUSED*/
syopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int error;

	if ((error = sycheck()) != 0)
		return (error);
	return (stropen(&u.u_procp->p_sessp->s_vp, flag, 0));
}

/*ARGSUSED*/
syread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int error;

	if ((error = sycheck()) != 0)
		return (error);
	strread(u.u_procp->p_sessp->s_vp, uio);
	return (u.u_error);
}

/*ARGSUSED*/
sywrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int error;

	if ((error = sycheck()) != 0)
		return (error);
	strwrite(u.u_procp->p_sessp->s_vp, uio);
	return (u.u_error);
}

/*ARGSUSED*/
syioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
{
	register int error;

	if (cmd == TIOCNOTTY) {
		register struct sess *sp;

		sp = u.u_procp->p_sessp;
		if (sp->s_sid == u.u_procp->p_pid && sp->s_members == 1) {
			SESS_VN_RELE(sp);
			return (0);
		} else {
			return (setsid(SESS_NEW));
		}
	}
	if ((error = sycheck()) != 0)
		return (error);
	strioctl(u.u_procp->p_sessp->s_vp, cmd, addr, flag);
	return (u.u_error);
}

/*ARGSUSED*/
syselect(dev, flag)
	dev_t dev;
	int flag;
{
	register int error;

	if ((error = sycheck()) != 0)
		return (error);
	return (strselect(u.u_procp->p_sessp->s_vp, flag));
}

/*
 * Check that the reference to the controlling tty is valid.
 * The process must have a controlling tty, and it must be an active
 * streams device.  (Note: this means that when the last process using that
 * tty closes it, it ceases to be accessible from "/dev/tty", and that it
 * becomes accessible again if it gets reopened.)
 * ADD ANY PERMISSIONS CHECKING HERE.
 */
int
sycheck()
{
	register struct vnode *vp;

	if ((vp = u.u_procp->p_sessp->s_vp) == NULL || vp->v_stream == NULL)
		return (ENXIO);
	return (0);
}
