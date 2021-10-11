/*	@(#)where_main.c 1.1 92/07/30 SMI	*/
#include <stdio.h>
#include <sys/param.h>

main(argc, argv)
	char **argv;
{
	char host[MAXPATHLEN];
	char fsname[MAXPATHLEN];
	char within[MAXPATHLEN];
	char *pn;
	int many;

	many = argc > 2;
	while (--argc > 0) {
		pn = *++argv;
		where(pn, host, fsname, within);
		if (many)
			printf("%s:\t", pn);
		printf("%s:%s%s\n", host, fsname, within);
	}
	exit(0);
	/* NOTREACHED */
}

