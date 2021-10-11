
#ifndef lint
static char sccsid[] = "@(#)dbconfig.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * dbconfig- "Configures" the dialbox. This program is responsible for
 * opening the designated serial port, setting its baud rate and other options
 * accordingly, removing all stream modules already pushed upon it (such as
 * ttcompat and ldterm) and pushing the dialbox to vuid stream ("db") on the
 * stream. Then it holds the the stream open to maintain the stream
 * configuration
 */

#include <stdio.h>
#include <sgtty.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <sys/conf.h>

#define DEVDIR	"/dev/"
#define MAXFNAME 64

main(argc, argv)
    int argc;
    char **argv;
{
    static char devname[MAXFNAME+1] = "/dev/dialbox";
    static char streamname[] = "db";
    static char exstreamname[FMNAMESZ+1];
    int fd_tty;
    struct sgttyb       sgttyb;
    int	modem_bits;

    extern char *sys_errlist[];
    extern int errno;
   
    if (argc < 2) {
    	syntax_err();
    } else {
	if (*argv[1] != '/') {
	    (void)strcpy(devname, DEVDIR);
	    (void)strncat(devname, argv[1], MAXFNAME - sizeof(DEVDIR));
	} else {
	    (void)strncpy(devname, argv[1], MAXFNAME);
	}
	/* Daemonize ourself */
	switch (fork()) {
	    case 0:
		break;
	    case -1:
		perror("Fork failed");
		break;
	    default:
		exit(0); /* OK */
	}
	(void)printf("dbconfig: configuring \"%s\"\n", devname);
	if ((fd_tty = open(devname, O_RDWR)) < 0) {
	    (void)fprintf(stderr,"dbconfig: Open failed for \"%s\" (%s)\n",
		    devname, sys_errlist[errno]);
	    exit(2);
	}
	(void)ioctl(fd_tty, TIOCGETP, &sgttyb);   /* get flags */
	sgttyb.sg_flags |= RAW | ANYP;      /* Raw, no parity */
	sgttyb.sg_flags &= ~(ECHO | CRMOD); /* No echo or CR-LF translations */
	sgttyb.sg_ispeed = sgttyb.sg_ospeed = B9600;  /* 9600 baud */
	(void)ioctl( fd_tty, TIOCSETP, &sgttyb);      /* set flags */

	/* The following modem control bits are set to provide power */
	/* (at least for the signals). DTR is set high, and RTS is Low */
	(void)ioctl(fd_tty, TIOCMGET, &modem_bits);	/* Get Modem bits */
	modem_bits |= TIOCM_DTR; /* Set DTR High */
	modem_bits &= ~TIOCM_RTS; /* and RTS Low */
	(void)ioctl(fd_tty, TIOCMSET, &modem_bits);	/* Set Modem Bits */

	while (ioctl(fd_tty, I_LOOK, exstreamname) != -1) {
	    (void)printf("dbconfig: popping %s\n", exstreamname);
	    if (ioctl(fd_tty, I_POP, 0) == -1) {
		perror("I_POP ioctl failed.\n");
		exit(3);
	    }
	}

	if (ioctl(fd_tty, I_PUSH, streamname) < 0) {
	    perror("I_PUSH ioctl failed");
	    exit(4);
	}
	(void)printf("dbconfig: %s configured.\n", devname);
	pause(); /* now hold it open forever to maintain stream */
    }
    exit(5); /* NOTREACHED unless interrupted */
}

syntax_err()
{
    (void)fprintf(stderr, "Usage: dbconfig serial_device\n");
    exit(1);
}
