static char     sccsid[] = "@(#)sptest.c 1.1 7/30/92 Copyright 1985 Sun Micro";

/******************************************************************************
*
*	name		"sptest.c"
*
*	synopsis	(cmd line) "sptest <source> <receiver>"
*
*			source	=>	full name of source device
*					e.g. "/dev/ttya" or "/dev/tty00-07"
*
*			receiver =>	full name of receiver device
*					(in the case of paired loopback cable)
*					e.g. "/dev/ttyb" or "/dev/tty07,02,04"
*
*	description	Writes data (0-FFh) to the source device
*			and then reads it back from the receiver device
*			verifying the data after each byte sent.
*			Status is returned in the error byte, 0 being
*			successful and non-zero being an error status (see
*			siodat.c)
*			If no receiver is given, it is assumed to be the
*			same as the source.  If a range is given, a table
*			of device names is constructed.  The same applies to
*			a list.  Construction  uses the full root, replacing
*			each instance with the number of bytes given in the
*			range or list.  Example:
*
*				"/dev/tty00-3"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty02
*				/dev/tty03
*
*			and
*				"/dev/tty00,2,7,11"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty02
*				/dev/tty07
*               		/dev/tty11
*
*
*			In addition, some special keys are provided:
*
*				single8	(first 8 ports)
*				single14(first 14 ports)
*				double8 (same as single8, but double port loop)
*				double12(double port loop ports 4-16)
*				double14(same as single14, double port loop)
*				all (all 16 ports, single loopback)
*				pairs (all 16 ports, double loop)
*
*                       "0.", "1.", "2.", "3.", "h.", "i.", "j.", or "k."
*                       may be inserted before any of the above keys to
*                       indicate which board to test.
*
*/

#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>

#ifdef SVR4
#include <sys/ttold.h>
#include <sys/stropts.h>
#endif SVR4

#ifndef SVR4
#include <sgtty.h>
#endif SVR4
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include "sdrtns.h"                            /* sundiag standard h file */
#include "../../../lib/include/libonline.h"	/* online library include */
#include "sptest.h"
#include "sptest_msg.h"

int             bleft;	       		/* # of bytes left in expand buffer */
char           *srcbuf, *desbuf;        /* addr of source & destination buf */
char           *port_ptr, *exbuf;	/* addr of device line & expand buf */
char           *root;	       		/* address of most recent root name */
unsigned char  *srcnam, *rcvnam;       /* source and receiver device names */
unsigned char   board;
unsigned char  *devnam;		       /* name of port under test */
unsigned char  *xmit_dev;	       /* name of transmit port under test */
unsigned char  *receive_dev;	       /* name of receive port under test */
unsigned char   srcbufm[BUF_SIZE];     /* expansion buffer for source names */
unsigned char   rcvbufm[BUF_SIZE];     /* expansion buffer for rcvr names */

static int      srcfd = 0, rcvfd = 0;  /* file decriptor for port */
static int      swapflag;	       /* port definition swap flag */
static short    srcsav, rcvsav;	       /* save existing tty mode */

static struct sgttyb srclist, rcvlist; /* list of saved port parameters */

char		device_sptestname[60] = "";
char	       *device = device_sptestname;
char		sundiag_device[60] = "";
char            perror_msg_buffer[30];
char           *perror_msg = perror_msg_buffer;

char            serial_port_names[50];
char           *serial_ports = serial_port_names;
char            test_ports[50];
char           *port_to_test = test_ports;
int             device_1 = FALSE, device_2 = FALSE;
int             keyword = FALSE, send_characters = 3000;
int             characters, ports_to_test;
int             times, connection= FALSE;

extern int      siodat();	       /* data test for serial ports */

