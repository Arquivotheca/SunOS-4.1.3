#ifndef lint
static char sccsid[] = "@(#)in.timed.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/* 
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

/*
 * in.timed  - Time server
 *
 *	Returns with time of day
 *	in seconds since Jan 1, 1900.  This is 
 *	86400(365*70 + 17) more than time
 *	since Jan 1, 1970, which is what get/settimeofday
 *	uses.  Called from inetd, so uses fd 0 as socket.
 */
#define	TOFFSET	(86400*(365*70 + 17))

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>

main()
{
	int time;
	struct timeval timestruct;

	if (gettimeofday(&timestruct, 0) == -1)
		_exit(1);
	time = htonl(timestruct.tv_sec + TOFFSET);
	if (write(0, (char *)&time, sizeof (int)) != sizeof (int)) {
		/* maybe a udp socket */
		int fds = 1;
		struct sockaddr from;

		timestruct.tv_sec = timestruct.tv_usec = 0;
		if ((select(32, &fds, 0, 0, &timestruct) > 0) && (fds == 1)) {
			fds = sizeof (from);
			if (recvfrom(0, &timestruct, sizeof (timestruct), 0,
			    &from, &fds) >= 0) {
				sendto(0, &time, sizeof (int), 0, &from, fds);
			}
		}
	}
	if (close(0) == -1)
		_exit(1);
}
