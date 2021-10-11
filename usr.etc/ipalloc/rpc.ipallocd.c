#ifndef lint
static char sccsid[] = "@(#)rpc.ipallocd.c 1.1 92/07/30 Copyright 1990 Sun Microsystems";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* rpc.ipalloc.c
 * 
 * An implementation of the remote procedure calls of the ipalloc
 * service.
 *
 * XXX	Very strong need to speed this up for large networks.
 * 	There's a requirement to deliver the address to the end
 *	user in under 2 minutes for PNP of most diskful systems.
 *	While the documented procedures say do PNP one system at 
 *	a time, that might not always occur.  Also, not all networks
 *	will be Class C or subnetted.
 */


#include <stdio.h>
#include <netdb.h>
#include <des_crypt.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <rpc/rpc.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <arpa/inet.h>

#include "ipalloc.h"
#include "netrange.h"


#define CACHEFILE	"/etc/ipalloc.cache"
#define MAXHOSTNAME	256


extern int	optind, getopt ();
extern char 	*optarg;
extern char	*ether_ntoa (), *inet_ntoa ();


struct cache_ent {
    struct cache_ent	*next;
    struct timeval	tv;
    struct ether_addr	ea;
    struct in_addr	ip;
};


static struct cache_ent *cache_head;		/* address queue */
int			debug;			/* print out debugging info */
static int		insecure;		/* allow non-DES auth */
static int		cache_flag;		/* keep the cache on disk */

void            read_cache();			/* forward */

static int
am_yp_master (map)
    char *map;
{
    char *dom, *master;
    char me [MAXHOSTNAME];
    int status;

    (void) yp_get_default_domain (&dom);
    yp_master (dom, map, &master);
    (void) gethostname (me, sizeof (me));
    status = !strcmp (me, master);
    (void) free (dom);
    (void) free (master);
    return status;
}


#define	NGID	10		/* extra */
#define	SYSPRIV	12		/* group # for network privilege */

int
has_sysprivs (netname)
    char *netname;
{
    int gid, uid, gids [NGID], ngid = NGID, i, vallen;
    char sysname [256], *domain, *val;

	/* NIS servers can do it
	 */
    if (netname2host (netname, sysname, sizeof (sysname))
            && !yp_get_default_domain (&domain)
            && !yp_match (domain, "ypservers",
                        sysname, strlen (sysname), &val, &vallen)) {
        (void) free (val);
	return 1;
    }
	/* and any user with the 'networks' privilege
	 * can do it as well.
	 */
    if (netname2user (netname, &uid, &gid, &ngid, gids)) {
	if (gid == SYSPRIV)
	    return 1;
	else for (i = 0; i < ngid; i++)
	    if (gids [i] == SYSPRIV)
		return 1;
    }
    return 0;
}

static int
is_auth_caller (flavor, auth)
    struct authdes_cred *auth;
{
    register char *name;

    if (insecure)
	return 1;
    else if (flavor != AUTH_DES)
	return 0;
    else
	return has_sysprivs (auth->adc_fullname.name);
}


static
have_encryption()
{
    static int status = -1;
    char key[8];
    char data[8];

    if (status < 0) {
	bzero(key, sizeof(key));
	status = ecb_crypt(key, data, sizeof(data), DES_ENCRYPT|DES_SW);
    }
    return (status == 0);
}

/*
 * main:
 */

