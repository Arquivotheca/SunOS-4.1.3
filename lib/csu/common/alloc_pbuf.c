#ifndef lint
static  char sccsid[] = "@(#)alloc_pbuf.c 1.1 92/07/30 SMI";
#endif
/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

#include <fcntl.h>
#include <sys/mman.h>

/* allocate the buffer to be used by profil(2)  */
char *
_alloc_profil_buf(size)
	int size;
{
	char *buf;
	int fd;

	if((fd = open("/dev/zero",O_RDONLY)) == -1 ) {
		return (char*) -1;
	}
	buf = (char*) mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);
	return buf;
}
