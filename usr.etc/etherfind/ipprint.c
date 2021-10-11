/*
 * This module prints out IP/TCP/UDP packets in a verbose form
 *
 * @(#)ipprint.c 1.1 92/07/30
 *
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <netinet/if_ether.h>

#include <stdio.h>

# define NND 1
#include "ndio.h"

#include <protocols/routed.h>
#include <arpa/tftp.h>

# define IP_OFFSET 0x1FFF	/* masked off DF and MF flags */
# ifndef IPPPORT_DOMAIN
# define IPPORT_DOMAIN 53	/* domain name server port */
# endif 

extern char *malloc();
extern char *sprintf();
extern char *inet_ntoa();

extern char *getname();
extern char *getportname();

char *prototable[];		/* IP protocol numbers to names */


extern int symflag[];		/* verbose printout needed */
extern int rpcflag[];		/* RPC printout needed */

char *icmptype();		/* forward declarations */

ipprint(index, name, ip, length)
    int index;
    char *name;
    struct ip *ip;
    int length;
  {
    int *data;
    unsigned char *opt;
    int datalength, optlength, curlength;

    data = (int *)ip;
    data += ip->ip_hl;
    datalength = ntohs(ip->ip_len) - ip->ip_hl*4;
    if (!symflag[index] && !rpcflag[index]) {
    	shortipprint( index, ip, length, (struct udphdr *)data );
	return;
    }
    if (ip->ip_off & IP_OFFSET)
      {
	switch (ip->ip_p)
 	  {
	    case IPPROTO_TCP: 	display(index,"TCP");		break;
	    case IPPROTO_UDP: 	display(index,"UDP");		break;	
	    case IPPROTO_ND: 	display(index,"ND");		break;
	    case IPPROTO_ICMP:  display(index,"ICMP");		break;
	    case IPPROTO_HELLO:  display(index,"Hello");	break;
	    default: 	display(index,"%s protocol %d", name, ip->ip_p);
		break;
	  }
        display(index," fragment offset=%d, length=%d ", 
	         (ip->ip_off & IP_OFFSET) * 8, datalength);
	display(index," from %s ", 	getname(ip->ip_src) );
	display(index,"to %s",	 	getname(ip->ip_dst) );
      }
    else switch (ip->ip_p)
      {
        case IPPROTO_TCP: 
		tcpprint(index, (struct tcphdr *)data, datalength, 
			ip->ip_src, ip->ip_dst);
		break;

        case IPPROTO_UDP: 
		udpprint(index, (struct udphdr *)data, datalength, 
				ip->ip_src, ip->ip_dst);
		break;
	
	case IPPROTO_ND:
		ndprint(index, (char *)data, datalength, ip->ip_src, ip->ip_dst);
		break;

	case IPPROTO_ICMP:
		icmpprint(index, (struct icmp *)data, datalength, 
			ip->ip_src, ip->ip_dst);
		break;

	case IPPROTO_HELLO:
		helloprint(index, (char *)data, datalength, 
			ip->ip_src, ip->ip_dst);
		break;

	default:
	 	display(index,"Unknown IP protocol %d", ip->ip_p);
		display(index," from %s ", 	getname(ip->ip_src) );
		display(index,"to %s",	 	getname(ip->ip_dst) );
		break;
      }
    if (ip->ip_off & IP_MF)
    	display(index, " (more fragments)");
    opt = (unsigned char *)ip;
    opt += sizeof(struct ip);
    optlength = ip->ip_hl*4 - sizeof(struct ip);
    if (optlength > 0)
       display(index, "\n  ip options:");
    while (optlength > 0) {
      /*
       * Decode IP options.
       */
       curlength = opt[1];
       switch (*opt) {
         case IPOPT_EOL:
	    return;
	 
	 case IPOPT_NOP:
	    opt++;
	    optlength--;
	    continue;
	 
	 case IPOPT_RR:
	    display(index," <record route> ");
	    ip_rrprint(index, opt, curlength);
	    break;
	 
	 case IPOPT_TS:
	    display(index," <time stamp>");
	    break;
	 
	 case IPOPT_SECURITY:
	    display(index," <security>");
	    break;
	 
	 case IPOPT_LSRR:
	    display(index," <loose source route> ");
	    ip_rrprint(index, opt, curlength);
	    break;
	 
	 case IPOPT_SATID:
	    display(index," <stream id>");
	    break;
	 
	 case IPOPT_SSRR:
	    display(index," <strict source route> ");
	    ip_rrprint(index, opt, curlength);
	    break;
	 
	 default:
	    display(index," <option %d, len %d>", *opt, curlength);
	    break;
       }
       /*
        * Following most options comes a length field
	*/
	opt += curlength;
	optlength -= curlength;
    }
  }


