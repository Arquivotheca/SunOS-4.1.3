

#ifndef lint
static char sccsid[] = "@(#)tvconfig.c	1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <fcntl.h>

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <sun/fbio.h>
#include <sun/tvio.h>


#define DEVDIR "/dev/"

/* tvconfig: Binds a video display buffer to an existing frame buffer */

main(argc, argv)
    int	argc;
    char **argv;
{
    char           *progname;
    char            tv_devname[MAXPATHLEN];
    char            fb_devname[MAXPATHLEN];
    int             tv_fd, fb_fd;
    int             c;
    int             redirect = 0;
    struct fbtype   fb;
    struct fbgattr  fbattr;
    struct stat     stat;

    extern char    *sys_errlist[];
    extern int      errno;
    extern char    *optarg;
    extern int      opterr, optind;


    progname = argv[0];

    if (argc <= 1) {
	fprintf(stderr, "Usage: %s [-f] fbdevice [ fbdevice ]\n", progname);
	exit(1);
    }
    while ((c = getopt(argc, argv, "f")) != -1) {
	switch (c) {
	    case 'f':
		redirect = 1;
		break;
	    default:
		fprintf(stderr,
		      "Usage: %s [-f] tvdevice [ fbdevice ]\n", progname);
		exit(1);
	}
    }
    if (optind < argc) {
	if (*argv[optind] != '/') {
	    (void) strcpy(tv_devname, DEVDIR);
	    (void) strncat(tv_devname, argv[optind],
			   MAXPATHLEN - sizeof (DEVDIR));
	} else {
	    (void) strncpy(tv_devname, argv[optind], MAXPATHLEN);
	}
    } else {
	(void) strncpy(fb_devname, "/dev/tvone0", MAXPATHLEN);
    }
    optind++;
    if (optind < argc) {
	if (*argv[optind] != '/') {
	    (void) strcpy(fb_devname, DEVDIR);
	    (void) strncat(fb_devname, argv[optind],
			   MAXPATHLEN - sizeof (DEVDIR));
	} else {
	    (void) strncpy(fb_devname, argv[optind], MAXPATHLEN);
	}
    } else {
	(void) strncpy(fb_devname, "/dev/fb", MAXPATHLEN);
    }
    if ((tv_fd = open(tv_devname, O_RDWR, 0)) == -1) {
	fprintf(stderr, "%s: open failed on device %s: %s\n",
		progname, tv_devname, sys_errlist[errno]);
	exit(2);
    }
    /* Open the device to bind to . Note the O_NDELAY is to stop the GP */
    /* from changing its minor number */
    if ((fb_fd = open(fb_devname, O_RDWR | O_NDELAY, 0)) == -1) {
	fprintf(stderr, "%s: open failed on device %s: %s\n",
		progname, fb_devname, sys_errlist[errno]);
	exit(2);
    }
    if (ioctl(tv_fd, FBIOGTYPE, &fb) == -1) {
	fprintf(stderr, "%s: device %s is not a frame buffer.\n", progname,
		tv_devname);
    }
    if (ioctl(tv_fd, FBIOGATTR, &fbattr) == -1) {
	perror("FBIOGATTR failed:");
	exit(3);
    };

    if (fbattr.real_type != FBTYPE_SUNFB_VIDEO) {
	fprintf(stderr, "%s: device %s is not a video display board", progname,
		tv_devname);
    }
    if (ioctl(fb_fd, FBIOGTYPE, &fb)) {
	fprintf(stderr, "%s: device %s is not a frame buffer.\n", progname,
		fb_devname);
	exit(4);
    }
    /* Check to see if device is legal. Since there will be new frame */
    /* buffers, only check to see what we don't support */
    switch (fb.fb_type) {
	case FBTYPE_SUN2BW:
	    /* This nonsense is required because a cg4 says its a */
	    /* bw2 */
	    {
		struct fbgattr  fb_attr;

		if (!ioctl(fb_fd, FBIOGATTR, &fb_attr)) {
		    if (fb_attr.real_type != FBTYPE_SUN2BW) {
			break;
		    }
		}
	    }
	    /* Now drop through */
	case FBTYPE_SUN1BW:

	    fprintf(stderr, "%s: cannot bind monochrome device %s\n",
		    progname, fb_devname);
	    exit(5);
	default:
	    break;
    }
    fstat(fb_fd, &stat);
    close(fb_fd);
    if (!ioctl(tv_fd, TVIOSBIND, &stat.st_rdev)) {
	printf("%s: %s bound to %s.\n", progname, tv_devname, fb_devname);
    } else {
	fprintf(stderr, "%s: Bind failed.\n", progname);
	exit(6);
    }
    printf("stat.st_rdev = %x\n", stat.st_rdev);
    if (redirect) {
	if (ioctl(tv_fd, TVIOREDIRECT)) {
	    fprintf(stderr, "%s: Redirect failed.\n", progname);
	    exit(7);
	}
	printf("%s: /dev/fb redirected to %s.\n", progname, tv_devname);
    }
    exit(0);
}
