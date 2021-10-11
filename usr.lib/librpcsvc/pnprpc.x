/*
 *	"@(#)pnprpc.x 1.1 92/07/30		(3/18/88)
 *	Copyright 1987, 1988, 1990 Sun Microsystems Inc.
 *
 * "Plug'n'Play" RPC protocol, for routine booting and
 * unattended installation of new systems into a centrally
 * administered network.
 */

const	PNP_POLLTIME		= 20;		/* seconds between POLL */
const	PNP_MISSEDPOLLS		= 6;		/* max #polls ok to lose */

const	MAX_ARCHLEN 		= 5;		/* 14 char tftp filename */
const	MAX_MACHINELEN		= 256;		/* MAX_MACHINE_NAME+1 */
const	MAX_DOMAINLEN		= 256;		/* from getdomainname(2) */
const	MAX_ENVSTRING		= 128;		/* MS-DOS limit */

    /* STATUS codes.
     *	All the RPCs return one of these status codes; they are
     *	protocol-significant.
     */

enum pnp_errcode {
    pnp_success = 0,			/* request succeeded */
    pnp_failure,			/* request failed */

	/* PNP_WHOAMI unique status
	 */
    pnp_wrong_address,			/* system has unexpected address */

	/* why does PNP_ACQUIRE or PNP_SETUP fail?
	 * applicable only to boot clients.
	 */
    pnp_no_software,			/* boot client or disk install failure */
    pnp_no_diskspace,			/* no space for diskless root/swap */
    pnp_nomore_clients,			/* boot server capacity reached */

	/* codes for b_tellme boot type (PNP_ACQUIRE only)
	 */
    pnp_do_bootserver,			/* configure as boot server */
    pnp_do_askuser,			/* ask user how to configure */
    pnp_do_bootclient,			/* become (diskful) boot client */
    pnp_do_localboot,			/* local boot (& /usr) */

	/* status for PNP_SETUP/PNP_POLL
	 */
    pnp_busy_retry,			/* (SETUP only) retry request later */
    pnp_in_progress,			/* request started */
    pnp_wait,				/* pending administrative action */
    pnp_unknown_client,			/* (POLL only) who are you? */
    pnp_database_error			/* couldn't update name service */
};


    /* PNP_WHOAMI
     *
     *	Called during the booting of network client systems.
     *	This returns information sufficient to bring the system
     *	up on the network.  If it fails (pnp_failure or
     *	pnp_wrong_address), a PNP_SETUP request should be issued
     *	to configure the system on the network.
     *
     * NOTE:  Use the RFC 868 time service to get the time, and
     *	the ICMP address format request to get the subnetwork mask.
     */

enum net_type {
    ethernet, ieee802_16, ptp, atalk
};

union hw_addr switch (net_type hw) {
    case ethernet:				/* 48-bit IEEE 802 */
	opaque	enetaddr [6];
    case ieee802_16:				/* 16-bit IEEE 802 */
	opaque	lanaddr [2];
    case ptp:					/* point-to-point link */
	void;
    case atalk:					/* Appletalk, NBP identifier */
	string	nbpid <99>;			/* name32:tag32@zone32 + 1 */
};

struct pnp_whoami_arg {
    hw_addr		linkaddr;		/* network (LAN/WAN) address */
    long		inetaddr;		/* internetwork address */
};

typedef string env_string <MAX_ENVSTRING>;	/* UNIX/DOS environment var. */

struct pnp_whoami_ret {
    pnp_errcode		status;
    string		name <MAX_MACHINELEN>;		/* hostname in NIS */
    string		domain <MAX_DOMAINLEN>;		/* NIS domain name */
    env_string		extension <>;

