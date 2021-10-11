
#ifndef lint
static	char sccsid[] = "@(#)sp_write.c 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc.";
#endif

/*
 *	sp_write.c -
 *
 *	This task write data to serial port.
 *
 *	This program is called from children process as follows :
 *
 *	execl(WR_PGM,WR_PGM,id,data,port,NULL)
 *	
 *     	where port is the port number, and fd is its file descriptor.
 *
 */

#include 	<stdio.h>
#include 	<strings.h>
#include 	<ctype.h>
#include 	<sys/errno.h>
#include 	<sys/types.h>
#include	<sys/termio.h>
#include        <sys/fcntl.h>
#include	<sys/ttold.h>
#include 	<sys/file.h>
#include 	<sys/wait.h>
#include        <sys/mman.h>
#include 	<signal.h>
#include 	"sdrtns.h"	/* should always be included */
#include "../../../lib/include/libonline.h"    /* online library include */

/* SPIF specific include files */

#include	<sbusdev/stcio.h>

#include	"spiftest.h"
#include 	"spif_dev.h"
#include 	"spif_msg.h"


static  void	transmit_timeout();
static 	void	child_terminate();
static	int	pid, target;

main(argc,argv)
int	argc;
char	*argv[];
{

	static	void	child_terminate();
	int	fd;
	u_long	*exp, *obs;
	int	i, wl;

	func_name = "sp_read";
	TRACE_IN

	signal(SIGINT,child_terminate);
	signal(SIGTERM,child_terminate);

	/* Put timeout for transmit line */
	alarm(TIMEOUT);
	signal(SIGALRM, transmit_timeout);

	exp = (u_long *) malloc(sizeof(long));
	obs = (u_long *) malloc(sizeof(long));

	pid = getpid();
	target = atoi(argv[1]);
	*exp = strtol(argv[2], (char **)NULL, 16);
	fd = atoi(argv[3]);
	*obs = 0;
	send_message(0, TRACE, "SP_WRITE: Initially, exp = 0x%x obs=0x%x\n", *exp, *obs);

	for (i = 0; i < MAX_RUNS; i++) {

	    wl = write(fd, exp, sizeof(long));

send_message(0, TRACE, "SP_WRITE, %s: (%d) data=0x%x, length=%d\n", sp_dev_name[target], i, *exp, wl);

	    if (wl != sizeof(long))
		send_message(WRITE_ERROR, ERROR, "Write error on device %s\n", sp_dev_name[target]);

	}

	alarm(0); 	/* turn alarm off */
	if (ioctl(fd, TCFLSH, 0) == FAIL)
	    send_message(IOCTL_ERROR, ERROR, "Ioctl TCFLSH error on %s\n", sp_dev_name[target]);

send_message(0, VERBOSE, "SP_WRITE done on %s\n", sp_dev_name[target]);

        TRACE_OUT
}


static	void
child_terminate()
{
    send_message(KILL_ERROR, ERROR, "Write pid = %d is terminated\n", pid);
}


static void
transmit_timeout()
{
    send_message(TIMEOUT_ERROR, ERROR, "Transmit timeout error on %s\n", sp_dev_name[target]);
}


clean_up()
{
  func_name = "clean_up";
  TRACE_IN
  TRACE_OUT
  return(0);
}
