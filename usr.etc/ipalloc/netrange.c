#ifndef lint
static char sccsid[] = "@(#)netrange.c 1.1 92/07/30 Copyright 1987 Sun Microsystems Inc.";
#endif

#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>

#include	"netrange.h"


extern u_long	inet_addr ();

extern int	debug;

char *netrange_file = "/etc/ipalloc.netrange";

static hostrange retval [MAX_RANGE + 1];

hostrange *
get_hostrange (ipnetnum)
    u_long ipnetnum;
{
    FILE *f = fopen (netrange_file, "r");
    u_long netnum;
    int i;
    char buf [8192], *cp;

	/* FILE FORMAT:
	 *   <Netnum><whitespace><Range>[,<Range>]*
	 * where
	 *   <Netnum> ... decimal dot notation, includes subnet #
	 *   <Range> ... <number> or <number>-<number>
	 * and malformed ranges are ignored.
	 *
	 * XXX this will turn to EZDB ops someday
	 */
    if (debug) {
	netnum = htonl(ipnetnum);
	printf ("get_hostrange %s\n", inet_ntoa (netnum));
    }
    if (!f)
	return (hostrange *) NULL;

    bzero ((char *) retval, sizeof (retval));
    while (!feof (f)) {
	if (!fgets (buf, sizeof (buf), f))
	    continue;
	if ((cp = strchr (buf, '#')) != (char *) NULL)
	    *cp = 0;

	if ((cp = strtok (buf, " \n\t")) == (char *) NULL)
	    continue;
	netnum = ntohl (inet_addr (cp));
if (debug) printf ("?? 0x%08x matches arg 0x%08x ??\n", netnum, ipnetnum);
	if (netnum != ipnetnum)
	    continue;
	
	for (i = 0; i < MAX_RANGE; i++) {
	    cp = strtok ((char *) NULL, ",");
/* XXX allow backslash-newline continuation */
	    if (!cp)
		goto done;
	    switch (sscanf (cp, "%ld-%ld",
		    &retval [i].h_start, &retval [i].h_end)) {
		case 1:
		    retval[i].h_end = retval [i].h_start;
			/* FALLTHROUGH */
		case 2:
if (debug) printf ("%d-%d\n", retval[i].h_start, retval[i].h_end);
		    continue;
		default:
		    i--;
		    continue;
	    }
	}
    }

done:
    (void) fclose (f);
    return retval;
}
