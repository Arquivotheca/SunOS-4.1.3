/*	@(#)hsfs.h 1.1 92/07/30 SMI  */

/*
 * Interface definitions for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

struct hsfs_args {
	char *fspec;	/* name of filesystem to mount */
	int norrip;
};
