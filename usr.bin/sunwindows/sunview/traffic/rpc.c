#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)rpc.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <sys/socket.h>
#include <rpcsvc/ether.h>
#include "traffic.h"
#include <netdb.h>

#define NUMERRS 10		/* number of rpc errs before giving up */
#define NUMPROTOS 0xff
/* 
 *  global variables
 */
static CLIENT *client;
static char *protostring[0xff];

/* 
 * returns 0 if ok, err otherwise
 */
initdevice()
{
	int err;
	
	if (running == 1)
		return (0);
	if (err = callrpc(host, ETHERSTATPROG, ETHERSTATVERS,
		ETHERSTATPROC_ON, xdr_void, 0, xdr_void, NULL)) {
		return (err);
	}
	running = 1;
	return (0);
}

/* 
 * not interested in reply, so use very short timeout
 */
deinitdevice()
{
	running = 0;
	mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS, ETHERSTATPROC_OFF,
	    xdr_void, 0, xdr_void, NULL);
}

setmask(am, proc)
	struct addrmask am;
{
	int err;
	
	if (err = callrpc(host, ETHERSTATPROG, ETHERSTATVERS, proc,
	    xdr_addrmask, &am, xdr_void, NULL)) {
		fprintf(stderr, "traffic: rpc call failed ");
		clnt_perrno(err);
		fprintf(stderr, "\n");
		return;
	}
}

/*
 *  convert string (which may be host or net address) to mask
 *  return true if conversion succeeded.
 */
traf_addrtomask(amaskp, str)
	struct addrmask *amaskp;
	char *str;
{
	struct hostent *hp;
	struct netent *np;
	int net, cnt, ok;
	char buf[80], *parts[4], *p, *q, *index();
	struct inputevent ie;

	/* 
	 *  string is in dot form
	 */
	ok = 1;
	if (index(str, '.')) {
		strcpy(buf, str);
		q = buf;
		cnt = 0;
		while (p = index(q, '.')) {
			parts[cnt] = q;
			cnt++;
			*p = 0;
			q = p+1;
		}
		parts[cnt] = q;
		amaskp->a_mask = 0xffff0000;
		amaskp->a_addr = strtoi(parts[0]) << 24;
		switch(cnt) {

		case 3:
			amaskp->a_addr |= (strtoi(parts[3]));
			amaskp->a_mask >>= 8;
			/* fall thru */
		case 2:
			amaskp->a_addr |= (strtoi(parts[2]) << 8);
			amaskp->a_mask >>= 8;
			/* fall thru */
		case 1:
			amaskp->a_addr |= (strtoi(parts[1]) << 16);
			break;
		default:
			sprintf(buf, "%s is ill formed address\n", str);
			traf_box(buf);
			ok = 0;
			amaskp->a_mask = 0;
			break;
		}
	}
	/* 
	 *  string is number representing host number
	 */
	else if (isdigit(str[0])) {
		amaskp->a_addr = htonl(strtoi(str));
		amaskp->a_mask = -1;
	}
	/* 
	 *  string is name of host or net
	 */
	else {
		hp = gethostbyname(str);
		if (hp == NULL) {
			np = getnetbyname(str);
			if (np == NULL) {
				sprintf(buf,
				    "%s unknown as host or net name\n", str);
				traf_box(buf);
				ok = 0;
				amaskp->a_mask = 0;
			}
			else {
				net = np->n_net;
				if (net <= 127) {
					amaskp->a_addr = net<<IN_CLASSA_NSHIFT;
					amaskp->a_mask = IN_CLASSA_NET;
				}
				else if (net <= 191) {
					amaskp->a_addr = net<<IN_CLASSB_NSHIFT;
					amaskp->a_mask = IN_CLASSB_NET;
				}
				else {
					amaskp->a_addr = net<<IN_CLASSC_NSHIFT;
					amaskp->a_mask = IN_CLASSC_NET;
				}
			}
		}
		else {
			amaskp->a_addr = *(int *)hp->h_addr;
			amaskp->a_mask = -1;
		}
	}
	return (ok);
}

