/*      @(#)nsports.c 1.1 92/07/30 SMI      */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident  "@(#)libns:nsports.c    1.11.4.1" */
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rfs/ns_xdr.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <sys/errno.h>
#include "nslog.h"
#include "stdns.h"
#include "nsports.h"
#include "nsdb.h"
#include <rfs/nserve.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>

/*
 * nsports.c contains the functions that control
 * communications for the name server.
 */

/* externs	*/
extern	int	errno;
struct address	*Caddress = NULL;	/* address of current requestor	*/
extern char	*Net_spec;

/* static declarations	*/
static struct nsport	Ports[NPORTS];
static struct first_msg	Loc_msg = { VER_HI, LOC_MSG, 0};

/*
 * nsconnect tries to make a connection with
 * the local name server.  It returns a
 * port id, which is used in subsequent calls.
 * It returns -1 if it fails.
 */
int
nsconnect(addr)
struct address	*addr;
{
	int	pd;
	struct nsport	*pptr=NULL;
	char	inblock[sizeof(struct first_msg)];
	char	*bptr = inblock;
	char	*atos();
	struct first_msg	rfm;

	LOG3(L_COMM,"(%5d) nsconnect: mode = LOCAL, address = %s\n",
		Logstamp, (addr)?atos(Logbuf,addr->addbuf,RRAW):"NULL");

	if ((pd = nsgetpd()) == -1) return(-1);

	pptr = pdtoptr(pd);
	pptr->p_mode = LOCAL;

	if ((pptr->p_fd = loc_conn(addr)) == -1) {
		LOG3(L_COMM,"(%5d) nsconnect: connect failed, errno=%d\n",
			Logstamp,errno);
		nsclose(pd);
		return(-1);
	}
	tcanon(rfs_first_msg,&Loc_msg,inblock);

	if (nswrite(pd,inblock,sizeof(struct first_msg)) == -1) {
		LOG3(L_ALL,"(%5d) nsconnect: can't write first msg, errno=%d\n",
			Logstamp,errno);
		nsclose(pd);
		return(-1);
	}
	if (nsread(pd,&bptr,sizeof(struct first_msg)) == -1) {
		LOG3(L_ALL,"(%5d) nsconnect: can't read first reply, errno=%d\n",
			Logstamp,errno);
		nsclose(pd);
		return(-1);
	}
	fcanon(rfs_first_msg,bptr,(char *) &rfm);
	LOG4(L_COMM,"(%5d) nsconnect: rfm.mode = %s, rfm.addr = %s\n",
		Logstamp, rfm.mode, rfm.addr);
	if (strcmp(rfm.mode,NOK_MSG) == NULL) {
		LOG3(L_ALL,"(%5d) nsconnect: connection rejected, msg = %s\n",
			Logstamp, rfm.addr);
		nsclose(pd);
		return(-1);
	}
	/* set the Net_spec address for this request	*/
	Net_spec = copystr(rfm.addr);
	LOG3(L_COMM,"(%5d) nsconnect: connect succeeded, net_spec = %s\n",
		Logstamp,Net_spec);
	return(pd);
}
/*
 * nswrite writes a block onto port pd.  It packetizes
 * the block and splits it into as many parts as necessary.
 */
