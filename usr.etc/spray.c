#ifndef lint
static  char sccsid[] = "@(#)spray.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <rpcsvc/spray.h>

#define	DEFBYTES 100000		/* default numbers of bytes to send */
#define	MAXPACKETLEN 1514

char	*adrtostr();
char	*host;
struct	in_addr adr;
int	lnth, cnt;
int	icmp;

main(argc, argv)
	char **argv;
{
	int	err, i;
	int	delay = 0;
	int	psec, bsec;
	int	buf[SPRAYMAX/4];
	struct	hostent *hp;
	struct	sprayarr arr;
	struct	spraycumul cumul;
	double	delta;

	if (argc < 2)
		usage();

	cnt = -1;
	lnth = SPRAYOVERHEAD;
	while (argc > 1) {
		if (argv[1][0] == '-') {
			switch (argv[1][1]) {
				case 'd':
					if (argc < 3)
						usage();
					delay = atoi(argv[2]);
					if (delay < 0)
						usage();
					argc--;
					argv++;
					break;
				case 'i':
					icmp++;
					break;
				case 'c':
					if (argc < 3)
						usage();
					cnt = atoi(argv[2]);
					if (cnt <= 0)
						usage();
					argc--;
					argv++;
					break;
				case 'l':
					if (argc < 3)
						usage();
					lnth = atoi(argv[2]);
					if (lnth <= 0)
						usage();
					argc--;
					argv++;
					break;
				default:
					usage();
			}
		} else {
			if (host)
				usage();
			else
				host = argv[1];
		}
		argc--;
		argv++;
	}
	if (host == NULL)
		usage();
	if (isinetaddr(host)) {
		adr.s_addr = htonl(inet_addr(host));
		host = adrtostr(adr);
	} else {
		if ((hp = gethostbyname(host)) == NULL) {
			fprintf(stderr, "%s is unknown host name\n", host);
			exit(1);
		}
		adr.s_addr = *((u_long *)hp->h_addr);
	}
	if (icmp)
		doicmp(adr, delay);
	if (cnt == -1)
		cnt = DEFBYTES/lnth;
	if (lnth < SPRAYOVERHEAD)
		lnth = SPRAYOVERHEAD;
	else if (lnth >= SPRAYMAX)
		lnth = SPRAYMAX;
	if (lnth <= MAXPACKETLEN && lnth % 4 != 2)
		lnth = ((lnth + 5)/4) * 4 - 2;
	arr.lnth = lnth - SPRAYOVERHEAD;
	arr.data = buf;
	printf("sending %d packets of lnth %d to %s ...", cnt, lnth, host);
	fflush(stdout);

	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_CLEAR,
	    xdr_void, NULL, xdr_void, NULL)) {
		fprintf(stderr, "SPRAYPROC_CLEAR ");
		clnt_perrno(err);
		return;
	}
	for (i = 0; i < cnt; i++) {
		callrpcnowait(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_SPRAY,
		    xdr_sprayarr, &arr, xdr_void, NULL);
		if (delay > 0)
			slp(delay);
	}
	if (err = mycallrpc(adr, SPRAYPROG, SPRAYVERS, SPRAYPROC_GET,
	    xdr_void, NULL, xdr_spraycumul, &cumul)) {
		fprintf(stderr, "SPRAYPROC_GET ");
		fprintf(stderr, "%s ", host);
		clnt_perrno(err);
		return;
	}
	delta = 1000000.0 * cumul.clock.tv_sec + cumul.clock.tv_usec;
	printf("\n\tin %.1f seconds elapsed time,\n", delta/1000000.0);
	if (cumul.counter < cnt) {
		printf("\t%d packets (%.2f%%) dropped\n",
			cnt - cumul.counter,
			100.0 * (cnt - cumul.counter)/cnt);
		psec = (1000000.0 * cnt) / delta;
		bsec = (lnth * 1000000.0 * cnt) / delta;
		printf("Sent:\t%d packets/sec, ", psec);
		if (bsec > 1024)
			printf("%.1fK", bsec / 1024.0);
		else
			printf("%d", bsec);
		printf(" bytes/sec\n");
		psec = (1000000.0 * cumul.counter) / delta;
		bsec = (lnth * 1000000.0 * cumul.counter) / delta;
		printf("Rcvd:\t%d packets/sec, ", psec);
		if (bsec > 1024)
			printf("%.1fK", bsec / 1024.0);
		else
			printf("%d", bsec);
		printf(" bytes/sec\n");
	} else {
		printf("\tno packets dropped\n");
		psec = (1000000.0 * cumul.counter) / delta;
		bsec = (lnth * 1000000.0 * cumul.counter) / delta;
		printf("\t%d packets/sec, ", psec, bsec);
		if (bsec > 1024)
			printf("%.1fK", bsec / 1024.0);
		else
			printf("%d", bsec);
		printf(" bytes/sec\n");
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * like callrpc, but with addr instead of host name
 */
mycallrpc(addr, prognum, versnum, procnum, inproc, in, outproc, out)
	struct in_addr addr;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static struct in_addr oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum &&
		adr.s_addr == oldadr.s_addr) {
		/* reuse old client */
	} else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 10;
		bzero(&server_addr, sizeof (server_addr));
		bcopy(&adr, &server_addr.sin_addr, sizeof (adr));
		server_addr.sin_family = AF_INET;
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
	struct in_addr adr;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;

	static CLIENT *client;
	static int socket = RPC_ANYSOCK;
	static int oldprognum, oldversnum, valid;
	static struct in_addr oldadr;

	if (valid && oldprognum == prognum && oldversnum == versnum &&
		oldadr.s_addr == adr.s_addr) {
		/* reuse old client */
	} else {
		close(socket);
		socket = RPC_ANYSOCK;
		if (client) {
			clnt_destroy(client);
			client = NULL;
		}
		timeout.tv_usec = 0;
		timeout.tv_sec = 0;
		bzero(&server_addr, sizeof (server_addr));
		bcopy(&adr, &server_addr.sin_addr, sizeof (adr));
		server_addr.sin_family = AF_INET;
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
	if (clnt_stat != RPC_SUCCESS && clnt_stat != RPC_TIMEDOUT) {
		fprintf(stderr, "spray: send error ");
		clnt_perrno(clnt_stat);
		valid = 0;
	}
	return ((int) clnt_stat);
}

