#ifndef lint
static        char sccsid[] = "@(#)vfs_syscalls.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/socket.h>
#include <net/if.h>
#include <stand/param.h>
#include <stand/saio.h>
#include "boot/vnode.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include "boot/iob.h"

#undef u
extern struct user u;

#ifdef	NFSDEBUG
static int nfsdebug = 10;
#endif	NFSDEBUG

int     errsys();
int	sys_mountroot();
int	sys_exitto();
int	sys_read();
int	sys_open();
int	sys_close();
int	sys_reopen();
int	sys_lseek();

struct sysent sysent[] = {
	0, sys_mountroot,		/*  0 = mountroot */
	1, sys_exitto,			/*  1 = exitto  */
	0, errsys,			/*  2 = illegal	*/
	3, sys_read,			/*  3 = read	*/
	0, errsys,			/*  4 = illegal	*/
	2, sys_open,			/*  5 = open	*/
	1, sys_close,			/*  6 = close	*/
	1, sys_reopen,			/*  7 = reopen 	*/
	0, errsys,			/*  8 = illegal	*/
	0, errsys,			/*  9 = illegal	*/
	0, errsys,			/* 10 = illegal	*/
	0, errsys,			/* 11 = illegal	*/
	0, errsys,			/* 12 = illegal	*/
	0, errsys,			/* 13 = illegal	*/
	0, errsys,			/* 14 = illegal	*/
	0, errsys,			/* 15 = illegal	*/
	0, errsys,			/* 16 = illegal	*/
	0, errsys,			/* 17 = illegal	*/
	0, errsys,			/* 18 = illegal	*/
	3, sys_lseek,			/* 19 = illegal	*/
	0, errsys,			/* 20 = illegal	*/
};

int  nsysent = sizeof (sysent) / sizeof (sysent[0]);

extern  struct fileops vnodefops;

extern    struct  iob     iob[NFILES];

init_syscalls ()
{
	int	vbr;

	vbr = getvbr1();
	set_vec(vbr);
	return;
}
int	sys_code;
caddr_t	sys_args;

/*
 * Called from the trap handler when a system call occurs
 */
syscall(code, args)
      int code;
      caddr_t args;
{
	int	stat;

	sys_code = code;
	sys_args = args;
#ifdef never
	printf ("syscall code 0x%x args 0x%x\n", code, args);
	printf ("syscall: sy_call 0x%x\n", sysent[code].sy_call);
#endif never
	stat = (sysent[code].sy_call)(args);
#ifdef never
	printf ("syscall: stat 0x%x\n", stat);
#endif never
	return (stat);
}

/*
 * This code is intended to implement the routines previously
 * found in sys.c, but using the vnode interface rather than
 * the stand-alone code used there.
 */

/*
 * Mountroot system call.
 */
int
sys_mountroot()
{

#ifdef never
	printf ("sys_mountroot()\n");
#endif never
	vfs_mountroot();
	return(u.u_error);
}

/*
 * Open system call.
 */
int
sys_open(aup)
	struct a {
		char	*str;
		int	how;
	} *aup;
{
	int	stat;

#ifdef never
	printf ("sys_open(aup 0x%x)\n", aup);
	printf ("sys_open: str 0x%x (%s) how 0x%x\n",
	    aup->str, aup->str, aup->how);
#endif never
        stat = copen(aup->str, aup->how - FOPEN, 0);
#ifdef never
	printf ("sys_open: stat 0x%x\n", stat);
#endif never
	return (stat);
}

int
sys_read(aup)
	struct a {
		int	fd;
		caddr_t	buf;
		int	num;
	} *aup;
{
	int	status;

#ifdef never
	printf ("sys_read: aup 0x%x\n", aup);
	printf ("sys_read: fd 0x%x buf 0x%x num 0x%x\n",
	    aup->fd, aup->buf, aup->num);
#endif never
	status = boot_read (aup->fd, aup->buf, aup->num);
#ifdef never
	printf ("sys_read: status 0x%x\n", status);
#endif never
	return (status);
}

int
sys_lseek(aup)
	struct a {
		int	fd;
		off_t	off;
		int	base;
	} *aup;
{
	int	status;

#ifdef never
	printf ("sys_lseek: aup 0x%x\n", aup);
	printf ("sys_lseek: fd 0x%x off 0x%x base 0x%x\n",
	    aup->fd, aup->off, aup->base);
#endif never
	status = boot_lseek (aup->fd, aup->off, aup->base);
#ifdef never
	printf ("sys_lseek: status 0x%x\n", status);
#endif never
	return (status);
}

