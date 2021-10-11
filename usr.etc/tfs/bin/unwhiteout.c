#ifndef lint
static char sccsid[] = "@(#)unwhiteout.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <nse/util.h>
#include <nse/tfs_calls.h>

main(argc, argv)
	int		argc;
	char		*argv[];
{
	int		i;
	char		*fname;
	Nse_err		result;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s file1 file2 ...\n", argv[0]);
		exit(-1);
	}
	nse_set_cmdname(argv[0]);
	for (i = 1; i < argc; i++) {
		fname = argv[i];
		if (result = nse_unwhiteout((char *) NULL, fname)) {
			nse_err_print(result);
		}
	}
}
