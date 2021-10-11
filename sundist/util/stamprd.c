/*
 * @(#)stamprd.c 1.1 92/07/30 Copyright 1989 Sun Micro
 *
 * stamprd.c - stamp a ramdisk image into a binary MUNIX file
 *	with the pre-loaded ramdisk driver.
 * usage: stamprd kernel_file_name munixfs_file_name
 */

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <nlist.h>
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif MIN

struct nlist nl[] = {
#define	PRE_FLAG	0
        { "_ramdiskispreloaded" },
#define	PRE_DATA	1
        { "_ramdisk0" },
#define	START		2
        { "_start" },
	{ "" }
};

#define nlist un_nlist
#include <a.out.h>
#undef nlist

struct exec exec;
struct stat statbuf;
char	buf[8192];

main(argc, argv)
int	argc;
char	*argv[];
{
	off_t	data;
	off_t	where;
	off_t	offset;
	int	fd;
	int	fd2;
	int	i;
	int	size;

	if (argc != 3) {
		printf("usage: stamprd kernel_file_name munixfs_file_name\n");
		exit (1);
	}

	/* read a.out header from kernel */
	if ((fd = open(argv[1], O_RDWR)) < 0) {
		printf("stamprd: can't open kernel \"%s\"\n", argv[1]);
		exit (1);
	}
	if (read(fd, &exec, sizeof(struct exec)) != sizeof(struct exec)) {
		printf("stamprd: can't read a.out header of kernel \"%s\"\n",
		    argv[1]);
		exit (1);
	}
	if (N_BADMAG(exec)) {
		printf("stamprd: bad a.out header of kernel \"%s\"\n",
		    argv[1]);
		exit (1);
	}

	/* find "ramdiskispreloaded" and "ramdisk0" in kernel's namelist */
	if (nlist(argv[1], nl) < 0) {
		printf("can't find names: %s and %s in namelist\n",
		    nl[0].n_name, nl[1].n_name);
		exit (1);
	}

	/* open the munixfs */
	if ((fd2 = open(argv[2], O_RDONLY)) < 0) {
		printf("stamprd: can't open munixfs \"%s\"\n", argv[2]);
		exit (1);
	}

	/* where in the file the data begins */
	data = (off_t)N_TXTOFF(exec);

	/* offset from start of text/data of place to stash munixfs */
	offset = nl[PRE_DATA].n_value - nl[START].n_value;

	/* stat(2) the munixfs image to gets its size */
	if (fstat(fd2, &statbuf) != 0) {
		printf("stamprd: can't stat munixfs image \"%s\"\n", argv[2]);
		exit (1);
	}
	size = (int)(statbuf.st_size);

	if (lseek(fd, data + offset, L_SET) != (data + offset)) {
		printf("stamprd: can't seek (1) into MUNIX \"%s\"\n", argv[1]);
		exit (1);
	}

	/* XXX check size by having a union in rd.c with the size it */
	/* was made to and seeing it it fits XXX TODO */
	if (read(fd, buf, sizeof(int)) != sizeof(int)) {
		printf("stamprd: can't read MUNIX \"%s\" for size check\n",
		    argv[1]);
		exit (1);
	}

	if (size > *((int *)buf)) {
		printf("stamprd: munixfs size: %d > space in MUNIX: %d\n",
		    size, *((int *)buf));
		exit (1);
	}

	if (lseek(fd, data + offset, L_SET) != (data + offset)) {
		printf("stamprd: can't seek (2) into MUNIX \"%s\"\n", argv[1]);
		exit (1);
	}

	while (size) {
		i = MIN(size, sizeof(buf));
		if (read(fd2, buf, i) != i) {
			printf("stamprd: bad read of munixfs\n");
			exit (1);
		}
		if (write(fd, buf, i) != i) {
			printf("stamprd: bad write of munixfs to kernel\n");
			exit (1);
		}
		size -= i;
	}

	/* XXX let shell and adb handle the setting of the flag */

	(void)close(fd);
	(void)close(fd2);
	exit (0);
}
