#ifndef lint
static        char sccsid[] = "@(#)uipc_domain.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


#include <sys/param.h>
#include <sys/socket.h>
#include "boot/protosw.h"
#include "boot/domain.h"
#include <sys/time.h>
#include <sys/kernel.h>

int	domains_inited = 0;

#define	ADDDOMAIN(x)	{ \
	extern struct domain x/**/domain; \
	x/**/domain.dom_next = domains; \
	domains = &x/**/domain; \
}

domaininit()
{
	register struct domain *dp;
	register struct protosw *pr;

	if (domains_inited != 0)
		return;
	else
		domains_inited = 1;

#ifndef lint
#ifdef	NEVER
	ADDDOMAIN(unix);
#endif	/* NEVER */
#ifdef INET
	ADDDOMAIN(inet);
#endif
#ifdef PUP
	ADDDOMAIN(pup);
#endif
#include "imp.h"
#if NIMP > 0
	ADDDOMAIN(imp);
#endif
#ifdef NIT
	ADDDOMAIN(nit);
#endif
#endif

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_init)
				(*pr->pr_init)();
	pffasttimo();
	pfslowtimo();
}

struct protosw *
pffindtype(family, type)
	int family, type;
{
	register struct domain *dp;
	register struct protosw *pr;


	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_type && pr->pr_type == type)
			return (pr);
	return (0);
}

struct protosw *
pffindproto(family, protocol)
	int family, protocol;
{
	register struct domain *dp;
	register struct protosw *pr;

	if (family == 0)
		return (0);
	for (dp = domains; dp; dp = dp->dom_next)
		if (dp->dom_family == family)
			goto found;
	return (0);
found:
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_protocol == protocol)
			return (pr);
	return (0);
}

#ifdef	NEVER
pfctlinput(cmd, arg)
	int cmd;
	caddr_t arg;
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, arg);
}
#endif	/* NEVER */

pfslowtimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_slowtimo)
				(*pr->pr_slowtimo)();
#ifdef	NEVER
	timeout(pfslowtimo, (caddr_t)0, hz/2);
#endif	/* NEVER */
}

pffasttimo()
{
	register struct domain *dp;
	register struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_fasttimo)
				(*pr->pr_fasttimo)();
#ifdef	NEVER
	timeout(pffasttimo, (caddr_t)0, hz/5);
#endif	/* NEVER */
}
