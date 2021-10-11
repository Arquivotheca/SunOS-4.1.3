#ifndef lint
static	char sccsid[] = "@(#)vfs_sys.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Basic file system reading code for standalone I/O system.
 * Simulates a primitive UNIX I/O system (read(), write(), open(), etc).
 * Does not attempt writes to file systems, tho writing to whole devices
 * is supported, when the driver supports it.
 */
#include "boot/param.h"
#include <sys/param.h>
#include <stand/saio.h>
#include "boot/systm.h"
#include <sys/dir.h>
#include <sys/time.h>
#include "boot/vnode.h"
#include <ufs/fs.h>
#include "boot/inode.h"
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/user.h>
#include "boot/iob.h"
#ifdef OPENPROMS
#include "boot/conf.h"
#endif

#undef u
extern struct user u;

/* These are the pools of buffers, iob's, etc. */
char		b[NBUFS][MAXBSIZE];
daddr_t		blknos[NBUFS];
struct iob	iob[NFILES];

static	int	dump_debug = 20;

extern	char	*gethex();

char	*string();
char	*dev_unit_file();
char	*pathname();
char	*server_path();

/*
 *
 */
int
xopen(str, how)
	char    *str;
	int	how;
{
	register char *p;
	char	dev[DEV_NAME_LEN+1];
	char	filename[FILE_NAME_LEN+1];
	char	server[SERVER_NAME_LEN+1];
	char	pathname[PATHNAME_LEN+1];
	int	ctlr,	unit,	part;

	if ((p = dev_unit_file(str, dev, &ctlr, &unit, &part, filename)) !=
	    (char *)-1)	{
		if (*p != '\0')	{
			printf ("garbage after filename '%s'\n",
				filename);
			return (-1);
		}
		return (open_dev_file(dev, ctlr, unit, part, filename));
	} else if ((p = server_path(str, server, pathname)) !=
	    (char *)-1)	{
		if (*p != '\0')	{
			printf ("garbage after pathname '%s'\n",
				pathname);
			return (-1);
		}
		return (open_server_path(server, pathname));
	} else	{
		printf ("syntax error: try again\n");
		return (-1);
	}

}

/*
 * Parse the construct:
 *
 *	<dev_unit_file> -> <dev>(<ctlr>, <unit>, <part>) <filename>
 */
char	*
dev_unit_file(str, dev, ctlr, unit, part, filename)
	char	*str;
	char	*dev;
	int	*ctlr;
	int	*unit;
	int	*part;
	char	*filename;
{
	char	*p;


	if ((p = string(str, dev)) == (char *)-1)	{
		printf ("%s: bad device\n", str);
		return ((char *)-1);
	}
	if (*p++ != '(')	{
		return ((char *)-1);
	}
	p = gethex(p, ctlr);
	if (*p++ != ',')	{
		return ((char *)-1);
	}
	p = gethex(p, unit);
	if (*p++ != ',')	{
		return ((char *)-1);
	}
	p = gethex(p, part);
	if (*p++ != ')')	{
		return ((char *)-1);
	}
	if ((p = string(p, filename)) == (char *)-1)	{
		return ((char *)-1);
	}
	return (p);
}

/*
 * Parses:
 *
 *	<server_path> -> <server> : <pathname>
 */
char	*
server_path(str, server, path)
	char	*str;
	char	*server;
	char	*path;
{
	char	*p;


	if ((p = string(str, server)) == (char *)-1)	{
		printf("%s: bad server name\n", str);
		return ((char *)-1);
	}
	if (*p++ != ':')	{
		printf("%s: no ':' following server name\n", str);
		return ((char *)-1);
	}
	if ((p = pathname(p, path)) == (char *)-1)	{
		printf("%s: bad pathname\n", str);
		return ((char *)-1);
	}
	return (0);
}

/*
 * Parses:
 *	<path> =  / <string>
 */
char	*
pathname(str, path)
	char	*str;
	char	*path;
{
	char	*p, *p1;


	for (p = str, p1 = path; *p; p++, p1++)
		*p1 = *p;

	*p1 = '\0';
	return (p);
}

/*
 * Parses:
 *
 *	<string> -> {[a-z][A-Z][0-9][._-,:+%]}*
 *
 * Terminates the output string with a zero byte.
 * Returns: pointer to the next character after the end of
 * the string, or -1 if there is no valid character in the
 * string.
 */
