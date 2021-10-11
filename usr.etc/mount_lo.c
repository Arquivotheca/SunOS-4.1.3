#ifndef lint
static  char sccsid[] = "@(#)mount_lo.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#define LOFS
/*
 * mount
 */
#include <stdio.h>
#include <sys/errno.h>
#include <mntent.h>
#include <sys/mount.h>

/*
 * usage: mount_lofs fsname dir type opts
 */
main(argc, argv)
	int argc;
	char **argv;
{
	char *dir, *type, *opts;
	extern int errno;
	struct mntent mnt;
	int flags;
	struct lo_args {
		char *fsdir;
	} args;

	if (argc != 5) {
		fprintf(stderr, "Usage:  %s fsname directory \"lo\" mountoptions\n", argv[0]);
		exit(EINVAL);
	}

	args.fsdir = argv[1];
	dir = argv[2];
	type = argv[3];
	mnt.mnt_opts = argv[4];
	flags = hasmntopt(&mnt, MNTOPT_RO) ? M_RDONLY : 0;

	if (mount(type, dir, flags | M_NEWTYPE, &args) < 0) {
		fprintf(stderr, "%s:  mount %s on %s:  ", argv[0], args.fsdir, dir);
		perror("");
		exit (errno);
	}
	exit (0);
	/* NOTREACHED */
}