/*
 * Print out a recorded route option.
 */
ip_rrprint(index, opt, length)
    int index;
    unsigned char *opt;
    int length;
{
    int pointer;
    struct in_addr addr;

    opt += IPOPT_OFFSET;
    length -= IPOPT_OFFSET;

    pointer = *opt++;
    pointer -= IPOPT_MINOFF;
    length--;
    
    while (length > 0) {
	bcopy((char *)opt, (char *)&addr, sizeof(addr));
	if (addr.s_addr == INADDR_ANY)
	    display(index, "-");
	else
	    display(index, "%s", getname(addr) );
	if (pointer == 0)
	    display(index, "(Current)");
	opt += sizeof(addr);
	length -= sizeof(addr);
	pointer -= sizeof(addr);
	if (length >0)
	    display(index, ", ");
    }
}


/*
 * short printout version (one-line) for Internet Packets
 */
shortipprint(index, ip, length, udp)
    int index;
    struct ip *ip;
    int length;
    struct udphdr *udp;
{
    int proto, frag;
    struct in_addr src, dst;
    char fchar;

	proto = ip->ip_p;
	frag = ip->ip_off & 0x1fff;
	fchar = (frag ? '*' : ' ');
	dst = ip->ip_dst;
	src = ip->ip_src;
	if (prototable[proto])
		display(index,"%c%4d %4s ", fchar, length, prototable[proto]);
	else
		display(index,"%c%4d   %02d ", fchar, length, proto);
	if (frag)
		return;

	display(index,"%15.15s ", getname(src));
	display(index,"%15.15s ", getname(dst));
	if (proto == IPPROTO_TCP || proto == IPPROTO_UDP) {
		display(index,"%10.10s ", getportname(udp->uh_sport, proto));
		display(index,"%10.10s",  getportname(udp->uh_dport, proto));
	}
	else if (proto == IPPROTO_ICMP) {
		display(index,"     %s", icmptype((struct icmp *)udp));
	}
}


tcpprint(index, tcp, length, src, dst)
    int index;
    register struct tcphdr *tcp;	/* points to tcp */
    int length;				/* in bytes of entire TCP */
    struct in_addr src, dst;		/* Internet addresses */
  {
	/*
	 * Print the given TCP segment
	 */
    int optlen;
    unsigned char *opt;

    display(index,"TCP from %s.%s ", 
    		getname(src), 
    		getportname(tcp->th_sport,IPPROTO_TCP) );
    display(index,"to %s.%s ", 
    		getname(dst), 
		getportname(tcp->th_dport,IPPROTO_TCP) );
    display(index,"seq %X, ", ntohl(tcp->th_seq));
    if (tcp->th_flags & TH_ACK)
    	display(index,"ack %X, ", ntohl(tcp->th_ack));
    if (tcp->th_flags & TH_SYN)
    	display(index,"SYN, ");
    if (tcp->th_flags & TH_RST)
    	display(index,"RESET, ");
    if (tcp->th_flags & TH_FIN)
    	display(index,"FIN, ");
    display(index," window %d, ", ntohs(tcp->th_win));
    optlen = tcp->th_off*4 - sizeof(struct tcphdr);
    opt = (unsigned char *)tcp;
    opt += sizeof(struct tcphdr);
    while (optlen > 0) {
     	  /*
	   * decode the TCP options.  Right now the only one is MSS,
	   * although Bob might want me to add SACK at some point.
	   */
        int mss, curlen;
        switch (*opt) {
	  case TCPOPT_EOL:
	  	optlen = 0;
		break;
	  case TCPOPT_NOP:
	  	optlen--;
		opt++;
		break;
	  case TCPOPT_MAXSEG:
	     curlen = opt[1];
	     mss = opt[2]*256 + opt[3];
	     optlen -= curlen;
	     opt += curlen;
	     display(index,"<mss %d> ", mss);
	     break;
	  default:
	     display(index,"<%d> ", *opt++);
	     optlen--;
	}
      }
    length -= tcp->th_off*4;
    if (length <= 0)
    	return;

    display(index,"%d bytes data", length);
    if (rpcflag[index] != 0)
	rpcprint(index, opt, length);
  }


