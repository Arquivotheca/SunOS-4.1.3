/*	@(#)rfrequest.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)libns:rfrequest.c	1.5" */
#include <sys/types.h>
#include <sys/mount.h>
#include <rpc/rpc.h>
#include <rfs/ns_xdr.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <rfs/pn.h>
#include "nslog.h"

extern int	t_errno;
extern int	errno;

int
rf_request(fd, iopcode)
int fd;
int iopcode;
{
	int i;
	char canonbuf[20];
	int flags = 0;
	int retval;
	pnhdr_t pnhdr;
	extern pntab_t sw_tab[];

	/*
	 * send header to server - 
	 *
	 * format of header is	4 char opcode		( buf[0..3] )
	 *			canon long hi version	( buf[4..7] )
	 *			canon long lo version	( buf[8..11] )
	 */

	LOG4(L_COMM,"(%5d) rf_request: enter, fd = %d, iopcode = %d\n",
		Logstamp, fd, iopcode);

	if (fd < 0) {
		LOG3(L_ALL,"(%5d) rf_request: bad fd=%d\n",Logstamp,fd);
		return(-1);
	}
	if (iopcode >= NUMSWENT || iopcode == RF_AK) {
		LOG3(L_ALL,"(%5d) rf_request: bad iopcode=%d\n",Logstamp,iopcode);
		return(-1);
	}
	(void) strcpy(pnhdr.pn_op, sw_tab[iopcode].sw_opcode);
	pnhdr.pn_lo = (long) LO_VER;
	pnhdr.pn_hi = (long) HI_VER;
	LOG5(L_COMM,"(%5d) rf_request: sending pnhdr, op=%s, lo_ver=%d, hi_ver=%d\n",
		Logstamp, pnhdr.pn_op, pnhdr.pn_lo, pnhdr.pn_hi);
	if ((i = tcanon(rfs_pnhdr,&pnhdr, canonbuf)) == 0) {
		LOG2(L_ALL, "(%5d) rf_request: tcanon failed\n",Logstamp);
		return(-1);
	}
	if ((retval=t_snd(fd, canonbuf, i, 0)) != i) {
		LOG5(L_ALL,"(%5d) rf_request: t_snd failed, ret=%d, t_err=%d, err=%d\n",
			Logstamp, retval, t_errno, errno);
		return(-1);
	}
	LOG2(L_COMM,"(%5d) rf_request: trying to receive response hdr\n",Logstamp);
	if (rf_rcv(fd, canonbuf, CANON_CLEN, &flags) != CANON_CLEN) {
		LOG4(L_ALL,"(%5d) rf_request: t_rcv failed, t_err=%d, err=%d\n",
			Logstamp, t_errno, errno);
		return(-1);
	}
	if (fcanon(rfs_pnhdr, canonbuf, &pnhdr) == 0) {
		LOG2(L_ALL, "(%5d) rf_request: fcanon failed\n",Logstamp);
		return(-1);
	}
	if (strncmp(pnhdr.pn_op, sw_tab[RF_AK].sw_opcode, OPCODLEN) != 0) {
		LOG2(L_ALL, "(%5d) rf_request: aknowledgement failed\n",Logstamp);
		return(-1);
	}
	/* extract version */
	if ((pnhdr.pn_hi < 0) || (pnhdr.pn_hi != pnhdr.pn_lo)) {
		LOG4(L_ALL, "(%5d) rf_request: version mismatch, l=%d, h=%d\n",
			Logstamp, pnhdr.pn_lo,pnhdr.pn_hi);
		return(-1);
	}
	LOG3(L_COMM,"(%5d) rf_request: version = %d\n",Logstamp, pnhdr.pn_hi);
	return(pnhdr.pn_hi);
}
