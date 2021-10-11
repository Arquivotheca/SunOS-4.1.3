
#ifndef lint
static	char sccsid[] = "@(#)sp_read.c 1.1 7/30/92 Copyright 1986 Sun Microsystems,Inc.";
#endif

/*
 *	sp_read.c -
 *
 *	This task read data from serial port and compare.
 *
 *	This program is called from children process as follows :
 *
 *	execl(RD_PGM,RD_PGM,id,data,port,NULL)
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
#include 	"sdrtns.h"		/* should always be included */
#include "../../../lib/include/libonline.h"    /* online library include */

/* SPIF specific include files */
#include	<sbusdev/stcio.h>

#include	"spiftest.h"
#include 	"spif_dev.h"
#include 	"spif_msg.h"


static  void	child_terminate();
static  void	receive_timeout();
static	int	pid;
static  int	target;

main(argc,argv)
int	argc;
char	*argv[];
{

	static	void	child_terminate();
	int	fd;
	u_long	*exp, *obs;
	int	i, j, rl, acc;

	func_name = "sp_read";
	TRACE_IN

	signal(SIGINT,child_terminate);
	signal(SIGTERM,child_terminate);

	/* Put timeout for receive line */
	alarm(TIMEOUT);
	signal(SIGALRM, receive_timeout);

	exp = (u_long *) malloc(sizeof(long));
	obs = (u_long *) malloc(sizeof(long));

	pid = getpid();
	target = atoi(argv[1]);
	*exp = strtol(argv[2], (char **)NULL, 16);
	fd = atoi(argv[3]);
	*obs = 0;
	send_message(0, TRACE, "SP_READ: Initially, exp = 0x%x obs = 0x%x\n", *exp, *obs);

	for (i = 0; i < MAX_RUNS; i++) {

	    acc = 0;
	    for (j = 0; (j < RETRIES) && (rl != sizeof(long)); j++) {
		if ((acc = read(fd, &tina[rl], sizeof(long)-rl)) < 0) {
		    send_message(READ_ERROR, ERROR, "Read error on %s\n", sp_dev_name[dev_num]);
		} else
		    rl += acc;
	    }

	    bcopy(tina, obs, 4);

	    send_message(0, TRACE, "%s: exp=0x%x obs=0x%x\n", sp_dev_name[dev_num], *exp, *obs);

	    if (*obs != *exp) {
		send_message(0, ERROR, "Expected = 0x%x , observed = 0x%x\n", *exp, *obs);
		send_message(CMP_ERROR, ERROR, "Data loopback failed on %s\n", sp_dev_name[dev_num]);
            }

	}

	alarm(0); 	/* turn alarm off */
	if (ioctl(fd, TCFLSH, 0) == FAIL)
	    send_message(IOCTL_ERROR, ERROR, "Ioctl TCFLSH error on %s\n", sp_dev_name[target]);

send_message(0, VERBOSE, "SP_READ, %s: Loopback test passed\n", sp_dev_name[target]);

        TRACE_OUT
}


static	void
child_terminate()
{
    send_message(KILL_ERROR, ERROR, "Read pid = %d is terminated\n", pid);
}


static void
receive_timeout()
{
    send_message(TIMEOUT_ERROR, ERROR "Receive timeout error on %s\n", sp_dev_name[target]);
}


clean_up()
{
  func_name = "clean_up";
  TRACE_IN
  TRACE_OUT
  return(0);
}
