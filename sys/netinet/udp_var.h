/*	@(#)udp_var.h 1.1 92/07/30 SMI; from UCB 7.1 6/5/86	*/

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * UDP kernel structures and variables.
 */

#ifndef _netinet_udp_var_h
#define _netinet_udp_var_h

struct	udpiphdr {
	struct 	ipovly ui_i;		/* overlaid ip structure */
	struct	udphdr ui_u;		/* udp header */
};
#define	ui_next		ui_i.ih_next
#define	ui_prev		ui_i.ih_prev
#define	ui_x1		ui_i.ih_x1
#define	ui_pr		ui_i.ih_pr
#define	ui_len		ui_i.ih_len
#define	ui_src		ui_i.ih_src
#define	ui_dst		ui_i.ih_dst
#define	ui_sport	ui_u.uh_sport
#define	ui_dport	ui_u.uh_dport
#define	ui_ulen		ui_u.uh_ulen
#define	ui_sum		ui_u.uh_sum

struct	udpstat {
	int	udps_hdrops;
	int	udps_badsum;
	int	udps_badlen;
	int	udps_fullsock;
};

#define	UDP_TTL		30		/* time to live for UDP packets */

#ifdef KERNEL
struct	inpcb udb;
struct	udpstat udpstat;
#endif

#endif /*!_netinet_udp_var_h*/