struct pkglst {
    char           *key;	       /* keyword to select device list */
    char           *src;	       /* lst of src devices */
    char           *rcv;	       /* lst of receiver devices */
} pkg[] = {
    "single8", "/dev/tty00-7", "/dev/tty00-7",
    "single14", "/dev/tty00-9,a-d", "/dev/tty00-9,a-d",
    "double8", "/dev/tty00,1,3,5", "/dev/tty07,2,4,6",
    "double12", "/dev/tty04,6,8,a,c,e", "/dev/tty05,7,9,b,d,f",
    "double14", "/dev/tty00,1,3,5,8,9,b", "/dev/tty07,2,4,6,d,a,c",
    "all", "/dev/tty00-9,a-f", "/dev/tty00-9,a-f",
    "pairs", "/dev/tty00,1,3,5,8,9,b,d", "/dev/tty07,2,4,6,f,a,c,e",
    "0.single8", "/dev/tty00-7", "/dev/tty00-7",
    "0.single14", "/dev/tty00-9,a-d", "/dev/tty00-9,a-d",
    "0.double8", "/dev/tty00,1,3,5", "/dev/tty07,2,4,6",
    "0.double12", "/dev/tty04,6,8,a,c,e", "/dev/tty05,7,9,b,d,f",
    "0.double14", "/dev/tty00,1,3,5,8,9,b", "/dev/tty07,2,4,6,d,a,c",
    "0.all", "/dev/tty00-9,a-f", "/dev/tty00-9,a-f",
    "0.pairs", "/dev/tty00,1,3,5,8,9,b,d", "/dev/tty07,2,4,6,f,a,c,e",
    "1.single8", "/dev/tty10-7", "/dev/tty10-7",
    "1.double12", "/dev/tty14,6,8,a,c,e", "/dev/tty15,7,9,b,d,f",
    "1.single14", "/dev/tty10-9,a-d", "/dev/tty10-9,a-d",
    "1.double8", "/dev/tty10,1,3,5", "/dev/tty17,2,4,6",
    "1.double14", "/dev/tty10,1,3,5,8,9,b", "/dev/tty17,2,4,6,d,a,c",
    "1.all", "/dev/tty10-9,a-f", "/dev/tty10-9,a-f",
    "1.pairs", "/dev/tty10,1,3,5,8,9,b,d", "/dev/tty17,2,4,6,f,a,c,e",
    "2.single8", "/dev/tty20-7", "/dev/tty20-7",
    "2.single14", "/dev/tty20-9,a-d", "/dev/tty20-9,a-d",
    "2.double8", "/dev/tty20,1,3,5", "/dev/tty27,2,4,6",
    "2.double12", "/dev/tty24,6,8,a,c,e", "/dev/tty25,7,9,b,d,f",
    "2.double14", "/dev/tty20,1,3,5,8,9,b", "/dev/tty27,2,4,6,d,a,c",
    "2.all", "/dev/tty20-9,a-f", "/dev/tty20-9,a-f",
    "2.pairs", "/dev/tty20,1,3,5,8,9,b,d", "/dev/tty27,2,4,6,f,a,c,e",
    "3.single8", "/dev/tty30-7", "/dev/tty30-7",
    "3.single14", "/dev/tty30-9,a-d", "/dev/tty30-9,a-d",
    "3.double8", "/dev/tty30,1,3,5", "/dev/tty37,2,4,6",
    "3.double12", "/dev/tty34,6,8,a,c,e", "/dev/tty35,7,9,b,d,f",
    "3.double14", "/dev/tty30,1,3,5,8,9,b", "/dev/tty37,2,4,6,d,a,c",
    "3.all", "/dev/tty30-9,a-f", "/dev/tty30-9,a-f",
    "3.pairs", "/dev/tty30,1,3,5,8,9,b,d", "/dev/tty37,2,4,6,f,a,c,e",
    "h.double8", "/dev/ttyh0,2,4,6", "/dev/ttyh1,3,5,7",
    "h.double12", "/dev/ttyh4,6,8,a,c,e", "/dev/ttyh5,7,9,b,d,f",
    "h.double14", "/dev/ttyh0,2,4,6,8,a,c", "/dev/ttyh1,3,5,7,9,b,d",
    "h.pairs", "/dev/ttyh0,2,4,6,8,a,c,e", "/dev/ttyh1,3,5,7,9,b,d,f",
    "i.double8", "/dev/ttyi0,2,4,6", "/dev/ttyi1,3,5,7",
    "i.double12", "/dev/ttyi4,6,8,a,c,e", "/dev/ttyi5,7,9,b,d,f",
    "i.double14", "/dev/ttyi0,2,4,6,8,a,c", "/dev/ttyi1,3,5,7,9,b,d",
    "i.pairs", "/dev/ttyi0,2,4,6,8,a,c,e", "/dev/ttyi1,3,5,7,9,b,d,f",
    "j.double8", "/dev/ttyj0,2,4,6", "/dev/ttyj1,3,5,7",
    "j.double12", "/dev/ttyj4,6,8,a,c,e", "/dev/ttyj5,7,9,b,d,f",
    "j.double14", "/dev/ttyj0,2,4,6,8,a,c", "/dev/ttyj1,3,5,7,9,b,d",
    "j.pairs", "/dev/ttyj0,2,4,6,8,a,c,e", "/dev/ttyj1,3,5,7,9,b,d,f",
    "k.double8", "/dev/ttyk0,2,4,6", "/dev/ttyk1,3,5,7",
    "k.double12", "/dev/ttyk4,6,8,a,c,e", "/dev/ttyk5,7,9,b,d,f",
    "k.double14", "/dev/ttyk0,2,4,6,8,a,c", "/dev/ttyk1,3,5,7,9,b,d",
    "k.pairs", "/dev/ttyk0,2,4,6,8,a,c,e", "/dev/ttyk1,3,5,7,9,b,d,f",
    "\0", "\0", "\0"
};

