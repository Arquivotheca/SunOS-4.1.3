/*	@(#)cirmgr.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include <sys/param.h>
#include <sys/types.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/rfs_xdr.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/kernel.h>
#include <sys/user.h>
#include <sys/errno.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/mount.h>
#include <rfs/comm.h>
#include <rfs/recover.h>
#include <rfs/rdebug.h>
#include <rfs/nserve.h>
#include <sys/vnode.h>
#include <rfs/cirmgr.h>
#include <nettli/tihdr.h>
#include <rfs/message.h>
#include <rfs/hetero.h>
#include <sys/debug.h>


extern	rcvd_t rd_recover;	/* special recovery rd */
extern int nulldev();
extern qenable();
static int gdp_rput(), gdp_rsrv(),gdp_wsrv();
static struct module_info gdp_info = {0, NULL, 0, INFPSZ, FILE_QSIZE*1024, 1024};
static struct qinit gdprdata = 
	{ gdp_rput, gdp_rsrv, nulldev, nulldev, nulldev, &gdp_info, NULL };
static struct qinit gdpwdata =
	{ nulldev, gdp_wsrv, nulldev, nulldev, nulldev, &gdp_info, NULL };
extern int msgflag;
extern void setq();
static int gdp_renable();

static mblk_t *splitmsg();
mblk_t *gdp_bp = NULL;

/*
 *	Obtain the queue pointer representing a remote machine.  Normally, 
 *	take the queue associated with fd (it had better be streams-based),
 *	and associate with it the token, for future reference.  If the
 *	fd is -1, then return the queue pointer previously associated
 *	with token.
 *	Will fail if a) the fd is -1, and token doesn't match any of
 *	those stored, or b) fd is OK, but no more entry in the table.
 */

queue_t *
get_circuit (fd,tokenp)
register struct token *tokenp;
{
	queue_t *init_circuit ();
	register struct gdp *tmp;

	for (tmp = gdp; tmp < &gdp[maxgdp] ; tmp++) {
		if (fd >= 0)  {
			if(tmp->flag == GDPFREE)
				return(init_circuit(fd,tmp));
		} else if ((tmp->flag & GDPCONNECT) && 
		       (tokcmp (&(tmp->token), tokenp))) {
		DUPRINT2(DB_GDP,"getcircuit: ckt exists qp = %x\n",gdp->queue);
		return (tmp->queue);
	    	}
	}

	DUPRINT1(DB_GDP,"getcircuit: circuit doesn't exist\n");
	u.u_error = ECOMM;
	return(NULL);
}

put_circuit (qp)
queue_t *qp;
{
	register struct gdp *gdpp = GDP(qp);

	DUPRINT2(DB_GDP,"put_circuit: qp %x\n", qp);
	if (gdpp->mntcnt) 
		return;
	clean_circuit (qp);
}


gdp_init ()
{
	register struct gdp *tmp;
	register int i;

	DUPRINT1(DB_GDP,"gdp_init: initializing gdp\n");
	/*
	 * reserve one buffer to gdp, may reserve more if so desired
	 */
	if ((gdp_bp = allocb(sizeof(struct request) + sizeof(struct message),
	    BPRI_MED)) == NULL) {
		DUPRINT1(DB_GDPERR, "gdp_init: no buffer\n");
		u.u_error = ENOMEM;
		return;
	}

	for (tmp = gdp, i = 0; i < maxgdp; tmp++) {
		tmp->queue = NULL;
		tmp->sysid = ++i; /* set the local half of sysid --
					the remote half is set when
					the first message arrives */
		tmp->flag = GDPFREE;
		tmp->token.t_id = 0;
		tmp->token.t_uname[0] = '\0';
		tmp->idmap[0] = 0;
		tmp->idmap[1] = 0;
		tmp->oneshot = 0;
		cleanhdr(tmp);
	}
}