udpprint(index, udp, length, src, dst)
    int index;
    struct udphdr *udp;
    struct in_addr src, dst;
  {    
    int ulen = ntohs(udp->uh_ulen);

    display(index,"UDP from %s.%s ", 
    		getname(src), 
    		getportname(udp->uh_sport,IPPROTO_UDP) );
    display(index,"to %s.%s ", 
    		getname(dst), 
		getportname(udp->uh_dport,IPPROTO_UDP) );
    display(index," %d bytes", ulen );
    if (length < ulen)
    	display(index," (only %d bytes long)", length);
    if (!rpcflag[index]) return;
    if (udp->uh_sport == htons(IPPORT_ROUTESERVER) ||
	udp->uh_dport == htons(IPPORT_ROUTESERVER))
    	routeprint(index, 
	       (struct rip *)((char *)udp + sizeof(struct udphdr)), 
	    	udp->uh_ulen - sizeof(struct udphdr) );
    else if (udp->uh_sport == htons(IPPORT_TFTP) ||
	     udp->uh_dport == htons(IPPORT_TFTP))
    	tftpprint(index, 
	       (struct tftphdr *)((char *)udp + sizeof(struct udphdr)), 
	    	udp->uh_ulen - sizeof(struct udphdr) );
    else if (udp->uh_sport == htons(IPPORT_DOMAIN) ||
	     udp->uh_dport == htons(IPPORT_DOMAIN))
    	domainprint(index, 
	       ((char *)udp + sizeof(struct udphdr)), 
	    	udp->uh_ulen - sizeof(struct udphdr) );
    else rpcprint(index, (char *)udp + sizeof(struct udphdr), 
    	ulen - sizeof(struct udphdr) );
  }

/*
 * Return a TFTP error code string, given number
 */
char *tftperror(code)
    unsigned short code;
{
    static char buf[128];

    switch (code) {
        case EUNDEF: 	return("not defined");
        case ENOTFOUND:	return("file not found");
        case EACCESS:	return("access violation");
        case ENOSPACE:	return("disk full or allocation exceeded");
        case EBADOP:	return("illegal TFTP operation");
        case EBADID:	return("unknown transfer ID");
        case EEXISTS:	return("file already exists");
        case ENOUSER:	return("no such user");
    }
    (void) sprintf(buf,"%d",code);
    return(buf);
}

/*
 * Print a Trivial File Transfer (TFTP) packet
 */
tftpprint(index, p, length)
    int index;
    struct tftphdr *p;
    int length;
{
    display(index," TFTP ");
    switch (ntohs(p->th_opcode)) {
	case RRQ:
	    display(index,"Read");
	    break;
	case WRQ:
	    display(index,"Write");
	    break;
	case DATA:
	    display(index,"Data block %d", ntohs(p->th_block));
	    break;
	case ACK:	
	    display(index,"Ack block %d", ntohs(p->th_block));
	    break;
	case ERROR:
	    display(index,"Error %s", tftperror(ntohs(p->th_code)) );
	    break;
    }
}


/*
 * Print a Routing Information Protocol (RIP) packet
 */
