#ifndef lint
static char	sccsid[] = "@(#)ft.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * ft reads a remote file via tftp it is only useful for initializing the ram
 * disk driver, rd.
 */

#include "ft.h"

#if NFT > 0

#include <sys/param.h>		/* Includes <sys/types.h> */
#include <sys/errno.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/kmem_alloc.h>
#include <ufs/fs.h>
#include <sundev/mbvar.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <netinet/in.h>
#include <arpa/tftp.h>
#include <net/if.h>

extern struct buf *bread();
extern struct fileops socketops;

#define	PKTSIZE	SEGSIZE+4

static int ftdebug = 0, dbblock = 5 * 1024 * 1024;

/* UNITIALIZED matches value of unitialized data structure */
typedef enum {
	UNINITIALIZED = 0, FAILED, INITIALIZED, RUNNING, EOF
} ftstate_t;

typedef struct {
	ftstate_t	state;
	struct sockaddr_in sin;
	char		*path;	/* pathname */
	int		psize;	/* size of path name buffer */
	char		*data;	/* data buffer */
	int		dsize;	/* size of data buffer */
	int		fd;	/* file descriptor */
	struct file    *fp;	/* file pointer for socket */
	int		opencnt;
	union {
		char	ackdata[PKTSIZE];
		struct tftphdr  ackheader;
	} ackbuf;
} tftpdev_t;

tftpdev_t	ft[NFT];

static		ip_if_ok = 0;

#ifndef	OPENPROMS
/* This structure exists because swapgeneric.c:getchardev() wants to see it. */
struct mb_driver ftdriver = {
	0,			/* probe:	see if a driver is really */
				/* there */
	0,			/* slave:	see if a slave is there */
	0,			/* attach:	setup driver for a slave */
	0,			/* go:		routine to start transfer */
	0,			/* done:	routine to finish transfer */
	0,			/* poll:	polling interrupt routine */
	0,			/* size:	mount of memory space needed */
	"ft",			/* dname:	name of a device */
	0,			/* dinfo:	backpointers to mbdinit */
				/* structs */
	"ft",			/* cname:	name of a controller */
	0,			/* cinfo:	backpointers to mbcinit */
				/* structs */
	MDR_PSEUDO,		/* flags:	Mainbus usage flags */
	0,			/* link:	interrupt routine linked list */
};
#endif	OPENPROMS


/*
 * ipprompt - prompt the console for an ip address
 *	XXX - ipprompt only takes one form of address. there are library
 *	XXX - routines that can handle the translation better.
 */
u_long
ipprompt(name)
	char *name;
{
	struct in_addr  addr;

	addr.s_addr = 0;

	while (addr.s_addr == 0) {
		char		line[80];
		register char  *p;
		register int    num;

		p = line;
		do {
			printf("%s IP address: ", name);
			gets(p);
		} while (!*p || *p < '0' || *p > '9');

		for (addr.s_addr = 0; *p && *p >= '0' && *p <= '9';) {
			for (num = 0; *p && *p >= '0' && *p <= '9'; ++p)
				num = (num * 10) + (*p - '0');

			if (!*p || *p == '.') {
				addr.s_addr = (addr.s_addr << 8) + num;
				if (*p)
					++p;
			} else {
				addr.s_addr = 0;
				break;
			}
		}
	}

	return (addr.s_addr);
}


/* kern_sockargs is sockargs() without copyin() */
static
kern_sockargs(aname, name, namelen, type)
	struct mbuf	**aname;
	caddr_t		name;
	int		namelen, type;
{
	register struct mbuf *m;

	if (namelen > MLEN)
		return (EINVAL);
	m = m_get(M_WAIT, type);
	if (m == NULL)
		return (ENOBUFS);
	m->m_len = namelen;
	{
		register caddr_t from, to;

		for (from = name, to = mtod(m, caddr_t); namelen; --namelen)
			*to++ = *from++;
	}
	*aname = m;
	return (0);
}


/*
 * tftpdev - initiate a tftp transfer but do not transfer any data. return
 * the status of the link (true/false).
 */
