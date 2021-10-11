#ifndef lint
static	char sccsid[] = "@(#)portmap.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * portmap.c, Implements the program, version to port number
 * mapping for rpc.
 */

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <net/if.h>		/* to find local addresses */
#include <setjmp.h>
#include <errno.h>
#include <malloc.h>
#include <arpa/inet.h>

#define	MYSTAKSIZE 2048

static int	restarted;	/* Whether the portmap was restarted */
static struct sigstack ss;	/* All the signal handling on stack */
static struct sigstack oss;
static int	signalstack[MYSTAKSIZE];
static jmp_buf	my_environment;
static int	sjret;
static int	debugging;
static int	verbose;	/* Catch all errors */
static int	forward_socket;	/* forward the PMAPPROC_CALLIT on this fd */
static int	tcp_fd = -1;	/* For checking whether TCP service is up */
static int	udp_fd = -1;	/* For checking whether UDP service is up */
static SVCXPRT	*udp_xprt;	/* server handle for UDP */
static SVCXPRT	*tcp_xprt;	/* server handle for TCP */
static struct sockaddr_in me;	/* for forwarding */
struct pmaplist *pmaplist;	/* port number list */

time_t		time();
void		svc_versquiet();

void		reg_service();
void		reap();
void		mysvc_run();
void		onsig();
void		detachfromtty();
u_long		forward_register();
bool_t		chklocal();

