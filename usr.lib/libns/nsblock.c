/*	@(#)nsblock.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:nsblock.c	1.7" */
#include <stdio.h>
#include <string.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include "nslog.h"
#include "nsdb.h"
#include "stdns.h"

static int	Mflag = FALSE;	/* did we malloc space	*/
static int	getqd();
static int	putqd();
static int	getrlist();
static int	putrlist();
static struct header	*getheader();
/*
 *	btoreq converts an ascii block in canonical format into
 *	a request structure.  It returns a pointer to that structure
 *	or NULL if it fails.  btoreq allocates the request structure,
 *	which should be freed by the caller when through.
 */
struct request	*
btoreq(block,size)
char	*block;
int	size;
{
	struct request *rp; /* pointer to request */
	struct header	*hp;
	place_p	pp;

	if ((rp = (struct request *) calloc(1,sizeof(struct request))) == NULL) {
		LOG3(L_ALL,"(%5d) btoreq: malloc(%d) FAILED\n",
			Logstamp, sizeof(struct request));
		return(NULL);
	}

	pp = setplace(block,size);

	if ((rp->rq_head = getheader(pp)) == NULL) {
		LOG2(L_BLOCK,"(%5d) btoreq: getheader failed\n",Logstamp);
		free(rp);free(pp);
		return(NULL);
	}

	hp = rp->rq_head;
	/* now get the other sections	*/

	if (getqd(pp,hp->h_qdcnt,&(rp->rq_qd)) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) btoreq: getqd failed\n",Logstamp);
		free(rp);free(pp);
		return(NULL);
	}
	if (getrlist(pp,hp->h_ancnt,&(rp->rq_an)) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) btoreq: getrlist failed on an\n",Logstamp);
		free(rp);free(pp);
		return(NULL);
	}
	if (getrlist(pp,hp->h_nscnt,&(rp->rq_ns)) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) btoreq: getrlist failed on ns\n",Logstamp);
		free(rp);free(pp);
		return(NULL);
	}
	if (getrlist(pp,hp->h_arcnt,&(rp->rq_ar)) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) btoreq: getrlist failed on ar\n",Logstamp);
		free(rp);free(pp);
		return(NULL);
	}
	free(pp);
	return(rp);
}
static struct header *
getheader(pp)
place_p	pp;
{
	struct header *hp;

	if ((hp = (struct header *) malloc(sizeof(struct header))) == NULL) {
		LOG3(L_ALL,"(%5d) getheader: malloc(%d) FAILED\n",
			Logstamp,sizeof(struct header));
		return(NULL);
	}
	hp->h_version = getlong(pp);
	hp->h_flags   = getlong(pp);
	hp->h_opcode  = getlong(pp);
	hp->h_rcode   = getlong(pp);
	hp->h_qdcnt   = getlong(pp);
	hp->h_ancnt   = getlong(pp);
	hp->h_nscnt   = getlong(pp);
	hp->h_arcnt   = getlong(pp);
	hp->h_dname   = getstr(pp,NULL,NULL);
	return(hp);
}
static int
getqd(pp,count,qpp)
place_p pp;
long	count;
struct question	***qpp;
{
	int	i;
	struct question	*qra;
	struct question	**qp;

	if (count == 0) {
		*qpp = (struct question **) NULL;
		return(RFS_SUCCESS);
	}
	if (*qpp == NULL)
		if ((*qpp = (struct question **)
		     calloc(count,sizeof(struct question *))) == NULL) {
			LOG4(L_ALL,"(%5d) getqd: *qpp = calloc(%d,%d) failed\n",
				Logstamp,count,sizeof(struct question *));
			return(RFS_FAILURE);
		}

	if ((qra = (struct question *)calloc(count,sizeof(struct question)))
	    == NULL) {
		LOG4(L_ALL,"(%5d) getqd: qra = calloc(%d,%d) failed\n",
			Logstamp,count,sizeof(struct question));
		return(RFS_FAILURE);
	}

