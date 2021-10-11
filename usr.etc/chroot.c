/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)chroot.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

# include <stdio.h>
# include <errno.h>

main(argc, argv)
char **argv;
{
	extern char *sys_errlist[];
	extern int sys_nerr;

	if(argc < 3) {
		printf("usage: chroot rootdir command arg ...\n");
		exit(1);
	}
	argv[argc] = 0;
	if(argv[argc-1] == (char *) -1) /* catches potential problems in
					 old 16 bit implimentations */
		argv[argc-1] = (char *) -2;
	if (chroot(argv[1]) < 0) {
		perror(argv[1]);
		exit(1);
	}
	if (chdir("/") < 0) {
		printf("Can't chdir to new root\n");
		exit(1);
	}
	execv(argv[2], &argv[2]);
	if((errno > 0) && (errno <= sys_nerr)) 
		printf("chroot: %s\n",sys_errlist[errno]);
	else printf("chroot: exec failed, errno = %d\n",errno);
	exit(1);
	/* NOTREACHED */
}