main(argc, argv)
	int		argc;
	char		**argv;
{
	register struct pmaplist *pml;
	struct sockaddr_in addr;
	int		sock;
	int		len = sizeof (struct sockaddr_in);
	int		dontblock = 1;
	int		on = 1;

	while (argc > 1) {
		if (strcmp("-v", argv[1]) == 0)
			verbose = 1;
		if (strcmp("-d", argv[1]) == 0) {
			debugging = 1;
			verbose = 1;
		}
		argc--;
		argv++;
	}

	if (debugging) {
		printf("portmap debugging enabled -- will abort on errors!\n");
	} else {
		detachfromtty();
	}

	/*
	 * This is an effort to make portmap really bullet proof so that
	 * it never dies.  We try to catch (almost) all the signals and
	 * then we restart portmapper with the same portlist.  On
	 * receiving any such signals, it also leaves a core file.
	 * All the signals are handled on a different stack.  This was
	 * done so that if  somehow the stack gets corrupted, atleast
	 * the signal handler can run on a clean one.  And we also did not
	 * want the signal stuff to be in the core file.
	 */

	ss.ss_sp = (char *)signalstack;
	ss.ss_onstack = 0;
	if (sigstack(&ss, &oss) < 0)
		perror("portmap: sigstack");

	/*
	 * Save the environment, so that the old one can be reused in case
	 * portmapper dies.
	 */
	if (sjret = setjmp(my_environment)) {
		if (verbose)
			fprintf(stderr, "portmap restarted signal %d\n", sjret);
		restarted = 1;
		tcp_fd = -1;
		udp_fd = -1;
		FD_ZERO(&svc_fdset);	/* lose all rpc sockets */
		svc_destroy(udp_xprt);	/* free svc vectors */
		svc_destroy(tcp_xprt);
	}

	/* create a socket to forward PMAPPROC_CALLIT calls */
	if ((forward_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("portmap cannot create forward socket");
		exit(1);
	}
	/*
	 * The forwarding socket may get many broadcast requests and
	 * hence it is important that it doesnt get blocked for any reasons
	 */
	(void) ioctl(forward_socket, FIONBIO, &dontblock);
	get_myaddress(&me);	/* destination for forwarding == me */
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	if (bind(forward_socket, (struct sockaddr *) &addr, len) != 0) {
		perror("portmap cannot bind forward_socket");
		exit(1);
	}
	if (debugging)
		printf("portmap: forward socket is %d on port %d\n",
				forward_socket, addr.sin_port);

	/* create a server handle for UDP */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("portmap cannot create socket");
		exit(1);
	}
	addr.sin_port = htons(PMAPPORT);
	if (bind(sock, (struct sockaddr *) &addr, len) != 0) {
		perror("portmap cannot bind udp");
		exit(1);
	}
	/*
	 * We use SO_REUSEADDR because there is a possibility of kernel
	 * disallowing the use of the same bind address for some time even
	 * after that corresponding socket has been closed.  In the case
	 * of TCP, some data transfer may still be pending.
	 */
	(void) setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
				(char *)&on, sizeof on);
	if ((udp_xprt = svcudp_create(sock)) == (SVCXPRT *) NULL) {
		fprintf(stderr, "portmap could not do svcudp_create\n");
		exit(1);
	}
	(void) svc_register(udp_xprt, PMAPPROG, PMAPVERS, reg_service, FALSE);
	/* make an entry for ourself */
	if (!restarted){
		pml = (struct pmaplist *)
			malloc((u_int) sizeof (struct pmaplist));
		if (pml == 0) {
			perror("portmap: malloc failed");
			exit(1);
		}
		pml->pml_next = 0;
		pml->pml_map.pm_prog = PMAPPROG;
		pml->pml_map.pm_vers = PMAPVERS;
		pml->pml_map.pm_prot = IPPROTO_UDP;
		pml->pml_map.pm_port = PMAPPORT;
		pmaplist = pml;
	}

	/* create a server handle for TCP */
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("portmap cannot create socket");
		exit(1);
	}
	(void) setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
				(char *)&on, sizeof on);
	if (bind(sock, (struct sockaddr *) &addr, len) != 0) {
		perror("portmap cannot bind tcp");
		exit(1);
	}
	if ((tcp_xprt = svctcp_create(sock, RPCSMALLMSGSIZE,
			RPCSMALLMSGSIZE)) == (SVCXPRT *) NULL) {
		fprintf(stderr, "portmap could not do svctcp_create\n");
		exit(1);
	}
	(void) svc_register(tcp_xprt, PMAPPROG, PMAPVERS, reg_service, FALSE);
	/* make an entry for ourself */
	if (!restarted) {
		pml = (struct pmaplist *)
			malloc((u_int) sizeof (struct pmaplist));
		if (pml == 0) {
			perror("portmap: malloc failed");
			exit(1);
		}
		pml->pml_map.pm_prog = PMAPPROG;
		pml->pml_map.pm_vers = PMAPVERS;
		pml->pml_map.pm_prot = IPPROTO_TCP;
		pml->pml_map.pm_port = PMAPPORT;
		pml->pml_next = pmaplist;
		pmaplist = pml;
	}

	/*
	 * sockets send this on write after disconnect --
	 * live and learn from SVR4 work
	 */
	(void) signal(SIGPIPE, SIG_IGN);

	if (!debugging) {
#ifdef	SIGABRT
		(void) signal(SIGABRT, onsig);	/* restart */
#endif
		(void) signal(SIGIOT, onsig);	/* restart */
		(void) signal(SIGILL, onsig);	/* restart */
		(void) signal(SIGBUS, onsig);	/* restart */
		(void) signal(SIGSEGV, onsig);	/* restart */
	}

	(void) signal(SIGCHLD, reap);

	/*
	 * Tell RPC library to shut up about version mismatches so that new
	 * revs of broadcast protocols don't cause all the old servers to
	 * say: "wrong version".
	 */
	svc_versquiet(udp_xprt);
	svc_versquiet(tcp_xprt);

	mysvc_run();
	fprintf(stderr, "portmap: svc_run returned unexpectedly\n");
	abort();	/* will restart portmap */
	/* NOTREACHED */
}

/*
 * This routine is called to make sure that the given port number is
 * still bound.  This is helpful in those cases where the server dies
 * and the client gets the wrong information.  It is better if the
 * client about this fact at create time itself.  If the given port
 * number is not bound, we return FALSE.
 */
bool_t
is_bound(pml)
	struct pmaplist *pml;
{
	int fd;
	int res;
	struct sockaddr_in myaddr;
	struct sockaddr_in *sin;

