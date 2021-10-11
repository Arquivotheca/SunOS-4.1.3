static char     sccsid[] = "@(#)printer.c 1.1 92/07/30 Copyright(c) 1987, Sun Microsystems, Inc.";

/*
 * Copyright(c) 1987, Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/dir.h>
#include <sys/time.h>
#include <sundev/mcpcmd.h>
#include "sdrtns.h"		/* sundiag standard header file */
#include "../../../lib/include/libonline.h"   /* online library include */
#include "printer.h"
#include "printer_msg.h"

static char     prt_name[12];		       /* to keep the printer name */
static int	prt_fd;

extern int process_printer_args();
extern int routine_usage();

main(argc, argv)
    int             argc;
    char           *argv[];
{
    versionid = "1.1";				/* SCCS version id */
    strcpy(prt_name, "/dev/mcpp0");	       /* default device name */
    device_name = prt_name;
    test_init(argc, argv, process_printer_args, routine_usage, test_usage_msg);

    prt_test();

    if (!verbose)
	sleep(5);

    test_end();		/* Sundiag normal exit */
}

prt_test()
{					       /* main test starts here */
    unsigned char   mode;
    struct sgttyb   setraw;

    if ((prt_fd = open(prt_name, O_WRONLY | O_NDELAY)) == -1) 
	send_message(-OPEN_ERROR, ERROR, opendev_err_msg, prt_name);

    ioctl(prt_fd, TIOCGETP, &setraw);
    setraw.sg_flags |= RAW;	       /* set the raw mode to be sure */
    ioctl(prt_fd, TIOCSETP, &setraw);

    mode = MCPRDIAG;		       /* self-test loopback mode */
    ioctl(prt_fd, MCPIOSPR, &mode);

    data_test(1, 0x100, MCPRPE, "PE");	   /* Test odd data  (bit 1,3,5,7) */
    data_test(2, 0x80, MCPRSLCT, "SLCT");  /* Test even data (bit 2,4,6,8) */

    close(prt_fd);
}

data_test(start, end, cond, cond_str)
    int           start, end; 
    unsigned char cond; 
    char          *cond_str;
{
    char            tmp_buf[82];
    unsigned char   pattern, prt_status;
    int i;

    tmp_buf[0] = 0xff;		       /* all 1's on the data line */
    if (write(prt_fd, tmp_buf, 1) != 1)
	send_message(-WRITE_ERROR, ERROR, writedev_err_msg, prt_name);

    ioctl(prt_fd, MCPIOGPR, &prt_status);
    send_message(0, DEBUG, write_status_msg, prt_status);

    if (!(prt_status & MCPRPE)) 
	send_message(-PE_ERROR, ERROR, prt_status_msg, "PE", prt_name);

    if (!(prt_status & MCPRSLCT)) 
	send_message(-SLCT_ERROR, ERROR, prt_status_msg, "SLCT", prt_name);

    for (i = start; i < end; i <<= 2) {
	pattern = ~i;
	if (write(prt_fd, &pattern, 1) != 1) 
	    send_message(-WRITE_ERROR, ERROR, writedev_err_msg, prt_name);

	ioctl(prt_fd, MCPIOGPR, &prt_status);
	send_message(0, DEBUG, send_status_msg, pattern, prt_status);

	if (prt_status & cond) 	      
	    send_message(-ODD_ERROR, ERROR, send_err_msg,
	                 pattern, cond_str, prt_name);
    }
}

routine_usage()
{
    send_message(0, CONSOLE, routine_msg);
}

process_printer_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
    if (argv[arrcount][0] == 'p') {    /* check printer port first */
        if (strlen(argv[arrcount]) == 2)
            if (argv[arrcount][1] >= '0' && argv[arrcount][1] <= '7') 
                strcpy(&prt_name[8], argv[arrcount]);
                        /* keep the last specified device name */
    } else 
	return (FALSE);

    return (TRUE);
}

/******************************************
 Dummy clean_up to keep libsdrtns.a quiet.
******************************************/
clean_up()
{
}