tftpdev_t *
tftpdev(dev)
	dev_t		dev;
{
	register tftpdev_t *ftp;
	register struct file *fp;
	int		error;
	char		line[100];

	/*
	 * Check the device status. if previous initializations failed or if
	 * the minor number is invalid, give up.
	 */
	ftp = &ft[minor(dev)];
	if (ftp < &ft[NFT] && ftp->state != UNINITIALIZED) {
		if (ftp->state != INITIALIZED &&
		    ftp->state == RUNNING &&
		    ftp->state == EOF)
			ftp = (tftpdev_t *) 0;
		return (ftp);
	} else if (ftp >= &ft[NFT])
		return ((tftpdev_t *) 0);

	/* The minor number is valid and the device is uninitialized */
	/* setup transfer params */

	printf("tftp path: ");
	gets(line);
	if (line[0] == '\0')
		(void) strcpy(line, "munix.fs");
	ftp->psize = (strlen(line) + 0x1f) & ~0x1f;
	if ((ftp->path = new_kmem_alloc(ftp->psize), KMEM_NOSLEEP) == NULL) {
		ftp->psize = 0;
		printf("ft: no memory\n");
		return (NULL);
	}
	(void) strcpy(ftp->path, line);
	printf("requesting %s\n", ftp->path);

	ftp->dsize = PKTSIZE;
	if ((ftp->data = new_kmem_alloc(ftp->dsize), KMEM_NOSLEEP) == NULL) {
		ftp->dsize = 0;
		printf("ft: no memory\n");
		return (NULL);
	}
	error = 0;

	/* socket() */
	{
		struct socket  *so;

		if ((fp = falloc()) == NULL)
			return (NULL);
		ftp->fd = u.u_r.r_val1;	/* this is bullshit */
		fp->f_flag = FREAD | FWRITE;
		fp->f_type = DTYPE_SOCKET;
		fp->f_ops = &socketops;
		error = socreate(AF_INET, &so, SOCK_DGRAM, 0);
		if (error == 0) {
			/* so->so_options |= SO_DONTROUTE; */
			fp->f_data = (caddr_t) so;
		} else {
			printf("ft: socket stuff fails with code %d\n", error);

			/* get rid of the file pointer */
			u.u_ofile[ftp->fd] = 0;
			crfree(fp->f_cred);
			fp->f_count = 0;
			fp = NULL;
		}
		ftp->fp = fp;
	}

	if (error == 0) {
		struct mbuf    *nam;

		bzero(&ftp->sin, sizeof (ftp->sin));

		ftp->sin.sin_family = AF_INET;

		/* code for bind() */

		error = kern_sockargs(&nam, &ftp->sin, sizeof ftp->sin, MT_SONAME);

		if (error != 0)
			printf("ft: kern_sockargs fails code=%d\n", error);
		else {
			error = sobind((struct socket *) fp->f_data, nam);
			m_freem(nam);
			if (error != 0)
				printf("ft: sobind fails code=%d\n", error);
		}

		ftp->sin.sin_port = IPPORT_TFTP;
		ftp->sin.sin_addr.s_addr = ipprompt("tftp server");
		if (ftp->sin.sin_addr.s_addr == 0) {
			ftp->state = FAILED;
			return (NULL);
		}
	}
	if (error != 0)
		ftp->state = FAILED;
	else
		ftp->state = INITIALIZED;

	if (!ip_if_ok) {
		struct ifnet *ifp, *ifb_ifwithaf();

		ifp = ifb_ifwithaf(AF_INET);
		if (ifp == 0) {
			printf("ft: zero ifp\n");
		}
		if (!address_known(ifp)) {
			printf("ft: calling revarp\n");
			revarp_myaddr(ifp);
			ip_if_ok = 1;
		}
	}
	return (ftp->state == INITIALIZED ? ftp : (tftpdev_t *) 0);
}


/* ARGSUSED */
ftopen(dev, wrtflag)
	dev_t		dev;
	int		wrtflag;
{
	register tftpdev_t
			*ftp;

	if ((ftp = tftpdev(dev)) == (tftpdev_t *)NULL)
		return (ENXIO);
	if (ftp->opencnt != 0)
		return (EBUSY);

	ftp->opencnt = 1;
	return (0);
}


/* ARGSUSED */
ftclose(dev, flag)
	dev_t		dev;
	int		flag;
{
	register tftpdev_t *ftp;

	if ((ftp = tftpdev(dev)) == NULL || (ftp->opencnt == 0))
		return (ENXIO);

	if (ftp->psize)
		kmem_free(ftp->path, ftp->psize);
	ftp->path = (caddr_t) 0;
	ftp->psize = 0;
	if (ftp->dsize)
		kmem_free(ftp->data, ftp->dsize);
	ftp->data = (caddr_t) 0;
	ftp->dsize = 0;

	ftp->opencnt = 0;
	return (0);
}


/*
 * ftsize - if this driver is made into a full fledged block device, this
 * this function will return the size of the remote file.
 */
/* ARGSUSED */
ftsize(dev)
	dev_t		dev;
{
	return (0);
}