main(argc, argv)
    int             argc;
    char           *argv[];
{
    extern int	    process_sptest_args();
    extern int      routine_usage();
    extern int      receive_timeout();
    int             i;			       
    int             status;	   /* intermediate error status */
    int		    srcfd1, rcvfd1, srcfd2, rcvfd2;
    unsigned char   c, *ptr;	   /* general character and pointer */

    versionid = "1.1";
						/* begin Sundiag test */
    test_init(argc, argv, process_sptest_args, routine_usage, test_usage_msg);
    signal(SIGALRM, receive_timeout);	       /* no receive message */

    if (getenv("SD_LOG_DIRECTORY"))
	exec_by_sundiag = TRUE;

    if (!device_1 && !keyword)  
        syspars(srcbufm, rcvbufm);

    srcnam = srcbufm;          /* point at start of source devices */
    rcvnam = rcvbufm;          /* point at start of receive devices */

    while (*srcnam != '\0' && *rcvnam != '\0') {
	ports_to_test++;
	if (strcmp(srcnam, rcvnam))
	    ports_to_test++;
	while (*srcnam++ != '\0')
            ;
	while (*rcvnam++ != '\0')
            ;
    }

    if ((send_characters = send_characters/ports_to_test) < 511)
	send_characters = 511;

    srcnam = srcbufm;	       /* point at start of source devices */
    rcvnam = rcvbufm;	       /* point at start of receive devices */

    while (*srcnam != '\0' && *rcvnam != '\0') {
            openfiles(srcnam, rcvnam, &srcfd1, &rcvfd1);
	    status = siodat(srcnam, rcvnam);   /* test named port */
	    if (strcmp(srcnam, rcvnam)) {      /* if testing two ports, */
		/* then reverse the direction */
		/* to test input and output for both */
		if (!quick_test && !verbose)
		    sleep(10);
		openfiles(rcvnam, srcnam, &srcfd2, &rcvfd2);
		status = siodat(rcvnam, srcnam);	
		closefiles(srcfd2, rcvfd2);
	    }
	    closefiles(srcfd1, rcvfd1);
	    if (quick_test && !single_pass)
		break;
	    if (!quick_test && !verbose)
		sleep(10);
	    while (*srcnam++ != '\0')
		;	       /* advance to next source name */
	    while (*rcvnam++ != '\0')
		;	       /* advance to next receiver name */
    }

    test_end();			/* Sundiag normal exit */
}

/******************************************************************************
*
*	Test data path for named serial ports (source and receive)
*
*	synopsis	error = siodat(source, receive)
*			error = error status or 0 if OK
*			source = full device name as it appears in /dev
*			receive = full device name as it appears in /dev
*
*	Siodat writes data to source port and then reads it from the receive
*	port for verification.  The entire byte range (0 - FFH) is written
*	and read.  Test stops on first error.  Status is returned.
*
*/