static
gdp_rput (q, bp)
register queue_t *q;
register mblk_t *bp;
{
	register union T_primitives *Tp;
	register mblk_t *bp1;
	register struct gdp *gp;
	extern int msgflag;

	gp = GDP(q);
	switch (bp->b_datap->db_type) {
	case M_PROTO:
	case M_PCPROTO:
		/*
		 * ASSUME the 1st block M_PROTO/M_PCPROTO is not fragmented
		 */
		Tp = (union T_primitives *)bp->b_rptr;
		DUPRINT2(DB_GDP,"gdp_rput: got tli primitive %d\n",Tp->type);
		switch(Tp->type) {
		case T_DATA_IND:
			ASSERT((bp->b_wptr-bp->b_rptr) == sizeof(struct T_data_ind));
			if (((struct T_data_ind *)Tp)->MORE_flag)
				gp->oneshot = 0;
			bp1 = bp->b_cont;
			bp->b_cont = NULL;
			freeb(bp);
			if (bp1->b_datap->db_type != M_DATA) {
				freemsg(bp1);
				return;
			}
			putq(q, bp1);
			break;

		case T_DISCON_IND:
		case T_ORDREL_IND:
/* disable: possible bug in npack
			ASSERT((bp->b_wptr-bp->b_rptr) == sizeof(struct T_discon_ind));
*/
			if ((bp->b_wptr-bp->b_rptr) != sizeof(struct T_discon_ind)) {
				DUPRINT1(DB_GDPERR,"gdp_rput: bad discon msg\n");
			}
			gp->flag = GDPDISCONN;
			msgflag |= DISCONN;
			wakeup((caddr_t) &rd_recover->rd_qslp);
			goto free;

		case T_INFO_ACK:
{
			long ss = ((struct T_info_ack *)Tp)->TSDU_size;
			u_long rq = (uint)sizeof(struct request);

			gp->maxpsz = ((struct T_info_ack *)Tp)->TIDU_size;
			if (ss != -2 && (uint)ss >= rq && gp->maxpsz >= rq)
				gp->oneshot = 1;
			goto free;
}

		default:
			DUPRINT2(DB_GDPERR,"gdp_rput: bad tli prim %d\n", Tp->type);
			goto free;
		} /*end switch */
		break;

	case M_DATA:
		putq (q, bp);
		break;

	case M_FLUSH:
		if (*bp->b_rptr & FLUSHR) flushq(q, FLUSHALL);
		if (*bp->b_rptr & FLUSHW) {
			*bp->b_rptr &= ~FLUSHR;
			putnext(WR(q), bp);
			break;
		}
		goto free;

	case M_ERROR:	
	case M_HANGUP:
		DUPRINT2(DB_GDPERR, "gdp_rput: M_ERROR/M_HANGUP on q %x\n", q);
		gp->flag = GDPDISCONN;
		msgflag |= DISCONN;
		wakeup((caddr_t) &rd_recover->rd_qslp);
		goto free;

	default:
		DUPRINT2(DB_GDPERR, "gdp_rput: bad msg type %x\n", bp->b_datap->db_type);
free:
		freemsg(bp);
		break;
	}
}

static
gdp_renable (addr)
long addr;
{
	register queue_t *qp = (queue_t *)addr;
	register struct gdp *gp;

	gp = GDP(qp);
	/*
	 * The bufcall is no longer pending.
	 */
	gp->rbufcid = 0;
	qenable(qp);
}

