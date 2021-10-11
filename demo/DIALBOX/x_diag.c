
#ifndef lint
static char     sccsid[] = "@(#)x_diag.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";
#endif
 
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sundev/vuid_event.h>
#include <sys/ioctl.h>
#include <errno.h>


#define DB_OK	      0
#define DB_OPEN_ERR  -1
#define DB_IOCTL_ERR -2
#define DB_WRITE_ERR -3
#define DB_READ_ERR  -4
#define DB_COMP_ERR  -5

#define message(s)  fprintf(stderr, (s))

#define DIAL_UNITS_PER_CYCLE (64*360)
#define NDIALS 8 
					/* converts dial units (64th of degrees) to radians */
#define DIAL_TO_RADIANS(n) \
	((float)(n) * 2.0 * M_PI) / (float) DIAL_UNITS_PER_CYCLE

void
diag_notify()
{
    switch(diag_test()) {
	case DB_OK:
	    message("OK.\n");
	    break;
	case DB_OPEN_ERR:
	    message("Cannot open device.\n");
	    break;
	case DB_IOCTL_ERR:
	    message("Ioctl failed.\n");
	    break;
	case DB_WRITE_ERR:
	    message("Write Error.\n");
	    break;
	case DB_READ_ERR:
	    message("No Response from Dialbox.\n");
	    break;
	case DB_COMP_ERR:
	    message("Selftest Failed.\n");
	    break;
	default:
	    message("Weird unknown kind of error.\n");
	    break;
    }
}

int
diag_test()
{
    int             db_fd, c, err, vuid_format;
	int i, j;

    static unsigned char diag_cmd[] = {0x10, 0, 0};
    static unsigned char expected_response[] = {0x1, 0x2};
    static unsigned char response[sizeof (expected_response)];
    static unsigned char reset_cmd[] = {0x1F, 0, 0};

    err = DB_OK;
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1) {
		perror("open");
		return(DB_OPEN_ERR);
    }
    
    
    /* set to "native mode", (This does not work for the 3.5 driver yet! */
    vuid_format = VUID_NATIVE;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
		perror("ioctl(VUIDSFORMAT, VUID_NATIVE)");
		err = DB_IOCTL_ERR;
    }

    /* Set up for "non-blocking" reads */
    if (fcntl(db_fd, F_SETFL, fcntl(db_fd, F_GETFL, 0) | O_NDELAY) == -1) {
		perror("fctntl()");
		err = DB_IOCTL_ERR;
    }
    while (read(db_fd, response, 1) > 0) {
		; /* just flush */
    }

    /* This does not work for the 3.5 driver yet. */
    if (write(db_fd, diag_cmd, sizeof (diag_cmd)) != sizeof (diag_cmd)) {
		perror("write()");
		err = DB_WRITE_ERR;
    }
	else {
		sleep(1);
		if ((c = read(db_fd, response, 2)) != 2) {
			err = DB_READ_ERR;
		}
		else if(strncmp(response, expected_response,
			sizeof(expected_response))) {
			err = DB_COMP_ERR;
		} 
		else {
			; /* OK */
		}
    }
    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
		perror("write()");
		err = DB_WRITE_ERR;
    }

	/* paint each dial in order on the screen and reset them */
	for(i = 0; i < NDIALS; i++) {
		float angle = M_PI;
		for(j = 0; j < DIAL_UNITS_PER_CYCLE; j++) {
			if(j) draw_dial(i, angle, 0);
			angle += DIAL_TO_RADIANS(1);
			draw_dial(i, angle, 1);
		}
		sleep(1);
		draw_dial(i, angle, 0);
	}
	draw_dials();

    /* back to VUID mode */
    vuid_format = VUID_FIRM_EVENT;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
		perror("ioctl (VUIDSFORMAT, VUID_FIRM_EVENT)");
		err = DB_IOCTL_ERR;
    }
    while (read(db_fd, response, 1) > 0) {
		; /* just flush (again) */
    }

    /* re install device to window system */
    
    return(err);
}