main(argc, argv)
    int             argc;
    char          **argv;
{
    int c;

    insecure = !have_encryption ();
    debug = 0;

    /*cache_flag = 1;			/**/

    while ((c = getopt (argc, argv, "dcis")) != EOF)
	switch (c) {
	    case 'c':
		cache_flag = 0;
		continue;
	    case 'd':
		debug++;
		continue;
	    case 'i':
		insecure = 1;
		continue;
	    case 's':
		insecure = 0;
		continue;
	    case '?':
	    default:
usage:
		(void) fprintf (stderr, "usage:  %s [-cis]\n", argv[0]);
		exit (1);
	}

    if (debug)
	(void) printf ("%s:  in debug mode\n", argv[0]);
    if (argc != optind)			/* non-option arguments */
	goto usage;

    if (!am_yp_master ("hosts.byaddr")) {
	if (debug)
	    printf ("%s:  not running on NIS master, exit\n", argv[0]);
	exit (1);
    } else if (getuid ()) {
	if (debug)
	    printf ("%s:  not running as root\n", argv [0]);
#ifdef NODEBUG
	exit (1);
#endif NODEBUG
    }
    
    if (!debug) {
	switch (fork ()) {
	    case 0:	/* kid */
		/* XXX need TIOCNOTTY, maybe set up fd 0 as client
		 */
		break;
	    case -1:
		exit (-1);
		    /* NOTREACHED */
	    default:	/* parent */
		exit (0);
		    /* NOTREACHED */
	}
    }

/*
 * XXX make this start up through inetd
 * XXX quit if there are no cache entries and
 *	if it's not been referenced in a while.
 */

    read_cache();

    /* start the server-side stub.  This never returns */
    serverstub_main();
}

    /* Time out entries in the cache.
     * Assumes that the cache timeout is long, so minor errors in the
     * calculation are OK.
     */

void
cache_timeout ()
{
    register struct cache_ent	*cp, **prevp;
    struct timeval	tv;

    (void) gettimeofday(&tv, (struct timezone *)NULL);

    for (cp = cache_head, prevp = &cache_head;
	    cp != (struct cache_ent *) NULL;
	    cp = cp->next) {
	if (tv.tv_sec > cp->tv.tv_sec + IPALLOC_TIMEOUT) {
	    *prevp = cp->next;
	    free(cp);
		/* cp still valid, e.g. for realloc ()
		 * or in particular cp = cp->next
		 */
	} else
	    prevp = &cp->next;
    }
}

    /* return a cache entry with this ethernet address
     */

static struct cache_ent *
cache_lookup_ether (ep)
    struct ether_addr *ep;
{
    register struct cache_ent	*cp;

    for (cp = cache_head; cp; cp = cp->next)
	if (!bcmp ((char *) ep, (char *) &cp->ea, sizeof (cp->ea))) {
	    (void) gettimeofday (&cp->tv, (struct timezone *)NULL);
	    return cp;
	}
    return (struct cache_ent *) NULL;
}

    /* allocate a new cache entry for this etheraddr on
     * this network and subnet.
     */

static struct cache_ent *
cache_add (ep, netnum, netmask)
    struct ether_addr *ep;
    u_long netnum, netmask;
{
    register struct cache_ent	*cp;
    struct in_addr		ip;

    if (!getfreeipaddr (netnum, netmask, &ip))
	return (struct cache_ent *) NULL;
    if (!(cp = (struct cache_ent *) malloc (sizeof (*cp))))
	return cp;
    (void) gettimeofday (&cp->tv, (struct timezone *)NULL);
    cp->ea = *ep;
    cp->ip = ip;
    cp->next = cache_head;
    cache_head = cp;
    return cp;
}

/*
 * read_cache:
 * 
 * Reads the allocated IP cache from a file into memory.
 * Should be called once when program is started.
 */

