#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)inet_lnaof.c 1.1 92/07/30 SMI"; /* from UCB 4.3 82/11/14 */
#endif

#include <sys/types.h>
#include <netinet/in.h>

/*
 * Return the local network address portion of an
 * internet address; handles class a/b/c network
 * number formats.
 */
inet_lnaof(in)
	struct in_addr in;
{
	register u_long i = ntohl(in.s_addr);

	if (IN_CLASSA(i))
		return ((i)&IN_CLASSA_HOST);
	else if (IN_CLASSB(i))
		return ((i)&IN_CLASSB_HOST);
	else
		return ((i)&IN_CLASSC_HOST);
}
