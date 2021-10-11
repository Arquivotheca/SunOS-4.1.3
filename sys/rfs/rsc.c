/*	@(#)rsc.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/rsc.c	10.19" */

#include <sys/types.h>
#include <rfs/sema.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <sys/uio.h>
#include <sys/mount.h>
#include <rpc/rpc.h>
#include <rfs/rfs_misc.h>
#include <rfs/comm.h>
#include <rfs/message.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/rdebug.h>
#include <rfs/hetero.h>
#include <rfs/rfs_xdr.h>

extern	rcvd_t	cr_rcvd ();
extern	mblk_t	*reusebuf();
extern	mblk_t	*getcbp();
extern int rfserr_size;

/*
 * remote system call
 *
 *	Send a request to the server and wait for the response.
 *	Handle intermediate responses (ACK, NACK, data movement).
 *	This routine should never be called by server.
 *	rp_bp and gift are returned as side effects.
 * 	If an error occurs no rp_bp is returned.
 */
int
rsc (sd, rq_bp, req_size, rp_bp, gift, uio)
sndd_t	sd;			/* which send descriptor to send on	*/
mblk_t	*rq_bp;			/* incoming request block ptr		*/
int	req_size;		/* how many bytes			*/
mblk_t	**rp_bp;		/* ptr to response back from server	*/
sndd_t	gift;			/* gift back from the server		*/
struct uio *uio;		/* for syscalls involving DUCOPYIN/DUCOPYOUT */
{
	mblk_t	*nbp;
	struct	response *resp;	/* the response msg body		*/
	struct	response *nresp;
	struct  request *req;
	rcvd_t  rd;		/* where to receive the response	*/
	int	error = 0;	/* return value 			*/
	int	size, i, n;	/* ignored 				*/
	int 	srsig = 0;	/* Sent remote signal */
	int 	fsz;
	int     rfs_op;		/* Remote system call */
	caddr_t uiobase;

	/* Make a receive descriptor to receive the response */
	if ((rd = cr_rcvd (FILE_QSIZE, SPECIFIC)) == NULL) {
		freemsg (rq_bp);
		return (ENOMEM);
	}
	/* keep track of sd we're going out on */
	rd->rd_sd = sd;

	/* For DUWRITE(I) put first chunk of data with syscall message */
	req = (struct request *) PTOMSG(rq_bp->b_rptr);
	rfs_op = req->rq_opcode;
	if ((rfs_op == DUWRITE || rfs_op == DUWRITEI) && uio) {
                fsz = (uio->uio_resid > DATASIZE) ? DATASIZE : uio->uio_resid;
                req->rq_prewrite = fsz;  /* amount of data present */
		if (error = uiomove(req->rq_data, fsz, UIO_WRITE, uio))
			goto out;
		req_size += fsz;
	}
	/* DUIOCTL, DUFCNTL use same buffer if data is passed both in and out, 
	so save uio args */
	if ((rfs_op == DUIOCTL || rfs_op == DUFCNTL) && uio)
		uiobase = uio->uio_iov->iov_base;

	/* Send the message */
	if (error = sndmsg(sd, rq_bp, req_size, rd))
		goto out;

	/* Get the response */
getit:	if (error = de_queue (rd, rp_bp, gift, &size, &srsig, sd))
		goto out;
	if ((sd->sd_stat & SDLINKDOWN) && gift) {
		DUPRINT1(DB_RECOVER, "rsc: sd went bad during de_queue \n");
		freemsg (*rp_bp);
		error = ENOLINK;
		goto out;
	}
	resp = (struct response *) PTOMSG((*rp_bp)->b_rptr);

        /* Convert RFS error and signal numbers to local client values */
	/* Protective hack -- rp_errno is garbage for some messages */
	if (resp->rp_errno < 0 || resp->rp_errno >= rfserr_size)
		resp->rp_errno = 0;
	if (resp->rp_sig < 0 || resp->rp_sig >= RFS_NSIG) 
		resp->rp_sig = 0;
        resp->rp_errno = RFSTOERR(resp->rp_errno);
        resp->rp_sig = RFSTOSIG(resp->rp_sig);

