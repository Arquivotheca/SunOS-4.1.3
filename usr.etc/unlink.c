#ifndef lint
static	char sccsid[] = "@(#)unlink.c 1.1 92/07/30 SMI"; /* from S5R2 1.1 */
#endif

main(argc, argv) char *argv[]; {
	if(argc!=2) {
		write(2, "Usage: unlink name\n", 24);
		exit(1);
	}
	unlink(argv[1]);
	exit(0);
	/* NOTREACHED */
}