	if (pml->pml_map.pm_prot == IPPROTO_TCP) {
		if (tcp_fd < 0)
			tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
		fd = tcp_fd;
	} else if (pml->pml_map.pm_prot == IPPROTO_UDP) {
		if (udp_fd < 0)
			udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
		fd = udp_fd;
	} else
		return (TRUE);
	if (fd < 0)
		return (TRUE);
	sin = &myaddr;
	bzero((char *)sin, sizeof (*sin));
	sin->sin_family = AF_INET;
	sin->sin_port = htons(pml->pml_map.pm_port);
	res = bind(fd, (struct sockaddr *)sin, sizeof (struct sockaddr_in));
	if (res < 0)
		return (TRUE);
	if (pml->pml_map.pm_prot == IPPROTO_TCP) {
		/* XXX: Too bad that there is no unbind(). */
		(void) close(tcp_fd);
		tcp_fd = -1;
	} else {
		(void) close(udp_fd);
		udp_fd = -1;
	}

	return (FALSE);
}

/*
 * Finds the service for prog, vers.  Even if the specified vers
 * is not registered, we still return the registeration for that
 * program number, with the hope that the client will figure out
 * the actual available version number on doing a clnt_call() via
 * the RPC_PROGVERSMISMTACH.
 */
static struct pmaplist *
find_service(prog, vers, prot)
	u_long		prog;
	u_long		vers;
	u_long		prot;
{
	register struct pmaplist *hit = NULL;
	register struct pmaplist *hitp = NULL;
	register struct pmaplist *pml;
	register struct pmaplist *prevpml;

	for (prevpml = NULL, pml = pmaplist; pml != NULL; pml = pml->pml_next) {
		if ((pml->pml_map.pm_prog != prog) ||
		    (pml->pml_map.pm_prot != prot)) {
			prevpml = pml;
			continue;
		}
		hit = pml;
		hitp = prevpml;
		if (pml->pml_map.pm_vers == vers)
			break;
		prevpml = pml;
	}
	if (hit == NULL)
		return (NULL);

	/* Make sure it is bound */
	if (is_bound(hit))
		return (hit);

	/* unhook it from the list */
	if (verbose)
		fprintf(stderr,
"portmap: service failed for prog=%d, vers=%d, prot=%d\n",
			hit->pml_map.pm_prog, hit->pml_map.pm_vers,
			hit->pml_map.pm_prot);

	pml = hit->pml_next;
	if (hitp == NULL)
		pmaplist = pml;
	else
		hitp->pml_next = pml;
	free((caddr_t) hit);

	/*
	 * Mostprobably other versions and transports for this
	 * program number were also registered by this same
	 * service, so we should check their bindings also.
	 * This may slow down the response time a bit.
	 */
	for (prevpml = NULL, pml = pmaplist; pml != NULL;) {
		if (pml->pml_map.pm_prog != prog) {
			prevpml = pml;
			pml = pml->pml_next;
			continue;
		}
		/* Make sure it is bound */
		if (is_bound(pml)) {
			prevpml = pml;
			pml = pml->pml_next;
			continue;
		} else {
			/* unhook it */
			hit = pml;
			pml = hit->pml_next;
			if (prevpml == NULL)
				pmaplist = pml;
			else
				prevpml->pml_next = pml;
			free((caddr_t) hit);
		}
	}
	return (NULL);
}

/*
 * 1 OK, 0 not
 */