char	*
string(str, dest)
	char	*str;
	char	*dest;
{
	char	*p, *p1;


	for (p = str; *p == ' '; p++);

	for (p1 = dest; *p; p++, p1++)	{
		if ((*p >= 'a') && (*p <= 'z'))
			*p1 = *p;
		else if ((*p >= 'A') && (*p <= 'Z'))
			*p1 = *p;
		else if ((*p >= '0') && (*p <= '9'))
			*p1 = *p;
		else if ((*p == '.') || (*p == '_') || (*p == '-') ||
			 (*p == ',') || (*p == ':') || (*p == '+') ||
			 (*p == '%') || (*p == '*') || (*p == '/'))
				*p1 = *p;
		else
			break;
	}

	if (p1 == dest)		{	 /* No valid characters. */
		dprint(dump_debug, 6,
			"string: no valid characters p 0x%x p1 0x%x\n", p, p1);
		*p1 = '\0';
		return ((char *)-1);
	}

	*p1 = '\0';

	return (p);	/* Next character after string */
}

open_server_path(server, path)
	char	*server;
	char	*path;
{
	return (-1);
}


int
open_dev_file(dev, ctlr, unit, part, file)
	char	*dev;
	int	ctlr;
	int	unit;
	int	part;
	char	*file;
{
	return (-1);
}

fopen(fnamep, fmode, cmode)
	char *fnamep;
	int fmode;
	int cmode;
{

}



boot_lseek(fd, off, sbase)
	int	fd;
	register off_t  off;
	int    sbase;
{
	struct iob *fp;

	u.u_error = getvnodefp(fd, &fp);
	if (u.u_error) {
#ifdef		DUMP_DEBUG
		dprint(dump_debug, 6, "lseek: bad fd 0x%x\n", fd);
#endif		DUMP_DEBUG
		return (-1);
	}

	if (((struct vnode *)fp->i_ino.i_devvp)->v_type == VFIFO) {
		u.u_error = ESPIPE;
		return (-1);
	}

	switch (sbase) {

	case L_INCR:
		fp->i_offset += off;
		break;

	case L_XTND: {
		struct vattr vattr;

		u.u_error = VOP_GETATTR((struct vnode *)fp->i_ino.i_devvp,
		    &vattr, u.u_cred);
		if (u.u_error)
			return (-1);
		fp->i_offset = off + vattr.va_size;
		break;
	}

	case L_SET:
		fp->i_offset = off;
		break;

	default:
		u.u_error = EINVAL;
	}
	u.u_r.r_off = fp->i_offset;
#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "lseek: new offset 0x%x\n", fp->i_offset);
#endif	DUMP_DEBUG1
	return (0);
}

/*ARGSUSED*/
boot_read(fdesc, buf, count)
	int	fdesc;
	register char   *buf;
	int	count;
{
	int	this_count = 0x2000;
	int	total_read = 0;
	int	this_read = 0;
	int	cc;

#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "read(fdesc %x, buf %x, count %x)\n",
		fdesc, buf, count);
#endif	DUMP_DEBUG1

	while (count > 0)	{
		cc = (count > this_count) ? this_count : count;
		this_read = xread(fdesc, buf, cc);
		if (u.u_error != 0)  {
#ifdef	DEBUG
			printf("boot_read: u.u_error = %d!\n", u.u_error);
#endif	DEBUG
			return (-1);
		}
		if (this_read == 0)  {		/* EOF */
#ifdef	DEBUG
			printf("boot_read: this_read = 0, total_read = %d\n",
			    total_read);
#endif	DEBUG
			break;
		}
		count -= this_read;
		buf += this_read;
		total_read += this_read;
		feedback();	/* Show activity */
	}

#ifdef	DEBUG
	printf("boot_read: returning total_read = %d\n", total_read);
#endif	DEBUG
	return (total_read);
}

xread(fdesc, buf, count)
	int	fdesc;
	register char   *buf;
	int	count;
{
	struct uio auio;
	struct iovec aiov;

#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6,
		"xread(fdesc %x, buf %x, count %x)\n",
		fdesc, buf, count);
#endif	DUMP_DEBUG1

	aiov.iov_base = buf;
	aiov.iov_len = count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	/*
	 *  It's all dark, really.
	 */
	auio.uio_seg = UIOSEG_KERNEL;
	rwuio(fdesc, &auio, UIO_READ);
	return (count-auio.uio_resid);
}

xclose(fdesc)
	int	fdesc;
{
#ifdef	lint
	fdesc = fdesc;
#endif	lint
}

exit()
{

	_stop((char *)0);
}


_stop(s)
	char    *s;
{

	if (s) printf("%s\n", s);
	_exit();
}

panic(s)
	char    *s;
{

	_stop(s);
}

/*
 * Open a device.   No files involved at this point.
 */