routeprint(index, p, length)
    int index;
    struct rip *p;
    int length;
{
    register struct netinfo *n;
    struct sockaddr_in *sin;
    int i;
    char *name;

    switch (p->rip_cmd) {
        case RIPCMD_REQUEST: 	display(index," Route Request ");	break;
        case RIPCMD_RESPONSE: 	display(index," Route Response ");	break;
        case RIPCMD_TRACEON: 	display(index," Route Trace On ");	break;
        case RIPCMD_TRACEOFF: 	display(index," Route Trace Off ");	break;
        case RIPCMD_POLL: 	display(index," Route Poll ");		break;
        case RIPCMD_POLLENTRY: 	display(index," Route Poll Entry ");	break;
	default:		return;	/* probably not a RIP packet */
    }
    switch (p->rip_cmd) {
        case RIPCMD_POLLENTRY:
	    if (length > 4 + sizeof(struct netinfo)) {
	    	register struct entryinfo *ep;
		
		ep = (struct entryinfo *)(p->rip_tracefile);
		display(index,"Reply\n ");
		sin = (struct sockaddr_in *)&ep->rtu_dst;
		name = getname(sin->sin_addr);
		display(index,"Dst %s(%s), ", name, inet_ntoa(sin->sin_addr));
		sin = (struct sockaddr_in *)&ep->rtu_router;
		name = getname(sin->sin_addr);
		display(index,"via %s(%s), ", name, inet_ntoa(sin->sin_addr));
		display(index,"metric %d, ifp %s",
			 ep->rtu_metric, ep->int_name);
	    	break;
	    }
	
        case RIPCMD_REQUEST: 	
        case RIPCMD_RESPONSE: 	
        case RIPCMD_POLL: 	
		/*
		 * skip header; rest are sockaddr/metric pairs, except
		 * in the case of POLLENTRY replies.
		 */
	    length -= 4;
	    for (n=p->rip_nets;length >= sizeof(struct netinfo); n++) {
		if (p->rip_vers > 0) {
			n->rip_dst.sa_family =
				ntohs(n->rip_dst.sa_family);
			n->rip_metric = ntohl((u_long)n->rip_metric);
		}
		display(index,"\n\t");
		sin = (struct sockaddr_in *)&n->rip_dst;
		if (sin->sin_port) {
		    display(index,"**port %d** ", 
		    	htons(sin->sin_port) & 0xFFFF);
		}
		for (i=6;i<13;i++)
		  if (n->rip_dst.sa_data[i]) {
		    display(index,"sockaddr[%d] = %d", i,
			n->rip_dst.sa_data[i] & 0xFF);
		}
		if (sin->sin_addr.s_addr==htonl(INADDR_ANY))
			name = "default";
		else name = getname(sin->sin_addr);
		display(index," %s(%s), metric %d", name,
			inet_ntoa(sin->sin_addr), n->rip_metric);
		length -= sizeof(struct netinfo);
	    }
	    return;

        case RIPCMD_TRACEON: 	
        case RIPCMD_TRACEOFF:
	    display(index,"\n Trace file name = %s", p->rip_tracefile);
	    return;
    }
}

ndprint(index, data, length, src, dst)
    int index;
    char *data;
    struct in_addr src, dst;
  {
	register struct ndpack *nd;

	nd = (struct ndpack *)(data - sizeof(struct ip));
	display(index,"ND from %s ", getname(src) );
	display(index,"to %s ",	getname(dst) );
	switch (nd->np_op & NDOPCODE) {
	    case NDOPREAD:  	display(index,"Read");		break;
	    case NDOPWRITE:  	display(index,"Write");		break;
	    case NDOPERROR:  	display(index,"Error");		break;
        }
	display(index," device %d blk %d count %d length %d", 
      		nd->np_min, htonl(nd->np_blkno), htonl(nd->np_bcount), length );
	if (nd->np_op & NDOPDONE) display(index, " Done");
  }