siodat(srcdev, rcvdev)
    unsigned char  *srcdev, *rcvdev;	       /* device names */
{
    int             i, j, l;	       /* general purpose loop counter */
    int		    return_code = 0;
    long            count;	       /* character count for port read */
    unsigned char   c, *ptr;	       /* general character and pointer */
    unsigned char   pattern[256];      /* pattern written to port */

    swapflag = 0;			       /* clear swap flag */

    /*
     * Open requested port
     */

    for (;;) {			      /* provide outside master loop */
	devnam = xmit_dev = srcdev;   /* point at attempted device name */
	devnam = receive_dev = rcvdev;/* point at attempted device name */

	if (!strcmp(srcdev, rcvdev)) 
	    rcvfd = srcfd;		      
	if ((i = fcntl(rcvfd, F_GETFL, 0)) == -1)
	    send_message(0, DEBUG, no_status_msg, "get", test_name, rcvfd);
	else if (fcntl(rcvfd, F_SETFL, i | O_NDELAY) == -1)	
	    send_message(0, DEBUG, no_status_msg, "set", test_name, rcvfd);

	devnam = srcdev;	       /* use source name for error msgs */

#ifdef SVR4
        /* 
         * Push the appropriate ttcompat on top of the line
discipline
         */
 
        if (ioctl(srcfd, I_PUSH, "ttcompat") == -1)
        {
            send_message(-WRITE_FAILED, ERROR, not_able_to_push_msg, test_name);
        }
#endif SVR4
	/*
	 * Adjust sio port parameters
	 */

	if (ioctl(srcfd, TIOCGETP, &srclist) == -1)	
	    send_message(0, DEBUG, not_able_msg, "get parm.", "S", 
	                 test_name, srcfd);

	if (ioctl(srcfd, TIOCEXCL, 0) == -1)   /* get EXCLUSIVE use of port */
	    send_message(0, DEBUG, not_able_msg, "get Exclusive", "S",
	                 test_name, srcfd);
	swapflag = 1;			       /* mark port exclusive */
	srcsav = srclist.sg_flags;	       /* save existing tty mode */
	srclist.sg_flags = RAW + ANYP;	       /* RAW, instant input */

	if (ioctl(srcfd, TIOCSETP, &srclist) == -1)	
	    send_message(0, DEBUG, not_able_msg, "set parm.", "S",
	                 test_name, srcfd);
	swapflag = 2;			       /* flag source port swapped */

	if (srcfd != rcvfd) {		       /* for ttym - ttyn only */
#ifdef SVR4
        /*
         * Push the appropriate ttcompat on top of the line
discipline
         */
 
            if (ioctl(rcvfd, I_PUSH, "ttcompat") == -1)
            {
                send_message(-WRITE_FAILED, ERROR, not_able_to_push_msg, test_name);
            }
#endif SVR4
	    if (ioctl(rcvfd, TIOCGETP, &rcvlist) == -1)	
	        send_message(0, DEBUG, not_able_msg, "get parm.", "R", 
	                     test_name, rcvfd);
	    if (ioctl(rcvfd, TIOCEXCL, 0) == -1)	
	        send_message(0, DEBUG, not_able_msg, "get Exclusive", "R",
	                     test_name, rcvfd);
	    swapflag = 3;		       /* mark port exclusive */
	    rcvsav = rcvlist.sg_flags;	       /* save existing tty mode */
	    rcvlist.sg_flags = RAW + ANYP;     /* RAW, instant input */
	    if (ioctl(rcvfd, TIOCSETP, &rcvlist) == -1)	
	        send_message(0, DEBUG, not_able_msg, "set parm.", "R",
	                     test_name, rcvfd);
	    swapflag = 4;		       /* flag receiver port swapped */
	}
	/*
	 * Set up for test
	 */
	strcpy(pattern, "!");		       /* initialize pattern buffer */

	/*
	 * Clean input and output of any garbage
	 */

	for (i = 0; i < 1000; i++) {
	    if (ioctl(rcvfd, FIONREAD, &count) == -1)	/* check for garbage */
	        send_message(0, DEBUG, not_complete_msg, "FIONREAD",
	                     test_name, rcvfd);
	    l = read(rcvfd, pattern, (int) count);	/* clean it off */
	    if (l != 0 && l != count)	       /* error while reading */
	        send_message(0, DEBUG, not_complete_msg, "reading",
	                     test_name, rcvfd);
	}

	/*
	 * Write data pattern to port
	 */
	board = (*(srcdev + 8));
	switch (board) {
	case '0':
	case '1':
	case '2':
	case '3':
	    characters = send_characters * 3;
	    break;
	default:
	    characters = send_characters;
	}
        send_message(0, DEBUG, ports_msg, ports_to_test, srcdev, board, 
		     characters);

	if (quick_test || debug)
	    characters = 100;
	for (c = 0, l = 0; l <= characters; l++, c++) {
	    pattern[0] = c;

	    if ((write(srcfd, pattern, 1)) == -1) {
	        perror(perror_msg);
	        return_code = WRITE_FAILED;
	        break;
	    }
	    pattern[0] = '\0';		       /* clear pattern */

	    if (!connection)  times = 1;
	    else  times = 3;

	    for (i = 0; i < times*NUM_RETRIES; ++i) {
		if ((j = read(rcvfd, pattern, 1)) == 1) {
		    connection = TRUE;
		    break;		       /* data can be read */
		}
	    }
	    if (i == times*NUM_RETRIES && j != 1)    /* bad port or connector */
		receive_timeout();	/* Prompt user to correct the error */

	    if (j != 1) {
		perror(perror_msg);
		return_code = READ_FAILED;
		break;
	    }

	    if (c != pattern[0]) {
		return_code = DATA_ERROR;      /* wrong data, flag error */
		break;			       /* exit pattern loop */
	    }
	}		       /* loop through each data pattern */
	break;				       /* done, report test */
    }

    /*
     * Display test results
     */

    strcpy(device, devnam);

    switch (return_code) {
    case TESTED_OK:		/* when return_code = 0 */
	send_message(0, VERBOSE, test_ok_msg, device);
	break;
    case DEV_NOT_OPEN:
	send_message(-DEV_NOT_OPEN, ERROR, open_err_msg, device);
	break;
    case WRITE_FAILED:
	send_message(-WRITE_FAILED, ERROR, transmit_err_msg, device);
	break;
    case READ_FAILED:
	strcpy(device, receive_dev);
	send_message(-READ_FAILED, ERROR, receive_err_msg, device);
	break;
    case DATA_ERROR:
	strcpy(device, receive_dev);
	send_message(-DATA_ERROR, ERROR, data_err_msg, device, c, pattern[0]);
    }

    clean_up();		       /* clean up ports and close devices */
    return (return_code);      /* return status to caller */
}

