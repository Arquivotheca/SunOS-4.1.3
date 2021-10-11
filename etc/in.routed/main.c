/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static	char sccsid[] = "@(#)main.c 1.1 92/07/30 SMI"; /* from UCB 5.7 4/20/86 */
#endif not lint

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/ioctl.h>
#include <sys/time.h>

#include <net/if.h>

#include <errno.h>
#include <signal.h>
#include <syslog.h>

int	maysupply = -1;		/* process may supply updates */
int	supplier = -1;		/* process should supply updates */
int	gateway = 0;		/* 1 if we are a gateway to parts beyond */

struct	rip *msg = (struct rip *)packet;
int	hup();

main(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr from;
	u_char retry;
	
	argv0 = argv;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(IPPORT_ROUTESERVER);
	s = getsocket(AF_INET, SOCK_DGRAM, &addr);
	if (s < 0)
		exit(1);
	argv++, argc--;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-s") == 0) {
			maysupply = 1;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-q") == 0) {
			maysupply = 0;
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-v") == 0) {
			argv++, argc--;
			tracing |= ACTION_BIT;
			continue;
		}
		if (strcmp(*argv, "-t") == 0) {
			tracepackets++;
			argv++, argc--;
			tracing |= INPUT_BIT;
			tracing |= OUTPUT_BIT;
			continue;
		}
		if (strcmp(*argv, "-d") == 0) {
			argv++, argc--;
			continue;
		}
		if (strcmp(*argv, "-g") == 0) {
			gateway = 1;
			argv++, argc--;
			continue;
		}
		fprintf(stderr,
			"usage: %s [ -s ] [ -q ] [ -t ] [-g]\n", argv0[0]);
		exit(1);
	}
#ifndef DEBUG
	if (!tracepackets) {
		int t;

		if (fork())
			exit(0);
		for (t = 0; t < 20; t++)
			if (t != s)
				(void) close(t);
		(void) open("/", 0);
		(void) dup2(0, 1);
		(void) dup2(0, 2);
		t = open("/dev/tty", 2);
		if (t >= 0) {
			ioctl(t, TIOCNOTTY, (char *)0);
			(void) close(t);
		}
	}
#endif
	/*
	 * Any extra argument is considered
	 * a tracing log file.
	 */
	if (argc > 0)
		traceon(*argv);
	/*
	 * Collect an initial view of the world by
	 * checking the interface configuration and the gateway kludge
	 * file.  Then, send a request packet on all
	 * directly connected networks to find out what
	 * everyone else thinks.
	 */
	rtinit();
	gwkludge();
	ifinit();
	if (gateway > 0)
		rtdefault();
	msg->rip_cmd = RIPCMD_REQUEST;
	msg->rip_vers = RIPVERSION;
	msg->rip_nets[0].rip_dst.sa_family = AF_UNSPEC;
	msg->rip_nets[0].rip_metric = HOPCNT_INFINITY;
	msg->rip_nets[0].rip_dst.sa_family = htons(AF_UNSPEC);
	msg->rip_nets[0].rip_metric = htonl(HOPCNT_INFINITY);
	toall(sendpacket);
	signal(SIGALRM, timer);
	signal(SIGHUP, hup);
	signal(SIGTERM, hup);
	timer();

	for (;;) {
		int ibits;
		register int n;

		ibits = 1 << s;
		n = select(20, &ibits, 0, 0, 0);
		if (n < 0)
			continue;
		if (ibits & (1 << s))
			process(s);
		/* handle ICMP redirects */
	}
}

process(fd)
	int fd;
{
	struct sockaddr from;
	int fromlen = sizeof (from), cc, omask;

	cc = recvfrom(fd, packet, sizeof (packet), 0, &from, &fromlen);
	if (cc <= 0) {
		if (cc < 0 && errno != EINTR)
			perror("recvfrom");
		return;
	}
	if (fromlen != sizeof (struct sockaddr_in))
		return;
	omask = sigblock(sigmask(SIGALRM));
	rip_input(&from, cc);
	sigsetmask(omask);
}

getsocket(domain, type, sin)
	int domain, type;
	struct sockaddr_in *sin;
{
	int s, on = 1;

	if ((s = socket(domain, type, 0)) < 0) {
		perror("socket");
		return (-1);
	}
/*	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof (on)) < 0) {
		perror("setsockopt SO_BROADCAST");
		close(s);
		return (-1);
	}
*/
	if (bind(s, sin, sizeof (*sin), 0) < 0) {
		perror("bind");
		close(s);
		return (-1);
	}
	return (s);
}