icmpprint(index, icp, length, src, dst)
    int index;
    register struct icmp *icp;		/* points to icmp packet */
    int length;				/* in bytes of entire icmp*/
    struct in_addr src, dst;		/* Internet addresses */
  {
	/*
	 * Print the given ICMP message
	 */
    display(index,"ICMP from %s ", getname(src) );
    display(index,"to %s ", getname(dst) );
    display(index,"%s ", icmptype(icp));
    switch (icp->icmp_type)
    {
      case ICMP_ECHO:
      case ICMP_ECHOREPLY:
	display(index, "%d data bytes", length);
	break;

      case ICMP_UNREACH:
      	    switch (icp->icmp_code)
	    {
	    case ICMP_UNREACH_NET:      display(index,"bad net");	break;
	    case ICMP_UNREACH_HOST:     display(index,"bad host");	break;
	    case ICMP_UNREACH_PROTOCOL: display(index,"bad protocol");	break;
	    case ICMP_UNREACH_PORT:     display(index,"bad port");	break;
	    case ICMP_UNREACH_NEEDFRAG: display(index,"needed to fragment");
									break;
	    case ICMP_UNREACH_SRCFAIL:  display(index,"src route failed");
	    								break;
	    default:	display(index,"subcode %d", icp->icmp_code);
	    }
	case ICMP_TIMXCEED:
	case ICMP_SOURCEQUENCH:
	    display(index,"\n  bad packet: ");
	    if (length > 28)
		ipprint(index, 0, (struct ip *)icp->icmp_data, 28);
	    break;

      case ICMP_REDIRECT:
      	    switch (icp->icmp_code)
	    {
	    case ICMP_REDIRECT_NET:     
	    	display(index,"for network");
		break;
	    case ICMP_REDIRECT_HOST:    
	    	display(index,"for host");
		break;
	    case ICMP_REDIRECT_TOSNET:  
	    	display(index,"for tos and net");	
		break;
	    case ICMP_REDIRECT_TOSHOST: 
	    	display(index,"for tos and host");	
	    	break;
	    default:
	       display(index,"subcode %d", icp->icmp_code);
	    }
	    display(index," to %s", getname(icp->icmp_gwaddr));
	    display(index,"\n  bad packet: ");
	    if (length > 28)
		ipprint(index, 0, (struct ip *)icp->icmp_data, 28);
      	   break;

      case ICMP_MASKREPLY:
	    display(index, getname(icp->icmp_mask));
	    break;

      default:
	    if (icp->icmp_code)
        	display(index,"subcode %d", icp->icmp_code);
    }
}

/*
 * Print the given Ethernet address
 */
char *printether(p)
	struct ether_addr *p;
 {
   static char buf[256];
   
   if (isbroadcast(p))
   	return("broadcast");
   
   sprintf(buf, "%x:%x:%x:%x:%x:%x", 
   	p->ether_addr_octet[0], 
   	p->ether_addr_octet[1], 
	p->ether_addr_octet[2], 
	p->ether_addr_octet[3], 
	p->ether_addr_octet[4], 
	p->ether_addr_octet[5]);
   return(buf);
 }

arpprint(index, name, arp, length)
  int index;
  char *name;
  struct ether_arp *arp;
  int length;
{
  struct in_addr ip;

    if (!symflag[index]) {
    	shortarpprint( index, name, arp, length);
	return;
    }

  switch (ntohs(arp->arp_pro)) {
  	case ETHERTYPE_IP: 	display(index,"ip");		break;
  	case ETHERTYPE_TRAIL: 	display(index,"trailer");	break;
  	case ETHERTYPE_TRAIL+1: display(index,"trailer(1)");	break;
  	case ETHERTYPE_TRAIL+2: display(index,"trailer(2)");	break;
  	case ETHERTYPE_TRAIL+3: display(index,"trailer(3)");	break;
  	case ETHERTYPE_TRAIL+4: display(index,"trailer(4)");	break;
  	case ETHERTYPE_TRAIL+5: display(index,"trailer(5)");	break;
  	case ETHERTYPE_TRAIL+6: display(index,"trailer(6)");	break;
  	case ETHERTYPE_TRAIL+7: display(index,"trailer(7)");	break;
	default: 	   display(index,"protocol %d", ntohs(arp->arp_pro));
  }
  display(index," %s ", name);
  switch (ntohs(arp->arp_op)) {
  	case ARPOP_REQUEST:	display(index,"request");	break;
  	case ARPOP_REPLY:	display(index,"reply");		break;
  	case REVARP_REQUEST:	display(index,"rev request");	break;
  	case REVARP_REPLY:	display(index,"rev reply");	break;
	default: 		display(index,"op %d", ntohs(arp->arp_op));
  }
  display(index," from ");
	/*
	 * Allow this to be compiled on 3.x systems as well as 4.x systems
	 */
#ifdef arp_spa
# undef arp_spa
# undef arp_sha
# undef arp_tpa
# undef arp_tha
# define arp_spa arp_xspa
# define arp_sha arp_xsha
# define arp_tpa arp_xtpa
# define arp_tha arp_xtha
#endif arp_spa

  bcopy((char *)arp->arp_spa, (char *)&ip, sizeof(ip));
  if (ip.s_addr != htonl(INADDR_ANY))
  	display(index,"%s", getname(ip));
  if (isknown(&arp->arp_sha))
  	display(index,"(%s)", printether(&arp->arp_sha));
  bcopy((char *)arp->arp_tpa, (char *)&ip, sizeof(ip));
  display(index," for ");
  if (ip.s_addr != htonl(INADDR_ANY))
	display(index,"%s", getname(ip));
  if (isknown(&arp->arp_tha))
  	display(index,"(%s)", printether(&arp->arp_tha));
}