/******************************************************************************
*
*	name		"syspars"
*
*	synopsis	status = syspars(source, destination)
*
*			status	=>	0 = success, non-zero = error
*
*			source	=>	address of buffer to hold expanded
*					list of source ports
*
*			destination =>	address of buffer to hold expanded
*					list of destination ports
*
*
*	description	The Sundiag environmental variables "SERIAL_PORTS_n"
*			are parsed into the source and destination fields.
*			Example:
*
*				SERIAL_PORTS_1 = a
*
*			expands to:
*
*				source         destination
*				------         -----------
*				/dev/ttya	/dev/ttya
*
*			and
*				SERIAL_PORTS_1 = a-b
*
*			expands to:
*
*				source         destination
*				------         -----------
*				/dev/ttya      /dev/ttyb
*
*/


/*
 * syspars program
 */

syspars(source, destination)
    char           *source, *destination;      /* ptrs source and destination
					        * buffers */

{
    int             i;			       /* general purpose loop
					        * variable */
    unsigned char   port, *ptr;		       /* port character and pointer */


    /*
     * Initialize
     */

    srcbuf = source;			       /* global ptr to source line */
    desbuf = destination;		       /* global ptr to destination
					        * buffer */

    /*
     * get Sundiag ports from "SERIAL_PORTS_n" envirmental variables.
     */

    for (i = 1; i < 100; i++) {
	sprintf(serial_ports, "SERIAL_PORTS_%d", i);
	if (getenv(serial_ports)) {
	    strcpy(port_to_test, "/dev/tty");
	    strcat(port_to_test, (getenv(serial_ports)));
	    send_message(0, DEBUG, ports_test_msg, serial_ports, port_to_test);
	    port_ptr = port_to_test;

	    while ((*srcbuf++ = *port_ptr++) != '\0') {
		if (*(port_ptr - 1) == '-') {
		    (*(srcbuf - 1)) = '\0';
		    *desbuf--;
		    if (*(desbuf - 1) != 'y')
			*desbuf--;
		    while (*port_ptr != '\0') {
			*desbuf++ = *port_ptr++;
		    }
		    break;
		} else {
		    *desbuf++ = (*(port_ptr - 1));
		}
	    }
	    *desbuf++ = '\0';		       /* mark end of device */

	    /*
	     * Check source port for device 0-3, if yes change 'tty' to
	     * 'ttys'
	     */

	    if (*(srcbuf - 3) == 'y') {
		port = (*(srcbuf - 2));
		switch (port) {
		case '0':
		case '1':
		case '2':
		case '3':
		    (*(srcbuf - 2)) = 's';
		    (*(srcbuf - 1)) = port;
		    *srcbuf++ = '\0';	       /* mark end of device */
		    break;
		}
	    }
	    /*
	     * Check destination port for device 0-3, if yes change 'tty' to
	     * 'ttys'
	     */

	    if (*(desbuf - 3) == 'y') {
		port = (*(desbuf - 2));
		switch (port) {
		case '0':
		case '1':
		case '2':
		case '3':
		    (*(desbuf - 2)) = 's';
		    (*(desbuf - 1)) = port;
		    *desbuf++ = '\0';	       /* mark end of device */
		    break;
		}
	    }
	} else {
	    if (i == 1) 
		send_message(-NO_SD_PORTS_SELECTED, ERROR, no_ports_msg);
	    break;
	}
    } /* end of for */
}

