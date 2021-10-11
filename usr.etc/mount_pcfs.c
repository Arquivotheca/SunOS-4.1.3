/*	@(#)mount_pcfs.c 1.1 92/07/30 SMI */

/*
 * mount_pcfs: fsname dir type opts
 * Copyright (c) 1990 by Sun Microsystem, Inc.
 */


#include <sys/mount.h>
#include <sys/errno.h>
#include <stdio.h>
#include <mntent.h>

extern int errno;
char *progname = "mount_pcfs";

main(argc, argv)
int argc;
char **argv;
{
	int 	flags;
	struct pcfs_args {
		char *fspec;
	} data;
	struct mntent mnt;

	if( argc > 0 ) {
	    progname = argv[0];
	}

	if (argc != 5) {
		(void) fprintf(stderr,
			"%s: bad arguments\n", progname);
		(void) fprintf(stderr,
			"\tUsage: %s fsname mountdir type options\n",
			progname);
		exit(1);
	}

	mnt.mnt_opts=argv[4];

	/* argv[1] = fsname(device); */
	/* argv[2] = mountdir; */
	/* argv[3] = type; */
	/* argv[4] = options; */

	flags = M_NEWTYPE;
	data.fspec= argv[1];

	flags |= (hasmntopt(&mnt, MNTOPT_REMOUNT) != NULL) ? M_REMOUNT : 0;
	flags |= (hasmntopt(&mnt, MNTOPT_RO) != NULL) ? M_RDONLY : 0;
	flags |= (hasmntopt(&mnt, MNTOPT_NOSUID) != NULL) ? M_NOSUID : 0;

	if (mount(argv[3], argv[2], flags, &data) < 0) {
		(void) fprintf(stderr, "%s: %s on %s type ",
			progname, argv[1], argv[2]);
		(void) perror(argv[3]);
		exit(errno);
	} else exit(0);
	/* NOTREACHED */

}
