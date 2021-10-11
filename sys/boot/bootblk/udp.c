/* @(#)udp.c 1.1 92/07/30 SMI */

/*
 * Stand-alone UDP
 */
#include "../lib/common/saio.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/if_ether.h>
#include <netinet/ip_var.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include "../lib/common/sainet.h"

#define ETHER_OFFSET	(sizeof(struct ether_header))
#define IP_OFFSET	(ETHER_OFFSET + sizeof(struct ip))
#define UDP_OFFSET (IP_OFFSET + sizeof(struct udphdr))

struct saioreq ether_si;
struct sainet ether_sain;
int random;

/*
 * Receive udp packet
 */
udp_input(sin, ether, myport, recvpacket, recvsize)
	struct sockaddr_in *sin;
	struct ether_addr *ether;
	u_short myport;
	char *recvpacket;
	int recvsize;
{
	struct sainet sain;
	struct ip *ip;
	struct udphdr *uh;
	int len;

	sain = ether_sain;
	if (ip_input(&ether_si, recvpacket, &sain) <= 0) {
		return(0);
	}

	ip = (struct ip *) (recvpacket + ETHER_OFFSET);
	if (ntohs(ip->ip_p) != IPPROTO_UDP) {
		return(-1);
	}

	uh = (struct udphdr *) (recvpacket + IP_OFFSET);
	if (ntohs(uh->uh_dport) != myport) {
		return(-1);
	}
	len = ntohs(uh->uh_ulen) - sizeof(struct udphdr);
	if (len > recvsize) {
		return(-1);
	}
	*ether = sain.sain_hisether;
	sin->sin_addr = ip->ip_src;
	sin->sin_port = uh->uh_sport;
	return(len);
}


/*
 * Send udp packet
 */
udp_output(sin, ether, myport, sendpacket, sendsize) 
	struct sockaddr_in *sin;
	struct ether_addr *ether;
	u_short myport;
	char *sendpacket;
	int sendsize;
{
	struct udphdr *uh;
	struct ip *ip;
	struct sainet sain;

	sain = ether_sain;
	sain.sain_hisether = *ether;
	sain.sain_hisaddr = sin->sin_addr;
	uh = (struct udphdr *) (sendpacket + IP_OFFSET);
	uh->uh_sport = htons(myport);
	uh->uh_dport = htons(sin->sin_port);
	uh->uh_ulen = htons(sendsize + sizeof(struct udphdr));
	uh->uh_sum = 0;

	ip = (struct ip *) (sendpacket + ETHER_OFFSET);
	ip->ip_v = IPVERSION;
	ip->ip_hl = sizeof(struct ip) >> 2;
	ip->ip_tos = 0;
	ip->ip_len = 
		htons(sendsize + sizeof(struct udphdr) + sizeof(struct ip));
	ip->ip_id = random++;
	ip->ip_off = 0;
	ip->ip_ttl = ~0;
	ip->ip_p = htons(IPPROTO_UDP);
	ip->ip_src = sain.sain_myaddr;
	ip->ip_dst = sain.sain_hisaddr;

	return(ip_output(&ether_si, sendpacket, sendsize + UDP_OFFSET,
		&sain, 0));
}
