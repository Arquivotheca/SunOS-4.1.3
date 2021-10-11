#ifndef lint
static char sccsid[] = "@(#)pnp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988-1990 by Sun Microsystems, Inc.
 */

#include	<sys/param.h>
#include 	<sys/dir.h>
#include	<sys/user.h>
#include	<sys/uio.h>
#include	<sys/socket.h>
#include	<sys/kernel.h>
#include	<sys/ioctl.h>
#include 	<sys/reboot.h>
#include 	<sys/bootconf.h>

#include	<net/if.h>
#include	<net/if_arp.h>

#include	<netinet/in.h>

#include	<rpc/rpc.h>
#include	<rpc/pmap_rmt.h>
#include	<rpc/pmap_prot.h>

#include	<rpcsvc/pnprpc.h>

#include	<stand/saio.h>

#ifndef sun3x
#include 	<mon/cpu.addrs.h>
#endif  sun3x

#include	<mon/sunromvec.h>
#include	<mon/idprom.h>

#ifdef sun386i
#include 	<i386/380.h>
#endif sun386i

#include	"boot/systm.h"
#include	"boot/if_ether.h"


#ifdef	DUMP_DEBUG
static	int	dump_debug = 20;
#endif	/* DUMP_DEBUG */

#undef u
struct	user	u;

#ifdef	DRARP_REQUEST
extern int	use_drarp;
#endif

#ifdef RPCDEBUG
extern int rpcdebug;
#endif

    /* Default "printf" is gated according to NVRAM verbosity control.
     * Bypass by using bpprintf ... here we REALLY want to see the
     * status, it's meaningful to very naive users.
     */

#ifdef sun386i
#define		printf	bpprintf
#endif sun386i

#define	if_broadaddr	if_addrlist->ifa_broadaddr

/*
 * PNP Boot program ... downloaded when the network has granted
 * an IP address to this system, and it issues TFTP boot requests,
 * but no hostname is stored for this IP address.
 *
 * This follows the "plug'n'play" protocol to get configured on
 * the network or to generate reasonable diagnostics.
 *
 * Note ... bastardized from boot.c, there's a lot in this
 * boot program that can be deleted.  All it really needs is to
 * issue RPCs!
 */


struct	bootparam	*boot_bp;

extern int		memory_avail;
extern struct ifnet	*if_ifwithaf();

#undef	u;
extern struct user	u;
extern struct in_addr	my_in_addr;

extern int		ethernet_started;


    /* Checkpoints for progress meter.  Defined according to
     * performance near FCS.
     */

#define	CHKPT_START	0

#define	CHKPT_VERIFY	5
#define	CHKPT_ACQUIRE	10
#define	CHKPT_SETUP	20
#define	CHKPT_POLL	30
#define	CHKPT_POLL_INCR	10
#define	CHKPT_BACKWARDS	75
#define	CHKPT_PNPDONE	100

#define	CHKPT_MAX	100

#ifdef	sun386
static int		chkpt_now = 0;
#define	PMeter(x)	PROGRESS(chkpt_now = (x))
#else
#define	PMeter(x)
#endif	sun386

/*
 * Verbose mode --
 * Not (yet) implemented.
 */
extern int verbosemode;


static char *msgs [] = {
    /* 000 series:  fatal errors
     */
#define	MSG_NONPNPNET		msgs[0]
    "This network does not support Automatic System Installation.",
#define	MSG_NOmesg		msgs[1]
    "",
#define	MSG_NOSOFTWARE		msgs[2]
    "No software for this architecture installed.",
#define	MSG_NOSPACE		msgs[3]
    "Not enough disk space to support a diskless client.",
#define	MSG_NOCLIENTS		msgs[4]
    "Unwilling to accept more clients.",

	/* 100 series:  diagnostics, the user must take some action
	 */
#define	MSG_SEEADMIN		msgs[5]
    "See your Network Administrator to resolve this problem.",
#define	MSG_PROTOERR		msgs[6]
    "A nonfatal protocol error ocurred.  Installation may not work.",

	/* 200 series:  diagnostics, information only
	 */
#define	MSG_ACQUIRETIMEOUT	msgs[7]
    "No server responded, restarting Automatic System Installation",
#define	MSG_POLLTIMEOUT		msgs[8]
    "The server installing this system may have crashed, restarting.",
#define	MSG_RETRY		msgs[9]
    "Retrying automatic system installation.",
#define	MSG_POLLERR		msgs[10]
    "An error ocurred during the POLL sequence.",
#define	MSG_BUSYRETRY		msgs[11]
    "The server is busy, retrying.",
#define	MSG_SETUPFAIL		msgs[12]
    "The SETUP request failed, retrying.",
#define	MSG_WAIT		msgs[13]
    "Your Network Administrator must configure this system.",
#define	MSG_WHOAREYOU		msgs[14]
    "The server seems to have rebooted.",
#define	MSG_SETUPERR		msgs[15]
    "Error in setup sequence.",
};

