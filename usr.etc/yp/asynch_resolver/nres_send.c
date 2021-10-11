/*
 * Copyright (c) 1985 Regents of the University of California. All rights
 * reserved.
 * 
 * Redistribution and use in source and binary forms are permitted provided that
 * this notice is preserved and that due credit is given to the University of
 * California at Berkeley. The name of the University may not be used to
 * endorse or promote products derived from this software without specific
 * prior written permission. This software is provided ``as is'' without
 * express or implied warranty.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char     copyright[] = "@(#)nres_send.c	6.19 (Berkeley) 3/7/88";
static	char sccsid[] = "@(#)nres_send.c 1.1 92/07/30 1989";
#endif				/* LIBC_SCCS and not lint */

/*
 * Send query to name server and wait for reply.
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "nres.h"

extern int      errno;



#ifndef FD_SET
#define	NFDBITS		32
#define	FD_SETSIZE	32
#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))
#endif

#define KEEPOPEN (RES_USEVC|RES_STAYOPEN)
nres_xmit(tnr)
	struct nres    *tnr;
{
	char           *buf;
	int             buflen;
	int              v_circuit ;
	u_short         len;

	struct iovec    iov[2];

	buf = tnr->question;
	buflen = tnr->question_len;

#ifdef DEBUG
	if (_res.options & RES_DEBUG) {
		printf("nres_xmit()\n");
		p_query(buf);
	}
#endif DEBUG
	if (!(_res.options & RES_INIT))
		if (res_init() == -1) {
			return (-1);
		}
	v_circuit = (_res.options & RES_USEVC) || buflen > PACKETSZ;
	if (tnr->using_tcp)
		v_circuit = 1;
	if (v_circuit)
		tnr->using_tcp = 1;

#ifdef DEBUG
	if (_res.options & RES_DEBUG)
		printf("this is retry %d\n", tnr->retries);
#endif

	if (tnr->retries >= _res.retry) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("nres_xmit -- retries exausted %d\n", _res.retry);
#endif
		return (-1);
	}
	if (tnr->current_ns >= _res.nscount) {
		tnr->current_ns = 0;
		tnr->retries = tnr->retries + 1;
	}
	tnr->nres_rpc_as.as_timeout_remain.tv_sec = (_res.retrans << (tnr->retries)) / _res.nscount;
	tnr->nres_rpc_as.as_timeout_remain.tv_usec = 0; 
	if (tnr->nres_rpc_as.as_timeout_remain.tv_sec < 1)
		tnr->nres_rpc_as.as_timeout_remain.tv_sec = 1;

	for (; tnr->current_ns < _res.nscount; tnr->current_ns++) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("Querying server (# %d) address = %s\n", tnr->current_ns + 1,
			       inet_ntoa(_res.nsaddr_list[tnr->current_ns].sin_addr));
#endif DEBUG
		if (v_circuit) {

			/*
			 * Use virtual circuit.
			 */
			if (tnr->tcp_socket < 0) {
				tnr->tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
				if (tnr->tcp_socket < 0) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("socket failed");
#endif DEBUG
					if (tnr->udp_socket < 0) return(-1);
				}
				if (connect(tnr->tcp_socket,(struct sockaddr *) &(_res.nsaddr_list[tnr->current_ns]),
					    sizeof(struct sockaddr)) < 0) {
#ifdef DEBUG
					if (_res.options & RES_DEBUG)
						perror("connect failed");
#endif DEBUG
					(void) close(tnr->tcp_socket);
					tnr->tcp_socket = -1;
					continue;
				}
			}
			/*
			 * Send length & message
			 */
			len = htons((u_short) buflen);
			iov[0].iov_base = (caddr_t) & len;
			iov[0].iov_len = sizeof(len);
			iov[1].iov_base = tnr->question;
			iov[1].iov_len = tnr->question_len;
			if (writev(tnr->tcp_socket, iov, 2) != sizeof(len) + buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("write failed");
#endif DEBUG
				(void) close(tnr->tcp_socket);
				tnr->tcp_socket = -1;
				continue;
			}
			/* reply will come on tnr->tcp_socket */
		} else {
			/*
			 * Use datagrams.
			 */
			if (tnr->udp_socket < 0)
				tnr->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (tnr->udp_socket < 0) return(-1);

			if (sendto(tnr->udp_socket, buf, buflen, 0,(struct sockaddr *) &_res.nsaddr_list[tnr->current_ns],
				   sizeof(struct sockaddr)) != buflen) {
#ifdef DEBUG
				if (_res.options & RES_DEBUG)
					perror("sendto");
#endif DEBUG
				continue;
			} else {
				if (tnr->retries == 0)
					return (0);
			}
		}
	}
	return (0);
}
