#ifndef lint
static char sccsid[] = "@(#)subr_dump.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Dump driver.
 * Allows reading and writing of crash dumps from the "dump file" (dumpvp).
 * The NEWDUMP (sparse physical memory) scheme requires that /dev/dump
 * be the beginning of the dump file; no offset is added.
 * It also requires that lseek(L_XTND) be allowed on /dev/dump, so that
 * savecore can seek backwards from the end of the file.
 */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/file.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <specfs/snode.h>

/*ARGSUSED*/
dumpopen(dev, flag)
	dev_t dev;
	int flag;	/* flags from open call */
{
	int		 error;
	struct vnode	*vp;
	struct vattr	 vattr;

	/*
	 * Get the size of the dump file and store it in the snode so
	 * that lseek(L_XTND) will work.
	 * XXX there is a hack in spec_getattr so that this will work.
	 */
	if ((vp = slookup(VCHR, dev)) == NULL)
		panic("dumpopen: vnode for /dev/dump not found");

	error = VOP_GETATTR(dumpvp, &vattr, u.u_cred);
	if (error)
		goto out;

	VTOS(vp)->s_size = vattr.va_size;	/* snarf the size */

out:
	VN_RELE(vp);	/* was held by slookup */
	return (error);
}

/*ARGSUSED*/
dumpread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (VOP_RDWR(dumpvp, uio, UIO_READ, 0, u.u_cred));
}

/*ARGSUSED*/
dumpwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	return (VOP_RDWR(dumpvp, uio, UIO_WRITE, 0, u.u_cred));
}