static
gdp_rsrv (qp)
register queue_t *qp;
{
	register mblk_t *bp, *bp1, *smgs;
	register struct gdp *gp;

	gp = GDP(qp);
	if (gp->oneshot) {
		while (bp = getq(qp)) {
			if (gdp_process(qp, bp) == 0) {
				goto loop;
			}
		}
		return;
	}

	while ((bp=getq(qp)) || gp->istate==GDPST1 || gp->istate==GDPST3) {
loop:
		switch(gp->istate) {
		case GDPST0:
		/* gathering header */
			ASSERT(bp && bp!=(mblk_t *)1 && (gp->hlen > 0));
			bp1 = splitmsg(bp, gp->hlen);
			if (bp1 == (mblk_t *)-1) {
				DUPRINT1(DB_GDPERR,"gdp_rsrv: split hdr failed\n");
				putbq(qp, bp);
				timeout((int (*)())qenable, (caddr_t)qp, hz);
				return;
			}
			if (gp->hdr)
				linkb(gp->hdr, bp);
			else
				gp->hdr = bp;
			if (bp1 == NULL) {
				/* not enough data for header */
				gp->hlen -= mgsize(bp);
				continue;
			}
			gp->istate = GDPST1;
			gp->hlen = 0;
			bp = bp1;
			/* falls thru */

		case GDPST1:
		/* header is completely assembled */
			ASSERT(gp->hdr);
			if (pullupmsg(gp->hdr, sizeof(struct message)) == 0) {
				DUPRINT1(DB_GDPERR,"gdp_rsrv: pullup header failed\n");
				if (bp && bp != (mblk_t *)1)
					putbq(qp, bp);
				/*
				 * Arrange to be called back only if there
				 * isn't an outstanding "bufcall" request
				 * since the required buffer size is fixed.
				 */
				if (gp->rbufcid == 0) {
					gp->rbufcid =
					    bufcall(sizeof(struct message),
						BPRI_MED, gdp_renable, (long)qp);
					/* If bufcall() fails, use timeout() */
					if (gp->rbufcid == 0)
						timeout((int (*)())qenable,
						    (caddr_t)qp, hz);
				}
				return;
			}
			gp->dlen = gdprfsize(gp->hdr, gp) - sizeof(struct message);

			gp->istate = GDPST2;
			if (bp == (mblk_t *)1)
				continue;
			/* falls thru */

		case GDPST2:
		/* work on data portion: struct request/response */
			ASSERT(bp && bp != (mblk_t *)1 && gp->dlen >= 0);
			bp1 = splitmsg(bp, gp->dlen);
			if (bp1 == (mblk_t *)-1) {
				DUPRINT1(DB_GDPERR,"gdp_rsrv: splitmsg failed\n");
				putbq(qp, bp);
				timeout((int (*)())qenable, (caddr_t)qp, hz);
				return;
			}

			if (gp->idata)
				linkb(gp->idata, bp);
			else
				gp->idata = bp;

			if (bp1 == NULL) {
				/* data block is not done yet */
				gp->dlen -= mgsize(bp);
				continue;
			}
			bp = bp1;
			gp->istate = GDPST3;
			gp->hdr->b_cont = gp->idata;
			gp->idata = NULL;
			/* falls thru */

		case GDPST3:
		/* total assembly is done */
			smgs = gp->hdr;
			if (gdp_process(qp, smgs) == 0) {
				if (bp && bp != (mblk_t *)1)
					putbq(qp, bp);
				return;
			}
			gp->hdr = NULL;
			cleanhdr(gp);

			if (bp == (mblk_t *)1)
				continue;
			else if (bp)
				goto loop;
			else
				return;

		default:
			DUPRINT2(DB_GDPERR,"gdp_rsrv: bad istate %x\n", gp->istate);
			ASSERT(0);
		}
	} /* end while */
}

/*
 * return the size of RFS message indicated in the RFS header
 * ASSUME bp has been pulled up and aligned
 */
static
gdprfsize(bp, gp)
register mblk_t *bp;
register struct gdp *gp;
{
	register struct message *mep;

	mep = (struct message *)bp->b_rptr;
	/* convert header back to local form */
	switch(gp->hetero) {
	default:
		DUPRINT2(DB_GDPERR,"gdprfsize: BAD hetero %d\n",gp->hetero);
		/* falls thru */

	case NO_CONV:
	case DATA_CONV:
		return(mep->m_size);

	case ALL_CONV:
		{
		struct message mg;
		(void) fcanon(rfs_message, (caddr_t) mep, (caddr_t) &mg);
		return(mg.m_size);
		}
	}
}

static
cleanhdr(gdpp)
register struct gdp *gdpp;
{
	if (gdpp->hdr) {
		freemsg(gdpp->hdr);
		gdpp->hdr = NULL;
	}
	if (gdpp->idata) {
		freemsg(gdpp->idata);
		gdpp->idata = NULL;
	}
	gdpp->hlen = sizeof(struct message);
	gdpp->dlen = 0;
	gdpp->istate = GDPST0;
}