extern enum clnt_stat	clntkudp_callit_addr ();

static void
twiddler (eh)
{
    static char dialbuf [] = "|/-\\";	/* 4 chars */
    static int i;

    if (!eh) {
	putchar (dialbuf [i++]);
	putchar ('\b');
	i = i % 4;
    } else {
	putchar (' ');
	putchar ('\b');
	i = 0;
    }
}

/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters.  This allows
 * programs to do a lookup and call in one step.
 */

static enum clnt_stat
pmap_rmtcall (call_addr, prog, vers, proc,
		xdrargs, argsp, xdrres, resp, tout, resp_addr)
    struct sockaddr_in *call_addr;
    u_long prog, vers, proc;
    xdrproc_t xdrargs, xdrres;
    caddr_t argsp, resp;
    struct timeval tout;
    struct sockaddr_in *resp_addr;
{
    register CLIENT *client;
    struct rmtcallargs a;
    struct rmtcallres r;
    enum clnt_stat stat;
    u_long port;


    call_addr->sin_port = htons(PMAPPORT);
    client = clntkudp_create(call_addr, PMAPPROG, PMAPVERS, 10, u.u_cred);
    if (client != (CLIENT *)NULL) {
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.args_ptr = argsp;
	a.xdr_args = xdrargs;
	r.port_ptr = &port;
	r.results_ptr = resp;
	r.xdr_results = xdrres;

	    /* XXX this call doesn't produce any sort of ongoing
	     * activity indicator at all. UNFRIENDLY !!
	     */
	stat = clntkudp_callit_addr(client, PMAPPROC_CALLIT,
		xdr_rmtcall_args, (caddr_t)&a, xdr_rmtcallres,
		(caddr_t)&r, tout, resp_addr);
	resp_addr->sin_port = htons(port);
	CLNT_DESTROY(client);
    } else {
	printf("PNPBoot: remote call failed - port mapper failure?\n");
    }
    return (stat);
}

static void
xsleep (nsec)
    register nsec;
{
    while (nsec-- > 0) {
	twiddler (0);
	DELAY (1000000);
    }
    twiddler (1);
}

static int
init_inet (ifpp)
    register struct ifnet **ifpp;
{
    register struct in_addr *ip1;

	/* ensure the net's started up and the tables are
	 * initialized.
	 */
    if (!ethernet_started) {
	ethersetup ();
	domaininit ();
	ip_init ();
	ethernet_started++;
    }

    *ifpp = if_ifwithaf (AF_INET);
    if (!*ifpp) {
	printf ("?? No INET interface ??\n");
	return -1;
    }
    if (!address_known (*ifpp))
	revarp_myaddr (*ifpp);

	/* this is to ensure the interface has a valid IP
	 * broadcast address.
	 */
    ip1 = &(((struct sockaddr_in *)&((*ifpp)->if_broadaddr))->sin_addr);
    ip1->s_net = my_in_addr.s_net;
    ip1->s_host = my_in_addr.s_host;
    ip1->s_lh = my_in_addr.s_lh;
    ip1->s_impno = 0;

    return 0;
}

    /* This code will never get loaded except during a PNP sequence
     * on a pnp network ... but servers go away sometimes.  Make
     * certain there's one to talk to.
     * TIME:  30 seconds max.
     */
static int
is_pnp_net (ifp)
    register struct ifnet *ifp;
{
    register int i;
    struct sockaddr_in pnpaddr;
    register enum clnt_stat stat;
    struct timeval tv;

    PMeter (CHKPT_VERIFY);

    tv.tv_sec = 15;
    tv.tv_usec = 0;

    for (i = 0; i < 2; i++) {
	twiddler (1);
	stat = pmap_rmtcall (&(ifp->if_broadaddr), (u_long)PNPD_PROG,
		(u_long)PNPD_VERS, NULLPROC,
		xdr_void, (char *) 0,
		xdr_void, (char *) 0,
		tv, &pnpaddr);

	if (!stat) {
	    twiddler (1);
	    return 1;
	} else
	    twiddler (0);
    }
    return 0;
}

    /* the only time this will succeed is if someone's changed the
     * network configuration, and this system is already known but
     * at a different location.
     *
     * This only supports automated installation just now, NOT
     * reconfiguration.
     */
