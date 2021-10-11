#ifndef lint
static	char sccsid[] = "@(#)devnm.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

main(argc, argv)
char	**argv;
{

	struct stat sbuf;
	struct direct *dbuf;
	DIR	*dv, *dv1, *dv2;
	short	fno;
	short found;
	char fname[MAXNAMLEN+20];

	if ((dv = opendir("/dev")) == NULL) {
		fprintf(stderr, "Cannot open /dev\n");
		exit(1);
	}
	dv1 = opendir("/dev/dsk");
	dv2 = opendir("/dev/rdsk");
	while(--argc) {
		found = 0;
		if (stat(*++argv, &sbuf) == -1) {
			fprintf(stderr,"devnm: ");
			perror(*argv);
			continue;
		}
		fno = sbuf.st_dev;
		rewinddir(dv);
		while((dbuf = readdir(dv)) != NULL) {
			sprintf(fname,"/dev/%s",dbuf->d_name);
			if (stat(fname, &sbuf) == -1) {
				fprintf(stderr, "/dev stat error\n");
				exit(1);
			}
			if ((fno != sbuf.st_rdev) || ((sbuf.st_mode & S_IFMT) !=
				S_IFBLK)) continue;
			if (strcmp(dbuf->d_name,"swap") != 0)
				found++;
			printf("%s %s\n", dbuf->d_name, *argv);
		}
		if (!found && dv1 != NULL) {
			rewinddir(dv1);
			while((dbuf = readdir(dv1)) != NULL) {
				sprintf(fname,"/dev/dsk/%s",dbuf->d_name);
				if (stat(fname, &sbuf) == -1) {
					fprintf(stderr, "/dev stat error\n");
					exit(1);
				}
				if ((fno != sbuf.st_rdev) ||
				 ((sbuf.st_mode & S_IFMT) != S_IFBLK)) continue;
				found++;
				printf("%s %s\n", fname, *argv);
			}
		}
		if (!found && dv2 != NULL) {
			rewinddir(dv2);
			while((dbuf = readdir(dv2)) != NULL) {
				sprintf(fname,"/dev/rdsk/%s",dbuf->d_name);
				if (stat(fname, &sbuf) == -1) {
					fprintf(stderr, "/dev stat error\n");
					exit(1);
				}
				if ((fno != sbuf.st_rdev) ||
				 ((sbuf.st_mode & S_IFMT) != S_IFBLK)) continue;
				printf("%s %s\n", fname, *argv);
			}
		}
	}
	exit (0);
	/* NOTREACHED */
}
