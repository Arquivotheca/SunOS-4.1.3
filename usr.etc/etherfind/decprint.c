/*
 * @(#)decprint.c 1.1 92/07/30 
 *
 * Module for printing DEC Ethernet packet types
 *
 * Copyright 1987 by Sun Microsystems, Inc.
 */

# include <sys/types.h>
# include <sys/socket.h>
# include <net/if.h>
# include <net/if_arp.h>
# include <netinet/in.h>
# include <netinet/if_ether.h>

extern int symflag[];		/* verbose printout needed */
	/*
	 * DECnet source and destination are evidently encoded in
	 * the raw Etehrnet header! 
	 */
extern struct ether_header eheader;

extern char *sprintf();

/*
 * Right now these don't do much.  The hope is that some day someone will
 * fill them in.
 */

char	*decaddr();

dec_xpprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_dlprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_rcprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_netprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_latprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_diagprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_cuseprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_lavcprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_bridgeprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_ltmprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

dec_loopprint(index, name, p, length)
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index, "%5d %s", length, name );
	display(index, " %s ->", decaddr(&eheader.ether_shost));
	display(index, " %s",    decaddr(&eheader.ether_dhost));
}

/* if p points to ethernet address, pick out decnet address */
char *decaddr(p)
	struct ether_addr *p;
{
	unsigned	x;
	static	char	buf[10];

	if (isbroadcast(p))
		return ("broadcast");
	else if (p->ether_addr_octet[0] & 1)
		return ("multicast");
	x = ((p->ether_addr_octet[5] & 0xff) << 8) + 
	     (p->ether_addr_octet[4] & 0xff);	/* byte swap */

	sprintf(buf, "%d.%d", x >> 10, x & 0x3FF);
	return(buf);
}