static pnp_errcode
pnp_whoami (ifp)
    struct ifnet *ifp;
{
    PMeter (CHKPT_START);

	/* XXX IMPLEMENT
	 *	This requires a broadcast RPC mechanism.  Pending
	 *	that, a complete restart of the boot sequence works,
	 *	though it's slower.
	 */
    xsleep (15);
    return pnp_success;
}

static void
lchex (where, val)
    char *where;
    u_char val;
{
    int i;
    static char x [17] = "0123456789abcdef";

    i = (val >> 4) & 0x0f;
    where [0] = x [i];
    i = val & 0x0f;
    where [1] = x [i];
}

    /* assumes only one bootable INET interface, and it's
     * Ethernet/IEEE 802 48 bit.
     */

static pnp_acquire_arg *
get_acquire_arg (ifp)
    struct ifnet *ifp;
{
    static pnp_acquire_arg pa;
    static int didit = 0;
    static char hostid [9];
    struct idprom id;
#ifdef OPENPROMS
    register struct memlist *pmem;
#endif

    if (!didit) {
	pa.linkaddr.hw = ethernet;
	myetheraddr (pa.linkaddr.hw_addr_u.enetaddr);

	pa.inetaddr = ntohl (my_in_addr.s_addr);
	/* pa.inetaddr = ntohl (ifp->if_addr.s_addr); */

	    /* architecture codes are 5 chars or less; they are 
	     * not always the same as the architecture.  Cases
	     * in mind:  68030 kernel won't run on a 68020, but
	     * is the same from user mode; 'sun386' is too many
	     * characters.  (TFTP boot filenames must be 14 chars
	     * or less, hence the 5 char limit.)
	     */
#if	sun2
	pa.arch = "sun2";
#endif	sun2
#if	sun3
	pa.arch = "sun3";
#endif	sun3
#if     sun3x
        pa.arch = "sun3x";
#endif  sun3x
#if	sun4
	pa.arch = "sun4";
#endif	sun4
#if	sun4c
	pa.arch = "sun4c";
#endif	sun4c
#if	sun386
	pa.arch = "s386";
#endif	sun386

	pa.how = b_diskless;
#ifdef	OPENPROMS
	/*
	 * Add up how much physical memory the prom has passed us.
	 */
	pa.memsize = 0;
	for (pmem = *romp->v_availmemory; pmem; pmem = pmem->next)
		pa.memsize += pmem->size;
#else
	pa.memsize = *romp->v_memorysize;
#endif
	pa.memsize /= 1024;
	pa.disksize = 0;
	pa.hostid = hostid;

	if (idprom (IDFORM_1, &id) == IDFORM_1) {
	    lchex (hostid + 0, id.id_machine);
	    lchex (hostid + 2, id.id_serial >> 16);
	    lchex (hostid + 4, id.id_serial >> 8);
	    lchex (hostid + 6, id.id_serial);
	} else
	    hostid [0] = '\0';

	didit++;
    }

    return &pa;
}

    /* We must acquire a server willing to give us a configuration
     * in the cases of new configurations and reconfigurations.
     * This times out ... caller must retry.
     * TIME:  2 minutes.
     */
static int
pnp_acquire (ifp, svrp, errp)
    register struct ifnet *ifp;
    struct sockaddr_in *svrp;
    pnp_errcode *errp;
{
    enum clnt_stat stat;
    struct timeval tv;
    pnp_acquire_arg *pap = get_acquire_arg ();
    pnp_errcode err;
    int i;

    PMeter (CHKPT_ACQUIRE);

    tv.tv_sec = 20;
    tv.tv_usec = 0;

	/* this version of the protocol requires that NO responses
	 * be generated for diskless ACQUIREs unless they're
	 * successful.
	 */
    for (i = 0; i < 6; i++) {
	stat = pmap_rmtcall (&(ifp->if_broadaddr), (u_long)PNPD_PROG,
		(u_long)PNPD_VERS, PNP_ACQUIRE,
		xdr_pnp_acquire_arg, (char *) pap,
		xdr_pnp_errcode, (char *) errp,
		tv, svrp);

	if (!stat) {
	    twiddler (1);
	    return 0;
	} else
	    twiddler (0);
    }

    *errp = pnp_failure;
    return -1;
}

    /* Issue the setup request to this system.  We've
     * already got the port number.
     * TIME:  2 minutes.
     */