void
reg_service(rqstp, xprt)
	struct svc_req 	*rqstp;
	SVCXPRT		*xprt;
{
	struct pmap	reg;
	struct pmaplist *pml, *prevpml, *fnd;
	int		ans;
	u_long		port;
	caddr_t		t;
	struct sockaddr_in *who;

	if (debugging)
		printf("portmap: request %d\n", rqstp->rq_proc);
	switch (rqstp->rq_proc) {

	case PMAPPROC_NULL:
		/*
		 * Null proc call
		 */
		if ((!svc_sendreply(xprt, xdr_void, (char *)NULL)) &&
			debugging) {
			fprintf(stderr, "portmap: svc_sendreply\n");
			abort();
		}
		break;

	case PMAPPROC_SET:
		/*
		 * Set a program, version to port mapping
		 */
		if (!svc_getargs(xprt, xdr_pmap, &reg)) {
			svcerr_decode(xprt);
			return;
		}
		who = svc_getcaller(xprt);
		if (chklocal(ntohl(who->sin_addr)) == FALSE) {
			if (verbose)
				fprintf(stderr,
"portmap: attempt to set prog %d vers %d port %d from host %s port %d\n",
					reg.pm_prog, reg.pm_vers, reg.pm_port,
					inet_ntoa(who->sin_addr),
					ntohs(who->sin_port));
			ans = 0;
			goto done_set;
		}
		/*
		 * check to see if already used find_service returns
		 * a hit even if the versions don't match, so check
		 * for it
		 */
		fnd = find_service(reg.pm_prog, reg.pm_vers, reg.pm_prot);
		if (fnd && fnd->pml_map.pm_vers == reg.pm_vers) {
			if (fnd->pml_map.pm_port == reg.pm_port) {
				ans = 1;
				goto done_set;
			} else {
				/* Caller should have done UNSET first */
				ans = 0;
				goto done_set;
			}
		} else {
			if ((reg.pm_port < IPPORT_RESERVED) &&
			    (ntohs(who->sin_port) >= IPPORT_RESERVED)) {
				if (verbose)
					fprintf(stderr,
"portmap: attempt to set (prog %d vers %d reserved port %d) from port %d\n",
						reg.pm_prog,
						reg.pm_vers,
						reg.pm_port,
						ntohs(who->sin_port));
				ans = 0;
				goto done_set;
			}
			/*
			 * add to END of list
			 */
			pml = (struct pmaplist *)
				malloc((u_int) sizeof (struct pmaplist));
			if (pml == 0) {
				perror("portmap: malloc failed in set");
				svcerr_systemerr(xprt);
				if (debugging)
					abort();
				return;
			}
			pml->pml_map = reg;
			pml->pml_next = 0;
			if (pmaplist == 0) {
				pmaplist = pml;
			} else {
				for (fnd = pmaplist; fnd->pml_next != 0;
					fnd = fnd->pml_next);
				fnd->pml_next = pml;
			}
			ans = 1;
		}
	done_set:
		if ((!svc_sendreply(xprt, xdr_long, (caddr_t) &ans)) &&
		    debugging) {
			fprintf(stderr, "portmap: svc_sendreply\n");
			abort();
		}
		break;

	case PMAPPROC_UNSET:
		/*
		 * Remove a program, version to port mapping.
		 */
		if (!svc_getargs(xprt, xdr_pmap, &reg)) {
			svcerr_decode(xprt);
			return;
		}
		ans = 0;
		who = svc_getcaller(xprt);
		if (chklocal(ntohl(who->sin_addr)) == FALSE) {
			if (verbose)
				fprintf(stderr,
"portmap: attempt to unset (prog %d vers %d) from host %s port %d\n",
					reg.pm_prog, reg.pm_vers,
					inet_ntoa(who->sin_addr),
					ntohs(who->sin_port));
			goto done_unset;
		}
		for (prevpml = NULL, pml = pmaplist; pml != NULL;) {
			if ((pml->pml_map.pm_prog != reg.pm_prog) ||
			    (pml->pml_map.pm_vers != reg.pm_vers)) {
				/* both pml & prevpml move forwards */
				prevpml = pml;
				pml = pml->pml_next;
				continue;
			}
			if ((pml->pml_map.pm_port < IPPORT_RESERVED) &&
			    (ntohs(who->sin_port) >= IPPORT_RESERVED)) {
				if (verbose)
					fprintf(stderr,
"portmap: attempt to unset (prog %d vers %d reserved port %d) port from %d\n",
					pml->pml_map.pm_prog,
					pml->pml_map.pm_vers,
					pml->pml_map.pm_port,
					ntohs(who->sin_port));
				goto done_unset;
			}
			/* found it; pml moves forward, prevpml stays */
			ans = 1;
			t = (caddr_t) pml;
			pml = pml->pml_next;
			if (prevpml == NULL)
				pmaplist = pml;
			else
				prevpml->pml_next = pml;
			free(t);
		}
	done_unset:
		if ((!svc_sendreply(xprt, xdr_long, (caddr_t) &ans)) &&
			debugging) {
			fprintf(stderr, "portmap: svc_sendreply\n");
			abort();
		}
		break;

	case PMAPPROC_GETPORT:
		/*
		 * Lookup the mapping for a program, version and
		 * return its port
		 */
		if (!svc_getargs(xprt, xdr_pmap, &reg)) {
			svcerr_decode(xprt);
			return;
		}
		fnd = find_service(reg.pm_prog, reg.pm_vers, reg.pm_prot);
		if (fnd)
			port = fnd->pml_map.pm_port;
		else
			port = 0;
		if ((!svc_sendreply(xprt, xdr_long, (caddr_t) &port)) &&
			debugging) {
			fprintf(stderr, "portmap: svc_sendreply\n");
			abort();
		}
		break;

	case PMAPPROC_DUMP:
		/*
		 * Return the current set of mapped program, version
		 */
		if ((!svc_sendreply(xprt, xdr_pmaplist,
			(caddr_t) &pmaplist)) && debugging) {
			fprintf(stderr, "portmap: svc_sendreply\n");
			abort();
		}
		break;

	case PMAPPROC_CALLIT:
		/*
		 * Calls a procedure on the local machine.  If the requested
		 * procedure is not registered this procedure does not return
		 * error information!! This procedure is only supported on
		 * rpc/udp and calls via rpc/udp.  It passes null
		 * authentication parameters.
		 */
		callit(rqstp, xprt);
		break;

	default:
		svcerr_noproc(xprt);
		break;
	}
	return;
}


