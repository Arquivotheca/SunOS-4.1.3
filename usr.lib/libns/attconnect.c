/*	@(#)attconnect.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:attconnect.c	1.10"
#include <sys/types.h>
#include <sys/mount.h>
#include <tiuser.h>
#include <rfs/nsaddr.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <fcntl.h>
#include <stdio.h>
#include <rfs/pn.h>
#include <string.h>
#include <errno.h>
#include "nslog.h"

extern int	errno;
extern int	t_errno;
struct t_info linfo;
struct t_bind *req;
struct t_call *sndcall;

int
att_connect(addr, serv_code)
struct address  *addr;
long serv_code;
{
	int	n, fd, rfd;

	for (n = 1; n <= C_RETRY; ++n) {
		if ((fd = snd_connect(addr)) == -1)
			return(-1);
		if ((rfd=rcv_connect(fd, serv_code)) != -1)
			break;
		if (t_errno == TLOOK && t_look(fd) == T_DISCONNECT && n <= C_RETRY) {
			LOG3(L_ALL,"(%5d) att_connect: CONNECT RETRY %d\n",
				Logstamp,n);
			t_close(fd);
		}
		else {
			LOG3(L_ALL,"(%5d) att_connect: Failed, %d tries\n",
				Logstamp, n);
			t_close(fd);
			return(-1);
		}
	}
	return(rfd);

}

snd_connect(addr)
struct address *addr;
{
	int fd;
	char	protdev[BUFSIZ];
	extern char	*t_alloc();

	LOG3(L_COMM | L_OVER, "(%5d) snd_connect(%s)\n",
		Logstamp,aatos(Logbuf,addr,KEEP | HEX));

	if (addr->protocol == NULL) {
		PLOG1("Cannot locate transport provider\n");
		return(-1);
	}

	(void) sprintf(protdev,DEVSTR,addr->protocol);

	if ((fd = t_open(protdev, O_RDWR|O_EXCL, &linfo)) == -1) {
		LOG5(L_ALL,
		    "(%5d) att_connect: t_open failed, ret=%d, t_err=%d, err=%d\n",
		    Logstamp, fd, t_errno, errno);
		return(-1);
	}
	/*
	 * Here we verify that the protocol
	 * will provide connection-mode service.
	 */
	if (linfo.servtype == T_CLTS) {
		PLOG2("(%5d) att_connect: connection mode service not supported\n",
			Logstamp);
		(void) t_close(fd);
		return(-1);
	}
	if((req = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR)) == NULL) {
		LOG4(L_ALL,"(%5d) att_connect: t_alloc failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_close(fd);
		return(-1);
	} 
	req->addr.len = 0;
	if(t_bind(fd ,req, (struct t_bind *)NULL) == -1) {
		LOG4(L_ALL,"(%5d) att_connect: t_bind failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_free(req, T_BIND);
		(void) t_close(fd);
		return(-1);
	}
	if((sndcall = (struct t_call *)t_alloc(fd,T_CALL,T_ALL)) == NULL) {
		LOG4(L_ALL,"(%5d) att_connect: t_alloc failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_free(req, T_BIND);
		(void) t_close(fd);
		return(-1);
	}
	(void) memcpy(sndcall->addr.buf,addr->addbuf.buf,addr->addbuf.len);
	sndcall->addr.len = addr->addbuf.len;
	sndcall->opt.len = 0;	/* make sure default options are used	*/
	sndcall->udata.len = 0;
	LOG2(L_COMM,"(%5d) att_connect: port open - sending connect\n",Logstamp);
	SET_NODELAY(fd);
	if( t_connect(fd, sndcall, (struct t_call *)NULL) != -1 || t_errno != TNODATA) {
		LOG3(L_ALL,"(%5d) att_connect: t_connect failed, t_errno = %d\n",
			Logstamp,t_errno);
		LOG3(L_ALL,"(%5d) att_connect: t_look returns %d\n",
			Logstamp,(int) t_look(fd));
		LOG4(L_ALL,"(%5d) att_connect: t_connect failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_free(req, T_BIND);
		(void) t_free(sndcall, T_CALL);
		(void) t_close(fd);
		return(-1);
	}
	LOG2(L_COMM,"(%5d) att_connect: connection req sent\n",Logstamp);
	CLR_NODELAY(fd);
	return(fd);
}
int
rcv_connect(fd, serv_code)
int	fd;
long	serv_code;
{
	char msgbuf[128];
	int i;

	if (t_rcvconnect(fd, (struct t_call *)NULL) == -1) {
		LOG4(L_ALL,"(%5d) att_connect: t_rcvconnect failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_free(req, T_BIND);
		(void) t_free(sndcall, T_CALL);
		return(-1);
	}
	sprintf(msgbuf, LISTNMSG, serv_code);
	i = strlen(msgbuf) + 1;
	LOG3(L_COMM,"(%5d) att_connect: sending attbuf service code = %d\n",
		Logstamp,serv_code);

	if (t_snd(fd, msgbuf, i, 0) != i) {
		LOG4(L_ALL,"(%5d) att_connect: t_snd failed, t_err=%d, err=%d\n",
		    Logstamp, t_errno, errno);
		(void) t_free(req, T_BIND);
		(void) t_free(sndcall, T_CALL);
		return(-1);
	}
	LOG3(L_COMM,"(%5d) att_connect: connection complete, fd = %d\n",Logstamp,fd);
	return(fd);
}
