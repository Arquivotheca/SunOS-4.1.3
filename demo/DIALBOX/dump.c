
#ifdef lint
static char     sccsid[] = "@(#)dump.c 1.1 92/07/30 Copyright 1988 Sun Micro";

#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sundev/vuid_event.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/file.h>

extern int win_fd;
#define RD_OK		    -10
#define RD_OPEN_ERR	    -20
#define RD_WRITE_ERR	    -30
#define RD_READ_ERR	    -40
#define RD_IOCTL_ERR	    -50

#define message(s)  fprintf(stderr, (s))


void
dump_notify()
{
    switch(dump_test()) {
	case RD_OK:
	    message("OK.\n");
	    break;
	case RD_OPEN_ERR:
	    message("Cannot open device.\n");
	    break;
	case RD_WRITE_ERR:
	    message("Write failed.\n");
	    break;
	case RD_READ_ERR:
	    message("Read failed.\n");
	    break;
	case RD_IOCTL_ERR:
	    message("ioctl error.\n");
	    break;
	default:
	    message("Weird unknown kind of error.\n");
	    break;
    }
}
int
dump_test()
{
    int             db_fd, c, err, vuid_format, j, k;
    FILE	    *fp, *fopen();
    static unsigned char dump_cmd[] = {0xA5};
    static unsigned char reset_cmd[] = {0x1F, 0, 0};
    static unsigned char response[257];
    static unsigned char dummy[257];

  
    err = RD_OK;
    if ((db_fd = open("/dev/dialbox", O_RDWR)) == -1) {
	perror("open");
	return(RD_OPEN_ERR);
    }
    win_remove_input_device(win_fd, "/dev/dialbox");

    /* set to "native mode" */
    vuid_format = VUID_NATIVE;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
	perror("ioctl(VUIDSFORMAT, VUID_NATIVE)");
	err = RD_IOCTL_ERR;
    }
    /* Set up for "non-blocking" reads */
    if (fcntl(db_fd, F_SETFL, fcntl(db_fd, F_GETFL, 0) | O_NDELAY) == -1) {
	perror("fctntl()");
	err = RD_IOCTL_ERR;
    }
    while (read(db_fd, dummy, 1) > 0) {
	; /* just flush */
    }
    if (write(db_fd, dump_cmd, sizeof (dump_cmd)) != sizeof (dump_cmd)) {
	perror("write()");
	err = RD_WRITE_ERR;
    } else {
	fd_set fdset;
	int r, i;
        struct timeval timeout; 

	i =0;
    for ( k = 0;k < 257; k++){
	response[k] = 0;
    }

	
	FD_ZERO(&fdset);
	FD_SET(db_fd, &fdset);
	{
	  int len = 257, nfds ;
	  while( len > 0 )
	  {
	    timeout.tv_sec = 0 ;
	    timeout.tv_usec = 250000;   /* 1/4 Second timeout */
	    nfds = select(getdtablesize(), &fdset, NULL,NULL, &timeout) ;
	    switch( nfds )
	    {
	      case -1: perror("select") ; len = 0 ; break ;
	      case 0: fprintf(stderr, "select timed out\n") ; len = 0 ; break ;
	      default:
		if( (r = read(db_fd, &response[i], len)) < 0 )
		{
		  if( errno != EWOULDBLOCK )
		  {
		    perror("read") ;
		    len = 0 ;
		  }
		}
		else
		{
		  i += r ;
		  len -= r ;
		}
	    }
	  }
	}
    }


    fp = fopen("ram_dump.dat", "w");
    for (j=0; j<257; j++){
	fprintf(fp, "Byte %d = %x\n", j+1, response[j]);
    }
    (void)fclose(fp);

    if (write(db_fd, reset_cmd, sizeof (reset_cmd)) != sizeof (reset_cmd)) {
	perror("write()");
	 err = RD_WRITE_ERR;
    }
    /* back to VUID mode */
    vuid_format = VUID_FIRM_EVENT;
    if (ioctl(db_fd, VUIDSFORMAT, &vuid_format) == -1) {
	perror("ioctl (VUIDSFORMAT, VUID_FIRM_EVENT)");
	err = RD_IOCTL_ERR;
    }
    while (read(db_fd, dummy, 1) > 0) {
	; /* just flush (again) */
    }
    /* re install device to window system */
    win_set_input_device(win_fd, db_fd, "/dev/dialbox");
    close(db_fd);
    return(err);
}
