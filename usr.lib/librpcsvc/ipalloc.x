/*
 *	"@(#)ipalloc.x 1.1 92/07/30		(10/13/87)
 *	Copyright 1990 by Sun Microsystems Inc.
 *
 * IP address allocation protocol, used to construct and maintain
 * a cache of temporary IP addresses and their associated link
 * level addresses.  (First implementation only, these link
 * addresses are restricted to 48 bit IEEE 802 addresses.)
 *
 * These RPCs may always be invoked on the NIS master of the
 * 'hosts.byaddr' map in a particular domain.
 */

const	IPALLOC_TIMEOUT	= 86400;	/* seconds cache timeout (1 day) */
const	MAX_MACHINELEN	= 256;		/* max size of system name */

    /* STATUS
     *
     *	Shared among all IP_* routines, and their calling "glue".
     */

enum ip_status {
    ip_success = 0,		/* success */
    ip_failure,			/* unspecified failure */
    ip_rpc,			/* rpc failure (returned by glue) */
    ip_no_addresses,		/* can't allocate an address */
    ip_no_system,		/* request for unknown IP addr */
    ip_no_priv			/* no priv to alloc/free address */
};

    /* IP_ALLOC
     *
     *	Used to allocate a temporary IP address on a specific
     *	network and subnetwork, corresponding to a specific
     *	48 bit IEEE 802 address.  This address is entered into
     *	a local cache, which might be restricted to one IP
     *	address per IEEE802 address.
     *
     *	If the name services available to the daemon report an
     *	address meeting the criteria, that is always returned
     *	in preference to allocating an address.
     *
     *	NOTE:  uses DES authentication, or otherwise must be called
     *	from a reserved port on the local machine.
     */
struct ip_alloc_arg {
    opaque	etheraddr [6];			/* ethernet & FDDI */
    u_long	netnum;				/* net in host order */
    u_long	subnetmask;			/* netmask in host order */
};

union ip_alloc_res switch (ip_status status) {
    case ip_success:
	u_long		ipaddr;			/* host order */
};

    /* IP_TONAME
     *
     *	This settles the issue of whether a given IP address is
     *	actually known to the name service or not.  Some name
     *	service protocols (e.g., NIS) allow a substantial delay
     *	between posting a {name, address} entry and the time
     *	that entry can be retrieved.  If the entry is stored in
     *	the name service available to the daemon, it returns it.
     */

struct ip_addr_arg {
    u_long		ipaddr;			/* host order */
};

union ip_toname_res switch (ip_status status) {
    case ip_success:
	string		name <MAX_MACHINELEN>;	/* system's name */
};

    /* IP_FREE
     *
     *	So that the cache may be uncluttered by means other than
     *	the cache timeout (after IPALLOC_TIMEOUT seconds), this
     *	routine exists.  This expects no guarantees that the entry
     *	was permanently registered with a name service, just deletes
     *	the entry from the cache.  It's a good idea to use this
     *	after registering a {etheraddr, name, address} mapping
     *	with the name server, in case another address needs to
     *	be assigned to correspond to that etheraddr.
     *
     *	NOTE:  uses DES authentication, or otherwise must be called
     *	from a secure port on the local machine.
     */

    /* the PROGRAM */
program IPALLOC_PROG {
    version IPALLOC_VERS {
	ip_alloc_res	IP_ALLOC (ip_alloc_arg) = 1;
	ip_toname_res	IP_TONAME (ip_addr_arg) = 2;
	ip_status	IP_FREE (ip_addr_arg) = 3;
    } = 2;
} = 100042;