/*
 * Stuff for the rmtcall service
 */
#define	ARGSIZE 9000

typedef struct encap_parms {
	u_long		arglen;
	char		*args;
};

static		bool_t
xdr_encap_parms(xdrs, epp)
	XDR		*xdrs;
	struct encap_parms *epp;
{
	return (xdr_bytes(xdrs, &(epp->args), &(epp->arglen), ARGSIZE));
}

typedef struct rmtcallargs {
	u_long		rmt_prog;
	u_long		rmt_vers;
	u_long		rmt_port;
	u_long		rmt_proc;
	struct encap_parms rmt_args;
};

static		bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR	*xdrs;
	register struct rmtcallargs *cap;
{
	/* does not get a port number */
	if (xdr_u_long(xdrs, &(cap->rmt_prog)) &&
	    xdr_u_long(xdrs, &(cap->rmt_vers)) &&
	    xdr_u_long(xdrs, &(cap->rmt_proc))) {
		return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
	}
	return (FALSE);
}

static		bool_t
xdr_rmtcall_result(xdrs, cap)
	register XDR	*xdrs;
	register struct rmtcallargs *cap;
{
	if (xdr_u_long(xdrs, &(cap->rmt_port)))
		return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
	return (FALSE);
}

/*
 * only worries about the struct encap_parms part of struct rmtcallargs. The
 * arglen must already be set!!
 */
static		bool_t
xdr_opaque_parms(xdrs, cap)
	XDR		*xdrs;
	struct rmtcallargs *cap;
{
	return (xdr_opaque(xdrs, cap->rmt_args.args, cap->rmt_args.arglen));
}

/*
 * This routine finds and sets the length of incoming opaque paraters and
 * then calls xdr_opaque_parms.
 */
static		bool_t
xdr_len_opaque_parms(xdrs, cap)
	register XDR	*xdrs;
	struct rmtcallargs *cap;
{
	register u_int	beginpos, lowpos, highpos, currpos, pos;

	beginpos = lowpos = pos = xdr_getpos(xdrs);
	highpos = lowpos + ARGSIZE;
	while ((int) (highpos - lowpos) >= 0) {
		currpos = (lowpos + highpos) / 2;
		if (xdr_setpos(xdrs, currpos)) {
			pos = currpos;
			lowpos = currpos + 1;
		} else {
			highpos = currpos - 1;
		}
	}
	xdr_setpos(xdrs, beginpos);
	cap->rmt_args.arglen = pos - beginpos;
	return (xdr_opaque_parms(xdrs, cap));
}

/*
 * Call a remote procedure service This procedure is very quiet when things
 * go wrong. The proc is written to support broadcast rpc.  In the broadcast
 * case, a machine should shut-up instead of complain, less the requestor be
 * overrun with complaints at the expense of not hearing a valid reply ...
 * The data is forwarded on a new xid.  When we hear back from the service,
 * we send the answer back to the client with the old xid patched in.
 */
struct bogus_data {
	u_int		buflen;
	u_long		xid;
};
#define	getbogus_data(xprt) ((struct bogus_data *) (xprt->xp_p2))