static
mysendit(ftp, mp, flags)
	register tftpdev_t *ftp;
	register struct msghdr *mp;
	int		flags;
{
	struct file *getsock();
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *to;
	int		len, error;

	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_offset = 0;	/* XXX */
	auio.uio_resid = 0;
	iov = mp->msg_iov;

	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
		if (iov->iov_len < 0) {
			error = EINVAL;
			return (error);
		}
		if (iov->iov_len == 0)
			continue;
		auio.uio_resid += iov->iov_len;
	}
	if (mp->msg_name) {
		error = kern_sockargs(
			    &to, mp->msg_name, mp->msg_namelen, MT_SONAME);
		if (error) {
			printf("ft: mysendit: kern_sockargs 0 = %d\n", error);
			return (error);
		}
	} else
		to = 0;

	len = auio.uio_resid;
	error = sosend((struct socket *) ftp->fp->f_data, to, &auio, flags, 0);
	if (ftdebug)
		printf("ft: mysendit: len=%d auio.uio_resid=%d\n", len, auio.uio_resid);
	u.u_r.r_val1 = len - auio.uio_resid;

	if (error)
		printf("ft: mysendit: sosend returns errno = %d\n", error);

	if (to)
		m_freem(to);

	return (error);
}


void
myrecvit(ftp, mp, flags, namelenp)
	register tftpdev_t *ftp;
	register struct msghdr *mp;
	int		flags, *namelenp;
{
	struct uio auio;
	register struct iovec *iov;
	register int i;
	struct mbuf *from, *rights;

	auio.uio_iov = mp->msg_iov;
	auio.uio_iovcnt = 1;
	auio.uio_segflg = UIO_SYSSPACE;	/* XXX */
	auio.uio_offset = 0;	/* XXX */
	auio.uio_resid = PKTSIZE;

	{
		int	len;

		len = auio.uio_resid;
		u.u_error =
			soreceive((struct socket *)ftp->fp->f_data,
					&from, &auio, flags, &rights);
		u.u_r.r_val1 = len - auio.uio_resid;
	}

	if (mp->msg_name) {
		int	len;

		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			bcopy(mtod(from, caddr_t), mp->msg_name, len);
		}
		*namelenp = len;
	}
	if (rights)
		m_freem(rights);
	if (from)
		m_freem(from);
	return;
}


static char    *opcodes[] = {"BAD", "RRQ", "WRQ", "DATA", "ACK", "ERROR"};