char *
adrtostr(adr)
struct in_addr adr;
{
	struct hostent *hp;
	char *inet_ntoa();

	hp = gethostbyaddr(&adr, sizeof (adr), AF_INET);
	if (hp == NULL)
		return (inet_ntoa(adr));
	else
		return (hp->h_name);
}

slp(usecs)
{
	struct timeval tv;

	tv.tv_sec = usecs / 1000000;
	tv.tv_usec = usecs % 1000000;
	select(32, 0, 0, 0, &tv);
}

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <signal.h>

#define	MAXICMP (2082 - IPHEADER) /* experimentally determined to be max */
#define	IPHEADER	34	/* size of ether + ip header */
#define	MINICMP		8	/* minimum icmp length */

struct	timeval tv1, tv2;
int	pid, rcvd;
int	die(), done();

doicmp(adr, delay)
struct in_addr adr;
{
	char buf[MAXICMP];
	struct icmp *icp = (struct icmp *)buf;
	int	i, s;
	int	fromlen;
	struct sockaddr_in to, from;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
		perror("ping: socket");
		exit(1);
	}
	if (lnth >= IPHEADER + MINICMP)
		lnth -= IPHEADER;
	else
		lnth = MINICMP;
	if (lnth > MAXICMP) {
		fprintf(stderr, "%d is max packet size\n",
		    MAXICMP + IPHEADER);
		exit(1);
	}
	if (cnt == -1)
		cnt = DEFBYTES/(lnth+IPHEADER);
	to.sin_family = AF_INET;
	to.sin_port = 0;
	to.sin_addr = adr;
	icp->icmp_type = ICMP_ECHO;
	icp->icmp_code = 0;
	icp->icmp_cksum = 0;
	icp->icmp_id = 1;
	icp->icmp_seq = 1;
	icp->icmp_cksum = in_cksum(icp, lnth);

	printf("sending %d packets of lnth %d to %s ...",
		cnt, lnth + IPHEADER, host);
	fflush(stdout);

	if ((pid = fork()) < 0) {
	    perror("ping: fork");
	    exit(1);
	}
	if (pid == 0) {		/* child */
		sleep(1);	/* wait a second to give parent time to recv */
		for (i = 0; i < cnt; i++) {
			if (sendto(s, icp, lnth, 0, &to, sizeof (to)) != lnth) {
				fprintf(stderr, "\n");
				perror("ping: sendto");
				kill (pid, SIGKILL);
				exit(1);
			}
			if (delay > 0)
				slp(delay);
		}
		sleep(1);	/* wait for last echo to get thru */
		exit(0);
	}

	if (pid != 0) {		/* parent */
		signal(SIGCHLD, done);
		signal(SIGINT, die);
		rcvd = 0;
		for (i = 0; ; i++) {
			fromlen = sizeof (from);
			if (recvfrom(s, buf, sizeof (buf), 0,
				&from, &fromlen) < 0) {
				perror("ping: recvfrom");
				continue;
			}
			if (i == 0)
				gettimeofday(&tv1, 0);
			else if (i == cnt - 1)
				gettimeofday(&tv2, 0);
			rcvd++;
		}
	}
}

in_cksum(addr, len)
	u_short *addr;
	int len;
{
	register u_short *ptr;
	register int sum;
	u_short *lastptr;

	sum = 0;
	ptr = (u_short *)addr;
	lastptr = ptr + (len/2);
	for (; ptr < lastptr; ptr++) {
		sum += *ptr;
		if (sum & 0x10000) {
			sum &= 0xffff;
			sum++;
		}
	}
	return (~sum & 0xffff);
}

die()
{
	kill (pid, SIGKILL);
	exit(1);
}

done()
{
	int	psec, bsec;

	if (tv2.tv_usec == 0 && tv2.tv_usec == 0) { /* estimate */
		gettimeofday(&tv2, 0);
	}
	if (rcvd != cnt)
		printf("\n\t%d packets (%.3f%%) dropped\n",
		    cnt - rcvd, 100.0 * (cnt - rcvd)/cnt);
	else
		printf("\n\tno packets dropped\n");
	if (tv2.tv_usec < tv1.tv_usec) {
		tv2.tv_usec += 1000000;
		tv2.tv_sec -= 1;
	}
	tv2.tv_sec -= tv1.tv_sec;
	tv2.tv_usec -= tv1.tv_usec;
	psec = (1000000.0 * cnt) / (1000000.0 * tv2.tv_sec + tv2.tv_usec);
	bsec = ((lnth + IPHEADER) * 1000000.0 * cnt)/
			(1000000.0 * tv2.tv_sec + tv2.tv_usec);
	printf("\t%d packets/sec, %d bytes/sec\n", psec, bsec);
	exit(0);
}

usage()
{
	fprintf(stderr,
		"Usage: spray [-i] [-c cnt] [-l lnth] [-d usecs] host\n%s",
		"       cnt > 0 AND lnth > 0 AND usecs >= 0\n");
	exit(1);
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
	while (*str) {
		if (((*str >= '0') && (*str <= '9')) || (*str == '.'))
			str++;
		else
			return (FALSE);
	}
	return (TRUE);
}