	for (i=0, qp = *qpp; i < count; i++, qp++, qra++) {
		*qp = qra;
		qra->q_name = getstr(pp,NULL,NULL);
		qra->q_type = getlong(pp);
	}
	return(RFS_SUCCESS);
}
static int
getrlist(pp,count,rpp)
place_p	pp;
long	count;
struct res_rec	***rpp;
{
	int	i;
	struct res_rec	*rra;
	struct res_rec	**rp;

	if (count == 0) {
		*rpp = (struct res_rec **) NULL;
		return(RFS_SUCCESS);
	}
	if (*rpp == NULL)
		if ((*rpp = (struct res_rec **)
		     calloc(count,sizeof(struct res_rec *))) == NULL) {
			LOG4(L_ALL,"(%5d) getrlist: *rpp = calloc(%d,%d) failed\n",
				Logstamp,count,sizeof(struct res_rec *));
			return(RFS_FAILURE);
		}

	if ((rra = (struct res_rec *)calloc(count,sizeof(struct res_rec)))
	    == NULL) {
		LOG4(L_ALL,"(%5d) getrlist: rra = calloc(%d,%d) failed\n",
			Logstamp,count,sizeof(struct question));
		return(RFS_FAILURE);
	}
	for (i=0, rp = *rpp; i < count; i++, rp++, rra++) {
		*rp = rra;
		if (getrrec(pp,rra) != RFS_SUCCESS)
			return(RFS_FAILURE);
	}
	return(RFS_SUCCESS);
}
int
getrrec(pp,rp)
place_p	pp;
struct res_rec	*rp;
{
	getstr(pp,rp->rr_name,NAMSIZ);
	rp->rr_type = getlong(pp);
	switch (rp->rr_type) {
	case RN:
		if ((rp->rr_rn = (struct rn *) malloc(sizeof(struct rn))) == NULL) {
			LOG3(L_ALL,"(%5d) getrrec: malloc(%d) failed\n",
				Logstamp,sizeof(struct rn));
			return(RFS_FAILURE);
		}
		rp->rr_owner = getstr(pp,NULL,NULL);
		rp->rr_desc = getstr(pp,NULL,NULL);
		rp->rr_path = getstr(pp,NULL,NULL);
		rp->rr_flag = getlong(pp);
		break;
	case DOM:
		if ((rp->rr_dom = (struct domain *) malloc(sizeof(struct domain)))
		    == NULL) {
			LOG3(L_ALL,"(%5d) getrrec: domain malloc(%d) failed\n",
				Logstamp,sizeof(struct domain));
			return(RFS_FAILURE);
		}
		break;
	default: /* all the rest fall in this category	*/
		rp->rr_data = getstr(pp,NULL,NULL);
		break;
	}
	return(RFS_SUCCESS);
}
/*
 *	reqtob writes a request structure into a block.
 *	If block == NULL, reqtob allocates enough space.
 *	reqtob returns a pointer to the block or NULL,
 *	if the operation fails.
 */