/*
 * In case someone insists on closing a file. See below.
 */
sys_close()
{
	return(0);
}

/*
 * Before transferring control to another program, be sure to close
 * all open files and devices.
 * XXX-Actually, just close the devices at this point. The close routine
 * needs a lot more work, since it doesn't really clear out enough
 * state yet and the boot os gets confused and goes south.
 */
sys_exitto(aup)
	struct a {
		int	(*func)();
	} *aup;
{
	register int i;
	register struct saioreq *sip;

	for (i = 0; i < NFILES; i++) {
		if (iob[i].i_flgs != 0) {
			sip = &iob[i].i_si;
			if (sip->si_boottab) {
				devclose(&iob[i].i_si);
				iob[i].i_flgs |= F_AJAR;
			}
		}
	}
/*
 * Return to the syscall stub, which will call _exitto(func).
 */
	return((int)aup->func);
}

/*
 * Walk the file table and reopen any devices that are "ajar:"
 * i.e., the device is inactive but the file descriptor isn't closed.
 * This is primarily for the ethernet devices.
 */
sys_reopen(aup)
	struct a {
		char	*str;
	} *aup;
{
	register int i;

	for (i = 0; i < NFILES; i++) {
		if (iob[i].i_flgs & F_AJAR) {
			devreopen(&iob[i].i_si);
			iob[i].i_flgs &= ~F_AJAR;
		}
	}
}

/*
 * Common code for open, creat.
 */
copen(pnamep, filemode, createmode)
        char *pnamep;
        int filemode;
        int createmode;
{
        register struct iob *file;
        int     fdesc;
	int	error;
	struct vnode *vp;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"copen:(pnamep '%s' filemode 0x%x createmode 0x%x)\n",
		pnamep, filemode, createmode);
#endif	NFSDEBUG

        /*
         * allocate a user file descriptor and file table entry.
         */
	fdesc = getiob();               /* Allocate an IOB */
        file = &iob[fdesc];

	/*
         * open the vnode.
         */
        error =
            vn_open(pnamep, UIOSEG_KERNEL,
                filemode, ((createmode & 07777) & ~u.u_cmask), &vp);
	if (error)	{
		(void) ungetiob(fdesc);
		printf ("copen: bad vn_open\n");
		return(-1);
	}
	/*
	 * This is a bit nasty.   A real inode contains the
	 * associated vnode within it, but our vnode for the
	 * remote file is in the rnode, so we will steal the
	 * i_devvp field to point to the vnode.
	 */
	file->i_ino.i_devvp = vp;
#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"copen: fdesc 0x%x file 0x%x vp 0x%x\n", fdesc, file, vp);
#endif	/* NFSDEBUG */

        return(fdesc);
}

/*
 * get directory entries in a file system independent format
 */
getdirents(fd, buf, count, basep)
	int     fd;
	char    *buf;
	unsigned count;
	long    *basep;
{
	struct iob *fp;
	struct uio auio;
        struct iovec aiov;

#ifdef	NFSDEBUG
	dprint(nfsdebug, 6,
		"getdirents(fd 0x%x buf 0x%x count 0x%x basep 0x%x)\n",
		fd, buf, count, basep);
#endif	/* NFSDEBUG */

	u.u_error = getvnodefp(fd, &fp);
	if (u.u_error)	{
	       return(-1); 
	}
	aiov.iov_base = buf;
        aiov.iov_len = count;
        auio.uio_iov = &aiov;
        auio.uio_iovcnt = 1;
        auio.uio_offset = fp->i_offset;
        auio.uio_seg = UIOSEG_KERNEL;
        auio.uio_resid = count;
        u.u_error = VOP_READDIR((struct vnode *)fp->i_ino.i_devvp,
			 &auio, u.u_cred);
        if (u.u_error)
                return(-1);
	u.u_r.r_val1 = count - auio.uio_resid;
	fp->i_offset = auio.uio_offset;
	return(0);
}

/*
 * display I/O statistics
 */
iostats()
{
	register i,j;
	struct ifnet *ifp, *if_ifwithafup();
        struct saioreq *sip;
	extern	int	ethernet_started;

	printf("Statistics:\n");

	/* client rpc stats (clnt_kudp) */
	rpcstats();

        if((ifp = if_ifwithafup(AF_INET)) == (struct ifnet *)0)
		return;
        if ((sip = (struct saioreq *)(ifp->if_lower)) == (struct saioreq*) 0)
		return;

	if (ethernet_started)
		if (sip->si_sif)
			if (sip->si_sif->sif_prstats)
				(*sip->si_sif->sif_prstats)();

	printf("\n");
}