void
read_cache()
{
    struct cache_ent	*cp;
    struct ether_addr	ea;
    u_char		*ip, i0, i1, i2, i3;
    int             n;
    struct tm      *tp;
    FILE		*fd;

    cache_head = (struct cache_ent *) 0;

    if (!cache_flag)
	return;

    if (!(fd = fopen (CACHEFILE, "r+")))
	return;

	/* XXX we need a much better way to store this info;
	 * binary mmap() file?  This will eliminate scan errors
	 * as well...
	 */
    for (i0 = 0, tp = localtime(&i0), n = 0;
	    fscanf(fd,
		"%d/%d/%d %d:%d:%d %x:%x:%x:%x:%x:%x %d.%d.%d.%d",
		 &tp->tm_mon, &tp->tm_mday, &tp->tm_year,
		 &tp->tm_hour, &tp->tm_min, &tp->tm_sec,
#define	eo	ea.ether_addr_octet
		 &eo [0], &eo [1], &eo [2],
		 &eo [3], &eo [4], &eo [5],
#undef	eo
		 &i0, &i1, &i2, &i3) == 16;
	    cp->next = cache_head, cache_head = cp, n++) {
	cp = (struct cache_ent *) malloc(sizeof(struct cache_ent));
	if (cp == (struct cache_ent *) 0) {
	    if (debug)
		(void) printf("ipalloc: init_cache: malloc failure.\n");
/* XXX nasty error handling */
	    exit(1);
	}
	cp->ea = ea;
	ip = (u_char *) & cp->ip.s_addr;

	ip[0] = (u_char) i0;
	ip[1] = (u_char) i1;
	ip[2] = (u_char) i2;
	ip[3] = (u_char) i3;
	cp->tv.tv_sec = timelocal (tp);
	cp->tv.tv_usec = 0;
    }
    (void) fclose (fd);
    if (debug)
	(void) printf("init_cache: read %d cache entries.\n", n);
    cache_timeout ();
}



/*
 * write_cache:
 * 
 * Writes the in-memory allocated IP address cache to a file.
 */

void
write_cache()
{
    u_char		*ea, *ip;
    struct cache_ent	*cp;
    int			n;
    struct tm		*tp;
    FILE		*fd;

    if (!cache_flag)
	return;

    if (!(fd = fopen (CACHEFILE, "w"))) {
	if (debug)
	    printf ("Couldn't open %s to write cache\n", CACHEFILE);
	return;
    }
    cache_timeout ();

    for (n = 0, cp = cache_head; cp; cp = cp->next) {
	ea = (u_char *) & cp->ea;
	ip = (u_char *) & cp->ip.s_addr;
	tp = localtime(&cp->tv.tv_sec);
	if (fprintf(fd, "%d/%d/%.2d %2d:%.2d:%.2d %x:%x:%x:%x:%x:%x %d.%d.%d.%d\n",
		    tp->tm_mon, tp->tm_mday, tp->tm_year,
		    tp->tm_hour, tp->tm_min, tp->tm_sec,
		    (u_int) ea[0], (u_int) ea[1], (u_int) ea[2],
		    (u_int) ea[3], (u_int) ea[4], (u_int) ea[5],
		    (u_int) ip[0], (u_int) ip[1], (u_int) ip[2],
		    (u_int) ip[3]) == EOF) {
	    if (debug)
		perror("ipalloc: write_cache");
/* XXX very bad to exit!! */
	    exit(1);
	}
	n++;
    }

    (void) fclose (fd);
    if (debug)
	(void) printf("write_cache: wrote %d entries.\n", n);
}

/*
 * getfreeipaddr:
 * 
 * Searches the hosts database and local cache for unassigned IP addresses
 * on the network and subnet specified.  Returns true iff successful in
 * storing an unused address in *ipp.
 */

int
getfreeipaddr (netnum, subnetmask, ipp)
    u_long          netnum;
    u_long          subnetmask;
    struct in_addr *ipp;
{
    u_long		max;
    u_long		lhost;
    struct in_addr	ip;
    struct cache_ent	*cp;
    hostrange		*hrp;

    netnum &= subnetmask;
    hrp = get_hostrange (netnum);

    if (debug) {
	u_long xnet = htonl(netnum), xmask = htonl(subnetmask);
	(void) printf ("getfreeipaddr: netnum = %s,",
	       inet_ntoa (*(struct in_addr *)&xnet));
	(void) printf ("subnetmask = %s\n",
	       inet_ntoa (*(struct in_addr *)&xmask));
    }