/******************************************************************************
*
*	name		"expars.c"
*
*	synopsis	status = expars(source, destination, limit)
*
*			status	=>	0 = success, non-zero = error
*
*			source	=>	set of arguments or ranges to be
*					parsed and expanded
*
*			destination =>	address of buffer to hold expanded
*					list of arguments
*
*			limit	=>	size of expand buffer
*
*
*
*	description	A single string (usually a command line argument) is
*			is parsed and expanded according to its content. If
*			a range is given, a table of device names is
*			 constructed.  The same applies to a a list.
*			  Construction  uses the full root, replacing
*			each instance with the number of bytes given in the
*			range or list.  Example:
*
*				"/dev/tty00-3"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty02
*				/dev/tty03
*
*			and
*				"/dev/tty00,2,7,11"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty02
*				/dev/tty07
*				/dev/tty11
*
*
*			conditions can be combined:
*
*				"/dev/tty00-4,8,a,c-e"
*
*			expands to:
*
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty03
*				/dev/tty04
*				/dev/tty08
*				/dev/tty0a
*				/dev/tty0c
*				/dev/tty0d
*				/dev/tty0e
*
*/


/*
 * expars program
 */

expars(compact, expand, limit)
    char           *compact, *expand;  /* ptrs source and expand buffers */
    int             limit;	       /* size of expand buffer */
{
    int             i;		       /* general purpose loop variable */
    int             status = 0;	       /* error status */
    unsigned char   c, *ptr;	       /* general character and pointer */

    bleft = limit - 2;		       /* set bytes left less a margin */
    srcbuf = compact;		       /* global ptr to source line */
    exbuf = expand;		       /* global ptr to expand buffer */
    root = expand;		       /* intial root name at expand buffer */
    *expand = '\0';		       /* set "null" expand buffer */

    /*
     * Parse source line
     */

    while ((*exbuf++ = *srcbuf++) != '\0') {
	if (--bleft == 0)
	    status = -1;	      /* prevent buffer overflow */

	switch (*(srcbuf - 1)) {
	case '-':		       /* range list */
	    status = exrange();
	    break;
	case ',':		       /* item list */
	    status = exlist();
	    break;
	}

	if (status)
	    break;		       /* error, stop parsing */
    }
    if (!status)
	*exbuf = '\0';		       /* mark end of buffer */
    return (status);		       /* exit with status */
}

/*
 * Expand range
 */

