#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)inet_netof.c 1.1 92/07/30 SMI"; /* from UCB 4.3 82/11/14 */
#endif

#include <sys/types.h>
#include <netinet/in.h>

/*
 * Return the network number from an internet
 * address; handles class a/b/c network #'s.
 */
inet_netof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return (((i)&IN_CLASSA_NET) >> IN_CLASSA_NSHIFT);
	else if (IN_CLASSB(i))
		return (((i)&IN_CLASSB_NET) >> IN_CLASSB_NSHIFT);
	else
		return (((i)&IN_CLASSC_NET) >> IN_CLASSC_NSHIFT);
}