/* count # of bytes, regardless its mesg type and ref count */
static int
mgsize(mp)
register mblk_t *mp;
{
	register int n;

	for (n = 0; mp; mp = mp->b_cont)
		n += mp->b_wptr - mp->b_rptr;
	return(n);
}

/*
 * leave the 1st len bytes in mp and return the remaining message.
 * olen = original mgsize(mp);
 * case (olen == len):	mp unchanged, return 1;
 * case (olen < len): mp unchanged, return NULL;
 * case (olen > len): mp is the 1st len byte, return the remaining;
 * return (mblk_t *)-1 if failed
 */
static mblk_t *
splitmsg(mp, len)
mblk_t *mp;
register int len;
{
	register mblk_t *bp, *bp2;

	for (bp = mp; bp != NULL; bp = bp->b_cont) {
		len -= bp->b_wptr - bp->b_rptr;
		if (len <= 0)
			break;
	}
	if (len > 0)
		return(NULL);
	if (len == 0) {
		if (bp->b_cont == NULL)
			return ((mblk_t *) 1);
		else {
			bp2 = bp->b_cont;
			bp->b_cont = NULL;
			return(bp2);
		}
	}

	if ((bp2 = dupb(bp)) == NULL) {
		DUPRINT1(DB_GDPERR,"splitmsg: dupb failed\n");
		return((mblk_t *)-1);
	}

	/* length is now a negative number */
	bp2->b_rptr = (bp->b_wptr += len);
	if (bp->b_cont) {
		bp2->b_cont = bp->b_cont;
		bp->b_cont = NULL;
	}
	return(bp2);
}

/*
 *	Call pullupmsg() to gather all data into one message and aligned.
 *	If pullupmsg() fails, try (1) in-place copy,
 *	(2) use reserved gdp buffer, (3) timeout and try again.
 *	The reserved gdp buffer will be freed sooner or later.
 *	if pullup succeeds, doing canon and calling "arrmsg", return 1.
 *	return 0 if failure, the  caller decides whether to put back the mesg.
 */
static int
gdp_process(qp, bp)
queue_t *qp;
register mblk_t *bp;
{
	register struct gdp *gp;

	gp = GDP(qp);
	if (!pullupmsg(bp, -1)) {
		register len = mgsize(bp);
		register len2;
		register mblk_t *bp1, *bp2;
		register char *cp;
		char tmp[sizeof(struct request) + sizeof(struct message)];

		/* fail to pullup all data, merge: reverse of splitmsg */
		if ((bp1 = bp->b_cont) &&
		    (bp1->b_cont == NULL) && (bp->b_datap == bp1->b_datap) &&
		    (bp->b_datap->db_ref == 2) && (bp->b_wptr == bp1->b_rptr)) {
			bp->b_wptr = bp1->b_wptr;
			freeb(bp1);
			bp->b_cont = NULL;
			goto goodbp;
		}

		/* try to do in place copy */
		for (bp1 = bp; bp1; bp1 = bp1->b_cont) {
			if ((bp1->b_datap->db_lim-bp1->b_datap->db_base>=len)
			    && (bp1->b_datap->db_ref == 1)) {
				DUPRINT1(DB_GDP,"gdp_process: in place copy\n");
				cp = &tmp[0];
				/* this block 'bp1' is big enough */
				while (bp2 = bp) {
					len2 = bp2->b_wptr - bp2->b_rptr;
					bcopy((caddr_t)bp2->b_rptr, cp, 
						(u_int)len2);
					cp += len2;
					bp = bp2->b_cont;
					if (bp2 != bp1)
						freeb(bp2);
				}
				bp1->b_rptr = bp1->b_datap->db_base;
				bp1->b_wptr = bp1->b_rptr + len;
				bcopy(&tmp[0], (caddr_t) bp1->b_rptr, 
					(u_int) len);
				bp1->b_cont = NULL;
				bp = bp1;
				goto goodbp;
			}
		}

		/* in place copy fails, try reserved buffer */
		if (gdp_bp->b_datap->db_ref == 1 && (bp1 = dupb(gdp_bp))) {
			DUPRINT1(DB_GDP,"gdp_process: use gdp buffer\n");
			bp1->b_rptr = bp1->b_datap->db_base;
			bp1->b_wptr = bp1->b_rptr + len;
			ASSERT(bp1->b_wptr <= bp1->b_datap->db_lim);
			cp = (char *)bp1->b_rptr;
			for (bp2 = bp; bp2; bp2 = bp2->b_cont) {
				len2 = bp2->b_wptr - bp2->b_rptr;
				bcopy((caddr_t) bp2->b_rptr, cp, (u_int) len2);
				cp += len2;
			}
			freemsg(bp);
			bp = bp1;
			goto goodbp;
		}

		/* can't find reserved buffer, timeout and try later */
		DUPRINT1(DB_GDPERR, "gdp_process: timeout\n");
		timeout((int (*)())qenable, (caddr_t)qp, hz);
		return(0);
	}
goodbp:
	/*
	 * the following checking is because the TSDU may
	 * be, say, 4K (cause oneshot==1) and TIDU is 1K
	 */
	if (gp->oneshot && (gdprfsize(bp, gp) != mgsize(bp))) {
		gp->oneshot = 0;
		return(0);
	}

	/*
	 *	convert RFS headers and data back to local forms
	 */
	if (!rffrcanon(bp, gp->hetero))
		return(0);
	/* mark queue from whence message came */
	((struct message *)bp->b_rptr)->m_queue = (long)qp; 
	arrmsg(bp);
	return(1);
}


