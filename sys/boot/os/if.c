#ifndef lint
static        char sccsid[] = "@(#)if.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

#include <sys/param.h>
#include "boot/systm.h"
#include <sys/socket.h>
#include <sys/socketvar.h>
#include "boot/protosw.h"
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/af.h>
#include <net/route.h>
#include <net/raw_cb.h>
#include <net/nit.h>

#include "../netinet/in.h"

int     ifqmaxlen = IFQ_MAXLEN;


/*
 * Network interface utility routines.
 *
 * Routines with if_ifwith* names take sockaddr *'s as
 * parameters.  Other routines take value parameters,
 * e.g. if_ifonnetof takes the network number.
 */

ifinit()
{
        register struct ifnet *ifp;

        for (ifp = ifnet; ifp; ifp = ifp->if_next)

                if (ifp->if_init) {

                        (*ifp->if_init)(ifp->if_unit);
                        if (ifp->if_snd.ifq_maxlen == 0)
                                ifp->if_snd.ifq_maxlen = ifqmaxlen;
                }
}

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
if_attach(ifp)
        struct ifnet *ifp;
{
        register struct ifnet **p = &ifnet;
 
        while (*p)
                p = &((*p)->if_next);
        *p = ifp; 
	ifp->if_next = (struct ifnet *)0;
}


/*
 * Find an interface using a specific address family
 */
struct ifnet *
if_ifwithaf(af)
        register int af;
{
        register struct ifnet *ifp;
	register struct ifaddr *ifa;

        for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr.sa_family == af)
			return (ifp);
	    }
	}
        return ((struct ifnet *)0);
}

/*
 * Find an UP interface using a specific address family
 */
struct ifnet *
if_ifwithafup(af)
        register int af;
{
        register struct ifnet *ifp;
	register struct ifaddr *ifa;
 
        for (ifp = ifnet; ifp; ifp = ifp->if_next)
	   for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                if ((ifp->if_flags & IFF_UP) && ifa->ifa_addr.sa_family == af)
                        return (ifp);
	   }
        return ((struct ifnet *)0);
}


/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifnet *
if_ifwithaddr(addr)
        struct sockaddr *addr;
{
        register struct ifnet *ifp;
	register struct ifaddr *ifa;

 
#define equal(a1, a2) \
        (bcmp((caddr_t)((a1)->sa_data), (caddr_t)((a2)->sa_data), 14) == 0)
        for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr.sa_family != addr->sa_family)
                        continue;
                if (equal(&ifa->ifa_addr, addr))
			return (ifp);
                if ((ifp->if_flags & IFF_BROADCAST) &&
                    equal(&ifa->ifa_broadaddr, addr))
			return (ifp);
	    }
        }
        return ((struct ifnet *)0);
}

/*
 * Locate an interface based on a complete destination.
 */
/*ARGSUSED*/
struct ifnet *
if_ifwithdstaddr(addr)
        struct sockaddr *addr;
{
        register struct ifnet *ifp;
        register struct ifnet *bifp = (struct ifnet *)0;
	register struct ifaddr *ifa;

        for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                if (ifa->ifa_addr.sa_family != addr->sa_family)
                        continue;
                if ((ifp->if_flags & IFF_POINTOPOINT) &&
                        equal(&ifa->ifa_dstaddr, addr))
			return (ifp);
                if (equal(&ifa->ifa_addr, addr))
			return (ifp);
                if ((ifp->if_flags & IFF_BROADCAST) &&
                    equal(&ifa->ifa_broadaddr, addr))
                        bifp = ifp;
	   }
        }
        return (ifp==0 ? bifp : ifp);
}

/*
 * Find an interface which reaches a specific destination.  If many, choice
 * is first found.
 */
struct ifnet *
if_ifwithnet(addr)
        register struct sockaddr *addr;
{
        register struct ifnet *ifp;
        register u_int af = addr->sa_family;
        register int (*netmatch)();
	register struct ifaddr *ifa;
 
        if (af >= AF_MAX)
                return (0);
        netmatch = afswitch[af].af_netmatch;
        for (ifp = ifnet; ifp; ifp = ifp->if_next) {
	    for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next) {
                if (af != ifa->ifa_addr.sa_family)
                        continue;
                if (ifp->if_flags&IFF_POINTOPOINT) {
                        if (equal(&ifa->ifa_dstaddr, addr))
				return (ifp);
                } else if ((*netmatch)(addr, &ifa->ifa_addr))
			return (ifp);
	    }
        }
        return ((struct ifnet *)0);
}     



/*
 * Set a new interface address.
 * Called by ND address assignment as well as above
 * XXX Most of what the drivers do is device independent!
 */
if_setaddr(ifp, sa)
        register struct ifnet *ifp;
        struct sockaddr *sa;
{
        if (ifp->if_ioctl == 0)
                return (EOPNOTSUPP);
        return ((*ifp->if_ioctl)(ifp, SIOCSIFADDR, (caddr_t)sa));
}

