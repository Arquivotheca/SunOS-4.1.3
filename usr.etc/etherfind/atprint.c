/*
 * @(#)atprint.c 1.1 92/07/30 
 *
 * Module for printing AppleTalk Ethernet packet types
 *
 * Copyright 1987 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

extern int symflag[];		/* verbose printout needed */

extern char *printether();
extern char *sprintf();
extern struct ether_header eheader;

char	*printaarp(), *ddptype();

/*
 * Print out an AppleTalk Datagram Protocol packet.
 * Note that there is this funny three-byte header on the front that
 * simulates the header of the AppleTalk data link level.
 */
atprint(index, name, p, length)
	int index;
	char *name;
	register u_char *p;
	int length;
{
	int lapsrc, lapdst, laptype;

	if (!symflag[index]) {
		display(index, "%5d  %s ", length, name );
		display(index, " %s ->", printether(&eheader.ether_shost));
		display(index, " %s",    printether(&eheader.ether_dhost));
		return;
	}
	lapdst = *p++;
	lapsrc = *p++;
	laptype = *p++;
	switch (laptype) {
	   case 1: 	shortddpprint(index, lapsrc, lapdst, p);return;
	   case 2: 	ddpprint(index, p);				return;
	   default: 	display(index, 
	         "Unknown Apple lap type %d from %d to %d",
	   	laptype, lapsrc, lapdst);
	}
}

/*
 * print the long-format datagram packet
 */
shortddpprint(index, src, dst, p)
	int index;
	int src, dst;
	u_char *p;
{
	int dlength, dstsock, srcsock;

	dlength = (*p++) & 3;
	dlength <<= 8;
	dlength += *p++;

	dstsock = *p++;
	srcsock = *p++;

	display(index, "short DDP from %d.%d to ", src, srcsock);
	if (dst == 255)
	  display(index, "broadcast");
	else
	  display(index, "%d", dst);
	display(index, ".%d %s of %d data bytes", dstsock, ddptype(*p), dlength);
}

/*
 * print the long-format datagram packet
 */
ddpprint(index, p)
	int index;
	u_char *p;
{
	int dlength, dstnet, srcnet, dstid, srcid, dstsock, srcsock;

	dlength = (*p++) & 3;
	dlength <<= 8;
	dlength += *p++;
	p += 2;			/* skip checksum */
	dstnet = *p++;
	dstnet <<= 8;
	dstnet += *p++;

	srcnet = *p++;
	srcnet <<= 8;
	srcnet += *p++;
	
	dstid = *p++;
	srcid = *p++;
	dstsock = *p++;
	srcsock = *p++;

	display(index, "DDP from %d.%d.%d", srcnet, srcid, srcsock);
	display(index, " to %d.", dstnet);
	if (dstid == 255)
	  display(index, "broadcast");
	else
	  display(index, "%d", dstid);
	display(index, ".%d %s of %d data bytes", dstsock, ddptype(*p), dlength);
}

/*
 * return the DDP protocol type string 
 */
char *ddptype(prot)
	u_char prot;
{
    static char buf[128];
    
    switch (prot) {
	case 1:		return("RTMP");
	case 2:		return("NBP");
	case 3:		return("ATP");
	case 4:		return("Echo");
	case 5:		return("RTMP Req");
	case 6:		return("Zip");
	case 10:	return("Data");
    }
    (void) sprintf(buf, "protocol %d", prot);
    return(buf);
}

aarp_print(index, name, a, length)
	int index;
	char *name;
	struct arphdr *a;
	int length;
{
	struct ether_addr *ether;
	char *p = (char *)a;

	if (!symflag[index]) {
		display(index, "%5d  %s ", length, name );
		display(index, " %s ->", printether(&eheader.ether_shost));
		display(index, " %s",    printether(&eheader.ether_dhost));
		return;
	}
	p += sizeof(struct arphdr);

	switch (a->ar_pro) {
  		case 0x809b:	display(index, "Apple");	break;
		default: 	display(index, "Apple Arp protocol %d", a->ar_pro);
	}
	switch (a->ar_op) {
	  case 1:	display(index, " arp request");			break;
	  case 2:	display(index, " arp reply");			break;
	  case 3:	display(index, " arp probe");			break;
	  default:	display(index, " op %d", a->ar_op);		break;
	}
	display(index, " from ");
	ether = (struct ether_addr *)p;
	p += a->ar_hln;
	if (isatalk(p))
		display(index, "%s", printaarp(p));
	if (isknown(ether))
		display(index, "(%s)", printether(ether));
	p += a->ar_pln;
	ether = (struct ether_addr *)p;
	p += a->ar_hln;
	display(index, " for ");
	if (isatalk(p))
		display(index, "%s", printaarp(p));
	if (isknown(ether))
  		display(index, "(%s)", printether(ether));
}


/*
 * return true if this is a non-zero AppleTalk address 
 */
isatalk(p)
	register char *p;
{
	if (*p++)
		return(1);
	if (*p++)
		return(1);
	if (*p++)
		return(1);
	if (*p++)
		return(1);
	if (*p++)
		return(1);
	return(*p != 0);
}

/*
 * print the pseudo-IP address form of AppleTalk address.
 * The high order bytes are all zero, third byte is node number
 */
char *printaarp(p)
	register char *p;
{
	static char buf[128];
	sprintf(buf, "%d", p[3] & 255);
	return(buf);
}
