#ifndef lint
static	char sccsid[] = "@(#)click.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Set the kbd click on the Sun-3.
 *	-d dev chooses the kbd device, default /dev/kbd.
 *	-y enables click,
 *	-n disables click and no -y or -n enables click.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sundev/kbio.h>
#include <sundev/kbd.h>
#include <stdio.h>

main (argc, argv)
    int argc;
    char *argv [];
{
    int fd;
    int cmd = KBD_CMD_CLICK;
    char *devstr = "/dev/kbd";
    char *badargstr =
"Invalid argument: Try -y (yes--enable click), -n (no--disable click) or\n\
-d device_name (default is /dev/kbd)\n";

    argv++;
    argc--;
    while (argc > 0) {
	if ((strlen(*argv) >= 2) && (*argv)[0] == '-') {
		switch ((*argv)[1]) {
		case 'd':
			if (argc > 1) {
				argv++;
				argc--;
				devstr = *argv;
			} else {
				fprintf(stderr, badargstr);
				exit(1);
			}
			break;

		case 'n':
			cmd = KBD_CMD_NOCLICK;
			break;

		case 'y':
			cmd = KBD_CMD_CLICK;
			break;
		default:
			fprintf(stderr, badargstr);
			exit(1);
			break;
		}
		argv++;
		argc--;
	} else {
		fprintf(stderr, badargstr);
		exit(1);
	}
    }
    fd = open(devstr, 1);
    if (fd < 0) {
	perror("click open");
        exit (1);
    }
    if (ioctl(fd, KIOCCMD, &cmd)) {
	perror("click ioctl");
	exit (1);
    }
    exit(0);

	/* NOTREACHED */
}
