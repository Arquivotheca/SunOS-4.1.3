/*
 * Filter interpeting module for the "Etherfind" program
 *
 * @(#)filter.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_var.h>
#include <netinet/udp_var.h>

#include "etherfind.h"

	/* printout routine declarations for following table */
extern misc_print();
extern ipprint();
extern arpprint();
extern dec_xpprint();
extern dec_dlprint();
extern dec_rcprint();
extern dec_netprint();
extern dec_latprint();
extern dec_diagprint();
extern dec_cuseprint();
extern dec_lavcprint();
extern dec_bridgeprint();
extern atprint();
extern dec_ltmprint();
extern aarp_print();
extern dec_loopprint();

/*
 * Master table of functions to print out various Ethernet types
 */
struct ether_entry {
	u_short ether_type;
	int (*ether_print)();
	char *ether_name;
    } ether_table[] = {
	{ETHERTYPE_IP,		ipprint,	"ip"},
	{ETHERTYPE_ARP,		arpprint,	"arp"},
	{ETHERTYPE_REVARP,	arpprint,	"rarp"},
	{0x0200,		misc_print,	"old PUP"},
	{0x0201,		misc_print,	"old PUP translation"},
	{0x0600,		misc_print,	"XNS"},
	{0x0601,		misc_print,	"XNS translation"},
	{0x0801,		misc_print,	"X.75 Internet"},
	{0x0802,		misc_print,	"NBS Internet"},
	{0x0803,		misc_print,	"ECMA Internet"},
	{0x0804,		misc_print,	"ChaosNet"},
	{0x0805,		misc_print,	"X.25 Level 3"},
	{0x0807,		misc_print,	"XNS Compatibility"},
	{0x081C,		misc_print,	"Symbolics"},
	{0x0888,		misc_print,	"Xyplex"},
	{0x0900,		misc_print,	"Ungermann-Bass"},
	{0x0A00,		misc_print,	"PUP"},
	{0x0A01,		misc_print,	"PUP translation"},
	{0x0BAD,		misc_print,	"Banyan Systems"},
	{0x1000,		misc_print,	"IP trailer (0)"},
	{0x1001,		misc_print,	"IP trailer (1)"},
	{0x1002,		misc_print,	"IP trailer (2)"},
	{0x1003,		misc_print,	"IP trailer (3)"},
	{0x1004,		misc_print,	"IP trailer (4)"},
	{0x1005,		misc_print,	"IP trailer (5)"},
	{0x1006,		misc_print,	"IP trailer (6)"},
	{0x1007,		misc_print,	"IP trailer (7)"},
	{0x1008,		misc_print,	"IP trailer (8)"},
	{0x1009,		misc_print,	"IP trailer (9)"},
	{0x100a,		misc_print,	"IP trailer (10)"},
	{0x100b,		misc_print,	"IP trailer (11)"},
	{0x100c,		misc_print,	"IP trailer (12)"},
	{0x100d,		misc_print,	"IP trailer (13)"},
	{0x100e,		misc_print,	"IP trailer (14)"},
	{0x100f,		misc_print,	"IP trailer (15)"},
	{0x1600,		misc_print,	"Valid"},
	{0x5208,		misc_print,	"BBN Simnet"},
	{0x6000,		dec_xpprint,	"DEC XP"},
	{0x6001,		dec_dlprint,	"DEC Dump/Load"},
	{0x6002,		dec_rcprint,	"DEC Remote Console"},
	{0x6003,		dec_netprint,	"DEC Routing"},
	{0x6004,		dec_latprint,	"DEC LAT"},
	{0x6005,		dec_diagprint,	"DEC Diagnostic"},
	{0x6006,		dec_cuseprint,	"DEC Custom"},
	{0x6007,		dec_lavcprint,	"DEC LAVC"},
	{0x6008,		misc_print,	"DEC"},
	{0x6009,		misc_print,	"DEC"},
	{0x7000,		misc_print,	"Ungermann-Bass DownLoad"},
	{0x7002,		misc_print,	"Ungermann-Bass Diag"},
	{0x8003,		misc_print,	"Cronus VLN"},
	{0x8004,		misc_print,	"Cronus Direct"},
	{0x8005,		misc_print,	"HP Probe"},
	{0x8006,		misc_print,	"Nestar"},
	{0x8010,		misc_print,	"NPACK"},
	{0x8013,		misc_print,	"SGI diag"},
	{0x8014,		misc_print,	"SGI game"},
	{0x8015,		misc_print,	"SGI"},
	{0x8016,		misc_print,	"SGI bounce"},
	{0x8019,		misc_print,	"Apollo"},
	{0x8038,		dec_bridgeprint,"DEC Bridge"},
	{0x8039,		misc_print,	"DEC"},
	{0x803A,		misc_print,	"DEC"},
	{0x803B,		misc_print,	"DEC"},
	{0x803C,		misc_print,	"DEC"},
	{0x803D,		misc_print,	"DEC"},
	{0x803E,		misc_print,	"DEC"},
	{0x803F,		dec_bridgeprint,"DEC Monitor"},
	{0x8040,		misc_print,	"DEC"},
	{0x8041,		misc_print,	"DEC"},
	{0x8042,		misc_print,	"DEC"},
	{0x805B,		misc_print,	"xV kernel"},
	{0x805C,		misc_print,	"V kernel"},
	{0x805D,		misc_print,	"old xV kernel"},
	{0x805E,		misc_print,	"old old xV kernel"},
	{0x807C,		misc_print,	"Merit"},
	{0x8080,		misc_print,	"Vitalink"},
	{0x809B,		atprint,	"AppleTalk"},
	{0x80DE,		misc_print,	"TRFS"},
	{0x80F3,		aarp_print,	"Apple arp"},
	{0x8107,		misc_print,	"Symbolics"},
	{0x8108,		misc_print,	"Symbolics"},
	{0x8109,		misc_print,	"Symbolics"},
	{0x8137,		misc_print,	"Novell"},
	{0x9000,		dec_loopprint,	"Loopback"},
	{0x9001,		misc_print,	"Bridge mgmt"},
	{0x9002,		misc_print,	"Bridge CS"},
	{0xFF00,		misc_print,	"BBN Vital"},
	{0,			0,		0}
    };