	/* the addresses must be in host order for these
	 * #defines to work
	 */
    if (IN_CLASSA(netnum))
	max = (IN_CLASSA_HOST - 1) & ~subnetmask;
    else if (IN_CLASSB(netnum))
	max = (IN_CLASSB_HOST - 1) & ~subnetmask;
    else if (IN_CLASSC(netnum))
	max = (IN_CLASSC_HOST - 1) & ~subnetmask;
    else {
	if (debug)
	    (void) printf ("class D or other unknown network:  %s\n",
		    inet_ntoa (*(struct in_addr *)&netnum));
	return 0;
    }

	/* Cycle through possible IP addresses.  Addresses are
	 * ruled out if on wrong subnet or unavailable.  They may
	 * be unavailable because this daemon doesn't have authority
	 * over them, because they're in the hosts database, or
	 * because they're in use.
	 * Count down through range of available addresses so that
	 * we're more likely to hit a free one soon.
	 */

    for (lhost = max ; lhost >= (hrp ? hrp->h_start : 1); lhost--) {
	if (!lhost || lhost & subnetmask)	/* nonzero? right subnet? */
	    continue;
		/* XXX expects 32 bit u_long !! */
	if (lhost == ~subnetmask)		/* reject broadcast */
	    break;
	if (hrp && hrp->h_start != 0) {		/* in legal range? */
	    if (lhost < hrp->h_start)
		continue;
	    if (lhost >= hrp->h_end)
		hrp++;
	}
	ip.s_addr = htonl (netnum | lhost);	/* construct address */

	    /* check hosts database, then cache ... maybe
	     * we have an address to return.
	     */
	if (!gethostbyaddr ((char *) &ip, sizeof(ip), AF_INET)) {
	    cache_timeout ();
	    for (cp = cache_head; cp != (struct cache_ent *) 0; cp = cp->next)
		if (cp->ip.s_addr == ip.s_addr)
		    break;
	    if (!cp) {
		if (debug)
		    (void) printf("getfreeipaddr: ip = %s.\n", inet_ntoa (ip));
		*ipp = ip;
		return 1;
	    }
	}
    }

    return 0;
}

static void
get_default_netmask (l, m)
    u_long l, *m;
{
    if (IN_CLASSA (l))
	*m = IN_CLASSA_NET;
    else if (IN_CLASSB (l))
	*m = IN_CLASSB_NET;
    else if (IN_CLASSC (l))
	*m = IN_CLASSC_NET;
    else
	*m = ~0;	/* error */
}

static int
right_net (in, arg)
    u_long in;
    struct ip_alloc_arg *arg;
{
    u_long m, a, desired;

    get_default_netmask (in = ntohl (in), &m);
    a = in & m;

    get_default_netmask (arg->netnum, &m);
    desired = arg->netnum & m;

    if (a != desired)			/* right ABC network? */
	return 0;
    
    m |= arg->subnetmask;		/* just in case */
    a = in & ~m;			/* local host # */

    if ((in & m) != (arg->netnum & m)	/* wrong subnet? */
	    || !a			/* local net #, lh = 0 */
	    || a == ~m)			/* local net broadcast, lh = ~0 */
	return 0;
    
    return 1;
}

/*
 * ipalloc (version 2):
 * 
 * Allocate a new IP address.   Caller provides the ethernet address of the
 * machine for which the IP address is being allocated, along with the
 * network number and subnet mask of the network where the new IP address is
 * to be allocated.  We first check to see if there is already an IP address
 * for this machine on this network+subnet. If there is not, we consider
 * allocating one.  We keep a cache of recently allocated IP addresses along
 * with the ethernet addresses of the machines to which they were allocated.
 * This cache is checked before a new IP address is allocated.
 */

struct ip_alloc_res *
ip_alloc_2(argument, svcreq)
    struct ip_alloc_arg *argument;
    struct svc_req *svcreq;
{
    struct ether_addr	ea;
    struct cache_ent	*cp;
    struct hostent	*hp;
    char		hostname [MAXHOSTNAME];

    static struct ip_alloc_res
			result;

