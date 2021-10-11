#ifndef lint
static        char sccsid[] = "@(#)uipc_socket.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


#include <sys/param.h>
#include "boot/systm.h"
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/mbuf.h>
#include <sys/un.h>
#include "boot/protosw.h"
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/route.h>
#include <netinet/in.h>
#include <net/if.h>

static int dump_debug = 10;

#undef	u
extern struct user u;

/*
 * Socket operation routines.
 * These routines are called by the routines in
 * sys_socket.c or from a system process, and
 * implement the semantics of socket operations by
 * switching out to the protocol specific routines.
 *
 * TODO:
 *	sostat
 *	test socketpair
 *	PR_RIGHTS
 *	clean up select, async
 *	out-of-band is a kludge
 */
/*ARGSUSED*/
socreate(dom, aso, type, proto)
	struct socket **aso;
	register int type;
	int proto;
{
	register struct protosw *prp;
	register struct socket *so;
	register struct mbuf *m;
	register int error;


	if (proto)
		prp = pffindproto(dom, proto);
	else
		prp = pffindtype(dom, type);
	if (prp == 0)	{
		dprint(dump_debug, 10,
			"socreate: EPROTONOSUPPORT\n");
		return (EPROTONOSUPPORT);
	}
	if (prp->pr_type != type)	{
		dprint(dump_debug, 10,
                        "socreate: EPROTOTYPE\n");
		return (EPROTOTYPE);
	}
	m = m_getclr(M_WAIT, MT_SOCKET);
	if (m == 0)	{
		dprint(dump_debug, 10, 
                        "socreate: ENOBUFS\n");
		return (ENOBUFS);
	}
	so = mtod(m, struct socket *);
	so->so_options = 0;
	so->so_state = 0;
	so->so_type = type;
	if (u.u_uid == 0)
		so->so_state = SS_PRIV;
	so->so_proto = prp;
	error =
	    (*prp->pr_usrreq)(so, PRU_ATTACH,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		dprint(dump_debug, 10,  
                        "socreate: prp->pr_usrreq error %d\n", error);
		so->so_state |= SS_NOFDREF;
		sofree(so);
		return (error);
	}
	*aso = so;
	return (0);
}

sobind(so, nam)
	struct socket *so;
	struct mbuf *nam;
{
	int s = splnet();
	int error;

	error = (*so->so_proto->pr_usrreq)(so, PRU_BIND,
		(struct mbuf *)0, nam, (struct mbuf *)0);
	(void) splx(s);
	return (error);
}

#ifdef	NEVER
solisten(so, backlog)
	register struct socket *so;
	int backlog;
{
	int s = splnet(), error;

	error =
	    (*so->so_proto->pr_usrreq)(so, PRU_LISTEN,
		(struct mbuf *)0, (struct mbuf *)0, (struct mbuf *)0);
	if (error) {
		(void) splx(s);
		return (error);
	}
	if (so->so_q == 0) {
		so->so_q = so;
		so->so_q0 = so;
		so->so_options |= SO_ACCEPTCONN;
	}
	if (backlog < 0)
		backlog = 0;
	so->so_qlimit = MIN(backlog, SOMAXCONN);
	(void) splx(s);
	return (0);
}
#endif	/* NEVER */

/*ARGSUSED*/
sofree(so)
	register struct socket *so;
{

#ifdef lint
	so = so;
#endif lint
}