#define ETHERTYPE_DECNETLOW	0x6000
#define ETHERTYPE_DECNETHIGH	0x6009

/*
 * There used to be a single structure with the Ethernet header and
 * TCP/IP header concatenated, but that does not work on the SPARC
 * or other longword aligned architectures.
 */

struct  ether_header eheader;

u_char *Raw;			/* pointer to encapsulated packet */
int Packet[4096];		/* long-aligned copy minus ether_header */

/*
 * convenience macros used to reference certain fields within the packet
 * and result in objects of the right type
 */
#define IP ((struct ip *)Packet)
#define ARP ((struct ether_arp *)Packet)
#define UDP (&((struct udpiphdr *)Packet)->ui_u)
#define	TYPE (ntohs(eheader.ether_type))

extern int printlength;			/* size to print */
extern char *printether();

/* execution time functions */

true()
{
	return (1);
}

false ()
{
	return (0);
}


and(p)
	register struct anode *p;
{

	if (p->L == 0) return (0);

	if (p->R == 0) return (0);

	return(((*p->L->F)(p->L)) && ((*p->R->F)(p->R))?1:0);
}

or(p)
	register struct anode *p;
{
	if (p->L == 0) {
		if (p->R == 0) return (0);
		return ((*p->R->F)(p->R)?1:0);
	} 

	if (p->R == 0) {
		return ((*p->L->F)(p->L)?1:0);
	}
	return(((*p->L->F)(p->L)) || ((*p->R->F)(p->R))?1:0);
}

not(p)
	register struct anode *p;
{
	return( !((*p->L->F)(p->L)));
}


struct ether_addr ether_broadcast = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
 * return true if the given Ethernet address is broadcast
 */
isbroadcast(p)
	struct ether_addr *p;
{
	return(bcmp((char *)p, (char *)&ether_broadcast, 
		sizeof(struct ether_addr) ) == 0);
}

broadcast()
{
	return(isbroadcast(&eheader.ether_dhost));
}

arp()
{
	return (TYPE == ETHERTYPE_ARP);
}

rarp()
{
	return (TYPE == ETHERTYPE_REVARP);
}

ip()
{
	return(TYPE == ETHERTYPE_IP);
}

decnet()
{
	return (TYPE >= ETHERTYPE_DECNETLOW && TYPE <= ETHERTYPE_DECNETHIGH);
}

apple()
{
	return (TYPE == 0x809B || TYPE == 0x803F);
}

proto(p)
	register struct { int f, u; } *p;
{
	return(TYPE == ETHERTYPE_IP && p->u == IP->ip_p);
}

tcp()
{
	return(TYPE == ETHERTYPE_IP && IP->ip_p == htons(IPPROTO_TCP));
}

udp()
{
	return(TYPE == ETHERTYPE_IP && IP->ip_p == htons(IPPROTO_UDP));
}

dst(p)
	register struct { int f, u; } *p; 
{
	u_char	a[2];

	if (TYPE == ETHERTYPE_IP)
		return(p->u == IP->ip_dst.s_addr);
	if (TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP)
		return(bcmp( (char *)&p->u, (char *)ARP->arp_tpa, 
	    		sizeof(struct in_addr)) == 0);
	if (TYPE >= ETHERTYPE_DECNETLOW && TYPE <= ETHERTYPE_DECNETHIGH) {
		a[0] = (p->u >> 8) & 0xff;
		a[1] = p->u & 0xff;
		return (!bcmp((char *)a, (char *)&eheader.ether_dhost.ether_addr_octet[4], 2));
	}
	return(0);
}