	/* Sun client considers these errors, so set error code */
	if (resp->rp_opcode == DUDOTDOT)  /* Impossible with current lookup */
		resp->rp_errno = EDOTDOT;
	if (resp->rp_opcode == DULBIN)    /* Not supported by AT&T */
		resp->rp_errno = BADERR;

	DUPRINT6(DB_RSC, "rsc: opcode %d, count %d, subyte %d, errno %d, val %d\n",
	 resp->rp_opcode, resp->rp_count, resp->rp_subyte, resp->rp_errno, resp->rp_rval);
	if (uio) {
	DUPRINT6(DB_RSC, "rsc: uio len %d, base %x, offset %d, seg %d, resid %d\n",
		uio->uio_iov->iov_len, uio->uio_iov->iov_base, 
		uio->uio_offset, uio->uio_seg, uio->uio_resid);
	}

	switch (resp->rp_opcode)  {
	/* move data from kernel on remote machine to user on this machine */
	case DUREAD:
	case DUREADI:
	case DUGETDENTS:
		/* Response has data and no errors */
		if (!resp->rp_subyte && !resp->rp_errno)
			error = uiomove(resp->rp_data, (int) resp->rp_count, 
					UIO_READ, uio);
		break;
	/* move data from kernel on remote machine to user on this machine */
	case DUCOPYOUT:
		if (!resp->rp_errno) {
		   	/* DUIOCTL, FCNTL move data in/out: reset uio params */
		   	if ((rfs_op == DUIOCTL || rfs_op == DUFCNTL) && uio) {
				uio->uio_iov->iov_base = uiobase;
				uio->uio_iov->iov_len = DATASIZE;
				uio->uio_resid = DATASIZE;
		   	}
		   	if (!error && rfs_op == DUGETDENTS && 
		       	    GDP(sd->sd_queue)->hetero != NO_CONV) {
				i = denfcanon((int) resp->rp_count, 
						resp->rp_data, resp->rp_data);
                              	if (i)
					resp->rp_count = i;
		   	}
		   	error = uiomove(resp->rp_data, (int) resp->rp_count,
					UIO_READ, uio);
		}
		/* Send ack, if required, for flow control */
		if (resp->rp_copysync) {
			nbp = getcbp(sizeof(struct response)-DATASIZE);
			nresp = (struct response *) PTOMSG(nbp->b_rptr);
			nresp->rp_type = RESP_MSG;
			nresp->rp_opcode = DUCOPYOUT;
			if (error = sndmsg (gift, nbp, sizeof(struct response)
						-DATASIZE, (rcvd_t)NULL)) {
				printf("rsc: copyout sndmsg on sd %x fail, remote syscall may hang\n",gift);
				freemsg (*rp_bp);
				break;
			}
		}
		freemsg (*rp_bp);
		goto getit;

	/*  copy from user on this machine to kernel on other machine	*/
	case DUCOPYIN:
		for(n = resp->rp_count; n > 0; n -= DATASIZE) {
			nbp = getcbp(sizeof (struct response));
			nresp = (struct response *) PTOMSG(nbp->b_rptr);
			nresp->rp_type = RESP_MSG;
			nresp->rp_opcode = DUCOPYIN;
			nresp->rp_count = (n > DATASIZE) ? DATASIZE : n;
			if (error = uiomove(nresp->rp_data, 
			      (int) nresp->rp_count, UIO_WRITE, uio)) {
				nresp->rp_count = 0;
				nresp->rp_errno = error;
				n = 0;
			}
		   	if (!error && rfs_op == DUUTIME && 
		       	    GDP(sd->sd_queue)->hetero != NO_CONV) {
				i = tcanon(rfs_utimbuf, 
					    nresp->rp_data, nresp->rp_data);
                              	if (i)
					nresp->rp_count = i;
		   	}
			if (error = sndmsg (gift, nbp, 
					(int) (sizeof (struct response) 
					- DATASIZE + nresp->rp_count), 
					(rcvd_t)NULL)) {
				freemsg(*rp_bp);
				goto out;
			}
		}
		freemsg (*rp_bp);
		goto getit;

	default:
		break;
	} /* end of switch */

out:
	free_rcvd (rd);
	return (error);
}