char	*
reqtob(rp,block,size)
struct request	*rp;
char		*block;
int		*size;
{
	int	rsize;	/* real size of current block	*/
	place_p	pp;
	struct header	*hp;

	if (block == NULL) {	/* allocate some space	*/
		if ((block = malloc(DBLKSIZ)) == NULL) {
			LOG3(L_ALL,"(%5d) reqtob: malloc(%d) failed\n",
				Logstamp,DBLKSIZ);
			return(NULL);
		}
		rsize = *size = DBLKSIZ;
		Mflag = TRUE;
	}
	else
		rsize = *size;

	pp = setplace(block,rsize);

	*size = 0;	/* set for error case, resets anyway if ok	*/
	hp = rp->rq_head;
	if (putheader(pp,hp) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) reqtob: putheader failed\n",Logstamp);
		if (Mflag) free(block);
		free(pp);
		return(NULL);
	}

	/* now get the other sections	*/

	if (putqd(pp,hp->h_qdcnt,rp->rq_qd) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) reqtob: putqd failed\n",Logstamp);
		if (Mflag) free(block);
		free(pp);
		return(NULL);
	}
	if (putrlist(pp,hp->h_ancnt,rp->rq_an) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) reqtob: putrlist failed on an\n",Logstamp);
		if (Mflag) free(block);
		free(pp);
		return(NULL);
	}
	if (putrlist(pp,hp->h_nscnt,rp->rq_ns) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) reqtob: putrlist failed on ns\n",Logstamp);
		if (Mflag) free(block);
		free(pp);
		return(NULL);
	}
	if (putrlist(pp,hp->h_arcnt,rp->rq_ar) == RFS_FAILURE) {
		LOG2(L_BLOCK,"(%5d) reqtob: putrlist failed on ar\n",Logstamp);
		if (Mflag) free(block);
		free(pp);
		return(NULL);
	}

	*size = pp->p_ptr - pp->p_start;
	free(pp);
	return(pp->p_start);
}
int
putheader(pp,hp)
place_p	pp;
struct header	*hp;
{
	/* check size first	*/
	if (overbyte(pp,c_sizeof(hp->h_dname)+8*L_SIZE) &&
	    explace(pp,0) == RFS_FAILURE) 
		return(RFS_FAILURE);

	if (putlong(pp,(long)NSVERSION)    == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_flags)  == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_opcode) == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_rcode)  == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_qdcnt)  == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_ancnt)  == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_nscnt)  == RFS_FAILURE ||
	    putlong(pp,(long)hp->h_arcnt)  == RFS_FAILURE ||
	    putstr(pp,hp->h_dname)        == RFS_FAILURE)
		return(RFS_FAILURE);

	return(RFS_SUCCESS);
}
static int
putqd(pp,count,qp)
place_p pp;
int	count;
struct question	**qp;
{
	int	i;

	/* put each query into block	*/
	for (i=0; i < count; i++, qp++) {
		/* size is strlen + room for NULL + type + class */
		if (overbyte(pp,c_sizeof((*qp)->q_name)+L_SIZE) &&
		    explace(pp,0) == RFS_FAILURE)
			return(RFS_FAILURE);

		putstr(pp,(*qp)->q_name);
		putlong(pp,(long)(*qp)->q_type);
	}
	return(RFS_SUCCESS);
}
static int
putrlist(pp,count,rp)
place_p	pp;
int	count;
struct res_rec	**rp;
{
	int	i;

	for (i=0; i < count; i++, rp++)
		if (putrrec(pp,*rp) != RFS_SUCCESS)
			return(RFS_FAILURE);

	return(RFS_SUCCESS);
}
int
putrrec(pp,rp)
place_p	pp;
struct res_rec	*rp;
{
	if (overbyte(pp,c_sizeof(rp->rr_name)+L_SIZE) && explace(pp,0) == RFS_FAILURE)
			return(RFS_FAILURE);

	putstr(pp,rp->rr_name);
	putlong(pp,(long)rp->rr_type);
	switch (rp->rr_type) {
	case RN:
		if (overbyte(pp,c_sizeof(rp->rr_owner) +
		    c_sizeof(rp->rr_desc) +
		    c_sizeof(rp->rr_path) + L_SIZE) &&
		    explace(pp,0) == RFS_FAILURE)
			return(RFS_FAILURE);
			
		putstr(pp,rp->rr_owner);
		putstr(pp,rp->rr_desc);
		putstr(pp,rp->rr_path);
		putlong(pp,(long)rp->rr_flag);
		break;
	case DOM:
		/* nothing else needed for domain	*/
		break;
	default:
		if (overbyte(pp,c_sizeof(rp->rr_data)) &&
		    explace(pp,0) == RFS_FAILURE)
			return(RFS_FAILURE);

		putstr(pp,rp->rr_data);
		break;
	}
	return(RFS_SUCCESS);
}