src(p)
	register struct { int f, u; } *p; 
{
	u_char	a[2];

	if (TYPE == ETHERTYPE_IP)
		return(p->u == IP->ip_src.s_addr);
	if (TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP)
		return(bcmp( (char *)&p->u, (char *)ARP->arp_spa, 
	    		sizeof(struct in_addr)) == 0);
	if (TYPE >= ETHERTYPE_DECNETLOW && TYPE <= ETHERTYPE_DECNETHIGH) {
		a[0] = (p->u >> 8) & 0xff;
		a[1] = p->u & 0xff;
		return (!bcmp((char *)a, (char *)&eheader.ether_shost.ether_addr_octet[4], 2));
	}
	return(0);
}

/*
 * etool compatibility only
 */
target(p) { return(host(p)); }

host(p)
	register struct { int f, u; } *p; 
{
	return(src(p) || dst(p));
}

between(p)
	register struct { int f; struct addrpair *u; } *p; 
{
	u_char	a[2], b[2];

	if (TYPE == ETHERTYPE_IP)
		return (((p->u->addr1 == IP->ip_src.s_addr)
		    && (p->u->addr2 == IP->ip_dst.s_addr))
		    || ((p->u->addr2 == IP->ip_src.s_addr)
		    && (p->u->addr1 == IP->ip_dst.s_addr)));
		    
	else if (TYPE == ETHERTYPE_ARP || TYPE == ETHERTYPE_REVARP)
                return (
               (bcmp((char*) &p->u->addr1, (char*) ARP->arp_spa,
                        sizeof (struct in_addr)) == 0 &&
                bcmp((char*) &p->u->addr2, (char*) ARP->arp_tpa,
                        sizeof (struct in_addr)) == 0 ) ||
               (bcmp((char*) &p->u->addr2, (char*) ARP->arp_spa,
                        sizeof (struct in_addr)) == 0 &&
                bcmp((char*) &p->u->addr1, (char*) ARP->arp_tpa,
                        sizeof (struct in_addr)) == 0 )
                );
	else if (TYPE >= ETHERTYPE_DECNETLOW && TYPE <= ETHERTYPE_DECNETHIGH) {
		a[0] = (p->u->addr1 >> 8) & 0xff;
		a[1] = p->u->addr1 & 0xff;
		b[0] = (p->u->addr2 >> 8) & 0xff;
		b[1] = p->u->addr2 & 0xff;

		return ((!bcmp((char *)a, (char *)&eheader.ether_shost.ether_addr_octet[4], 2)
			&& !bcmp((char *)b, (char *)&eheader.ether_dhost.ether_addr_octet[4], 2))
		   ||
		(!bcmp((char *)a, (char *)&eheader.ether_dhost.ether_addr_octet[4], 2)
			&& !bcmp((char *)b, (char *)&eheader.ether_shost.ether_addr_octet[4], 2)));
	} else
		return(0);
}

dst_port(p)
	register struct anode *p;
{
	return(TYPE == ETHERTYPE_IP &&
	    (IP->ip_p == IPPROTO_UDP || IP->ip_p == IPPROTO_TCP)
	    && (u_short)(p->L) == UDP->uh_dport);
}

src_port(p)
	register struct anode *p;
{
	return(TYPE == ETHERTYPE_IP &&
	    (IP->ip_p == IPPROTO_UDP || IP->ip_p == IPPROTO_TCP)
	    && (u_short)(p->L) == UDP->uh_sport);
}


rpc ()
{
	struct ip *ip = IP;
	struct udphdr *udpp;
	struct rpc_msg *msg;

	udpp = (struct udphdr *)((int)ip + (ip->ip_hl << 2));
	msg = (struct rpc_msg *)((char *)udpp + sizeof(struct udphdr));
	
	return(udp() && valid_rpc(msg));
}

nfs ()
{
	struct ip *ip = IP;
	struct udphdr *udpp;
	struct rpc_msg *msg;

	udpp = (struct udphdr *)((int)ip + (ip->ip_hl << 2));
	msg = (struct rpc_msg *)((char *)udpp + sizeof(struct udphdr));

	return (udp() && valid_nfs(msg));
}

byte(p)
	register struct { int f; struct colon *u; } *p; 
{
	switch(p->u->op) {
	case '=':	return (Raw[p->u->byte] == (u_char)p->u->value);
	case '<':	return (Raw[p->u->byte] < (u_char)p->u->value);
	case '>':	return (Raw[p->u->byte] > (u_char)p->u->value);
	case '&':	return (Raw[p->u->byte] & (u_char)p->u->value);
	case '|':	return (Raw[p->u->byte] | (u_char)p->u->value);
	default:
		return(0);
	}
}

