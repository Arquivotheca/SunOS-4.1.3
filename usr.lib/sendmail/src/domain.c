/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *  Sendmail
 *  Copyright (c) 1986  Eric P. Allman
 *  Berkeley, California
 */

# include "sendmail.h"

SCCSID(@(#)domain.c 1.1 92/07/30 SMI); /* from UCB 5.14 5/3/88 */

# include <arpa/nameser.h>
# include <resolv.h>
# include <netdb.h>

typedef union {
	HEADER qb1;
	char qb2[PACKETSZ];
} querybuf;

static char hostbuf[MAXMXHOSTS*PACKETSZ];
extern char *CurHostName;


/*
 * Look up Mail Exchanger records, first in cache, then to name server.
 * mxnumber of -1 means there was some error as indicated in the rcode.
 */
struct mxinfo *getmxrr(host, localhost, rcode)
	char *host, *localhost;
	int *rcode;
{

	register u_char *eom, *cp;
	register int i, j, n, nmx;
	register char *bp;
	HEADER *hp;
	querybuf answer;
	STAB *st;
	int ancount, qdcount, buflen, seenlocal;
	u_short pref, localpref, type, prefer[MAXMXHOSTS];
	register struct mxinfo *mx;
	
#ifdef DEBUG
	if (tTd(8, 2)) 
		_res.options |= RES_DEBUG;
#endif
	st = stab(host, ST_MX, ST_FIND);
	if (st != NULL) {
	    /*
	     * found in the cache, just return it.
	     */
	     mx = &st->s_value.sv_mxinfo;
	     AlreadyKnown = (mx->mx_number == -1);
	     if (mx->mx_number == -1)
	     	*rcode = mx->mx_rcode;
	     return(mx);
	}
	AlreadyKnown = FALSE;
	st = stab(host, ST_MX, ST_ENTER);
	mx = &st->s_value.sv_mxinfo;
	if (host[0] == '[') {
		/*
		 * Handle explicit IP addresses 
		 */
		goto punt;
	}

	n = res_search(host, C_IN, T_MX, (char *)&answer, sizeof(answer));
	if (n < 0) {
#ifdef DEBUG
		if (tTd(8, 1))
			printf("res_search failed (errno=%d, h_errno=%d)\n",
			    errno, h_errno);
#endif
		switch(h_errno) {
		case NO_DATA:
		case NO_RECOVERY:
			goto punt;
		case HOST_NOT_FOUND:
			*rcode = EX_NOHOST;
			break;
		case TRY_AGAIN:
			*rcode = EX_TEMPFAIL;
			errno = ENAMESER;
			CurHostName = host;
			break;
		}
		mx->mx_number = -1;
		mx->mx_rcode = *rcode;
		return(mx);
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	cp = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + n;
	for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
		if ((n = dn_skipname(cp, eom)) < 0)
			goto punt;
	nmx = 0;
	seenlocal = 0;
	buflen = sizeof(hostbuf);
	bp = hostbuf;
	ancount = ntohs(hp->ancount);
	while (--ancount >= 0 && cp < eom && nmx < MAXMXHOSTS) {
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		GETSHORT(type, cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		GETSHORT(n, cp);
		if (type != T_MX)  {
#ifdef DEBUG
			if (tTd(8, 3) || _res.options & RES_DEBUG)
				printf("answer type %d, size %d\n",
				    type, n);
#endif
			cp += n;
			continue;
		}
		GETSHORT(pref, cp);
		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		if (sameword(bp, localhost))
		{
			if (seenlocal == 0 || pref < localpref)
				localpref = pref;
			seenlocal = 1;
			continue;
		}
		prefer[nmx] = pref;
		mx->mx_hosts[nmx++] = newstr(bp);
		n = strlen(bp) + 1;
		bp += n;
		buflen -= n;
	}
	  /*
	   * Scan the response for useful additional records, too.
	   * Normally these will be address records for the forwarder,
	   * or the host itself (if there are no MX records).
	   */
	for (ancount = ntohs(hp->arcount);
	   --ancount >= 0 && cp < eom; cp += n) {
		struct in_addr sin, *addlist;
		u_short class;

		if ((n = dn_expand((char *)&answer, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		GETSHORT(type, cp);
		GETSHORT(class, cp);
 		cp += sizeof(u_long);
		GETSHORT(n, cp);
		if (type != T_A || class != C_IN)  {
#ifdef DEBUG
			if (tTd(8, 3) || _res.options & RES_DEBUG)
				printf("additional type %d, size %d\n",
					type, n);
#endif
			continue;
		}
		(void) bcopy(cp, &sin, n);
		st = stab(bp, ST_HOST, ST_FIND);
		if (st == NULL) {
		    /*
		     * not found in the cache, add it.
		     */
		    st = stab(bp, ST_HOST, ST_ENTER);
		    if (st == NULL)
			continue;
		    st->s_value.sv_host.h_addrlist[0].s_addr = INADDR_ANY;
		    st->s_value.sv_host.h_down = 0;
		}
		st->s_value.sv_host.h_valid = 1;
		st->s_value.sv_host.h_exists = 1;
		addlist = st->s_value.sv_host.h_addrlist;
		while (addlist->s_addr != INADDR_ANY) {
		    if (addlist->s_addr == sin.s_addr)
			break;
		    addlist++;
		}
		if (addlist->s_addr == INADDR_ANY) {
		    addlist++->s_addr = sin.s_addr;
		    addlist->s_addr = INADDR_ANY;
		}
	}
	if (nmx == 0) {
punt:		mx->mx_hosts[0] = newstr(host);
		mx->mx_number = 1;
		return(mx);
	}
	/* sort the records */
	for (i = 0; i < nmx; i++) {
		for (j = i + 1; j < nmx; j++) {
			if (prefer[i] > prefer[j]) {
				register int temp;
				register char *temp1;

				temp = prefer[i];
				prefer[i] = prefer[j];
				prefer[j] = temp;
				temp1 = mx->mx_hosts[i];
				mx->mx_hosts[i] = mx->mx_hosts[j];
				mx->mx_hosts[j] = temp1;
			}
		}
		if (seenlocal && prefer[i] >= localpref) {
			/*
			 * truncate higher pref part of list; if we're
			 * the best choice left, we should have realized
			 * awhile ago that this was a local delivery.
			 */
			if (i == 0) {
				*rcode = EX_SOFTWARE;
				mx->mx_number = -1;
				mx->mx_rcode = *rcode;
				return(mx);
			}
			nmx = i;
			break;
		}
	}
	mx->mx_number = nmx;
	return(mx);
}

getcanonname(host, hbsize)
	char *host;
	int hbsize;
{
	register u_char *eom, *cp;
	register int n; 
	HEADER *hp;
	querybuf answer;
	u_short type;
	int first, ancount, qdcount, loopcnt;
	char nbuf[PACKETSZ];

	loopcnt = 0;
loop:
	n = res_search(host, C_IN, T_CNAME, (char *)&answer, sizeof(answer));
	if (n < 0) {
#ifdef DEBUG
		if (tTd(8, 1))
			printf("getcanonname:  res_search failed (errno=%d, h_errno=%d)\n",
			    errno, h_errno);
#endif
		return;
	}

	/* find first satisfactory answer */
	hp = (HEADER *)&answer;
	ancount = ntohs(hp->ancount);

	/* we don't care about errors here, only if we got an answer */
	if (ancount == 0) {
#ifdef DEBUG
		if (tTd(8, 1))
			printf("rcode = %d, ancount=%d\n", hp->rcode, ancount);
#endif
		return;
	}
	cp = (u_char *)&answer + sizeof(HEADER);
	eom = (u_char *)&answer + n;
	for (qdcount = ntohs(hp->qdcount); qdcount--; cp += n + QFIXEDSZ)
		if ((n = dn_skipname(cp, eom)) < 0)
			return;

	/*
	 * just in case someone puts a CNAME record after another record,
	 * check all records for CNAME; otherwise, just take the first
	 * name found.
	 */
	for (first = 1; --ancount >= 0 && cp < eom; cp += n) {
		if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
		    sizeof(nbuf))) < 0)
			break;
		if (first) {			/* XXX */
			(void)strncpy(host, nbuf, hbsize);
			host[hbsize - 1] = '\0';
			first = 0;
		}
		cp += n;
		GETSHORT(type, cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		GETSHORT(n, cp);
		if (type == T_CNAME)  {
			/*
			 * assume that only one cname will be found.  More
			 * than one is undefined.  Copy so that if dn_expand
			 * fails, `host' is still okay.
			 */
			if ((n = dn_expand((char *)&answer, eom, cp, nbuf,
			    sizeof(nbuf))) < 0)
				break;
			(void)strncpy(host, nbuf, hbsize); /* XXX */
			host[hbsize - 1] = '\0';
			if (++loopcnt > 8)	/* never be more than 1 */
				return;
			goto loop;
		}
	}
}
