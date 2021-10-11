#ifndef lint
static	char sccsid[] = "@(#)mount_tmp.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <mntent.h>
#include <string.h>
#include <sys/mount.h>

#define MTAB	"/etc/mtab"

main(argc, argv)
	int argc;
	char *argv[];
{
	int kbytes;
	int bsize;
	char *args = NULL;
	struct mntent mnt;
	FILE *fp;
	
	if (argc != 5) {
		(void)fprintf(stderr, 
			      "usage: %s VM <dir> tmp \n", argv[0]);
		exit(1);
	}
	mnt.mnt_fsname = argv[1];
	mnt.mnt_dir = argv[2];
	mnt.mnt_type = argv[3];
	mnt.mnt_opts = NULL;
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;
	if (mount("tmp", mnt.mnt_dir, M_NEWTYPE, args) < 0) {
		perror(argv[0]);
		exit(1);
	}
	if ((fp = setmntent(MTAB,"r+")) == NULL) {
	    perror(argv[0]);
	    exit(1);
	}
	(void)addmntent(fp,&mnt);
	(void)endmntent(fp);
	exit(0);
	/* NOTREACHED */
}