exrange()
{
    int             upsize;		       /* size of upper limit */

    *(exbuf - 1) = '\0';		       /* tag end root name
					        * (overwrite '-') */
    for (upsize = 0; isalnum(*srcbuf++); upsize++)
	; 				/* get size of upper limit */
    if (!upsize)
	return (-1);		       /* no upper limit, return error */
    while ((strncmp(srcbuf - 1 - upsize, exbuf - 1 - upsize, upsize)) > 0) {
	/* expand until ranges meet */
	while ((*exbuf++ = *root++) != '\0') {
	    /* copy previous root name */
	    if (--bleft == 0)
		return (-1);		       /* prevent buffer overflow */
	}
	strinc(exbuf - 1 - upsize);	       /* yet upper limit, increment */
    }
    srcbuf--;		       /* end of upper range in source line */
    exbuf--;		       /* end of latest name in buffer */
    return(0);		       /* return success */
}

/*
 * Expand list
 */

exlist()
{
    int             newsize;		/* size of new item */

    *(exbuf - 1) = '\0';		/* tag end root name(overwrite ',') */
    for (newsize = 0; (isalnum(*srcbuf++)); newsize++)
	; 				/* get size of new item */
    if (!newsize)
	return (-1);		       /* no new item, return error */
    while ((*exbuf++ = *root++) != '\0') { /* copy previous root name */
	if (--bleft == 0)
	    return (-1);	       /* prevent buffer overflow */
    }
    srcbuf -= newsize + 1;	       /* point at new item replacement */
    exbuf -= newsize + 1;	       /* point at old part to replace */
    while (newsize--) 
	*exbuf++ = *srcbuf++;	       /* replace old part of item name */
    return(0);			       /* return success */
}


/*
 * Increment a string
 */

strinc(ptr)
    char           *ptr;		       /* string to increment */
{
    int             size;		       /* size of string */

    for (size = 0; *ptr++ != '\0'; size++)
	;    					/* measure string */
    ptr--;				       /* back up to end of string */
    while (size--) {
	ptr--;				       /* back up to next character */
	(*ptr)++;			       /* increment last character */

	switch (*ptr) {		/* check for "rollover" of * range */
	case '9' + 1:	       /* end of numeric range? */
	    *ptr = '0';	       /* "roll over" for carry */
	    break;	       /* try again on next char */
	case 'Z' + 1:	       /* end of upper case alpha range? */
	    *ptr = 'A';	       /* "roll over" for carry */
	    break;	       /* try again on next char */
	case 'z' + 1:	       /* end of lower case range? */
	    *ptr = 'a';	       /* "roll over" for carry */
	    break;	       /* try again on next char */
	default:
	    return (0);	       /* done, exit */
	}
    }
    return (-1);	       /* no more places for carry, * error */
}

receive_timeout()
{
    strcpy(device, receive_dev);
    send_message(-NOT_READY, ERROR, no_response_msg, device);
}

clean_up()
{
    if (fcntl(srcfd, F_GETFL, 0) != -1) {    
	if (srcfd > 0) {
	    if (swapflag > 0) {
		if (ioctl(srcfd, TIOCNXCL, 0) == -1)	
		    send_message(0, DEBUG, no_release_msg,"S",test_name,srcfd);
	    }
	    if (swapflag > 1) {
		srclist.sg_flags = srcsav;     /* restore original tty mode */
		if (ioctl(srcfd, TIOCSETP, &srclist) == -1)	
		    send_message(0, DEBUG, no_restore_msg,"S",test_name,srcfd);
	    }
	    send_message(0, DEBUG, close_xmit_msg, xmit_dev);
	}
    }
    if (rcvfd > 0 && rcvfd != srcfd) {
	if (fcntl(rcvfd, F_GETFL, 0) != -1) {		
	    if (swapflag > 2) {
		if (ioctl(rcvfd, TIOCNXCL, 0) == -1)	
		    send_message(0, DEBUG, no_release_msg,"R",test_name,rcvfd);
	    }
	    if (swapflag > 3) {
		rcvlist.sg_flags = rcvsav;     /* restore original tty mode */
		if (ioctl(rcvfd, TIOCSETP, &rcvlist) == -1)
		    send_message(0, DEBUG, no_restore_msg,"R",test_name,rcvfd);
	    }
	}
    }
}