initprototable()
{
	protostring[IPPROTO_ND] = "nd";
	protostring[IPPROTO_UDP] = "udp";
	protostring[IPPROTO_TCP] = "tcp";
	protostring[IPPROTO_ICMP] = "icmp";
}

/*
 *  convert string (which may be proto number or name) to mask
 *  return true if conversion succeeded.
 */

prototomask(amaskp, str)
	struct addrmask *amaskp;
	char *str;
{
	char buf[80];
	int num;
	register char **p, **q;
	struct inputevent ie;
	
	amaskp->a_mask = 0xff;
	if (isdigit(str[0])) {
		num = atoi(str);
		if (num >= 0 && num <= 0xff) {
			amaskp->a_addr = num;
			return (1);
		}
		else {
			amaskp->a_mask = 0;
			return (0);
		}
	}
	q = protostring + NUMPROTOS;
	for (p = protostring; p < q; p++) {
		if (*p && strcmp(*p, str) == 0) {
			amaskp->a_addr = p - protostring;
			return (1);
		}
	}
	sprintf(buf, "%s is unknown protocol\n", str);
	traf_box(buf);
	amaskp->a_mask = 0;
	return (0);
}
/*
 *  convert string (which may lnth or range) to mask
 *  return true if conversion succeeded.
 * a_addr is lowvalue, a_mask is high value
 */

lnthtomask(amaskp, str)
	struct addrmask *amaskp;
	char *str;
{
	char *p, *index();
	char buf[80];
	int lnth, low, high;
	struct inputevent ie;
	
	if (p = index(str, '-')) {
		low = atoi(str);
		high = atoi(p+1);
		if (low > high || low < MINPACKETLEN || high > MAXPACKETLEN) {
			sprintf(buf, "%s is invalid range\n", str);
			traf_box(buf);
			amaskp->a_mask = 0;
			return (0);
		}
		else {
			amaskp->a_addr = low;
			amaskp->a_mask = high;
			return (1);
		}
	}
	else {
		lnth = atoi(str);
		if (lnth >= MINPACKETLEN && lnth <= MAXPACKETLEN) {
			amaskp->a_addr = lnth;
			amaskp->a_mask = lnth;
			return (1);
		}
		else {
			sprintf(buf, "%s is invalid size\n", str);
			traf_box(buf);
			amaskp->a_mask = 0;
			return (0);
		}
	}
}

/* 
 * returns 0 if OK
 */
mycallrpc(h, prognum, versnum, procnum, inproc, in, outproc, out)
	char	*h;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	static int cnt;
	int err;
	
	err = shortcallrpc(h, prognum, versnum, procnum, inproc,
	    in, outproc, out);
	if (err) {
		cnt++;
		if (cnt > NUMERRS) {
			fprintf(stderr, "traffic: rpc call failed ");
			clnt_perrno(err);
			fprintf(stderr, "\n");
			exit(1);
		}
		return (1);
	}
	else {
		cnt = 0;
		return (0);
	}
}

/* 
 * just like callrpc, but with shorter timeout
 */
shortcallrpc(h, prognum, versnum, procnum, inproc, in, outproc, out)
	char *h;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval tout, tottout;

	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static char oldhost[256];

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& strcmp(oldhost, h) == 0) {
		/* reuse old client */		
	}
	else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		if ((hp = gethostbyname(h)) == NULL)
			return ((int) RPC_UNKNOWNHOST);
		tout.tv_sec = 0;
		tout.tv_usec = 800000;
		bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, tout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		strcpy(oldhost, h);
	}
	tottout.tv_sec = 2;
	tottout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	return ((int) clnt_stat);
}

freeargs(outproc, out)
	xdrproc_t outproc;
	char *out;
{
	clnt_freeres(client, outproc, out);
}

/*
 *  convert string representing number in some base to integer
 */
strtoi(str)
	char *str;
{
	int ans;
	
	if (str[0] == '0' && (040|str[1]) == 'x')
		sscanf(str+2, "%x", &ans);
	else if (str[0] == '0')
		sscanf(str, "%o", &ans);
	else
		sscanf(str, "%d", &ans);
	return ans;
}