static
callit(rqstp, xprt)
	struct svc_req *rqstp;
	SVCXPRT		*xprt;
{
	struct rmtcallargs a;
	struct pmaplist *pml;
	u_long		port;
	struct sockaddr_in *who;
	int		outlen;
	XDR		outxdr;
	struct rpc_msg	call_msg;
	int		addrlen;
	char		buf[ARGSIZE];
	char		outbuf[ARGSIZE];
	struct bogus_data *bd;
	AUTH		*auth;

	a.rmt_args.args = buf;	/* arguments on stack */
	if (!svc_getargs(xprt, xdr_rmtcall_args, &a))
		return;
	if (a.rmt_prog == PMAPPROG) {
		if (verbose) {
			who = svc_getcaller(xprt);
			fprintf(stderr,
	"portmap: self callit reqst for proc=%d, from host=%s, port=%d\n",
				a.rmt_proc, inet_ntoa(who->sin_addr),
				ntohs(who->sin_port));
		}
		return;
	}
	if ((pml = find_service(a.rmt_prog, a.rmt_vers,
		(u_long) IPPROTO_UDP)) == NULL)
		return;
	port = pml->pml_map.pm_port;
	if (debugging)
		printf("portmap: callit entered port=%d\n", port);
	me.sin_port = htons(port);	/* destination */
	bd = getbogus_data(xprt);
	who = svc_getcaller(xprt);
	call_msg.rm_xid = forward_register(bd->xid, who, (u_short)port);
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = a.rmt_prog;
	call_msg.rm_call.cb_vers = a.rmt_vers;
	xdrmem_create(&outxdr, outbuf, ARGSIZE, XDR_ENCODE);
	if (!xdr_callhdr(&outxdr, &call_msg)) {
		if (debugging)
			fprintf(stderr, "portmap: cant xdr callheader\n");
		return;
	}
	if (!xdr_u_long(&outxdr, &(a.rmt_proc)))
		return;

	if (rqstp->rq_cred.oa_flavor == AUTH_NULL) {
		auth = authnone_create();
	} else if (rqstp->rq_cred.oa_flavor == AUTH_UNIX) {
		struct authunix_parms *au;

		au = (struct authunix_parms *)rqstp->rq_clntcred;
		auth = authunix_create(au->aup_machname,
			au->aup_uid, au->aup_gid, au->aup_len, au->aup_gids);
		if (auth == NULL) /* fall back */
			auth = authnone_create();
	} else {
		/* we do not support any other authentication scheme */
		return;
	}
	if (auth == NULL)
		return;
	if (!AUTH_MARSHALL(auth, &outxdr)) {
		AUTH_DESTROY(auth);
		return;
	}
	AUTH_DESTROY(auth);
	if (!xdr_opaque_parms(&outxdr, &a))
		return;
	outlen = (int) XDR_GETPOS(&outxdr);
	addrlen = sizeof (struct sockaddr_in);
	(void) sendto(forward_socket, outbuf, outlen, 0, &me, addrlen);
	return;
}

/*
 * this does an asynchronous get (manually) and uses svc_udpreplyto to
 * forward it.  Hand generate the entire RPC packet for delivery to
 * the initial caller.
 */
#define	MAXREPLY 9000