    if (!is_auth_caller (svcreq->rq_cred.oa_flavor, svcreq->rq_clntcred)) {
	if (debug)
		(void) fprintf (stderr, "ip_alloc_2: invalid auth\n");
	result.status = ip_no_priv;
	return &result;
    }

    if (debug)
	fprintf (stderr, "--- ip_alloc %s\n",
		ether_ntoa (argument->etheraddr));

    bzero((char *) &result, sizeof(result));
    bcopy((char *) &argument->etheraddr[0], (char *) &ea, sizeof(ea));
    bzero((char *) &hostname[0], MAXHOSTNAME);

	/* Do ether --> hosts database lookup.
	 * Maybe we can return the results.
	 */
    if (ether_ntohost(&hostname[0], &ea) == 0) {
	if ((hp = gethostbyname(&hostname[0])) != (struct hostent *) NULL) {
	    if (hp->h_addrtype == AF_INET
		    && right_net (*(u_long *)hp->h_addr, argument)) {
		(void) bcopy((char *) hp->h_addr,
		    (char *) &result.ip_alloc_res_u.ipaddr,
		    sizeof(result.ip_alloc_res_u.ipaddr));
		result.status = ip_success;
		return &result;
	    }
	}
    }

	/* The ether --> hosts database lookup failed;
	 * return an existing cache entry or a newly
	 * created one, if possible.
	 */
    cache_timeout ();
    if ((cp = cache_lookup_ether (&ea))
	    || (cp = cache_add (&ea, argument->netnum, argument->subnetmask))) {
	result.ip_alloc_res_u.ipaddr = ntohl (cp->ip.s_addr);
	result.status = ip_success;
	if (debug)
	    printf ("Success, %s is %s\n",
		    ether_ntoa (&cp->ea), inet_ntoa (cp->ip));
	write_cache ();
	return &result;
    }

	/* Couldn't return an address, so sorry.
	 */
    result.status = ip_no_addresses;
    return &result;
}



/*
 * iptoname (version 1):
 * 
 * Map an IP address into the hostname associated with it in the hosts database.
 * Basically, just a call to gethostbyaddr.  Used by clients who don't
 * believe their "local" hosts database to check with a more authoritative
 * database.
 */

    /* ARGSUSED */
struct ip_toname_res *
ip_toname_2(argument, svcreq)
    struct ip_addr_arg *argument;
    struct svc_req *svcreq;
{
    struct hostent *hp;
    static struct ip_toname_res result;
    long the_addr = htonl (argument->ipaddr);

    if (debug)
	fprintf (stderr, "--- ip_toname %s\n", inet_ntoa (the_addr));

    hp = gethostbyaddr((char *) &the_addr, sizeof(argument->ipaddr),
		       AF_INET);

    if (hp == (struct hostent *) 0)
	result.status = ip_failure;
    else {
	result.status = ip_success;
	result.ip_toname_res_u.name = hp->h_name;
    }

    if (debug)
	fprintf (stderr, "\t--> %d\n", result.status);

    return &result;
}

ip_status *
ip_free_2 (arg, svcreq)
    register ip_addr_arg *arg;
    struct svc_req *svcreq;
{
    static ip_status s;
    register struct cache_ent	*cp, **prevp;

    if (!is_auth_caller (svcreq->rq_cred.oa_flavor, svcreq->rq_clntcred)) {
	s = ip_no_priv;
	return &s;
    }

    if (debug) {
	fprintf (stderr, "--- ip_free %s\n",
		inet_ntoa (ntohl (arg->ipaddr)));
    }
    for (cp = cache_head, prevp = &cache_head, s = ip_failure;
	    cp != (struct cache_ent *) NULL;
	    cp = cp->next) {
	if (ntohl (cp->ip.s_addr) == arg->ipaddr) {
	    *prevp = cp->next;
	    free(cp);
	    s = ip_success;
	    write_cache ();
	    break;
	} else
	    prevp = &cp->next;
    }
    if (debug)
	fprintf (stderr, "\t--> %d\n", s);
    
    return &s;
}