opendev(fdesc, file, how)
	int	fdesc;
	struct	iob	*file;
	int	how;
{
#ifdef OPENPROMS
	if (prom_getversion() > 0)
		return (obp_opendev(fdesc, file, how));
#endif
#ifndef sun4m
	/*
	 * Only set ctlr field if device is an IPI ctlr.  This allows
	 * UNIX to be booted from multiple IPI ctlrs.  Can't set
	 * ctlr number in this fashion if sd device (such as CD-ROM)
	 * since the bits are used for unit info (see below)
	 */
	if (major(file->i_ino.i_dev) == 1)
		file->i_si.si_ctlr = minor(file->i_ino.i_dev) >> 6;
	/* XXX
	 * Purposely don't "AND" unit with 0x7, might break
	 * other drivers. Driver must get rid of the ctlr
	 * explicitly.
	 */
	file->i_si.si_unit = minor(file->i_ino.i_dev) >> 3;
	file->i_si.si_boff = minor(file->i_ino.i_dev) & 0x7;
#ifndef OPENPROMS
	/*
	 * XXX HACK HACK HACK - the non-OPENPROM "sd" driver (among others)
	 * assumes that it will be given "unit" as a PROM style
	 * (target * 8) + lun number.  But getblockdev() needs to give
	 * a real UNIX minor number (or loose some bits), thus it does
	 * if ("sd") unit = (((unit & 0xF8) >> 2) | (unit & 0x01));
	 * Now we convert the si_unit for the "sd" driver back into
	 * (target * 8) + lun.
	 * This HACK is here instead of fixing "sd" driver so we
	 * don't have to fix tpboot and standalone copy (may they die soon!)
	 * as this work is being done betwee 4.1 preFCS and FCS!!!
	 */
	/* HARDCODE "sd" major number */
	if (major(file->i_ino.i_dev) == 7)
		file->i_si.si_unit =
		    ((minor(file->i_ino.i_dev) & 0xF0) >> 1) |
		    ((minor(file->i_ino.i_dev) & 0x08) >> 3);
#endif OPENPROMS
	if (devopen(&file->i_si)) {
		file->i_flgs = 0;
#ifdef		DUMP_DEBUG1
		dprint(dump_debug, 6, "opendev: bad open\n");
#endif		DUMP_DEBUG1
		return (-1);	/* if devopen fails, open fails */
	}

	file->i_flgs |= how+1;
	file->i_cc = 0;
	file->i_offset = 0;
	return (fdesc+3);
#endif !sun4m
}

#ifdef OPENPROMS
/*
 * Open a device.   No files involved at this point.
 */
obp_opendev(fdesc, file, how)
	int	fdesc;
	struct	iob	*file;
	int	how;
{
	register struct boottab *dp;
	register struct binfo *bd;
	register int	phandle;
	char	 devtype[32];

	dp = file->i_boottab;
	bd = (struct binfo *)kmem_alloc(sizeof (struct binfo));
	bzero((caddr_t)bd, sizeof (struct binfo));
	(struct binfo *)file->i_si.si_devdata = bd;
	bd->name = dp->b_desc;
	bd->ihandle = 0;
	if (devopen(&file->i_si)) {
		file->i_flgs = 0;
#ifdef		DUMP_DEBUG1
		dprint(dump_debug, 6, "opendev: bad open\n");
#endif		DUMP_DEBUG1
		return (-1);	/* if devopen fails, open fails */
	}
	/*
	 * find out if the open device is a block device
	 */
	phandle = prom_getphandle(bd->ihandle);
	devtype[0] = '\0';
	(void)prom_getprop(phandle, "device_type", devtype);
	switch (dp->b_dev[0]) {
	case 'b':
		if ((strcmp (devtype, "block") != 0) &&
		    (strcmp (devtype, "byte") != 0)) {
			devclose(&file->i_si);
			file->i_flgs = 0;
#ifdef		DUMP_DEBUG
			dprint(dump_debug, 6,
			"opendev: open device is not a block or byte device\n");
#endif		DUMP_DEBUG
			return (-2);	/* if devopen fails, open fails */
		}
		break;
	case 'n':
		if (strcmp (devtype, "network") != 0) {
			devclose(&file->i_si);
			file->i_flgs = 0;
#ifdef		DUMP_DEBUG
			dprint(dump_debug, 6,
			    "opendev: open device is not a network device\n");
#endif		DUMP_DEBUG
			return (-2);	/* if devopen fails, open fails */
		}
		break;
	default:
		devclose(&file->i_si);
		file->i_flgs = 0;
#ifdef		DUMP_DEBUG
		dprint(dump_debug, 6,
		"opendev: open device is not a block, byte or net device\n");
#endif		DUMP_DEBUG
		return (-2);	/* if devopen fails, open fails */
	}
	file->i_flgs |= how+1;
	file->i_cc = 0;
	file->i_offset = 0;
	return (fdesc+3);
}
#endif

rwuio(fdesc, uio, rw)
	int	fdesc;
	register struct uio *uio;
	enum uio_rw rw;
{
	register struct iob *file;
	register struct iovec *iov;
	int i;

#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "rwuio(fdesc 0x%x uio 0x%x rw 0x%x)\n",
		fdesc, uio, rw);