int
handle_reply()
{
	int		inlen;
	struct sockaddr_in from;
	struct sockaddr_in to;
	int		fromlen;
	XDR		reply_xdrs;
	struct rpc_msg	reply_msg;
	struct rpc_err	reply_error;
	int		pos;
	int		len;
	int		ok;
	u_long		xto;
	struct rmtcallargs a;
	char		buffer[MAXREPLY];

	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = 0;
	reply_msg.acpted_rply.ar_results.proc = xdr_void;

	fromlen = sizeof (struct sockaddr);
	inlen = recvfrom(forward_socket, buffer, MAXREPLY, 0,
			&from, &fromlen);
	if (inlen < 0) {
		if (debugging)
			perror("portmap: recvfrom in handle_reply");
		return (-1);
	}
	xdrmem_create(&reply_xdrs, buffer, (u_int)inlen, XDR_DECODE);
	ok = xdr_replymsg(&reply_xdrs, &reply_msg);
	if (!ok) {
		if (debugging)
			fprintf(stderr, "portmap: handle_reply: bad message\n");
		return (-2);
	}
	_seterr_reply(&reply_msg, &reply_error);
	if (reply_error.re_status != RPC_SUCCESS) {
		/* We keep quite in cases of RPC failure */
		if (debugging)
			fprintf(stderr, "portmap: handle_reply: RPC error\n");
		return (-3);
	}
	pos = XDR_GETPOS(&reply_xdrs);
	len = inlen - pos;
	a.rmt_args.args = &buffer[pos];
	a.rmt_args.arglen = len;
	a.rmt_port = (u_long) from.sin_port;
	if (forward_find(reply_msg.rm_xid, from.sin_port, &to, &xto)) {
		if (chklocal(from.sin_addr) == FALSE) {
			if (verbose)
				fprintf(stderr,
"portmap: attempt to forward reply from host %s port %d to host %s port%d\n",
					inet_ntoa(from.sin_addr),
					ntohs(from.sin_port),
					inet_ntoa(to.sin_addr),
					ntohs(to.sin_port));
			return (-3);
		}
		if (debugging)
			printf(
"portmap: lookup ok for xid=%d port=%d to=%s.%d xto=%d\n",
				reply_msg.rm_xid, from.sin_port,
				inet_ntoa(to.sin_addr),
				ntohs(to.sin_port), xto);
		return (svc_udpreplyto(udp_xprt, &to, xto,
				xdr_rmtcall_result, (char *)&a));
	} else {
		if (verbose)
			fprintf(stderr, "portmap: cant lookup xid=%d port=%d\n",
					reply_msg.rm_xid, from.sin_port);
		return (-4);
	}
}

/*
 * udpreplyto is like sendreply but it only works for udp and it allow the
 * caller to specify the reply xid and destination
 */
int
svc_udpreplyto(xprt, who, xid, xdr_results, a)
	SVCXPRT		*xprt;
	struct sockaddr_in *who;
	u_long		xid;
	xdrproc_t	xdr_results;
	char		*a;
{
	struct bogus_data *bd;	/* bogus */

	*(svc_getcaller(xprt)) = *who;
	bd = getbogus_data(xprt);
	bd->xid = xid;		/* set xid on reply */
	return (svc_sendreply(xprt, xdr_results, a));
}

#define	NFS 256
struct finfo {
	u_long		xid;
	struct sockaddr_in repto;
	u_short		port;
	time_t		time;
	u_long		callerxid;
};
struct finfo		FINFO[NFS];

static u_long
forward_register(callerxid, who, port)
	u_long		callerxid;
	struct sockaddr_in *who;
	u_short		port;
{
	int		i;
	int		j;
	time_t		mint;
	static u_long	lastxid;

	mint = FINFO[0].time;
	j = 0;
	if (lastxid == 0)
		lastxid = time((time_t *)0) * NFS; /* initialize value */
	for (i = 0; i < NFS; i++) {
		if ((FINFO[i].callerxid == callerxid) &&
		    (FINFO[i].port == port) &&
		    (FINFO[i].repto.sin_addr.s_addr == who->sin_addr.s_addr) &&
		    (FINFO[i].repto.sin_port == who->sin_port)) {
			FINFO[i].time = time((time_t *)0);
			return (FINFO[i].xid);	/* forward on this xid */
		}
		if (FINFO[i].time < mint) {
			j = i;
			mint = FINFO[i].time;
		}
	}
	FINFO[j].time = time((time_t *)0);
	FINFO[j].callerxid = callerxid;
	FINFO[j].port = port;
	FINFO[j].repto = *who;
	lastxid = lastxid + NFS;
	FINFO[j].xid = lastxid + j;	/* encode slot */
	return (FINFO[j].xid);		/* forward on this xid */
}

int
forward_find(xid, port, who, outxid)
	u_int		xid;
	u_short		port;
	struct sockaddr_in *who;
	u_long		*outxid;
{
	int		i;

	i = xid % NFS;
	if (i < 0)
		i += NFS;
	if ((FINFO[i].xid == xid) && (port == FINFO[i].port)) {
		*who = FINFO[i].repto;
		*outxid = FINFO[i].callerxid;
		return (1);
	}
	return (0);
}

