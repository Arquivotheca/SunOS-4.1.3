#ifndef lint
#ident "@(#)xxx.c 1.1 92/07/30 SMI"
#endif
/*
 * xxx.c	Copyright 1992 Sun Microsystems, Inc.
 *
 * statically linked helper program for stub
 * implements (toy versions of):
 *	umount <mountpoint_name>
 *	rm <file>
 *	rmdir <dir>
 *	reboot <bootargs>
 *	sync <dummy>
 *
 * usage: xxx cmd_name [args]
 */
#include <sys/reboot.h>

char um[] = "usage: xxx umount|rm|rmdir|reboot|sync arg\n";

main(argc, argv)
int	argc;
char	*argv[];
{
	int	rc;

	if (argc != 3)
		goto usage;

	if (strcmp(argv[1], "umount") == 0) {
		rc = unmount(argv[2]);
	} else
	if (strcmp(argv[1], "rm") == 0) {
		rc = unlink(argv[2]);
	} else
	if (strcmp(argv[1], "rmdir") == 0) {
		rc = rmdir(argv[2]);
	} else
	if (strcmp(argv[1], "reboot") == 0) {
		rc = reboot(RB_STRING, argv[2]);
	} else
	if (strcmp(argv[1], "sync") == 0) {
		rc = sync();
		sleep(4);
	} else {
usage:
		write(2, um, sizeof(um));
		exit (1);
	}
	exit (rc);
}