#endif	DUMP_DEBUG1

	file = &iob[fdesc];
	uio->uio_resid = 0;
	/*
	 * It's all dark, really.
	 */
	uio->uio_seg = UIOSEG_KERNEL;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
#ifdef	DEBUG
			printf("rwuio: EINVAL: iov_len = %d\n", iov->iov_len);
#endif	DEBUG
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
#ifdef	DEBUG
			printf("rwuio: EINVAL: iov_len %d, uio_resid %d\n",
			    iov->iov_len, uio->uio_resid);
#endif	DEBUG
			return;
		}
		iov++;
	}
	/*
	 * file->i_offset should be set correctly by lseek.
	 */
	uio->uio_offset = file->i_offset;
	uio->uio_fmode = 0 /* fp->f_flag */;

	if ((i = vno_rw(file, rw, uio)) != 0) {
#ifdef	DEBUG
		printf("rwuio: EIO: error %d from vno_rw!\n", i);
#endif	DEBUG
		u.u_error = EIO;
		return;
	}
	file->i_offset = uio->uio_offset;
#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "rwuio: new offset 0x%x\n", file->i_offset);
#endif	DUMP_DEBUG1
}

vno_rw(fp, rw, uiop)
	struct iob *fp;
	enum uio_rw rw;
	struct uio *uiop;
{
	struct vnode *vp;
	register int error;

	/*
	 * We store a pointer to the vnode in i_devvp.
	 */
	vp = fp->i_ino.i_devvp;
#ifdef	DUMP_DEBUG1
	dprint(dump_debug, 6, "vno_rw: fp 0x%x vp 0x%x\n", fp, vp);
#endif	DUMP_DEBUG1
	if (vp->v_type == VREG) {
		error = VOP_RDWR(vp, uiop, rw, IO_UNIT, u.u_cred);
	} else if (vp->v_type == VFIFO) {
		/*
		 * NOTE: Kludge to ensure that FAPPEND stays set.
		 *	  This ensures that fp->f_offset is always accurate.
		 *
		 *	fp->f_flag |= FAPPEND;
		 */

		/*
		 * NOTE: Kludge to ensure 'no delay' bit passes thru
		 */
		error = VOP_RDWR(vp, uiop, rw, IO_APPEND, u.u_cred);
	} else {
		error = VOP_RDWR(vp, uiop, rw, 0, u.u_cred);
	}
	if (error)  {
#ifdef	DEBUG
		printf("vno_rw: returning error %d from VOP_RDWR, v_type ",
		    error);
		switch (vp->v_type)  {
		case VREG:	printf("VREG\n"); break;
		case VFIFO:	printf("VFIFO\n"); break;
		default:	printf("default (%d)\n", vp->v_type); break;
		}
#endif	DEBUG
		return (error);
	}
	return (0);
}

/*
 * Get the file structure entry for the file descrpitor, but make sure
 * its a vnode.
 */
int
getvnodefp(fd, fpp)
	int fd;
	struct iob **fpp;
{
	register struct iob *fp;

#ifdef	NEVER
	fp = getf(fd);
	if (fp == (struct file *)0)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);
#endif	NEVER
	fp = &iob[fd];

	*fpp = fp;
	return (0);
}

#define		DIR_BUFF_LEN	512

list_directory(dir)
	char	*dir;
{
	char	buffer[DIR_BUFF_LEN];
	long	basep;
	int	directory;
	struct	direct	*dp;

#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6, "list_directory('%s')\n", dir);
#endif	DUMP_DEBUG

	if ((directory = open (dir, O_RDONLY)) == -1)	{
		dprint(dump_debug, 10,
			"list_directory: bad directory %s\n", dir);
		return (-1);
	}
#ifdef	DUMP_DEBUG
	dprint(dump_debug, 6,
		"list_directory: directory '%s' fd 0x%x\n", dir, directory);
#endif	DUMP_DEBUG

	while (1)	{
		if (getdirents(directory, buffer, DIR_BUFF_LEN, &basep) == -1) {
#ifdef		DUMP_DEBUG
		dprint(dump_debug, 10, "list_directory: bad getdirents\n");
#endif		DUMP_DEBUG
		return (-1);
		}
		if (u.u_r.r_val1 == 0)	{
#ifdef			DUMP_DEBUG
			dprint(dump_debug, 6,
				"list_directory: end of directory\n");
#endif			DUMP_DEBUG
			return (0);
		}
		for (dp = (struct direct *)buffer; dp < (struct direct *)
		    ((char *)buffer + u.u_r.r_val1); /* enpty */) {
			printf("ino 0x%x: name '%s'\n",
			dp->d_fileno, &(dp->d_name[0]));
			dp = (struct direct *)((char *)dp + dp->d_reclen);
		}
	}
}
