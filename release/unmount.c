#ifndef lint
static	char sccsid[] = "@(#)unmount.c 1.1 92/07/30 SMI";
#endif

char *name = "umount";

/* ARGSUSED */
main(argc, argv)
int	argc;
char	*argv[];
{
	int	uid;

	uid = geteuid();
	(void) setuid(uid);

	execvp(name, argv);
	perror("exec failed");
	exit(1);
}