static pnp_errcode
pnp_setup (clnt, errp)
    register CLIENT *clnt;
    pnp_errcode *errp;
{
    int i;
    enum clnt_stat stat;
    pnp_setup_arg ps;
    struct timeval tv;
    struct sockaddr_in sp;

    PMeter (CHKPT_SETUP);

    ps.pa = *get_acquire_arg ();
    ps.name = "*";			/* request name allocation */
    ps.keydata = "";			/* pointless on diskless systems */

    tv.tv_sec = 20;
    tv.tv_usec = 0;

    for (i = 0; i < 6; i++) {
	stat = clntkudp_callit_addr (clnt, PNP_SETUP,
		xdr_pnp_setup_arg, (char *) &ps,
		xdr_pnp_errcode, (char *) errp,
		tv, &sp);

	if (!stat) {
	    twiddler (0);
	    return 0;
	} else
	    twiddler (1);
    }

    *errp = pnp_failure;
    return -1;
}

    /* after a successful setup request, poll the server until
     * timeout, failure, or success.
     */
static pnp_errcode
pnp_poll (clnt, errp)
    register CLIENT *clnt;
    pnp_errcode *errp;
{
    int i;
    register enum clnt_stat stat;
    pnp_setup_arg ps;
    struct timeval tv;
    struct sockaddr_in sp;
    int missedpolls;

    PMeter (CHKPT_POLL);

    ps.pa = *get_acquire_arg ();
    ps.name = "*";			/* request name allocation */
    ps.keydata = "";			/* pointless on diskless systems */

    tv.tv_sec = PNP_POLLTIME;
    tv.tv_usec = 0;

    missedpolls = 0;

    *errp = pnp_success;

    do {
	    /* XXX the timeout here isn't 20 seconds, it's
	     * more like 1:25 !!
	     */
	stat = clntkudp_callit_addr (clnt, PNP_POLL,
		xdr_pnp_setup_arg, (char *) &ps,
		xdr_pnp_errcode, (char *) errp,
		tv, &sp);

#ifdef	sun386
	if (stat || *errp != pnp_in_progress) {
	    if (chkpt_now < CHKPT_PNPDONE - CHKPT_POLL_INCR)
		PMeter (chkpt_now += CHKPT_POLL_INCR);
	    else
		printf("Installation is taking more time than expected ...\n");
	}
#else
	if (stat || *errp != pnp_in_progress)
	    (void) putchar ('.');
#endif	sun386

	if (stat) {
	    missedpolls++;
	    *errp = pnp_in_progress;
	} else {
	    missedpolls = 0;
	    if (*errp == pnp_in_progress)
		xsleep (PNP_POLLTIME);
	}

    } while (missedpolls < PNP_MISSEDPOLLS
	    && *errp == pnp_in_progress);

    if (missedpolls < PNP_MISSEDPOLLS)
	return 0;

    *errp = pnp_failure;
    return -1;
}

    /* perform all the pnp setup procedures; the caller should
     * reboot if this returns zero, else exit to monitor or halt.
     */