/*
 * Return TRUE if the Ethernet address is known
 */
isknown(a)
	struct ether_addr *a;
  {
    register u_char *p = a->ether_addr_octet;
    int i;

    for (i=0; i<6; i++)
      if (*p++) return(1);
    return(0);
  }

/*
 * one-line version of the ARP printout routine
 */
shortarpprint(index, name, arp, length)
  int index;
  char *name;
  struct ether_arp *arp;
  int length;
{
    struct in_addr src, dst;

    display(index,"%5d %4s ", length, name);

    bcopy((char *)arp->arp_spa, (char *)&src, sizeof(src));
    bcopy((char *)arp->arp_tpa, (char *)&dst, sizeof(dst));

    display(index,"%15.15s ", getname(src));
    display(index,"%15.15s ", getname(dst));
}


struct hellohdr {
	u_short h_cksum;
        u_short h_date;
        u_long  h_time;
        u_short h_tstp;
};

struct m_hdr {
	u_char  m_count ;
	u_char  m_type ;
};
		
struct type0pair {
	u_short d0_delay;
	u_short d0_offset;
};
			

struct type1pair {
	struct in_addr d1_dst;
	u_short d1_delay ;
	short   d1_offset ;
};


/*
 * The "HELLO" routing information protocol
 */
helloprint(index, data, length, src, dst)
    int index;
    int length;
    u_char *data;
    struct in_addr src, dst;
  {
	struct hellohdr *h = (struct hellohdr *)data;
	struct m_hdr *mh;
	struct type1pair *t1p;
	u_short delay;
	int i;

	length -= sizeof(struct hellohdr);
	data += sizeof(struct hellohdr);

	display(index,"Hello from %s ", getname(src) );
	display(index,"to %s length %d",getname(dst), length );
	if (!rpcflag[index]) return;

	display(index,"\n");
	while (length > 0) {
	    mh = (struct m_hdr *)data;
	    length -= sizeof(struct m_hdr);
	    data += sizeof(struct m_hdr);
	    switch (mh->m_type) {
	      case 0:
		length -= sizeof(struct type0pair);
		data += sizeof(struct type0pair);
		display(index,"  Type %d\n", mh->m_type);
		break;
	      case 1:
		for (i = 0; i < mh->m_count && length > 0; i++) {
			t1p = (struct type1pair *)data;
			bcopy( &t1p->d1_dst, &dst, 
				sizeof(struct in_addr));
			delay = ntohs(t1p->d1_delay) & 0x7fff;
			display(index, "  %s delay %d offset %d\n", 
				getname(dst), delay, ntohs(t1p->d1_offset));
			length -= sizeof(struct type1pair);
			data += sizeof(struct type1pair);
		}
		break;
	      default:
		    	display(index,"  Type %d\n", mh->m_type);
			break;
		}
	}
  }
