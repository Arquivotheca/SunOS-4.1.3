
#ifndef lint
static	char sccsid[] = "@(#)mkSCCSdir.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <strings.h>

char	*srcbase = "/usr/src/";
char	*SCCSbase = "/usr/src/SCCS_DIRECTORIES";
char	*SCCSdir = "/SCCS";
char	pathname[MAXPATHLEN];
char	SCCSdirpath[MAXPATHLEN];
char	string[(MAXPATHLEN * 2) + 32];


main(argc, argv)
int	argc;
char	*argv[];
{
	char	*dir; 
	char	*ptr;
	char	*dirname;

	if (argc != 2) {
		fprintf(stderr, "No directory specified.\n");
		exit(1);
	}

	if (strncmp(argv[1], srcbase, strlen(srcbase)) != 0) {
		fprintf(stderr, "Directory must be in \"%s\".\n", srcbase);
		exit(1);
	}

	dir = argv[1] + strlen(srcbase);

	ptr = dir;
	strcpy(pathname, SCCSbase);
	strcat(SCCSdirpath, "../");
	while ((ptr = index((ptr + 1), '/')) != NULL) {
		dirname = ptr - 1;
		while (*dirname-- != '/');
		strncat(pathname, (dirname + 1), (ptr - dirname - 1));
		test_for_directory(pathname);
		strcat(SCCSdirpath, "../");
	}

	if ((ptr = rindex(dir, '/')) != NULL) {
		strcat(pathname, ptr);
	} else {
		strcat(pathname, "/");
		strcat(pathname, dir);
	}
	test_for_directory(pathname);

	strcat(pathname, SCCSdir);
	sprintf(string, "mkdir %s", pathname);
	system(string);

	strcat(SCCSdirpath, (pathname + strlen(srcbase)));
	sprintf(string, "(cd %s; ln -s %s)", argv[1], SCCSdirpath);
	system(string);
		
}


test_for_directory(dir)
char	*dir;
{
	struct stat	buf;

	if (stat(dir, &buf) < 0) {
		sprintf(string, "mkdir %s", dir);
		system(string);
	} 
}