/* attaches the stream described by fd to gdp struct pointed at by gdpp */
static queue_t *
init_circuit (fd, gdpp)
register struct gdp *gdpp;
{
	register queue_t *qp;
	register struct stdata *stp;
	register struct file *fp;
	register int s;
	struct vnode *vp;
	register mblk_t *bp, *mp;

	/* Deny for lots of reasons.... */
	if ((fd < 0 || fd >= NOFILE) || !(fp = u.u_ofile[fd]) || 
		(fp->f_count != 1)) {
	    DUPRINT4(DB_GDP,"bad fd to init_circuit fd %x, fp %x, f_count %d\n",
	     	fd, fp, fp->f_count);
	    u.u_error = EBADF;
	    return(NULL);
	}

	vp = (struct vnode *) fp->f_data;
	dnlc_purge_vp(vp);
	if ((vp->v_count != 1) || !(stp = vp->v_stream)) {
	    DUPRINT4(DB_GDP,"bad fd to init_circuit vp %x, stp %x, v_count %x\n",
	     	vp, stp, vp->v_count);
	    u.u_error = EBADF;
	    return(NULL);
	}

	if ((bp = allocb(sizeof(struct T_info_req), BPRI_HI)) == NULL) {
		u.u_error = ENOMEM;
		return(NULL);
	}

	s = splstr();

	qp = RD(stp->sd_wrq); 		/* steal the stream head's queue's */
	stp->sd_flag |= STPLEX;		/* in case someone else can open late */
	fp->f_count++;			/* for consistency */

	/* point gdp at q */
	gdpp->queue = qp;
	gdpp->mntcnt = 0;
	gdpp->time = 0;

	/* point q at gdp structure  and intialize q */
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t)gdpp;
	setq (qp,&gdprdata,&gdpwdata);
	qp->q_flag |= QWANTR;
	WR(qp)->q_flag |= QWANTR;

	gdpp->file = fp;
	gdpp->flag = GDPCONNECT;
	/*
	 * strip M_PROTO:T_DATA_IND
	 */
	for (mp = qp->q_first; mp; ) {
		mblk_t *emp, *tmp;

		switch(mp->b_datap->db_type) {
		case M_PROTO:
			if (((union T_primitives *)mp->b_rptr)->type != T_DATA_IND)
				goto stripbad;
			emp = mp->b_next;
			rmvq(qp, mp);
			tmp = (mblk_t *)unlinkb(mp);
			freeb(mp);
			insq(qp, emp, tmp);
			mp = emp;
			break;

		case M_DATA:
			mp = mp->b_next;
			break;

		default:
stripbad:
			freeb(bp);
			u.u_error = EPROTO;
			(void) splx(s);
			return(NULL);
		}
	}

	if(qp->q_first)
		qenable(qp);
	(void) splx(s);

	/* request TSDU_size from the provider */
	gdpp->oneshot = 0;	/* default: unless provider says otherwise */
	bp->b_datap->db_type = M_PCPROTO;
	((struct T_info_req *)(bp->b_wptr))->PRIM_type = T_INFO_REQ;
	bp->b_wptr += sizeof(struct T_info_req);
	putnext(WR(qp), bp);
	return (qp);
}

