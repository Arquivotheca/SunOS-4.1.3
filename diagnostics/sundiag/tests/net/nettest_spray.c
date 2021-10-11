#ifndef lint
static  char sccsid[] = "@(#)nettest_spray.c	1.1 7/30/92 17:35:54, Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/**************************************************************************
  nettest_spray.c

  nettest_spray sends a one-way stream of packets to "host" using RPC  
  (host can either be a name or an internet address).  Nettest_spray 
  returns the percentage of packets that were dropped, otherwise, exits 
  if an error occurs.  If verbose mode is specified, nettest_spray reports 
  the number of packets that were dropped as well as the transfer rate.  
**************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <varargs.h>
#include "sdrtns.h"
#include "nettest.h"
#include "nettest_spray.h"

char	*adrtostr();
char	*host;			/* host to send packets to */
int	adr;

nettest_spray(from, cnt, lnth, delay)
struct  sockaddr_in from;       /* address of the responding node  */
int   cnt; 			/* cnt = numbers of packets to send */
int   lnth;			/* lnth = number of bytes in the ethernet */
                                /* packet that holds the RPC call message  */
int   delay; 			/* user specific command that indicates */
                                /* how many microseconds to pause between */
                                /* sending each packet, default is 0 */
{
	float   packets_dropped = 0;
	int	err, i;
	int	psec, bsec;        /* packets/second & bytes/second rates */
        int     small_spray = FALSE;
	int	buf[SPRAYMAX/4];
	struct	hostent *hp;
	struct	sprayarr arr;	
	struct	spraycumul cumul;
	char*   err_msg;

	func_name = "nettest_spray";
	TRACE_IN

        if ((hp = gethostbyaddr (&from.sin_addr.s_addr,
             sizeof (struct in_addr),AF_INET)) == NULL)	
	{
             send_message(-NO_HOST_NAME, ERROR,"%s: Address trying to spray to is invalid.", func_name);
	}

	host = hp->h_name;

	adr = *((int *)hp->h_addr);

	if (lnth == 86)
        {
                lnth = SPRAYOVERHEAD * 2;
		small_spray = TRUE;
	}

	arr.lnth = lnth - SPRAYOVERHEAD;
	arr.data = buf;
	send_message(0, VERBOSE, "Sending %d packets of length %d bytes each to %s ...", cnt, (small_spray ? lnth/2 : lnth) , host);

	/* set up host to be sprayed to */
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_CLEAR,
	    xdr_void, NULL, xdr_void, NULL)) {
		err_msg = clnt_sperrno(err);
		send_message(0, VERBOSE, "SPRAYPROC_CLEAR : %s when trying to spray to %s", err_msg, host);
		TRACE_OUT
		return(-1);
	}

	/* spraying host */
	for (i = 0; i < cnt; i++) {
		callrpcnowait(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_SPRAY,
		    xdr_sprayarr, &arr, xdr_void, NULL);
		if (delay > 0)
			slp(delay);
	}

        /* getting the number of packets received by host & store into cumul */ 
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_GET,
	    xdr_void, NULL, xdr_spraycumul, &cumul)) {
		err_msg = clnt_sperrno(err);
		send_message(0, VERBOSE, "SPRAYPROC_GET : %s when trying to get the number of packets received after spraying to %s", err_msg, host);
                TRACE_OUT
                return(-1);
	}

	psec = (1000000.0 * cumul.counter)
	    / (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);
	bsec = (lnth * 1000000.0 * cumul.counter)/
	    (1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec);

	/* if the number of packets received is less than the number of */
        /* packets "sprayed", find percentage of packets dropped */
	if (cumul.counter < cnt) {
                packets_dropped = 100.0*(cnt - cumul.counter)/cnt;
		send_message(0, VERBOSE,"%d packets (%.3f%%) dropped by %s.   %d packets/sec, %d bytes/sec", cnt - cumul.counter, packets_dropped, host, psec, bsec);
	}
	else {
		send_message(0, VERBOSE, "no packets dropped by %s %d packets/sec, %d bytes/sec", host, psec, bsec);
                packets_dropped = 0;
	}
	TRACE_OUT
	return(packets_dropped);
}

/* 
 * like callrpc, but with addr instead of host name
 */
mycallrpc(addr, prognum, versnum, procnum, inproc, in, outproc, out)
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;  	/* client refers to host passed in */
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& adr == oldadr) {
		/* reuse old client */		
	}
	else {
		/* close old client and set up new client with clntudp_create */
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 10;
		bcopy(&adr, &server_addr.sin_addr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
	}
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		valid = 0;
	return ((int) clnt_stat);
}

callrpcnowait(adr, prognum, versnum, procnum, inproc, in, outproc, out)
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static int oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum
		&& oldadr == adr) {
		/* reuse old client */		
	}
	else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
		bcopy(&adr, &server_addr.sin_addr, sizeof(adr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port =  0;
		if ((client = clntudp_create(&server_addr, prognum,
		    versnum, timeout, &socket)) == NULL)
			return ((int) rpc_createerr.cf_stat);
		valid = 1;
		oldprognum = prognum;
		oldversnum = versnum;
		oldadr = adr;
	}
	tottimeout.tv_sec = 0;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
	/* 
	 * if call failed, empty cache
	 * since timeout is zero, normal return value is RPC_TIMEDOUT
	 */
	if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT)
		valid = 0;
	return ((int) clnt_stat);
}

char *
adrtostr(adr)
int adr;
{
	struct hostent *hp;
	char buf[100];		/* hope this is long enough */
	
	hp = gethostbyaddr(&adr, sizeof(adr), AF_INET);
	if (hp == NULL) {
	    	sprintf(buf, "0x%x", adr);
		return buf;
	}
	else
		return hp->h_name;
}

slp(usecs)
{
	struct timeval tv;
	
	tv.tv_sec = usecs / 1000000;
	tv.tv_usec = usecs % 1000000;
	select(32, 0, 0, 0, &tv);
}


/*
 * A better way to check for an inet address : scan the entire string for
 * nothing but . and digits. If a letter is found return FALSE. Yes, you can
 * get some degenerate cases by it, but who names a host with *all* numbers?
 */

int
isinetaddr(str)

char	*str;

{
	int	i;
	while (*str) 
		if (((*str >= '0') && (*str <= '9')) || (*str == '.')) str++;
		else return(FALSE);
	return(TRUE);
}
