/* Copyright (C) 1988, Sun Microsystems, Inc. */
#ifndef lint
static	char sccsid[] = "@(#)dkctl.c 1.1 92/07/30 SMI";
#endif
/*
 * dkctl - general interface program to a number of interesting disk
 * ioctls.
 *
 * m. jacob. 5/13/88
 */
#include <sys/types.h>
#include <sun/dkio.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

main(a,v)
char *v[];
{
	extern int errno;
	auto int flag, newval;
	register fd, com;
	static char *onoff[2] = { "off", "on" };

	if(a < 3 || v[1] == (char *) 0 || v[2] == (char *) 0) {
		exit(usage());
	} else if((fd = open(v[1],O_RDONLY | O_NDELAY)) < 0) {
		perror(v[1]);
		exit(1);
	}

	if(strcmp(v[2],"wchk") == 0) {
		com = DKIOCWCHK;
		newval = flag = 1;
	} else if(strcmp(v[2],"-wchk") == 0) {
		com = DKIOCWCHK;
		newval = flag = 0;
	} else
		exit(usage());

	if(com == DKIOCWCHK) {
		if(ioctl(fd,com,&flag) < 0) {
			if (errno == ENOTTY)
				(void) fprintf(stderr,
					"Operation not supported on device\n");
			else
				perror("ioctl");
			exit(1);
		} 
		(void) fprintf(stdout,
			"write check was turned %s, now turned %s\n",
			onoff[flag&0x1],onoff[newval&0x1]);
	}
	exit(0);
	/*NOTREACHED*/
}

usage()
{
	(void) fprintf(stderr, "usage: dkctl rawdevice [-]wchk\n");
	return(1);
}
