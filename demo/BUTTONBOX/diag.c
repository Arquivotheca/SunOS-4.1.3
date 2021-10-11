#ifndef lint
static char     sccsid[] = "@(#)diag.c 1.1 92/07/30 Copyright 1988, 1990 Sun Micro";
#endif
 
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sundev/vuid_event.h>
#include <sundev/dbio.h>
#include <sys/ioctl.h>
#include <errno.h>

extern int win_fd;
extern long led_mask;

#define DB_OK	      0
#define DB_OPEN_ERR  -1
#define DB_IOCTL_ERR -2
#define DB_WRITE_ERR -3
#define DB_READ_ERR  -4
#define DB_COMP_ERR  -5

#define message(s)  fprintf(stderr, (s))

void
diag_notify()
{
    switch(diag_test()) {
	case DB_OK:
	    /* message("OK.\n"); */
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
	    message("No Response from buttonbox.\n");
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
	int i;

    /* 0x20 for no error, 0x21 for fail */
    static unsigned char expected_response_ok[] = {0x20};
    static unsigned char expected_response_er[] = {0x21};
    static unsigned char response[sizeof (expected_response_ok)];
    static unsigned char reset_cmd[] = {0x20};

    err = DB_OK;
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1) {
		perror("open");
		return(DB_OPEN_ERR);
    }
    win_remove_input_device(win_fd, "/dev/dialbox");
    
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
	sleep(1);

    /* This does not work for the 3.5 driver yet. */
    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
		perror("write()");
		err = DB_WRITE_ERR;
    }
	else {
		sleep(1);
		if ((c = read(db_fd, response, 1)) != 1) err = DB_READ_ERR;
		else if (response[0] == expected_response_er[0]) err = DB_COMP_ERR;
    }
	
	/* reinitialize */
    led_mask = 0;
    if (ioctl (db_fd, DBIOBUTLITE, &led_mask) == -1) {
		perror ("Ioctl(DBIOBUTLITE)");
    }	
	draw_buttons();

	/* now toggle each button */

	for(i = 1; i < 33; i++) {
		button_on(i);
		sleep(1);
		button_off(i);
	}
    
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
    win_set_input_device(win_fd, db_fd, "/dev/dialbox");
    close(db_fd);
    return(err);
}
