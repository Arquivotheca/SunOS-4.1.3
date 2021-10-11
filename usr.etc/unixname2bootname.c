/*	Copyright (c) 1984 AT&T */
/*	 All Rights Reserved   */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static  char sccsid[] = "@(#)unixname2bootname.c 1.1 92/07/30 SMI";
#endif

/*
 *
 *
 * a program to translate unix style device names to bootprom style
 *	names.
 *
 * unixname2bootname name
 *
 * for use with OBP kernels - if it calls /dev/openprom that doesn't
 * know about the new ioctl - it punts/assumes old proms.
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sundev/openpromio.h>

extern int errno;

main(argc, argv)
int	argc;
char	*argv[];
{
	int	fd;
	struct oprom {
		u_int	oprom_size;
		char	oprom_array[OPROMMAXPARAM];
	} op;
	char	*cp;


	if (argc != 2) {
		(void)fprintf(stderr,"usage: unixname2bootname name\n");
		exit (1);
	}

	if ((fd = open("/dev/openprom", O_RDONLY)) < 0) {
		perror("cannot open /dev/openprom");
		exit (1);
	}

	if ((cp = strrchr(argv[1], '/')) == (char *)NULL)
		cp = argv[1];
	else
		cp++;

	bzero(op, sizeof (struct oprom));
	op.oprom_size = OPROMMAXPARAM;
	(void)strncpy(op.oprom_array, cp, OPROMMAXPARAM);

	if (ioctl(fd, OPROMU2P, &op) < 0) {
		if (errno != EINVAL) {
			perror("unixname2bootname: ioctl OPROMU2P failed");
			exit (1);
		}
		/* must be down-rev proms, cons up downrev name */
		(void)printf("%c%c(0,%c,%d)\n", cp[0], cp[1], cp[2],
		    cp[3] - 'a');
		exit (0);
	}
	if (op.oprom_array[0] != '/')
		(void)printf("%c%c(0,%c,%d)\n", cp[0], cp[1], cp[2],
		    cp[3] - 'a');
	else
		(void)printf("%s\n", op.oprom_array);
	exit (0);
}
