#if defined (LIBC_SCCS) && !defined(lint)
static	char sccsid[] = "@(#)nget_answer.c 1.1 92/07/30 1989";
#endif
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <strings.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "nres.h"

#ifndef NO_DATA
#define NO_DATA NO_ADDRESS
#endif

#define	MAXALIASES	35
#define	MAXADDRS	35

static char    *h_addr_ptrs[MAXADDRS + 1];

static struct hostent host;
static char    *host_aliases[MAXALIASES];
static char     hostbuf[BUFSIZ + 1];

typedef union {
	HEADER          hdr;
	u_char          buf[MAXPACKET];
}               querybuf;

static union {
	long            al;
	char            ac;
}               align;


extern          errno;

struct hostent *
nres_getanswer(temp)
struct nres * temp;
{
	querybuf       *answer;
	int             anslen;
	int             iquery;
	register HEADER *hp;
	register u_char *cp;
	register int    n;
	u_char         *eom;
	char           *bp, **ap;
	int             type, class, buflen, ancount, qdcount;
	int             haveanswer, getclass = C_ANY;
	char          **hap;

	answer = (querybuf *) temp->answer;
	anslen = temp->answer_len;
	iquery = (temp->reverse == REVERSE_PTR);

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = answer->buf + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = dn_expand((char *) answer->buf, eom,
					   cp, bp, buflen)) < 0) {
				temp->h_errno = NO_RECOVERY;
				return ((struct hostent *) NULL);
			}
			cp += n + QFIXEDSZ;
			host.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			temp->h_errno = HOST_NOT_FOUND;
		else
			temp->h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
	ap = host_aliases;
	host.h_aliases = host_aliases;
	hap = h_addr_ptrs;
#if BSD >= 43 || defined(h_addr)/* new-style hostent structure */
	host.h_addr_list = h_addr_ptrs;
#endif
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = dn_expand((char *) answer->buf, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = _getshort(cp);
		cp += sizeof(u_short);
		class = _getshort(cp);
		cp += sizeof(u_short) + sizeof(u_long);
		n = _getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &host_aliases[MAXALIASES - 1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (iquery && type == T_PTR) {
			if ((n = dn_expand((char *) answer->buf, eom,
					   cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			cp += n;
			host.h_name = bp;
			return (&host);
		}
		if (iquery || type != T_A) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
				       type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != host.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			host.h_length = n;
			getclass = class;
			host.h_addrtype = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				host.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += sizeof(align) - ((u_long) bp % sizeof(align));

		if (bp + n >= &hostbuf[sizeof(hostbuf)]) {
#ifdef DEBUG
			if (_res.options & RES_DEBUG)
				printf("size (%d) too big\n", n);
#endif
			break;
		}
		bcopy((char *)cp, *hap++ = bp, n);
		bp += n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		*ap = NULL;
#if BSD >= 43 || defined(h_addr)/* new-style hostent structure */
		*hap = NULL;
#else
		host.h_addr = h_addr_ptrs[0];
#endif
		return (&host);
	} else {
		temp->h_errno = TRY_AGAIN;
		return ((struct hostent *) NULL);
	}
}

nres_chkreply(temp)
struct nres * temp;
{
	char           *answer;
	int             anslen;
	HEADER         *hp;
	answer = temp -> answer;
	anslen = temp -> answer_len;
	if (anslen <= 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("nres_chkreply: send error\n");
#endif
		temp->h_errno = TRY_AGAIN;
		return (anslen);
	}
	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#ifdef DEBUG
		if (_res.options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode,
			       ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
		case NXDOMAIN:
			temp->h_errno = HOST_NOT_FOUND;
			break;
		case SERVFAIL:
			temp->h_errno = TRY_AGAIN;
			break;
		case NOERROR:
			temp->h_errno = NO_DATA;
			break;
		case FORMERR:
		case NOTIMP:
		case REFUSED:
		default:
			temp->h_errno = NO_RECOVERY;
			break;
		}
		return (-1);
	}
	return (anslen);
}