/*
 * Recvd some signal. We never say die, and we try to move along
 * because we do not want to loose our registrations.
 */
void
onsig(x)
{
	int	t;
	int	ds = getdtablesize();

	for (t = 0; t < ds; t++)
		(void) close(t);	/* close any sockets */

	if (fork() == 0) {
#ifdef SIGABRT
		signal(SIGABRT, SIG_DFL);
#endif
		(void) signal(SIGIOT, SIG_DFL);
		(void) sigsetmask(0);
		abort();	/* snapshot */
		exit(-1);
	}

	longjmp(my_environment, x);
	exit(-1);
	/* NOTREACHED */
}

void
detachfromtty()
{
	int	ds = getdtablesize();
	int	pid, t;

	pid = fork();
	if (pid < 0) {
		perror("portmap: fork");
		exit(1);
	}
	if (pid != 0)
		exit(0);
	for (t = 0; t < ds; t++)
		(void) close(t);
	t = open("/dev/console", 2);
	if (t >= 0) {
		(void) dup2(t, 1);
		(void) dup2(t, 2);
	}
	t = open("/dev/tty", 2);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *) 0);
		(void) close(t);
	}
	/* more disassociation */
	(void) setpgrp(getpid(), getpid());
}

/*
 * This is required when portmapper forks off to create its new
 * copy because of some signal.
 */
void
reap()
{
	while (wait3((union wait *)NULL, WNOHANG, NULL) > 0);
}

/* how many interfaces could there be on a computer? */
#define	MAX_LOCAL 64
static int num_local = -1;
static struct in_addr addrs[MAX_LOCAL];

bool_t
chklocal(taddr)
	struct in_addr	taddr;
{
	int		i;
	struct in_addr	iaddr;

	if (taddr.s_addr == INADDR_LOOPBACK)
		return (TRUE);
	if (num_local == -1) {
		num_local = getlocal();
		if (debugging)
			printf("portmap: %d interfaces detected.\n", num_local);
	}
	for (i = 0; i < num_local; i++) {
		iaddr.s_addr = ntohl(addrs[i].s_addr);
		if (bcmp((char *) &taddr, (char *) &(iaddr),
			sizeof (struct in_addr)) == 0)
			return (TRUE);
	}
	return (FALSE);
}

int
getlocal()
{
	struct ifconf	ifc;
	struct ifreq	ifreq, *ifr;
	int		n, j, sock;
	char		buf[UDPMSGSIZE];

	ifc.ifc_len = UDPMSGSIZE;
	ifc.ifc_buf = buf;
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return (FALSE);
	if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0) {
		perror("portmap:icotl SIOCGIFCONF");
		(void) close(sock);
		return (FALSE);
	}
	ifr = ifc.ifc_req;
	j = 0;
	for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, ifr++) {
		ifreq = *ifr;
		if (ioctl(sock, SIOCGIFFLAGS, (char *) &ifreq) < 0) {
			perror("portmap:icotl SIOCGIFFLAGS");
			continue;
		}
		if ((ifreq.ifr_flags & IFF_UP) &&
			ifr->ifr_addr.sa_family == AF_INET) {
			if (ioctl(sock, SIOCGIFADDR, (char *) &ifreq) < 0) {
				perror("SIOCGIFADDR");
			} else {
				addrs[j] = ((struct sockaddr_in *)
						& ifreq.ifr_addr)->sin_addr;
				j++;
			}
		}
		if (j >= (MAX_LOCAL - 1))
			break;
	}
	(void) close(sock);
	return (j);
}

void
mysvc_run()
{
	fd_set	readfds;
	int	dtbsize = _rpc_dtablesize();

	for (;;) {
		readfds = svc_fdset;
		FD_SET(forward_socket, &readfds);
		switch (select(dtbsize, &readfds,
			(fd_set *)0, (fd_set *)0, (struct timeval *)0)) {
		case -1:
			if (errno == EINTR) {
				continue;
			}
			perror("select error");
			return;
		case 0:
			continue;
		default:
			if (FD_ISSET(forward_socket, &readfds)) {
				(void) handle_reply();
				continue;
			};
			svc_getreqset(&readfds);
		}
	}
}