tokclear(gp)
register struct gdp *gp;
{
	gp->token.t_id = 0;
	gp->token.t_uname[0] = '\0';
}

clean_circuit (qp)
register queue_t *qp;
{
	extern struct	qinit strdata, stwdata;
	register struct gdp *gdpp;
	register struct stdata *stp;
	register int s;

	DUPRINT2(DB_GDP,"clean_circuit %x\n",qp);
	gdpp = GDP(qp);
	if (gdpp->flag == GDPFREE)
		return;
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (gdpp->rbufcid) {
		unbufcall(gdpp->rbufcid);
		gdpp->rbufcid = 0;
	}
	/* restore queues to their rightful owner, the stream head */

	stp = ((struct vnode *)gdpp->file->f_data)->v_stream;
	s = splstr();
	setq(qp,&strdata,&stwdata);
	qp->q_ptr = WR(qp)->q_ptr = (caddr_t)stp;
	(void) splx(s);

	stp->sd_flag &= ~STPLEX;
	closef(gdpp->file);	/* and close it */
	gdpp->file = NULL;
	gdpp->queue = NULL;
	gdpp->flag = GDPFREE;
	gdpp->sysid = gdpp - gdp + 1;
	gdpp->hetero = 0;
	gdpp->version = 0;
	gdpp->time = 0;
	gdpp->mntcnt = 0;
	tokclear (gdpp);
	if (gdpp->idmap[0])
		freeidmap(gdpp->idmap[0]);
	if (gdpp->idmap[1])
		freeidmap(gdpp->idmap[1]);
	gdpp->idmap[0] = 0;
	gdpp->idmap[1] = 0;
	gdpp->oneshot = 0;
	cleanhdr(gdpp);
}			

kill_gdp()
{
	register struct gdp *tmp;

	DUPRINT1(DB_GDP,"kill_gdp\n");
	for (tmp = gdp; tmp < &gdp[maxgdp] ; tmp++)
	    if (tmp->flag != GDPFREE) 
		clean_circuit (tmp->queue);

	if (gdp_bp) {
		freemsg(gdp_bp);
		gdp_bp = NULL;
	}
}


tokcmp(n1, n2)
struct token *n1, *n2;
{
	register char	*p1=n1->t_uname, *p2=n2->t_uname;
	if (n1->t_id != n2->t_id)
		return(0);
	if (strncmp(p1,p2,MAXDNAME) == 0)
		return(1);
	return(0);
}

/*
 * if canput(next), send msg along and fragment it if needed
 * note that the msg has been canonized
 */
static
gdp_wsrv(qp)
register queue_t *qp;
{
	register mblk_t *bp;
	register struct gdp *gp;

	DUPRINT2(DB_GDP,"gdp_wsrv qp %x\n",qp);
	gp = GDP(qp);
	while (bp = getq(qp)) {
frag:
		if (!canput(qp->q_next)) {
			putbq(qp,bp);
			break;
		}
		if (mgsize(bp) > gp->maxpsz) {
			register mblk_t *bp1;

			bp1 = splitmsg(bp, (int) gp->maxpsz);
			if (bp1 == (mblk_t *)-1) {
				DUPRINT1(DB_GDPERR,"gdp_wsrv: split failed\n");
				putbq(qp, bp);
				return;
			}
			putnext(qp, bp);
			bp = bp1;
			goto frag;
		} else
			putnext(qp, bp);
	}
	if (canput(qp))	/*don't wake up server unless necessary*/
		wakeup(qp->q_ptr);
}
