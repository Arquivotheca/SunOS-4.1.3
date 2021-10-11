/*      @(#)mount_hsfs.c 1.1 92/07/30 SMI */

/*
 * mount_hsfs: fsname dir type opts
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */


#include <sys/mount.h>
#include <sys/errno.h>
#include <stdio.h>
#include <mntent.h>

extern int errno;

main(argc, argv)
int argc;
char **argv;
{
int 	flags;
struct hsfs_args {
	char *fspec;
	int norrip;
} data;
struct mntent mnt;

	if (argc != 5) {
		(void) fprintf(stderr, "mount_hsfs: bad arguments\n");
		exit(1);
	}

	if (strcmp(argv[3], "hsfs") == 0);
	else {
		(void) fprintf(stderr, "mount_hsfs: not hsfs\n");
		exit(1);
	}

	mnt.mnt_opts=argv[4];

	if (hasmntopt(&mnt, MNTOPT_RO) == NULL) {
		(void) fprintf(stderr,
			"mount_hsfs: must be mounted readonly\n");
		exit(1);
	}

	/* argv[1] = fsname(device); */
	/* argv[2] = mountdir; */
	/* argv[3] = type; */
	/* argv[4] = options; */

	flags = M_NEWTYPE;
	data.fspec= argv[1];
	data.norrip = 0;

	flags |= (hasmntopt(&mnt, MNTOPT_REMOUNT) != NULL) ? M_REMOUNT : 0;
	flags |= (hasmntopt(&mnt, MNTOPT_RO) != NULL) ? M_RDONLY : 0;
	flags |= (hasmntopt(&mnt, MNTOPT_NOSUID) != NULL) ? M_NOSUID : 0;

	if( hasmntopt(&mnt, "norrip") != NULL )
		data.norrip = 1;

	if (mount(argv[3], argv[2], flags, &data) < 0) {
		(void) fprintf(stderr, "mount_hsfs: %s on ", argv[1]);
		(void) perror(argv[2]);
		exit(errno);
	} else exit(0);
	/* NOTREACHED */

}
