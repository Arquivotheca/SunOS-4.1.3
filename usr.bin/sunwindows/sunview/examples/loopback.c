#ifndef lint
static char sccsid[] = "@(#)loopback.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

#define bit(c)	(1 << (c))

main(argc, argv)
int argc;
char **argv;
{
	int fds, buflen, fdin, fdout;
	char buf[512];

	if (argc != 3) {
		write(1, "Usage: loopback fdin fdout\n", 27);
		sleep(5);
		exit(1);
	}
	fdin = argv[1][0];
	fdout = argv[2][0];
	for (;;) {
		fds = bit(0) | bit(fdin);
		if (select(32, &fds, 0, 0, 0) < 0)
			continue;
		if (fds & bit(0)) {
			buflen = read(0, buf, 512);
			if (buflen > 0)
				write(fdout, buf, buflen);
		}
		if (fds & bit(fdin)) {
			buflen = read(fdin, buf, 512);
			if (buflen > 0)
				write(1, buf, buflen);
		}
	}

	/* NOTREACHED */
}