int
nswrite(pd,block,size)
int	pd;	/* port id	*/
register char	*block;	/* block pointer	*/
int	size;	/* size of block	*/
{
	int		fd;
	struct nsport	*ppd;
	char		*bend;	/* end of block		*/
	register char	*pptr;	/* start of packet	*/
	char		*pend;	/* end of packet	*/
	struct pkt	*pk;

	LOG5(L_COMM,"(%5d) nswrite: pd=%d, block=%x, size=%d\n",
		Logstamp,pd,block,size);
	if ((ppd = pdtoptr(pd)) == NULL) {
		LOG3(L_ALL,"(%5d) nswrite: bad port descriptor %d\n",
			Logstamp, pd);
		return(-1);
	}
	pk = ppd->p_wpkt;
	pend = ((char *) pk) + PK_MAXSIZ;
	bend = block + size;

	fd = ppd->p_fd;
	pk->pk_id = getpid();
	pk->pk_total = size;
	pk->pk_fill = 0;

	for (pk->pk_index = 1; block < bend; pk->pk_index++) {
		for (pptr = pk->pk_data; pptr < pend && block < bend;
		     *pptr++ = *block++)
			;
		pk->pk_size = pptr - pk->pk_data;
		LOG4(L_COMM,"(%5d) nswrite: write packet # %d, size = %d\n",
			Logstamp,pk->pk_index,pk->pk_size);
		tcanon(rfs_pkt_hd,&(pk->pk_hd),&(pk->pk_hd));
		if (write(fd,pk,PK_MAXSIZ) == -1) {
			LOG3(L_ALL,"(%5d) nswrite: write failed, errno = %d\n",
				Logstamp,errno);
			return(-1);
		}
	}
	return(size);
}
/*
 * nsread reads a block from port pd.  It depacketizes
 * the packets and assembles them into a block.
 * If *block is NULL, it allocates a block of the
 * necessary size (the parameter size is ignored).
 * If *block is non-NULL, it uses it, writing up to
 * size bytes.  If there are more than size bytes
 * to read, the excess is truncated.  Nsread returns
 * the number of bytes in the block, or -1 if there was
 * a failure.  If the value returned is greater than
 * size (and *block is non-NULL), it means that a block
 * larger than "size" bytes was read.  However, only
 * size bytes are copied, the rest are discarded.
 */
int
nsread(pd,block,size)
int	pd;	/* port id	*/
char	**block; /* block pointer	*/
int	size;
{
	int	i, j;
	int	fd;
	struct pkt	*pk;
	struct nsport	*pptr;
	int		pkdata;	/* max size of data in a packet	*/
	int		bsize;	/* amount of data in block	*/
	int		total;	/* total # of packets to read	*/
	char		*bend;	/* pointer to end of block	*/
	register char	*bptr;	/* pointer into block		*/
	register char	*ptr;	/* pointer into packet		*/
	int		mflag=FALSE;	/* set if space was allocated here */

	LOG5(L_COMM,"(%5d) nsread: pd=%d, *block=%d, size=%d\n",
		Logstamp,pd,*block,size);

	if ((pptr = pdtoptr(pd)) == NULL) {
		LOG3(L_ALL,"(%5d) nsread: bad port descriptor %d\n",
			Logstamp, pd);
		return(-1);
	}
	fd = pptr->p_fd;
	pk = pptr->p_rpkt;
	pkdata = (((char *) pk) + PK_MAXSIZ) - pk->pk_data;

	if (read(fd,pk,PK_MAXSIZ) != PK_MAXSIZ) {
		LOG3(L_ALL,"(%5d) nsread: first read failed, errno = %d\n",
			Logstamp,errno);
		return(-1);
	}

	fcanon(rfs_pkt_hd,&(pk->pk_hd),&(pk->pk_hd));
	bsize = pk->pk_total;

	LOG4(L_COMM,"(%5d) nsread: first packet, bsize=%d, pk->pk_total=%d\n",
		Logstamp,bsize,pk->pk_total);

	if (*block == NULL) {
		if ((*block = malloc(bsize)) == NULL) {
			LOG3(L_ALL,"(%5d) nsread: malloc(%d) failed\n",
				Logstamp,bsize);
			return(-1);
		}
		size = bsize;
		mflag = TRUE;
	}
	else { if (bsize > size)
		LOG4(L_COMM,
		    "(%5d) nsread: WARNING block (%d) > buffer (%d), TRUNCATED\n",
			Logstamp,bsize,size);
	     else
		size = bsize;
	}

	total = bsize/pkdata + ((bsize % pkdata)?1:0);
	bptr = *block;
	bend = bptr + size;
	ptr = pk->pk_data;

	for (i=0; i < pk->pk_size && bptr < bend; i++)
		*bptr++ = *ptr++;

	for (j=2; j <= total && read(fd,pk,PK_MAXSIZ) != -1; j++) {
		fcanon(rfs_pkt_hd, &(pk->pk_hd), &(pk->pk_hd));
		LOG3(L_COMM,"(%d) nsread: reading pkt # %d\n",Logstamp,j);
		if (j != pk->pk_index) {
			LOG4(L_COMM,"(%5d) nsread: expect index %d, got %d\n",
				Logstamp,j,pk->pk_index);	
			if (mflag) {
				free(*block);
				*block = NULL;
			}
			return(-1);
		}
		/* copy packet into block	*/
		for (i=0, ptr=pk->pk_data; i < pk->pk_size; i++)
			*bptr++ = *ptr++;
	}
	if (j != total + 1) {	/* didn't get it all	*/
		LOG3(L_ALL,"(%5d) nsread: read failed, errno = %d\n",
			Logstamp,errno);
		if (mflag) {
			free(*block);
			*block = NULL;
		}
		size = -1;
	}
	return(size);
}
/*
 * nsclose closes port pd.
 */