ftread(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register tftpdev_t *ftp;
	register struct tftphdr *ap;
	u_short		block, size;
	struct msghdr send_msg, recv_msg;
	struct iovec recv_iov, send_iov;
	int		fromlen, error, ret, bytes, count;
	label_t		jumpbuf;
	int		setjmp();
	void		longjmp();

	if ((ftp = tftpdev(dev)) == (tftpdev_t *) 0)
		return (EINVAL);

	ap = &ftp->ackbuf.ackheader;

	block = btodb(uio->uio_offset);
	bytes = uio->uio_resid;
	while (bytes > 0) {
		if (ftp->state == INITIALIZED) {
			char	*cp;

			ftp->sin.sin_port = IPPORT_TFTP;

			ap->th_opcode = htons((u_short) RRQ);
			cp = ap->th_stuff;

			(void) strcpy(cp, ftp->path);
			cp += strlen(ftp->path);
			*cp++ = '\0';

			(void) strcpy(cp, "octet");
			cp += strlen("octet");
			*cp++ = '\0';

			size = cp - (char *) ap;

			if (ftdebug)
				printf(
			"ft: start xfer opcode=%s path=\"%s\" mode=\"%s\"\n",
				    opcodes[RRQ], ftp->path, "octet");
		} else if (ftp->state == RUNNING) {
			ap->th_opcode = htons((u_short) ACK);
			ap->th_block = htons(block);
			if (ftdebug)
				printf("ft: opcode=%s block=%d\n", opcodes[ACK], block);
			size = 4;
		} else if (ftp->state == EOF) {
			return (0);
		}
		/* need a timeout */
		if (setjmp(&jumpbuf))
			printf("<timeout>");

		/* sendto() */
		send_msg.msg_name = (caddr_t) & ftp->sin;
		send_msg.msg_namelen = sizeof (ftp->sin);
		send_msg.msg_iov = &send_iov;
		send_msg.msg_iovlen = 1;
		send_iov.iov_base = (caddr_t) ap;
		send_iov.iov_len = size;
		send_msg.msg_accrights = 0;
		send_msg.msg_accrightslen = 0;

		/* recvfrom() */
		recv_msg.msg_name = (caddr_t) & ftp->sin;
		recv_msg.msg_namelen = sizeof (ftp->sin);
		recv_msg.msg_iov = &recv_iov;
		recv_msg.msg_iovlen = 1;
		recv_iov.iov_base = ftp->data;	/* data buffer addr */
		recv_iov.iov_len = ftp->dsize;	/* length of data buffer */
		recv_msg.msg_accrights = 0;
		recv_msg.msg_accrightslen = 0;

		error = mysendit(ftp, &send_msg, 0);
		ret = u.u_r.r_val1;
		if (ret != size) {
			printf("ft: SENT size=%d, sendit()==%d, errno=%d\n",
			    size, ret, error);
			printf("ft: mysendit fails, %d != size\n", ret);
			return (error);
		}
		timeout(longjmp, &jumpbuf, 60 * 30);

		count = 0;
		error = 0;
		fromlen = 0;
		while (count == 0 && error == 0 && fromlen == 0) {
			myrecvit(ftp, &recv_msg, 0, &fromlen);
			count = u.u_r.r_val1;
			error = u.u_error;
			if (count || error || fromlen)
				if (ftdebug)
					printf(
				"ft: myrecvit()==%d, errno = %d fromlen=%d\n",
					    count, error, fromlen);
		}
		untimeout(longjmp, &jumpbuf);
		if (ftdebug)
			printf("OK\n");

		ap = (struct tftphdr *) ftp->data;
		ap->th_opcode = ntohs(ap->th_opcode);
		ap->th_block = ntohs(ap->th_block);

		if (ftdebug)
			printf("ft: RECEIVED opcode=%s, block=%d\n",
			    opcodes[ap->th_opcode], ap->th_block);
		if (ftdebug) {
			int	k;

			printf("ft: recv_msg:name=");
			for (k = 0; k < recv_msg.msg_namelen;) {
				printf("%d",
				    ((unsigned char *) recv_msg.msg_name)[k]);
				if (++k < recv_msg.msg_namelen)
					printf(".");
			}
			printf(" iov=x%x iovlen=%d accrights=%X accrightslen=%d\n",
			    recv_msg.msg_iov, recv_msg.msg_iovlen,
			    recv_msg.msg_accrights,
			    recv_msg.msg_accrightslen);
		}
		if (error)
			return (error);

		if (block + 1 != ap->th_block)
			continue;

		if (count <= 4)
			continue;
		count -= 4;

		ftp->state = RUNNING;

		uio->uio_resid = count;
		uio->uio_iov->iov_len = count;

		if (ftdebug)
			printf("ft: iov_len = %x iov_base = %x uio_resid = %x\n",
			    uio->uio_iov->iov_len,
			    uio->uio_iov->iov_base,
			    uio->uio_resid);

		error = uiomove(ap->th_data, count, UIO_READ, uio);
		if (error) {
			if (ftdebug)
				printf("ft: uiomove error = %d\n", error);
			return (error);
		}
		/* uio->uio_iov->iov_base += count; */
		bytes -= count;
		if (++block >= dbblock)
			ftdebug = 1;

		if (count < SEGSIZE) {
			if (ftdebug)
				printf("EOF DETECTED\n");
			ftp->state = EOF;
			break;
		}
	}
	return (0);
}


/* ARGSUSED */
ftwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	return (EINVAL);
}


ftstrategy(bp)
	register struct buf *bp;
{
	register tftpdev_t *ftp;
	register long offset = dbtob(bp->b_blkno);
	int	error;

	if ((ftp = tftpdev(bp->b_dev)) == (tftpdev_t *) 0) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
	} else {
		struct uio	uio;
		struct iovec    iov;

		if (bp->b_flags & B_PAGEIO)
			bp_mapin(bp);

		iov.iov_len = bp->b_bcount;
		iov.iov_base = bp->b_un.b_addr;
		uio.uio_iov = &iov;
		uio.uio_iovcnt = 1;

		uio.uio_offset = offset;
		uio.uio_segflg = UIO_SYSSPACE;	/* XXX */
		uio.uio_fmode = ftp->fp->f_flag;
		uio.uio_resid = iov.iov_len;

		if (bp->b_flags & B_READ)
			error = ftread(bp->b_dev, &uio);
		else
			error = ftwrite(bp->b_dev, &uio);

		if (error) {
			bp->b_error = error;
			bp->b_flags |= B_ERROR;
		} else {
			bp->b_error = 0;
			bp->b_flags &= B_ERROR;
		}

		bp->b_resid = uio.uio_resid;
	}
	iodone(bp);
}
#endif
