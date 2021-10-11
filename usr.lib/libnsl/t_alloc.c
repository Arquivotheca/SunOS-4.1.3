#ifndef lint
static	char sccsid[] = "@(#)t_alloc.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
#ident	"@(#)libnsl:nsl/t_alloc.c	1.4.1.1"
*/
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <nettli/tihdr.h>
#include <nettli/timod.h>
#include <nettli/tiuser.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/signal.h>

extern struct _ti_user *_t_checkfd();
extern char *calloc();
extern void free();
extern int errno, t_errno;
extern int ioctl();

char *
t_alloc(fd, struct_type, fields)
int fd;
int struct_type;
int fields;
{
	struct strioctl strioc;
	struct T_info_ack info;
	union structptrs {
		char	*caddr;
		struct t_bind *bind;
		struct t_call *call;
		struct t_discon *dis;
		struct t_optmgmt *opt;
		struct t_unitdata *udata;
		struct t_uderr *uderr;
		struct t_info *info;
	} p;
	unsigned dsize;
	int savemask;

	if (_t_checkfd(fd) == NULL)
		return(NULL);
	
	savemask = sigblock(sigmask(SIGPOLL));

	/*
	 * Get size info for T_ADDR, T_OPT, and T_UDATA fields
	 */
	info.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = -1;
	strioc.ic_len = sizeof(struct T_info_req);
	strioc.ic_dp = (char *)&info;
	if (ioctl(fd, I_STR, &strioc) < 0) {
		sigsetmask(savemask);
		t_errno = TSYSERR;
		return(NULL);
	}
	sigsetmask(savemask);
	if (strioc.ic_len != sizeof(struct T_info_ack)) {
		errno = EIO;
		t_errno = TSYSERR;
		return(NULL);
	}
	

	/*
	 * Malloc appropriate structure and the specified
	 * fields within each structure.  Initialize the
	 * 'buf' and 'maxlen' fields of each.
	 */
	switch (struct_type) {

	case T_BIND:
		if ((p.bind = (struct t_bind *)
			calloc(1, (unsigned)sizeof(struct t_bind))) == NULL)
				goto out;
		if (fields & T_ADDR) {
			if (_alloc_buf(&p.bind->addr, info.ADDR_size) < 0)
				goto out;
		}
		return((char *)p.bind);

	case T_CALL:
		if ((p.call = (struct t_call *)
			calloc(1, (unsigned)sizeof(struct t_call))) == NULL)
				goto out;
		if (fields & T_ADDR) {
			if (_alloc_buf(&p.call->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT) {
			if (_alloc_buf(&p.call->opt, info.OPT_size) < 0)
				goto out;
		}
		if (fields & T_UDATA) {
			dsize = _t_max(info.CDATA_size, info.DDATA_size);
			if (_alloc_buf(&p.call->udata, dsize) < 0)
				goto out;
		}
		return((char *)p.call);

	case T_OPTMGMT:
		if ((p.opt = (struct t_optmgmt *)
			calloc(1, (unsigned)sizeof(struct t_optmgmt))) == NULL)
				goto out;
		if (fields & T_OPT){
			if (_alloc_buf(&p.opt->opt, info.OPT_size) < 0)
				goto out;
		}
		return((char *)p.opt);

	case T_DIS:
		if ((p.dis = (struct t_discon *)
			calloc(1, (unsigned)sizeof(struct t_discon))) == NULL)
				goto out;
		if (fields & T_UDATA){
			if (_alloc_buf(&p.dis->udata, info.DDATA_size) < 0)
				goto out;
		}
		return((char *)p.dis);

	case T_UNITDATA:
		if ((p.udata = (struct t_unitdata *)
			calloc(1, (unsigned)sizeof(struct t_unitdata))) == NULL)
				goto out;
		if (fields & T_ADDR){
			if (_alloc_buf(&p.udata->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT){
			if (_alloc_buf(&p.udata->opt, info.OPT_size) < 0)
				goto out;
		}
		if (fields & T_UDATA){
			if (_alloc_buf(&p.udata->udata, info.TSDU_size) < 0)
				goto out;
		}
		return((char *)p.udata);

	case T_UDERROR:
		if ((p.uderr = (struct t_uderr *)
			calloc(1, (unsigned)sizeof(struct t_uderr))) == NULL)
				goto out;
		if (fields & T_ADDR){
			if (_alloc_buf(&p.uderr->addr, info.ADDR_size) < 0)
				goto out;
		}
		if (fields & T_OPT){
			if (_alloc_buf(&p.uderr->opt, info.OPT_size) < 0)
				goto out;
		}
		return((char *)p.uderr);

	case T_INFO:
		if ((p.info = (struct t_info *)
			calloc(1, (unsigned)sizeof(struct t_info))) == NULL)
				goto out;
		return((char *)p.info);

	default:
		errno = EINVAL;
		t_errno = TSYSERR;
		return(NULL);
	}

	/*
	 * Clean up. Set errno to ENOMEM if
	 * memory could not be allocated.
	 */
out:
	if (p.caddr)
		t_free(p.caddr, struct_type);

	t_errno = TSYSERR;
	errno = ENOMEM;
	return(NULL);
}

_alloc_buf(buf, n)
struct netbuf *buf;
{
	switch(n)
	{
		case -1:
			if ((buf->buf = calloc(1, 1024)) == NULL)
				return(-1);
			else buf->maxlen = 1024;
			break;

		case 0:
		case -2:
			buf->buf = NULL;
			buf->maxlen = 0;
			break;

		default:
			if ((buf->buf = calloc(1, n)) == NULL)
				return(-1);
			else buf->maxlen = n;
			break;
	}
	return(0);
}
