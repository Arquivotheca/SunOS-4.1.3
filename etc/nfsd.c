#ifndef lint
static	char sccsid[] = "@(#)nfsd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

/* NFS server */

#include <sys/param.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <nfs/nfs.h>
#include <stdio.h>
#include <signal.h>
extern int errno;

catch()
{
}

main(argc, argv)
char	*argv[];
{
	register int sock;
	struct sockaddr_in addr;
	int len = sizeof(struct sockaddr_in);
	char *dir = "/";
	int nservers = 1;
	int pid, t;

	if (argc > 2) {
		fprintf(stderr, "usage: %s [servers]\n", argv[0]);
		exit(1);
	}
	if (argc == 2) {
		nservers = atoi(argv[1]);
	}

	/*
	 * Set current and root dir to server root
	 */
	if (chroot(dir) < 0) {
		perror(dir);
		exit(1);
	}
	if (chdir(dir) < 0) {
		perror(dir);
		exit(1);
	}

#ifndef DEBUG
	/*
	 * Background 
	 */
        pid = fork();
	if (pid < 0) {
		perror("nfsd: fork");
		exit(1);
	}
	if (pid != 0)
		exit(0);

	{ int s;
	for (s = 0; s < 10; s++)
		(void) close(s);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
#endif
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}

	addr.sin_addr.s_addr = 0;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(NFS_PORT);
	if ( ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	    || (bind(sock, &addr, len) != 0)
	    || (getsockname(sock, &addr, &len) != 0) ) {
		(void)close(sock);
		fprintf(stderr, "%s: server already active", argv[0]);
		exit(1);
	}
	/* register with the portmapper */
	pmap_unset(NFS_PROGRAM, NFS_VERSION);
	pmap_set(NFS_PROGRAM, NFS_VERSION, IPPROTO_UDP, NFS_PORT);
	while (--nservers) {
		if (!fork()) {
			break;
		}
	}
	signal(SIGTERM, catch);
	nfssvc(sock);
	/* Should never come here */
	exit(errno);
	/* NOTREACHED */
}