static int
do_pnp ()
{
    pnp_errcode		sts;
    how_to_boot		how;
    struct sockaddr_in	server;
    char		*msg;
    struct ifnet	*ifp;
    register CLIENT	*clnt = (CLIENT *) 0;

#ifdef	sun386

#ifndef	REDRAW
#define	REDRAW	0		/* not 54 */
#define	DOIT	54
#endif	REDRAW

    (*romp->v_init_pmeter)("Install", DOIT); /**/
    printf ("\033[H\033[J");
#else
    (void) putchar ('\n');
    (void) printf ("Trying to start Automatic System Installation. . .\n");
    (void) putchar ('\n');
#endif

    PMeter (CHKPT_START);

    if (init_inet (&ifp) < 0)
	return -1;

    PMeter (CHKPT_VERIFY);
    if (!is_pnp_net (ifp)) {
	printf ("%s\n\n%s\n\n", MSG_NONPNPNET, MSG_SEEADMIN);
	return -1;
    }

    goto acquire_server;

restart_pnp:

    if (clnt) {
	CLNT_DESTROY (clnt);
	clnt = (CLIENT *)0;
    }
	/* In the case of some packet-lost errors, it's
	 * possible to be set up, but not know it.
	 * See if that happened.
	 */
    if ((sts = pnp_whoami (ifp)) == pnp_success)
	return 0;

	/* the top:  start the pnp sequence by finding a
	 * willing server, then requesting it set this system
	 * up, and polling until it's done.  timeouts and
	 * some errors will cause a restart of the whole
	 * sequence.
	 */
acquire_server:

    if (pnp_acquire (ifp, &server, &sts) < 0) {
startup_timeout:
	printf ("%s\n", MSG_ACQUIRETIMEOUT);
	goto restart_pnp;
    }
	/* XXX this is a protocol error for diskless systems.
	 */
    if (sts != pnp_success) {
	printf ("%s\n%s\n\n%s\n\n", MSG_PROTOERR, MSG_RETRY, MSG_SEEADMIN);
	xsleep (10);
	goto restart_pnp;
    }

got_server:
    printf ("Using installation server at ");
    {
	    char *p = (char *)&(server.sin_addr);

#define	UC(b)	(((int)b)&0xff)
	    printf("%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
    }
    clnt = clntkudp_create (&server, PNPD_PROG, PNPD_VERS, 10, u.u_cred);

    if (!clnt) {
printf ("** CLNTKUDP_CREATE ERROR\n");
	goto acquire_server;
    }
    if (pnp_setup (clnt, &sts) < 0)
	goto startup_timeout;

    switch (sts) {
	case pnp_success:		/* rare at best */
	    goto pnpdone;
	case pnp_in_progress:
	    break;
	case pnp_busy_retry:
	    printf ("%s\n", MSG_BUSYRETRY);
	    xsleep (PNP_POLLTIME);
	    goto restart_pnp;
	case pnp_wait:
	    printf ("%s\n\n", MSG_WAIT);
	    for (;;)
		continue;
	    /* NOTREACHED */
	case pnp_no_software:
	    msg = MSG_NOSOFTWARE;
	    goto explain;
	case pnp_no_diskspace:
	    msg = MSG_NOSPACE;
	    goto explain;
	case pnp_nomore_clients:
	    msg = MSG_NOCLIENTS;
	    goto explain;
	case pnp_failure:
	default:
	    msg = MSG_SETUPFAIL;
    explain:
	    printf ("%s\n", msg);
	    CLNT_DESTROY (clnt);
	    clnt = (CLIENT *)0;
	    goto acquire_server;
    }

    printf ("Waiting for Installation to complete.\n");

	/* if poll times out, there's the possibility that the
	 * server crashed after complete setup but before 
	 * sending out completion status
	 */
    if (pnp_poll (clnt, &sts) < 0) {
	printf ("%s\n", MSG_POLLTIMEOUT);
	goto restart_pnp;
    }

    switch (sts) {
	case pnp_success:
pnpdone:
	    printf ("Installation is complete.  Rebooting SunOS.\n");
	    CLNT_DESTROY (clnt);
	    xsleep (5);
	    return 0;
	case pnp_unknown_client:
	    msg = MSG_WHOAREYOU;
	    break;
	case pnp_no_software:
	    msg = MSG_NOSOFTWARE;
	    break;
	case pnp_no_diskspace:
	    msg = MSG_NOSPACE;
	    break;
	case pnp_nomore_clients:
	    msg = MSG_NOCLIENTS;
	    break;
	case pnp_failure:
	default:
	    printf ("%d\n", sts);
	    msg = MSG_POLLERR;
	    break;
    }
    printf ("%s\n%s\n\n%s\n\n", MSG_SETUPERR, msg, MSG_RETRY);
    goto restart_pnp;
}

main()
{
    register struct bootparam *bp;
    int sts;
#ifdef sun2
    int	*mem_p;
#endif sun2

    if (romp->v_bootparam == 0)
	bp = (struct bootparam *)0x2000;	/* Old Sun-1 address */
     else
	bp = *(romp->v_bootparam);		/* S-2: via romvec */
    boot_bp = bp;

#ifdef sun2
    mem_p = (int *)&(bp->bp_strings[96]);
    *mem_p = 0x220000;
    bp->bp_strings[95] = '\0';
    memory_avail = 0x220000;
#endif sun2

#if defined(sun3) || defined(sun3x)
    /*
     * Allocate memory starting below the top of physical
     * memory, to avoid tromping on kadb.
     */
    memory_avail = *romp->v_memoryavail - 0x100000;
#endif sun3 || sun3x

#ifdef sun4
    memory_avail = *romp->v_memoryavail - 0x200000;
#endif sun4
#ifdef OPENPROMS
	/*
	 * We'll avoid dealing with holes in the physical memory by
	 * only using the first chunk (which is guaranteed to be mapped
	 * by the monitor). This should be enough.
	 */
	memory_avail = (*(romp->v_availmemory))->address + 
			(*(romp->v_availmemory))->size - 0x100000;
#endif OPENPROMS

#ifdef	DRARP_REQUEST
    use_drarp = 1;
#endif	DRARP_REQUEST

    kmem_init();
    bhinit();
    binit();
    init_syscalls();

#ifdef RPCDEBUG
    rpcdebug = 4;
#endif

    sts = do_pnp ();
    (void) putchar ('\n');

    if (sts)
	(*romp->v_exit_to_mon)();		/* or halt */
    else
	(*romp->v_boot_me)("");
}