	/* Currently defined extension strings are:
	 *
	 *   TZ=<X/OPEN timezone specifier ... XXXnYYY>
	 *   TIMEZONE=<SunOS timezone specifier>
	 *   ROUTER=<IP forwarding address, dot notation>
	 *   DOMAIN=<Internet domain name>
	 *   ROLE=<standard role type>
	 *
	 * DOMAIN presumed to be identical to "domain" unless
	 * specified in the extensions; else if "domain" begins
	 * with a '+' or '.' then this character is removed to
	 * generate DOMAIN.
	 *
	 * ROLE is one of "unknown", "master_bootserver", "slave_bootserver",
	 * "network_client", "diskful_client", "diskless_client".  Boot
	 * servers always provide PNP service (though they may refuse
	 * to configure new systems) and a network name service, normally
	 * the Network Information Services.
	 */
};


    /* PNP_ACQUIRE
     *
     *	When PNP_WHOAMI fails with pnp_failure, systems that
     *	require more boot resources than reported by PNP_WHOAMI
     *	need to issue a PNP_ACQUIRE request.  This includes
     *	three basic classes of system:
     *
     *	- Diskless.  This includes floppy-only systems.
     *	- Diskful Clients.  Mostly applicable to UNIX.
     *	- Diskful systems wishing to be told by the network
     *	  how to configure.
     *
     *	The latter category is useful under UNIX, where there are
     *	a rather confusing number of ways to use a networked disk;
     *	it allows (e.g.) maximization of free disk space or
     *	performance.
     */

enum how_to_boot {
    b_diskless,				/* remote root, swap, usr */
    b_diskful,				/* local root, swap, usr */
    b_diskful_client,			/* local root, swap; remote usr */
    b_boot_server,			/* b_diskful, + name (NIS) server */
    b_gateway,				/* more than 1 net interface */
    b_tellme,				/* how should I boot? */
    b_install_disk			/* get MUNIX + disk installation */ 
};

struct pnp_acquire_arg {
    hw_addr		linkaddr;		/* typically ethernet */
    long		inetaddr;		/* via RARP */
    string		arch <MAX_ARCHLEN>;	/* sun[234], s386, pcnfs, ... */
    how_to_boot		how;			/* server resources needed? */
    long		memsize;		/* memory in Kbytes */
    long		disksize;		/* total disk space, Kbytes */
    string		hostid <>;		/* architecture dependent */
};
    

    /* PNP_SETUP, PNP_POLL
     *
     *	This is the RPC that causes the system to be entered into
     *	the administrative databases of the domain, and perhaps to
     *	be assigned some boot resources such as a diskless root,
     *	a swap partition, a /usr partition, or a C: drive.
     *
     *	The SETUP RPC is issued first; it typically returns
     *	pnp_in_progress, in which case the client should issue
     *	PNP_POLL every PNP_POLLTIME seconds until some other
     *	status is returned.  If the server and client can't
     *	handshake for PNP_MISSEDPOLLS, the assumption is that the
     *	other party crashed; the client would recover by restarting
     *	the PNP sequence, and the server would back completely
     *	out of the client setup.
     */

struct pnp_setup_arg {
    pnp_acquire_arg	pa;			/* machine info from above */
    string		name <MAX_MACHINELEN>;	/* selected hostname, or "*" */
    string		keydata <>;		/* publickey encryption data */
};

#ifdef	RPC_HDR
%#define	ps_linkaddr	pa.linkaddr
%#define	ps_inetaddr	pa.inetaddr
%#define	ps_arch		pa.arch
%#define	ps_how		pa.how
%#define	ps_memsize	pa.memsize
%#define	ps_disksize	pa.disksize
%#define	ps_hostid	pa.hostid
#endif	RPC_HDR

    /* PNPD program */
program PNPD_PROG {
    version PNPD_VERS {
	    /* void NULLPROC (void) = 0; */
	pnp_whoami_ret PNP_WHOAMI (pnp_whoami_arg) = 1;
	pnp_errcode PNP_ACQUIRE (pnp_acquire_arg) = 2;
	pnp_errcode PNP_SETUP (pnp_setup_arg) = 3;
	pnp_errcode PNP_POLL (pnp_setup_arg) = 4;
    } = 2;
} = 100041;
