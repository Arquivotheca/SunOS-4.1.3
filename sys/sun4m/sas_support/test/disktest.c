#ident "@(#)disktest.c	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

#include "faketest.h"

char buf[4][512];
char mesg[] = "fakeprom test!\n\r";
char dev[] = "sd0a";


struct sunromvec * romp;

start(rp)
struct sunromvec * rp;
{
	int fd;
	int i;

	romp = rp;

	printf(mesg);
	for (i = 0; i < 3; ++i) {
		dev[3] = 'a' + i;
		fd = open(dev);
		printf("open(%s)=%d\n", dev, fd);
		
		if (fd >= 0)
			readblk(fd, 1, 0, buf[i]);
		close(fd);
	}
	printf("end\n\r");
	while(1);
}