srcnet(p)
	register struct { int f, u; } *p; 
{
	return(TYPE == ETHERTYPE_IP
	    && p->u == inet_netof(IP->ip_src));
}

dstnet(p)
	register struct { int f, u; } *p;
{
	return(TYPE == ETHERTYPE_IP
	    && p->u == inet_netof(IP->ip_dst));
}

less(p)
	register struct { int f, u; } *p; 
{
	return(sp_ts_len <= p->u);
}

greater(p)
	register struct { int f, u; } *p; 
{
	return(sp_ts_len >= p->u);
}

/*
 * main interface from the OS-specific module
 */
filter(index, cp, length, tvp, drops)
	char *cp;
	u_int length;
	struct timeval *tvp;
	u_int drops;
{
	static int i = 0;

	bcopy(cp, (char *)&eheader, sizeof(eheader));
	if (ntohs(eheader.ether_type) <= 1514) {
		/*
		 * fake out the IEEE 802.3 packets.
		 * Should be DSAP=170, SSAP=170, control=3,
		 * Then three padding bytes of zero, followed by normal
		 * Ethernet-type packet
		 */
		bcopy(cp + sizeof(eheader) + 8, (char *)Packet, 
			(int) (length - sizeof(eheader)) );
		eheader.ether_type = *(u_short *)(cp + sizeof(eheader) + 6);
	}
	else bcopy(cp + sizeof(eheader), (char *)Packet, 
			(int) (length - sizeof(eheader)) );

	Raw = (u_char *)cp;

	if ((*(exlist[index]->F))(exlist[index]) == 0)
		return;

	if (trflag) {
		int	threshold = trigger_count[0] / 5;

		if (i > trigger_count[0]) {
			tr_triggered ();
		} else {
			if ((i % threshold) == 0)
				tr_display (i);
			tr_save (cp, length, tvp);
		}
	} else
		printit(index, length, tvp, drops);
	i++;
	if (cflag && i >= cnt)
		exit(0);
	
}

static struct timeval start_time = { 0, 0 };	/* starting timestamp */
static u_int olddrops = 0;		/* previous value for packets dropped */

/*
 * XXX: drop counts don't line up in the same column for all packet types.
 */
printit(index, length, tvp, drops)
	int index;
	u_int length;
	struct timeval *tvp;
	u_int drops;
{
	u_int dropsdelta;
	register struct ether_entry *ee;

	dropsdelta = drops - olddrops;
	olddrops = drops;

	if (timeflag[index]) {
		if (start_time.tv_sec == 0) {
			start_time = *tvp;
		}
		display(index,"%5.2f ", (float) 
		  (tvp->tv_sec - start_time.tv_sec) +
		  (tvp->tv_usec - start_time.tv_usec)/1000000.0 );
	}

	/*
	 * search the table for matching ethernet type, and then call
	 * the appropriate printout routine
	 */
	for (ee = ether_table; ee->ether_type; ee++)
		if (ee->ether_type == TYPE) break;
	
	if (ee->ether_type) 
		(*ee->ether_print)(index, ee->ether_name, Packet, length);
	else {
		display(index,"%5d  type %04x", length, TYPE & 0xffff);
		display(index," %s ->", printether(&eheader.ether_shost));
		display(index," %s",    printether(&eheader.ether_dhost));
	}

	if (dflag && dropsdelta > 0)
		display(index,"  [%d]\n", dropsdelta);
	else
		display(index,"\n");

	if (xflag[index]) {
		int i, j;
		u_char *p;
	 /*
	  * Dump the raw data within the packet, up to the max of
	  * its real size, or a specified size.  
	  */
		p = (u_char *)Raw;
		if (printlength && length > printlength) 
			length = printlength;
		j = length;
		i = 0;
		while (j-- > 0) {
			i++;
			display(index," %02x", *p++);
			if (i == 16) {
				i = 0;
				display(index,"\n");
			}
		}
		if (i != 0)
			display(index,"\n\n");
		else
			display(index,"\n");
	}
	log_flush(index);
}


/*
 * flush the stdio buffers on signals
 */
void
flushit()
{
	fflush(stdout);
	exit(1);
}


/*
 * Prototypical printout routine
 */
misc_print(index, name, p, length)
	int index;
	char *name;
	char *p;
	int length;
{
# ifdef lint
	p = p;
# endif lint

	display(index,"%5d %s", length, name );
	display(index," %s ->", printether(&eheader.ether_shost));
	display(index," %s",    printether(&eheader.ether_dhost));
}