int
nsclose(pd)
int	pd;
{
	struct nsport	*pptr;

	LOG3(L_COMM,"(%5d) nsclose: pd=%d\n",Logstamp,pd);
	if ((pptr = pdtoptr(pd)) == NULL)
		return;
	pptr->p_mode = UNUSED;
	if (pptr->p_fd != -1) {
		close(pptr->p_fd);
		pptr->p_fd = -1;
	}
	return;
}
int
nsgetpd()
{
	register int	pd;
	struct nsport	*pptr=NULL;

	for (pd=0; pd < NPORTS; pd++)
		if (Ports[pd].p_mode == UNUSED) {
			pptr = pdtoptr(pd);
			break;
		}
	LOG4(L_COMM,"(%5d) nsgetpd: found port # %d %s\n",
		Logstamp,pd,(pd==NPORTS)?"TABLE FULL":"");

	if (pd == NPORTS)
		return(-1);

	if (pptr->p_wpkt == NULL &&
	    (pptr->p_wpkt = (struct pkt *) malloc(PK_MAXSIZ)) == NULL) {
		PLOG2("(%5d) nsgetpd: wpkt malloc failed\n",Logstamp);
		return(-1);
	}
	if (pptr->p_rpkt == NULL &&
	    (pptr->p_rpkt = (struct pkt *) malloc(PK_MAXSIZ)) == NULL) {
		PLOG2("(%5d) nsgetpd: rpkt malloc failed\n",Logstamp);
		free(pptr->p_wpkt);
		pptr->p_wpkt = NULL;
		return(-1);
	}

	pptr->p_mode = RESERVED;
	pptr->p_fd = -1;

	return(pd);
}
static int
loc_conn(addr)
struct address	*addr;
{
	int sfd;
	int pfd[2];

	LOG3(L_ALL,"(%5d) loc_conn: Try to open channel to %s\n",
		Logstamp,addr->addbuf.buf);


	if ((sfd = open(addr->addbuf.buf, O_RDWR)) < 0) {
		LOG4(L_ALL,"loc_conn: can't open channel %s to nserve, errno=%d\n",
			Logstamp,addr->addbuf.buf,errno);
		return(-1);
	}
	if (spipe(pfd) < 0) {
		LOG2(L_ALL,"loc_conn: can't open spipe, errno=%d\n",errno);
		close(sfd);
		return(-1);
	}
	if (ioctl(sfd, I_SENDFD, pfd[1]) < 0) {
		LOG2(L_ALL,"(%5d) loc_conn: can't give fd to nserve\n",Logstamp);
		close(sfd);
		close(pfd[0]);
		close(pfd[1]);
		return(-1);
	}
	close(sfd);
	close(pfd[1]);

	LOG3(L_ALL,"(%5d) loc_conn: channel open to %s\n",
		Logstamp,addr->addbuf.buf);

	return(pfd[0]);
}
struct nsport *
pdtoptr(pd)
int	pd;
{
	if (pd < 0 || pd > NPORTS)
		return(NULL);

	return (&Ports[pd]);
}
int
ptrtopd(pptr)
struct nsport *pptr;
{
	if (pptr < Ports || pptr > &Ports[NPORTS])
		return(-1);

	return(pptr - Ports);
}
int
fdtopd(fd)
int	fd;
{
	register struct nsport	*pptr;

	for (pptr = &Ports[0]; pptr < &Ports[NPORTS]; pptr++)
		if (pptr->p_mode != UNUSED &&
		    pptr->p_fd == fd)
			return(pptr - Ports);

	return(-1);
}