openfiles(srcdev, rcvdev, srcfdes, rcvfdes)
    unsigned char	*srcdev, *rcvdev;
    int		        *srcfdes, *rcvfdes;
{
    if ((*srcfdes = open(srcdev, O_RDWR, 7)) == -1) {
	perror(perror_msg);
	exit(DEV_NOT_OPEN);
    }
    send_message(0, DEBUG, open_port_msg, "transmit", srcdev, *srcfdes);
    if ( !(strcmp(srcdev, rcvdev)) ) {
	*rcvfdes = *srcfdes;
    } else if ((*rcvfdes = open(rcvdev, O_RDWR, 7)) == -1) {
	perror(perror_msg);
	exit(DEV_NOT_OPEN);
    }
    if (strcmp(srcdev, rcvdev)) 
	send_message(0, DEBUG, open_port_msg, "receive", rcvdev, *rcvfdes);
    srcfd = *srcfdes;
    rcvfd = *rcvfdes;
}


closefiles(srcfdes, rcvfdes)
    int	srcfdes, rcvfdes;
{
    if (close(srcfdes) == -1) 
	send_message(0, DEBUG, no_close_msg, test_name, srcfdes);

    send_message(0, DEBUG, close_port_msg, "transmit", srcfdes);

    if (srcfdes != rcvfdes) {
	if (close(rcvfdes) == -1) 
	    send_message(0, DEBUG, no_close_msg, test_name, rcvfdes);
        if (srcfdes != rcvfdes)
            send_message(0, DEBUG, close_port_msg, "receive", rcvfdes);
    }
}


routine_usage()
{
    printf("\
Routine specific arguments [defaults]:\n\
device = serial port(s) to test [none]\n\
	Supported devices:\n\
		/dev/ttya, /dev/ttyb\n\
		/dev/ttys0 to /dev/ttys3\n\
		/dev/tty00 to /dev/tty3f\n\
		/dev/ttyh0 to /dev/ttyhf\n\
		/dev/ttyi0 to /dev/ttyif\n\
		/dev/ttyj0 to /dev/ttyjf\n\
		/dev/ttyk0 to /dev/ttykf\n\
	Supported keywords:\n\
		{board.}double8 - first 8 ports, double loop\n\
		{board.}double12 - 12 ports 4-16, double loop\n\
		{board.}double14 - first 14 ports, double loop\n\
		{board.}pairs - 16 ports, double loop\n\
	The keywords may be prefixed with the board, 0-3 or h-k. [0.]:\n"
	);
}

process_sptest_args(argv, arrcount)
char    *argv[];
int     arrcount;
{
    int	i, match = FALSE;

    if (strncmp(argv[arrcount], "D=", 2) == 0) {
        match = TRUE;
        strcpy(sundiag_device, argv[arrcount]+2);
        device_name = sundiag_device;
    } else if (!strncmp(argv[arrcount], "/dev/tty", 8) && !keyword) {
	match = TRUE;
        if (!device_1) {
            device_1 = TRUE;
            if (expars(argv[arrcount], srcbufm, BUF_SIZE - 1))
                printf("%s: %s unable to expand the source argument.\n",                               test_name, argv[arrcount]);
            if (expars(argv[arrcount], rcvbufm, BUF_SIZE - 1))
                printf("%s: %s unable to expand the receiver argument.\n",                               test_name, argv[arrcount]);
	    send_message(0, VERBOSE, "Testing %s.", argv[arrcount]);
        } else {
            if (!device_2) {
                device_2 = TRUE;
                if (expars(argv[arrcount], rcvbufm, BUF_SIZE - 1))
                    printf("%s: %s unable to expand the receiver argument.\n",
                           test_name, argv[arrcount]);
		send_message(0, VERBOSE, "Testing %s.",argv[arrcount]);
            } else
                match = FALSE;
        }
    }

    if (!match && !device_1 && !keyword) {
        for (i = 0; *(pkg[i].key) != '\0'; i++) {
            if (!strcmp(argv[arrcount], pkg[i].key)) {
                if (expars(pkg[i].src, srcbufm, BUF_SIZE - 1))
                    send_message(0, DEBUG, no_expand_msg, "S", 
		                 test_name, pkg[i].src);
                if (expars(pkg[i].rcv, rcvbufm, BUF_SIZE - 1))
                    send_message(0, DEBUG, no_expand_msg, "R", 
		                 test_name, pkg[i].rcv);
                keyword = TRUE;
                match = TRUE;
		send_message(0, VERBOSE, "Testing %s.",argv[arrcount]);
                break;
            }
        }
    }
    return (match);
} 
