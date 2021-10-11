static char     sccsid[] = "@(#)sunlink.c 1.1 7/30/92 3.24 90/04/03 Sun Micro";

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <rpc/rpc.h>


/*
 * #include <sundev/syncmode.h> #include <sundev/syncstat.h>
 */

#define NO_PORTS_SELECTED       3
#define NO_SD_PORTS_SELECTED	4
#define NO_LOAD			5
#define NO_ATTACH		6
#define NO_LAYER		7
#define NO_LAYER_LOOPBACK	8
#define NO_SOCKET		9
#define NO_GETSYNC		10
#define NO_SETSYNC		11
#define NO_CHECK_SETSYNC	12
#define NO_BSC			13
#define NO_ASYNC		14
#define ILLEGAL_PROTOCOL	15
#define DEV_NOT_OPEN		16
#define WRITE_FAILED		17
#define READ_FAILED		18
#define RECEIVE_TIMEOUT		19
#define COMPARE_ERROR		20
#define NO_STAT_SOCKET		21
#define SIGBUS_ERROR            22
#define SIGSEGV_ERROR           23
#define BAD_SD_PORTS_SPEC	24
#define END_ERROR               25
#define DEV_NOT_AVAIL		26
#define PORT_NUM_OUT_OF_RANGE	27
#define ACTIVITY_ON_LINE	28

#define TTYB			0
#define DCP			1
#define MCP			2
#define HSS			3

#define NORMAL			0		/* HSI loopback modes */
#define NONE			3	
#define SIMPLE			4	
#define CLOCKLESS		5
#define SILENT			6
#define SILENT_CLOCKLESS	7

#define NO			0		/* hs_param loopbck flag vals */
#define YES			1
#define SLNT			2

#include "sdrtns.h"			       	/* sundiag include file */
#include "../../../lib/include/libonline.h"	/* online library include */
#include <net/if.h>
#include "syncmode.h"
#include "syncstat.h"
#include "hsparam.h"          /* additional syncmode fields for HSI board */

#define SDLC_PROTOCOL		0
#define BSC_PROTOCOL		1
#define ASYNC_PROTOCOL		2
#define LOOPBACK		3
#define MAX_LENGTH		1024
#define RECEIVE_WAIT_TIME	900
#define ERROR_THRESHOLD		4
#define MAX_IFD			20		/* max no. of /dev/ifd files */

char		device_sunlinkname[30];
char		*device = device_sunlinkname;
char		msg_buffer[MESSAGE_SIZE];
char		*msg = msg_buffer;
char            perror_msg_buffer[30];
char            *perror_msg = perror_msg_buffer;

int		sd_quick_test = FALSE;
int		either_quick_test = FALSE;
int		quick_check_flag = FALSE;
int             check_point = FALSE;
int		errors = 0;
int		match = 0;
int		return_code = 0;
int		simulate_error = 0;

u_char          pattern = 0x55, protocol;
u_char          load_dcp_kernal = FALSE;
int             port_ptr = 1;
int             internal_loopback = FALSE;
int             ttyb_test = FALSE, status_dcp = FALSE;
int             sock_type;
int             loop_count, max_frame_len, min_frame_len;
int             err_frame_no;
int             no_receive_response();
u_char          sbarray[1024], rbarray[1024];
char            tmpbuf[128], pattn_type = 'r', clock_type;
char           *lb_mptr, *nrzi_mptr, *txc_mptr, *rxc_mptr;
long            baudrate;
int             frame_count;
int             load_dcp_a = FALSE;
int             load_dcp_b = FALSE;
int             load_dcp_c = FALSE;
int             load_dcp_d = FALSE;
int             attach_a[4];
int             attach_b[4];
int             attach_c[4];
int             attach_d[4];
int             first_dev = TRUE;
int             second_dev = FALSE;
int             port_to_port = FALSE;
char            sl_port_names[10];
char           *sl_ports = sl_port_names;

int		its_hih = FALSE;       /* device is hih (HDLC) not hss */
int		its_his = FALSE;       /* device is his (SDLC) not hss */
int		cur_test;              /* current selected test */
int		loopback_type;         /* loopback type for hss */
char	       *from_ports;	       /* list of source ports */
char	       *to_ports;	       /* list of destination ports */

struct dev {
    char            device[10];
    int             fid;
    int             s;
    int             dcp;
    char            board;
    u_char          port;
    int             int_port;
    char            ifdname[8];
    u_char	    hp_v35;
    u_char	    hp_obclk;
}               testing[2] = {
    {
	"mcph0",
	0,
	0,
	FALSE,
	'h',
	'0',
	0,
	"ifd0"
    },
    {
	"mcph0",
	0,
	0,
	FALSE,
	'h',
	'0',
	0,
	"ifd0",
    }
};
struct dev     *xmit_device, *rec_device, *using_device;

struct timeval short_time = { 1, 0 };
struct timeval long_time = { 4, 0 };
struct ifreq    ifr;
struct syncmode sm;
struct syncmode *ssm = (struct syncmode *) ifr.ifr_data; 
struct hs_param *hsm = (struct hs_param *) ifr.ifr_data; /* hss param ptr */ 
struct ss_dstats sd;
struct ss_estats se;

int             s;
char           *yesno[] = {
    "no",
    "yes",
    0,
};
char           *txnames[] = {
    "txc",
    "rxc",
    "baud",
    "pll",
    0,
};

char           *rxnames[] = {
    "rxc",
    "txc",
    "baud",
    "pll",
    0,
};

static char  *test_usage_msg = 
		"[device {device}] [p={c/i/d/r}] [i{4-7}] [k] [st]";
/*
 * ------------------------------------------------------------------------
 * The main program to handle the command line The format of the command
 * line:
 * ------------------------------------------------------------------------
 */
main(argc, argv)
    int             argc;		       /* # of argument(s) */
    char           *argv[];		       /* pointer array to the
					        * arguments */
{
    extern int  process_sunlink_args();
    extern int  routine_usage();
    int error_in_first_check = 0, exit_code;

    versionid = "3.24";		/* sccs version id */
    from_ports = 0;             /* init source ports list */
    to_ports   = 0;             /* init dest ports list */

				/* begin Sundiag test */
    test_init(argc, argv, process_sunlink_args, routine_usage, test_usage_msg);

    signal(SIGALRM, no_receive_response);
			        /* assign trap handlers for SIGALRM signal */

    if (quick_test || sd_quick_test)
        either_quick_test = TRUE;
 
    if (!either_quick_test) /* no need for quick check if running quick test */
    {
	quick_check_flag = TRUE;
    	error_in_first_check = quick_check();
    }
    if (error_in_first_check)
    {
	exit(exit_code);
    }
    else
    {
	quick_check_flag = FALSE;
    	exit_code = exec_test();    /* execute test of ports */ 
    	if (exit_code) exit(exit_code);  
    }
    test_end();			/* Sundiag normal exit */
}

/*****************************************************************
 Process Sunlink specific arguments.
******************************************************************/
process_sunlink_args(argv, argindex)
char  *argv[];
int    argindex;
{
    func_name = "process_sunlink_args";
    TRACE_IN
    if (!det_ttyb(argv[argindex])) 		   /* ttyb option? */	
       if (!det_dcp(argv[argindex])) 	 	   /* dcp option? */
          if (!det_mcp(argv[argindex]))    	   /* mcp option? */
             if (!det_hss(argv[argindex]))         /* hss option? */
                if (!det_options(argv[argindex]))  /* other sunlink options? */
	        {
    		    TRACE_OUT
		    return FALSE;
		}
    TRACE_OUT
    return TRUE;
}

/* **********
 * det_ttyb
 * **********/

/* Determine if TTYB option selected */

det_ttyb(arg)
char *arg;
{
/* Make sure ttyb was not previously selected. */

    func_name = "det_ttyb";
    TRACE_IN
    if (strcmp(arg, "ttyb") == 0 && cur_test != TTYB) 
    {
	using_device = &testing[0];
	using_device->dcp = TRUE;
	exec_by_sundiag = FALSE;
	strcpy(using_device->device, "ttyb");
	ttyb_test = TRUE;
        cur_test = TTYB;
	testing[1] = testing[0];
	device_name = "ttyb";
        TRACE_OUT
	return TRUE;
    } 

    TRACE_OUT
    return FALSE;
}

/* *********
 * det_dcp
 * *********/

/* Determine if DCP option selected */

det_dcp(arg)
char *arg;  
{

/* Make sure 2 DCP ports had not been previously selected */
/* DCP argument should be of the form 'dcpxn' where 'x'=a-d and 'n'=0-3 */
/* Save source and destination ports lists for later processing */

    func_name = "det_dcp";
    TRACE_IN
    if (strncmp(arg, "dcp", 3) == 0 && (first_dev || second_dev))
	if (!(second_dev && cur_test != DCP)) {  /* previous device = DCP? */
	    cur_test = DCP;
	    set_dev_args(arg);			/* save source & dest lists */
	    device_name = "dcp";
	    TRACE_OUT
	    return TRUE;
	}

    TRACE_OUT
    return FALSE;
}

/* *********
 * det_mcp
 * *********/

/* Determine if MCP option selected */

det_mcp(arg)
char *arg;
{

/* Make sure 2 MCP ports had not been previouly selected */
/* MCP argument should be of form 'mcpnn' where 'nn'=0-15 */
/* Save source and destination ports lists for later processing */

    func_name = "det_mcp";
    TRACE_IN
    if (strncmp(arg, "mcp", 3) == 0 && (first_dev || second_dev)) 
	if (!(second_dev && cur_test != MCP)) {   /* previous device = MCP? */
	    cur_test = MCP;
            set_dev_args(arg);                    /* save source & dest lists */
	    device_name = "mcp";
	    TRACE_OUT
	    return TRUE;
	}

    TRACE_OUT
    return FALSE;
}

/* *********
 * det_hss
 * *********/

/* Determine if HSS device selected */

det_hss(arg)
char *arg;
{

/* Make sure 2 HSS ports have not been previously selected */
/* HSS argument should be of the form 'hssxny' where 'x'=e/o,'n'=0-3,'y'=r/v */
/* Save source and destination ports lists for later processing */
  
    func_name = "det_hss";
    TRACE_IN
    if (strncmp(arg, "hss", 3) == 0 && (first_dev || second_dev)) 
    {
	if (!(second_dev && cur_test != HSS))   /* previous device = HSS? */
	{   
            cur_test = HSS; 
            set_dev_args(arg);                  /* save source & dest lists */
	    device_name = "hss";
	    TRACE_OUT
	    return TRUE;
	}
    }
    else if (strncmp(arg, "hih", 3) == 0 && (first_dev || second_dev)) 
    {
	if (!(second_dev && cur_test != HSS))  /* previous device = HSS? */
        {   
            cur_test = HSS; 
            set_dev_args(arg);                 /* save source & dest lists */
	    device_name = "hih";
	    its_hih = TRUE;
	    TRACE_OUT
	    return TRUE;
	}
     }
     else
     {
        if (strncmp(arg, "his", 3) == 0 && (first_dev || second_dev)) 
	if (!(second_dev && cur_test != HSS))   /* previous device = HSS? */
	{  
            cur_test = HSS; 
            set_dev_args(arg);                  /* save source & dest lists */
	    device_name = "his";
	    its_his = TRUE;
	    TRACE_OUT
	    return TRUE;
	}
     }
    TRACE_OUT
    return FALSE;
}

/* **************
 * set_dev_args
/* **************

/* Save lists of source and destination ports */

set_dev_args(arg)
char *arg;
{

    func_name = "set_dev_args";
    TRACE_IN
    if (first_dev)
	from_ports = &arg[3];		/* pt to list of source ports */
    else if (second_dev) 
	to_ports = &arg[3];		/* pt to list of dest ports */

    second_dev = FALSE;
    if (first_dev) {
        first_dev = FALSE;
        second_dev = TRUE;
    }
    TRACE_OUT
}

/* *************
 * det_options
 * *************/

/* Check for other sunlink options */

det_options(arg)
char *arg;
{
    func_name = "det_options";
    TRACE_IN
    if (strncmp(arg, "p=", 2) == 0) {
	if (arg[2] == 'c') {
	    pattn_type = 'c';
	} else if (arg[2] == 'i') {
	    pattn_type = 'i';
	} else if (arg[2] == 'd') {
	    pattn_type = 'd';
	} else if (arg[2] == 'r') {
	    pattn_type = 'r';
	}
	else
	{
	    TRACE_OUT
	    return FALSE;
	}
    }
    else if (arg[0] == 'i') {
	loopback_type = atoi(&arg[1]);		/* get loopback type value */
	if (loopback_type >= SIMPLE && loopback_type <= SILENT_CLOCKLESS) {
	    internal_loopback = TRUE;
	}
	else
	{
	    TRACE_OUT
	    return FALSE;
	}
    }
    else if (arg[0] == 'I') {
	internal_loopback = TRUE;
    }
    else if (strcmp(arg, "st") == 0) {
	status_dcp = TRUE;
    }
    else if (strcmp(arg, "k") == 0) {
	load_dcp_kernal = TRUE;
    }
    else
    {
	TRACE_OUT
	return FALSE;
    }

    TRACE_OUT
    return TRUE;
}

/* ***********
 * quick_check:  The test duration is approximately 8mins/port.  If all four
 * ports are being tested in one pass, (the ports are being tested sequentially)
 * it would be a lengthy period of time to wait just to find out wether the 
 * loopbacks are in place.  Quick_check allows a quick check of the specified
 * ports.  
 * ***********/
quick_check()
{
    func_name = "quick_check";
    TRACE_IN
    init_parm();
    loop_count    = 1;
    min_frame_len = 10;
    max_frame_len = 11;

    proc_msg();
    proc_sund_ports();

    TRACE_OUT;
    return (return_code);
}

/* ***********
 * exec_test
 * ***********/

/* Execute selected test */

exec_test()
{
    func_name = "exec_test";
    TRACE_IN
    init_parm();        /* initialize sunlink parameters */

    proc_msg();         /* handle messages to be sent out */

    proc_sund_ports();	/* test selected ports for sundiag */

    TRACE_OUT
    return(return_code);
}

/* **********
 * proc_msg
 * **********/

/* Display exec_test messages */

proc_msg()
{
    func_name = "proc_msg";
    TRACE_IN
    sprintf(msg, "pattern= 0x%x, count= %d, min frame= %d, max frame= %d, ",
            pattern, loop_count, min_frame_len, max_frame_len);
    switch (protocol) {
    case SDLC_PROTOCOL:
        strcat(msg, "protocol= SDLC\n");
        break;
    case BSC_PROTOCOL:
        strcat(msg, "protocol= BSC\n");
        break;
    case ASYNC_PROTOCOL:
        strcat(msg, "protocol= ASYNC\n");
        break;
    case LOOPBACK:
        strcat(msg, "protocol= LOOPBACK\n");
        break;
    default:
        strcat(msg, "protocol= ILLEGAL\n");
    }
    send_message (0, DEBUG, msg);
    TRACE_OUT 
}

/* *****************
 * proc_sund_ports
 * ****************/

/* Test selected ports for sundiag */

proc_sund_ports()
{
    int from_ptr = 0, to_ptr = 0;

    func_name = "proc_sund_ports";
    TRACE_IN

    while (sund_ports_to_test(&from_ptr, &to_ptr)) 
    {
        set_ports_test();		/* setup env and test ports */
        if (!either_quick_test && !verbose && !ttyb_test && !status_dcp)
	    sleep(3);
    }
    TRACE_OUT
}

/* ****************
 * set_ports_test
 * ***************/

/* Control environment setup for ports test */

set_ports_test()
{

    func_name = "set_ports_test";
    TRACE_IN
    xmit_device = &testing[0];
    rec_device = &testing[1];

    if (strcmp(xmit_device->device, rec_device->device)) {
	port_to_port = TRUE;
    } else {
	port_to_port = FALSE;
	rec_device = &testing[0];
    }

    if (!ttyb_test) {
	using_device = xmit_device;
	set_environment();
	if (port_to_port) {
	    using_device = rec_device;
	    set_environment();
	}
    }
    test_ports(FALSE);
    if (port_to_port && !status_dcp) {
	rec_device = &testing[0];
	xmit_device = &testing[1];
	test_ports(TRUE);
    }
    TRACE_OUT
}

/* ********************
 * sund_ports_to_test
 * ********************/

sund_ports_to_test(from_ptr, to_ptr)
int *from_ptr;
int *to_ptr;
{
    func_name = "sund_ports_to_test"; 
    TRACE_IN 
    using_device = &testing[0];
						/* source port exists */
    if (get_sund_port(from_ports, from_ptr)) {	
	using_device = &testing[1];
						/* dest port does not exist */
	if (!get_sund_port(to_ports, to_ptr))  	
	    testing[1] = testing[0];		/* set for single port loopbk */
        TRACE_OUT
	return TRUE;
    } 
    TRACE_OUT
    return FALSE;
}

/* ***************
 * get_sund_port
 * ***************/

get_sund_port(ports, ptr)
char *ports;					/* list of ports */
int  *ptr;					/* current position in list */
{
#define GETTOKEN(from,to,f_indx,t_indx) while(from[f_indx] != ',' &&\
	from[f_indx] != '\0') to[t_indx++] = from[f_indx++];\
	if (from[f_indx]==',') f_indx++;\
	to[t_indx]='\0';

    char temp_str[20];
    int  str_ptr;
    int  port_ptr;
    int  dummy;					/* dummy return value */

    func_name = "get_sund_port"; 
    TRACE_IN 
    if (ports == 0 || ports[*ptr] == '\0')	/* if no more source ports */
    {
 	TRACE_OUT
	return FALSE;
    }
    
    match = FALSE;
    port_ptr = *ptr;				/* current ptr of ports list */
    if (cur_test == DCP) 
    {
	temp_str[0] = ports[0];			/* get board type */
	str_ptr = 1;
        if (!port_ptr)
	    port_ptr++;
	GETTOKEN(ports, temp_str, port_ptr, str_ptr)
	if (det_sd_dcp(temp_str, &dummy)) 
 	{
		TRACE_OUT
		return(FALSE);
	}
    }
    else {
	if (cur_test == MCP) {
	    str_ptr = 0;
  	    GETTOKEN(ports, temp_str, port_ptr, str_ptr) 
	    det_sd_mcp(temp_str, &dummy);
	}
	else {
	    str_ptr = 0;
	    GETTOKEN(ports, temp_str, port_ptr, str_ptr)
	    if ( (its_hih) || (its_his) )	/* if Sbus HSI devices */
	    	det_sd_hih(temp_str, &dummy);
	    else				/* else VME hsi */
	    	det_sd_hss(temp_str, &dummy);
	}
    }
    if (!match) {
        send_message(-BAD_SD_PORTS_SPEC, ERROR, "Invalid port specification.");
    }
    *ptr = port_ptr;
    TRACE_OUT
    return(match);
}

/* ************
 * det_sd_dcp
 * ************/

/* Determine DCP device values */

int	det_sd_dcp(arg, return_ptr)
char *arg;
int  *return_ptr;
{
    func_name = "det_sd_dcp"; 
    TRACE_IN 
    if (arg[0] >= 'a' && arg[0]  <= 'd')
    {
      if (arg[1] >= '0' && arg[1] < '4' && (arg[2] == '-' || arg[2] == '\0')) 
      {
        match = TRUE;
        using_device->dcp = TRUE;
        using_device->board = arg[0];
        using_device->port = arg[1];
        sprintf(using_device->device, "dcp%c%c", arg[0], arg[1] );
        using_device->int_port = atoi(&using_device->device[4]);
        if (arg[2] == '-')
            *return_ptr = 3;
 
        send_message(0, VERBOSE, "Testing '%s', board %c, port %c.", 
		using_device->device, using_device->board, using_device->port);
      }
      else
	if (arg[1] == '\0') 
        {
		TRACE_OUT
		return(TRUE);
	}
    }
    TRACE_OUT
    return(FALSE);
}
 
/* ************
 * det_sd_hss
 * ************/

/* Determine HSS device values */
 
det_sd_hss(arg, return_ptr)
char *arg;
int  *return_ptr;
{
 
    if ((arg[0]=='e' || arg[0]=='o') && arg[1]>='0' && arg[1]<='3' &&
        (arg[2]=='r' || arg[2]=='v') && (arg[3] == '-' || arg[3] == '\0'))
    {
        match = TRUE;
        using_device->dcp = FALSE;
        sprintf(using_device->device, "hss%c", arg[1]);
        using_device->int_port = atoi(&using_device->device[3]);
 
        if (using_device->int_port < 2)
            using_device->board = 'm';
        else if (using_device->int_port < 4)
            using_device->board = 'n';

        using_device->hp_obclk = FALSE;
        using_device->hp_v35   = FALSE;

        if (arg[0] == 'o')      /* if on-board clock */
            using_device->hp_obclk = TRUE;
        if (arg[2] == 'v')      /* if V.35 */
            using_device->hp_v35 =TRUE;

        if (arg[3] == '-')
            *return_ptr = 4;

        send_message(0, VERBOSE, "Testing '%s', board %c, port %d.",
          using_device->device, using_device->board, using_device->int_port);
    }
}

/* ************
 * det_sd_hih
 * ************/
 
/* Determine Sbus HSI device values */
 
det_sd_hih(arg, return_ptr)
char *arg;
int  *return_ptr;
{ 
    func_name = "det_sd_hih"; 
    TRACE_IN 

    if ((arg[0]=='e' || arg[0]=='o') && (arg[1]>='0' && arg[1]<='9') &&
        ((arg[2]>='0' && arg[2]<='9') || arg[2] == '-' || arg[2] == '\0'))
    {
        match = TRUE;
        using_device->dcp = FALSE;

        using_device->int_port = atoi(&arg[1]);
	if (!((using_device->int_port >=0) && (using_device->int_port < 20)))
		send_message (-PORT_NUM_OUT_OF_RANGE, ERROR, "Port number specified is out of range.");

	if (its_hih)
        	sprintf(using_device->device, "hih%d", using_device->int_port);
	else 		/* if (its_his) */ 
	      	sprintf(using_device->device, "his%d", using_device->int_port);
 
        if (using_device->int_port < 4)
            using_device->board = '1';
        else if (using_device->int_port < 8)
            using_device->board = '2';
        else if (using_device->int_port < 12)
            using_device->board = '3';
        else if (using_device->int_port < 16)
            using_device->board = '4';
        else
            using_device->board = '5';
 
        using_device->hp_obclk = FALSE;
        using_device->hp_v35   = FALSE;
 
        if (arg[0] == 'o')      /* if on-board clock */
            using_device->hp_obclk = TRUE;
  
        if (arg[2] == '-')
            *return_ptr = 3;
  
	if (quick_check_flag)
	{
    	    send_message(0, VERBOSE, "Quick check of port connection(s): device '%s', board %c, port %d.",
              using_device->device,using_device->board, using_device->int_port);
        }
	else
	{
	    send_message(0, VERBOSE, "Testing '%s', board %c, port %d.",
	      using_device->device,using_device->board, using_device->int_port);
	}
    }
    TRACE_OUT
} 
  
  
/* ************
 * det_sd_mcp
 * ************/
 
/* Determine MCP device values */
  
det_sd_mcp(arg, return_ptr)
char *arg;
int  *return_ptr;
{
    func_name = "det_sd_mcp"; 
    TRACE_IN 
    if (isdigit(arg[0])) 
    {
        if (isdigit(arg[1]) || arg[1]=='-' || arg[1]=='\0') 
        {
            sprintf(using_device->device, "mcph%c%c", arg[0], arg[1] != '-' ? arg[1] : '\0');
            using_device->int_port = atoi(&using_device->device[4]);
            if (using_device->int_port >= 0 && using_device->int_port < 16) 
	    {
                match = TRUE;
                using_device->dcp = FALSE;
 
                if (using_device->int_port < 4)
                    using_device->board = 'h';
                else if (using_device->int_port < 8)
                    using_device->board = 'i';
                else if (using_device->int_port < 12)
                    using_device->board = 'j';
                else
                    using_device->board = 'k';
 
                if (arg[1] == '-')
                    *return_ptr = 2;
                else if (arg[2] == '-')
                    *return_ptr = 3;
      
		send_message(0, VERBOSE, "Testing '%s', board %c, port %d.", 
			using_device->device, using_device->board, 
			using_device->int_port);
            }
        }
    } 
    TRACE_OUT
}

set_environment()
{
    /*
     * ---------------------------------------------------------------- set
     * the environment of the DCP board by the system function call. 1)
     * download the DCP kernel, by dcpload command (dcpload dcpmon.image). 2)
     * initialize the selected channel, by dcpattach command (dcpattach
     * /dev/dcpXX). 3) configure the selected channel to the selected
     * protocol, by dcplayer command (dcplayer dcpXX YYY ZZZ).
     * -----------------------------------------------------------------
     */

    int             load_kernal = FALSE, attach_dcp = FALSE;


    func_name = "set_environment"; 
    TRACE_IN 
    set_dcp_parms(&load_kernal,&attach_dcp); /* define parm values for dcp */
    load_dcp_kern(load_kernal);              /* download dcp kernal */
    init_dcp_chnel(attach_dcp);              /* init selected channel for dcp */

/* Setup conditions for selected protocol. */
/* MCP and HSS are concerned only with SDLC */

    switch (protocol) {
    case SDLC_PROTOCOL:

        config_dcp_chnel(attach_dcp);        /* config selected channel with */
                                             /* selected protocol */
        init_brd_sdlc();                     /* syncinit board */

	break;

    case BSC_PROTOCOL:
	send_message(-NO_BSC, ERROR,"BSC protocol is not implemented for '%s'.",
                using_device->device);
	break;
    case ASYNC_PROTOCOL:
	send_message(-NO_ASYNC, ERROR, 
		"ASYNC protocol is not implemented for '%s'.",
		 using_device->device);
	break;
    case LOOPBACK:

	if (using_device->dcp) {
	    if (simulate_error == NO_LAYER_LOOPBACK)
		sprintf(tmpbuf, "/no/dcplayer %s lo%c", using_device->device, using_device->port);
	    else
		sprintf(tmpbuf, "/usr/sunlink/dcp/dcplayer %s lo%c",
			using_device->device, using_device->port);
	    send_message(0, DEBUG, "Executing %s\n", tmpbuf);
            if (system(tmpbuf))
		couldnt_execute(NO_LAYER_LOOPBACK);
	}
	break;

    default:
	send_message(-ILLEGAL_PROTOCOL, ERROR, 
		"Illegal protocol specified for '%s'.", using_device->device);
	break;
    }
    TRACE_OUT
}

/* ***************
 * set_dcp_parms
 * ***************/

/* Define parameter values if testing dcp board */

set_dcp_parms(load_kernal, attach_dcp)
int  *load_kernal;
int  *attach_dcp;
{

    func_name = "set_dcp_parms"; 
    TRACE_IN 
    switch (using_device->board) {
    case 'a':
	*load_kernal = load_dcp_a;
	load_dcp_a = FALSE;
	*attach_dcp = attach_a[using_device->int_port];
	attach_a[using_device->int_port] = FALSE;
	break;
    case 'b':
	*load_kernal = load_dcp_b;
	load_dcp_b = FALSE;
	*attach_dcp = attach_b[using_device->int_port];
	attach_b[using_device->int_port] = FALSE;
	break;
    case 'c':
	*load_kernal = load_dcp_c;
	load_dcp_c = FALSE;
	*attach_dcp = attach_c[using_device->int_port];
	attach_c[using_device->int_port] = FALSE;
	break;
    case 'd':
	*load_kernal = load_dcp_d;
	load_dcp_d = FALSE;
	*attach_dcp = attach_d[using_device->int_port];
	attach_d[using_device->int_port] = FALSE;
	break;
    }
    TRACE_OUT
}

/* ***************
 * load_dcp_kern
 * ***************/

/* Download the dcp kernal */

load_dcp_kern(load_kernal)
int  load_kernal;
{

    func_name = "load_dcp_kern"; 
    TRACE_IN 
    if (load_kernal) 
    {
	if (simulate_error == NO_LOAD)
	    sprintf(tmpbuf,
		    "/no/dcpload -b %c /usr/sunlink/dcp/dcpmon.image", 
	    	    using_device->board);
	else
	    sprintf(tmpbuf,
		 "/usr/sunlink/dcp/dcpload -b %c /usr/sunlink/dcp/dcpmon.image", 		 using_device->board);

	send_message(0, DEBUG, "Executing %s\n", tmpbuf);
	if (system(tmpbuf))
		couldnt_execute(NO_LOAD);
    }
    TRACE_OUT
}

/* ****************
 * init_dcp_chnel 
 * ****************/

/* Initialize the selected channel */

init_dcp_chnel(attach_dcp)
int  attach_dcp;
{

    func_name = "init_dcp_chnel"; 
    TRACE_IN 
    if (attach_dcp) 
    {
	if (simulate_error == NO_ATTACH)
	    sprintf(tmpbuf, "/no/dcpattach /dev/%s", using_device->device);
	else
	    sprintf(tmpbuf, "/usr/sunlink/dcp/dcpattach /dev/%s > /dev/null", using_device->device);

	send_message(0, DEBUG, "Executing %s\n", tmpbuf);
	if (system(tmpbuf))
		couldnt_execute(NO_ATTACH);
    }
    TRACE_OUT
}

/* ******************
 * config_dcp_chnel
 * ******************/

/* Configure selected channel to the selected protocol by dcplayer command */

config_dcp_chnel(attach_dcp)
int  attach_dcp;
{

    	func_name = "config_dcp_chnel"; 
    	TRACE_IN 
	if (attach_dcp) 
        {
	    if (simulate_error == NO_LAYER)
		sprintf(tmpbuf, "/no/dcplayer %s zss%c", using_device->device, using_device->port);
	    else
		sprintf(tmpbuf, "/usr/sunlink/dcp/dcplayer %s zss%c", 
                        using_device->device, using_device->port);

	    send_message(0, DEBUG, "Executing %s\n", tmpbuf);
	    if (system(tmpbuf))
		    couldnt_execute(NO_LAYER);
	}
        TRACE_OUT
}

/* ***************
 * init_brd_sdlc 
 * ***************/

/* Syncinit board thru driver */

init_brd_sdlc()
{

    	func_name = "init_brd_sdlc"; 
    	TRACE_IN 
	if (simulate_error == NO_SOCKET)
	    sock_type = 7;
	else
	    sock_type = SOCK_DGRAM;

/* Get socket for ioctl communication with driver */
	if ((using_device->s = socket(AF_INET, sock_type, 0)) < 0) 
 	{
	    perror("sunlink: socket, perror says");
	    send_message(-NO_SOCKET, ERROR, "Couldn't open a socket for '%s'.",
                    using_device->device);
	}
	if (simulate_error == NO_GETSYNC)
	    s = 0;

/* Set ifr_name to identify device driver for communication */
	strcpy(ifr.ifr_name, using_device->device);

/* Get current sync mode information */
	if (ioctl(using_device->s, SIOCGETSYNC, &ifr)) 
 	{
	    perror("sunlink: SIOCGETSYNC, perror says");
	    send_message(-NO_GETSYNC, ERROR, 
		"Couldn't get sync mode info for '%s'.", using_device->device);
	}
	if (debug) 
	{
	    if (simulate_error == NO_CHECK_SETSYNC)
		s = 0;
	    strcpy(ifr.ifr_name, using_device->device);
	    if (ioctl(using_device->s, SIOCGETSYNC, &ifr)) 
	    {
		    perror("sunlink: SIOCGETSYNC, perror says");
		    send_message(-NO_CHECK_SETSYNC, ERROR, 
				"Couldn't check SETSYNC for '%s'.",
                                using_device->device);
	    }
	}

	if (cur_test == DCP)
            set_sync_dcp();
        else
            if (cur_test == MCP)
                set_sync_mcp();
            else
		if ((its_his) || (its_hih))
                	set_sync_hih();
		else
                	set_sync_hss();

	send_message(0, DEBUG, "clock type = %c\n", clock_type);

	/*
	 * if (debug) { printf("txc=%s, rxc=%s\n", txc_mptr, rxc_mptr);
	 * printf("syncinit '%s' %d loopback=%s nrzi=%s txc=%s rxc=%s",
	 * using_device->device, baudrate, lb_mptr, nrzi_mptr, txc_mptr,
	 * rxc_mptr); sprintf(tmpbuf, "/usr/src/sunlink/inr/syncinit %s %d
	 * loopback=%s nrzi=%s txc=%s rxc=%s", using_device->device,
	 * baudrate, lb_mptr, nrzi_mptr, txc_mptr, rxc_mptr);
	 * printf("Executing %s\n", tmpbuf); system(tmpbuf); }
	 */

	if (simulate_error == NO_SETSYNC)
	    s = 0;

/* Set ifr_name to identify driver for communication */
	strcpy(ifr.ifr_name, using_device->device);

/* Set new sync mode information */ 
	if (ioctl(using_device->s, SIOCSETSYNC, &ifr)) {
	    perror("sunlink: SIOCSETSYNC, perror says");
	    send_message(-NO_SETSYNC, ERROR, 
		"Couldn't set sync mode info for '%s'.", using_device->device);
	}
	if (debug) 
   	{
	    if (simulate_error == NO_CHECK_SETSYNC)
		s = 0;
	    strcpy(ifr.ifr_name, using_device->device);
	    if (ioctl(using_device->s, SIOCGETSYNC, &ifr)) 
   	    {
		    perror("sunlink: SIOCGETSYNC, perror says");
		    send_message(-NO_CHECK_SETSYNC, ERROR, 
				"Couldn't check SETSYNC for '%s'.",
                                using_device->device);
	    }
	    printf
		("speed= %d, internal loopback= %s, nrzi= %s, txc= %s, rxc= %s\n",
	     ssm->sm_baudrate, yesno[ssm->sm_loopback], yesno[ssm->sm_nrzi],
		 txnames[ssm->sm_txclock], rxnames[ssm->sm_rxclock]);
	}
	strcpy(ifr.ifr_name, using_device->device);
        if (ioctl(using_device->s, SIOCGETSYNC, &ifr)) 
   	{
           perror("sunlink: SIOCGETSYNC, perror says");
           send_message(-NO_CHECK_SETSYNC, ERROR, 
			    "Couldn't check SETSYNC for '%s'.",
                            using_device->device);
        }
	TRACE_OUT
}

/* **************
 * set_sync_dcp
 * **************/

/* Set sync mode data for DCP testing */

set_sync_dcp()
{
    func_name = "set_sync_dcp"; 
    TRACE_IN 
    ssm->sm_baudrate = baudrate;
    ssm->sm_txclock = TXC_IS_BAUD;
    ssm->sm_rxclock = RXC_IS_BAUD;
    if (internal_loopback)
	ssm->sm_loopback = (char) 1;           /* do internal loopback */
    else {               		       /* clock_type=0 for int loopbk */
	lb_mptr = "no";
    	ssm->sm_loopback = (char) 0;
    }
    TRACE_OUT
}

/* **************
 * set_sync_mcp
 * **************/

/* Set sync mode data for MCP testing */

set_sync_mcp()
{
    func_name = "set_sync_mcp"; 
    TRACE_IN 
    ssm->sm_baudrate = baudrate;
    ssm->sm_txclock = TXC_IS_BAUD;
    ssm->sm_rxclock = RXC_IS_RXC;              /* use the loopback clock only
                                                * on mcp ports */
    if (internal_loopback)
	ssm->sm_loopback = (char) 1;           /* do internal loopback */
    else                 		       /* clock_type=0 for int loopbk */
    {
	lb_mptr = "no";
    	ssm->sm_loopback = (char) 0;
    }
    TRACE_OUT
}

/* **************
 * set_sync_hih
 * **************/

/* Set sync mode data (in ssm struct fields) for Sbus-HSI testing */
/* S-Bus HSI only uses the ssm (syncmode struct) structure and not */
/* the syncmode structure inside the hsm structure; therefore, */
/* values are only assigned to the ssm struct */

set_sync_hih()
{
    func_name = "set_sync_hih"; 
    TRACE_IN 
    ssm->sm_baudrate = 100000;   /* 100,000 kbits per sec */
				 /* could also be set to 2.048 Mbits per sec 
				    but may have some difficulties testing
				    more than one port at this speed */
    if (internal_loopback)	 /* S-Bus HSI only allows on-board clock */
    {				 /* source when running internal loopback */
	ssm->sm_loopback = YES;
        ssm->sm_txclock = TXC_IS_BAUD;   /* use on-board clock */
        ssm->sm_rxclock = RXC_IS_BAUD;  
    }
    else 
    {
        lb_mptr = "no";
        ssm->sm_loopback = NO;
        if (using_device->hp_obclk)          /* if using on-board clock source */ 
        {
            ssm->sm_txclock = TXC_IS_BAUD;
            ssm->sm_rxclock = RXC_IS_RXC;
        }
        else				     /* if using external clock source, */ 
        {				     /* user must physically loopback */
            ssm->sm_txclock = TXC_IS_TXC;    /* xmit & recv pins and provide */
            ssm->sm_rxclock = RXC_IS_RXC;    /* the external clock source */
        }
    }    
    TRACE_OUT
}

/* **************
 * set_sync_hss
 * **************/

/* Set sync mode data for HSS testing */

set_sync_hss()
{
    func_name = "set_sync_hss"; 
    TRACE_IN 
                                      /* set syncmode field of hss struct */
    ssm = (struct syncmode *)&hsm->hp_mode;
    hsm->hp_mode.sm_baudrate = 9600;  /* 2.048 Mbits per sec */
    hsm->hp_tpval = 0;			 /* poll time value */
    hsm->hp_fwm = 2;
    hsm->hp_burst = 2;
    if (using_device->hp_v35)
	hsm->hp_v35 = 1;
    else
	hsm->hp_v35 = 0;
    
    if (internal_loopback) 
	switch(loopback_type) {
	    case SIMPLE :
    		ssm->sm_loopback = YES;
		hsm->hp_lback = SIMPLE;	       /* assert loopback */
	    	hsm->hp_mode.sm_txclock = TXC_IS_BAUD;	 /* use onboard clock */
		hsm->hp_mode.sm_rxclock = RXC_IS_BAUD;
		break;
	    case CLOCKLESS :
		ssm->sm_loopback = YES;
		hsm->hp_lback = CLOCKLESS;
		hsm->hp_mode.sm_txclock = TXC_IS_SYSCLK; /* use sysclock */
		hsm->hp_mode.sm_rxclock = RXC_IS_SYSCLK;
		break;
	    case SILENT :
		ssm->sm_loopback = SLNT;
		hsm->hp_lback = SILENT;	       /* assert silent loopback */
    		hsm->hp_mode.sm_txclock = TXC_IS_BAUD;
		hsm->hp_mode.sm_rxclock = RXC_IS_BAUD;
                break;
            case SILENT_CLOCKLESS :
		ssm->sm_loopback = SLNT;
		hsm->hp_lback = SILENT_CLOCKLESS;
                hsm->hp_mode.sm_txclock = TXC_IS_SYSCLK;
                hsm->hp_mode.sm_rxclock = RXC_IS_SYSCLK;
                break; 
	}
    else 
    {
	hsm->hp_lback = NORMAL;
        lb_mptr = "no";
	ssm->sm_loopback = NO;
	if (using_device->hp_obclk) 
        {
	    hsm->hp_mode.sm_txclock = TXC_IS_BAUD;
	    hsm->hp_mode.sm_rxclock = RXC_IS_RXC;
	}
	else 
  	{
	    hsm->hp_mode.sm_txclock = TXC_IS_TXC;
	    hsm->hp_mode.sm_rxclock = RXC_IS_RXC;
	}
    }
    TRACE_OUT
}


/* **************
 * couldnt_execute
 * **************/

couldnt_execute(who)
int       who;
{
    char            file_name_buffer[30];
    char           *file_name = file_name_buffer;

    func_name = "couldnt_execute"; 
    TRACE_IN 
    switch (who) {
    case NO_LOAD:{
	    sprintf(file_name, "load' for dcp %c", using_device->board);
	    break;
	}
    case NO_ATTACH:{
	    sprintf(file_name, "attach' for '%s'", using_device->device);
	    break;
	}
    case NO_LAYER:
    case NO_LAYER_LOOPBACK:{
	    sprintf(file_name, "layer' for '%s'", using_device->device);
	}
    }

    send_message(-who, ERROR, 
	"Couldn't successfully execute '/usr/sunlink/dcp/dcp%s.", file_name);
    TRACE_OUT
}

/* **************
 * exercise_dcp
 * **************/

exercise_dcp()
{
    register        i, j, k, transmit_length, lp1;
    u_char         *sbufptr, *rbufptr;

    func_name = "exercise_dcp"; 
    TRACE_IN 
    sbufptr = sbarray;
    rbufptr = rbarray;
    frame_count = max_frame_len - min_frame_len + 1;
    if (simulate_error == WRITE_FAILED) 
    {
	flock(xmit_device->fid, LOCK_UN);	/* unlock file */
        close(xmit_device->fid);
    }
    if (simulate_error == READ_FAILED) 
    {
	flock(rec_device->fid, LOCK_UN);
        close(rec_device->fid);
    }

    for (lp1 = 1; lp1 <= loop_count; lp1++) 
    {
        transmit_length = min_frame_len;
	prepare_buffer();
	for (i = 0; i < frame_count; i++) 
        {
	    for (j = 0; j <= transmit_length; j++)  
	    {
		if (ttyb_test)
		    rbarray[j] = sbarray[j];
		else
		    rbarray[j] = 0;		/* prepare receive buffer */
	    }

	    line_check(rbufptr, transmit_length);
	    xmit_buf(sbufptr, transmit_length, i);
	    receive_buf(rbufptr, transmit_length);
	    compare_buf(sbufptr, rbufptr, transmit_length);
	    ++transmit_length;
	}
    }
    TRACE_OUT
}


/*
 *	line_check:  checks for any current activity on port
 */

line_check(rbufptr, transmit_length)
u_char  *rbufptr;
u_long  transmit_length;
{
        int readmask;
 
    	func_name = "line_check"; 
    	TRACE_IN 
        send_message(0, DEBUG, "Checking for activity on line.");
        readmask = 1 << rec_device->fid;
        while (select(32, &readmask, 0, 0, &short_time) == 1) 
        {
                (void) read(rec_device->fid, rbufptr, transmit_length);
                readmask = 1 << rec_device->fid;
        }
        readmask = 1 << rec_device->fid;
        if (select(32, &readmask, 0, 0, &long_time) == 1) 
        {
                send_message(-ACTIVITY_ON_LINE, ERROR,"Packet received but none sent!  \
                   Activity on-line.  Quiesce other end before starting.");
        }
	TRACE_OUT
}


/* **********
 * xmit_buf 
 * **********/

/* Transmit buffer */

xmit_buf(sbufptr, transmit_length, buf_cnt)
u_char  *sbufptr;
u_long  transmit_length;
int	buf_cnt;
{
    int             nbytes;

    func_name = "xmit_buf"; 
    TRACE_IN 
    send_message(0, DEBUG, "Starting transmit '%s'", xmit_device->device);

    if (simulate_error != RECEIVE_TIMEOUT) 
    {
	if ((nbytes = write(xmit_device->fid, sbufptr, transmit_length)) == -1) 
	{
	    perror(perror_msg);
	    errors++;
	    send_message(-WRITE_FAILED, ERROR, "transmit failed on '%s'.", 
				xmit_device->device);
	}
    }
    send_message(0, DEBUG, "Transmit complete '%s', buffer %d length %d",
               xmit_device->device, buf_cnt + 1, transmit_length);
    if (debug)
	sleep(1);

    if (check_point) 
    {
	printf("Data transmitted on device '%s', data = %x %x %x.\n", xmit_device->device, sbarray[0], sbarray[1], sbarray[2]);
    }
    TRACE_OUT
}

/* *************
 * receive_buf
 * *************/

receive_buf(rbufptr, transmit_length)
u_char  *rbufptr;
u_long  transmit_length;
{
    int             nbytes;

    func_name = "receive_buf"; 
    TRACE_IN 
    send_message(0, DEBUG, "Starting receive '%s'", rec_device->device);
    
    if (either_quick_test)
	alarm(4);
    else
	alarm(RECEIVE_WAIT_TIME);
    if (!ttyb_test || simulate_error == READ_FAILED || simulate_error == RECEIVE_TIMEOUT)
	if ((nbytes = read(rec_device->fid, rbufptr, transmit_length)) == -1) 
	{
	    alarm(0);
	    perror(perror_msg);
	    errors++;
	    send_message(-READ_FAILED, ERROR, "receive failed on '%s'.", 
		rec_device->device);
	}
    alarm(0);

    send_message(0, DEBUG, "Receive complete '%s', length %d.",
                rec_device->device, nbytes);

    if (check_point) 
    {
	printf("Data received on device '%s', data = %x %x %x.\n", rec_device->device, rbarray[0], rbarray[1], rbarray[2]);
    }
    TRACE_OUT
}

/* *************
 * compare_buf
 * *************/

/* Compare transmitted and received buffers */

compare_buf(sbufptr, rbufptr, transmit_length)
u_char  *sbufptr;
u_char  *rbufptr;
u_long  transmit_length;
{
    register	    j;

    func_name = "compare_buf"; 
    TRACE_IN 
    if (simulate_error == COMPARE_ERROR)
        sbarray[transmit_length - 1]--;  /* manipulate data for compare error */
   
    if (!ttyb_test || simulate_error == COMPARE_ERROR)
	if (bcmp(sbufptr, rbufptr, transmit_length)) 
	{
	    errors++;
	    for (j = 0; j <= transmit_length; j++) 
	    {
		if (rbarray[j] != sbarray[j]) 
		    break;
	    }
	    sprintf(msg, "data compare error on '%s', exp = 0x%x, actual = 0x%x, offset = %d.", rec_device->device, sbarray[j], rbarray[j], j);
	    if (!run_on_error || errors >= ERROR_THRESHOLD) 
		send_message(-COMPARE_ERROR, ERROR, msg);
	    else
		send_message(0, WARNING, msg); 

	    return_code = COMPARE_ERROR;
	}
    TRACE_OUT
}

no_receive_response()
{
    func_name = "no_receive_response"; 
    TRACE_IN 
    send_message(-RECEIVE_TIMEOUT, ERROR, 
	"'%s' does not respond, check loopback connector.", rec_device->device);
    TRACE_OUT
}

/*
 * -------------------------------------------------------------------
 * prepare sending buffer
 * -------------------------------------------------------------------
 */

prepare_buffer()
{
    register int    k, j;
    register u_char *mptr;

    func_name = "prepare_buffer"; 
    TRACE_IN 
    mptr = sbarray;
    switch (pattn_type) {

    case 'c':
	for (j = 0; j < MAX_LENGTH; j++)      /* filling */
	    *mptr++ = pattern;
	break;
    case 'i':
	k = pattern;
	for (j = 0; j < MAX_LENGTH; j++)      /* filling */
	    *mptr++ = (u_char) k++;
	break;
    case 'd':
	k = pattern;
	for (j = 0; j < MAX_LENGTH; j++)      /* filling */
	    *mptr++ = (u_char) k--;
	break;
    case 'r':
	for (j = 0; j < MAX_LENGTH; j++)      /* filling */
	    *mptr++ = (u_char) random();
	break;
    }
    TRACE_OUT
}

/*
 * -----------------------------------------------------------------------
 * this subroutine displays the statistic
 * -----------------------------------------------------------------------
 */

statistics()
{
    int             io_op;
    int             ss;

    func_name = "statistics"; 
    TRACE_IN 	
    if (simulate_error == NO_STAT_SOCKET)
	sock_type = 7;
    else
	sock_type = SOCK_DGRAM;
    if ((ss = socket(AF_INET, sock_type, 0)) < 0) 
    {
	perror("sunlink: socket, perror says");
	send_message(0, INFO, "Couldn't open a socket for statistics, '%s'.",
            using_device->device);
	if (simulate_error != NO_STAT_SOCKET)
	{
	    TRACE_OUT
	    return;
	}
    }
    strcpy(ifr.ifr_name, using_device->device);
    if (ioctl(using_device->s, SIOCGETSYNC, &ifr)) 
    {
	perror("sunlink: SIOCGETSYNC, perror says");
	fprintf(stderr,
		"sunlink: Couldn't get sync mode statistics for '%s'.\n", using_device->device);
    }
    sm = *(struct syncmode *) ifr.ifr_data;
    strcpy(ifr.ifr_name, using_device->device);
    if (ioctl(using_device->s, SIOCSSDSTATS, &ifr)) 
    {
	perror("sunlink: SIOCSSDSTATS, perror says");
	fprintf(stderr,
		"sunlink: Couldn't get data statistics for '%s'.\n", using_device->device);
    }
    sd = *(struct ss_dstats *) ifr.ifr_data;
    strcpy(ifr.ifr_name, using_device->device);
    if (ioctl(using_device->s, SIOCSSESTATS, &ifr)) 
    {
	perror("sunlink: SIOCSSESTATS, perror says");
	fprintf(stderr,
		"sunlink: Couldn't get error statistics for '%s'.\n", using_device->device);
    }
    if (ss)
	close(ss);
    se = *(struct ss_estats *) ifr.ifr_data;

    printf("\n%s: ", using_device->device);
    printf("baud %d", sm.sm_baudrate);
    printf("  pkts: i %d", sd.ssd_ipack);
    printf("  o %d", sd.ssd_opack);
    printf("  chars: i %d", sd.ssd_ichar);
    printf("  o %d\n", sd.ssd_ochar);
    printf("       underruns %d", se.sse_underrun);
    printf("  overruns %d", se.sse_overrun);
    printf("  aborts %d", se.sse_abort);
    printf("  crcs %d", se.sse_crc);
    printf("  error frames %d\n\n", err_frame_no);
    
    TRACE_OUT
}

layer(dir, name, oname)
int	dir;
char	*name, *oname;
{
    struct ifreq    ifr_tmp;

    func_name = "layer"; 
    TRACE_IN 
    strncpy(ifr_tmp.ifr_name, name, sizeof(ifr_tmp.ifr_name));
    strncpy(ifr_tmp.ifr_oname, oname, sizeof(ifr_tmp.ifr_oname));
    TRACE_OUT
    return ioctl(using_device->s, dir, (caddr_t) & ifr_tmp);
}

/*
 * ------------------------------------------------------------- set default
 * parameters for the sunlink
 * -------------------------------------------------------------
 */
init_parm()
{
    int             i, disp;

    func_name = "init_parm"; 
    TRACE_IN 
    protocol = SDLC_PROTOCOL;
    if (simulate_error != 0) 
    {
        if (simulate_error == NO_LAYER_LOOPBACK)
            protocol = LOOPBACK;
        if (simulate_error == NO_BSC)
            protocol = BSC_PROTOCOL;
        if (simulate_error == NO_ASYNC)
            protocol = ASYNC_PROTOCOL;
        if (simulate_error == ILLEGAL_PROTOCOL)
            protocol = ILLEGAL_PROTOCOL;
    }

    baudrate = 9600;
    if (internal_loopback)
	clock_type = '0';
    else
	clock_type = '1';
    loop_count = 1;
    if (debug || ttyb_test || simulate_error) 
    {
	min_frame_len = 10;
	max_frame_len = 13;
    } 
    else 
    {
	if (internal_loopback) 
        {
	/*  min_frame_len = 1; */
	    min_frame_len = 2;
	    max_frame_len = 255;
	} 
        else 
	{
	    min_frame_len = MAX_LENGTH;
	    max_frame_len = MAX_LENGTH;
	    if (!either_quick_test)
		loop_count = 100;
	}
    }
    disp = FALSE;
    if (load_dcp_kernal) 
    {
	load_dcp_a = TRUE;
	load_dcp_b = TRUE;
	load_dcp_c = TRUE;
	load_dcp_d = TRUE;
	disp = TRUE;
    }
    for (i = 0; i < 4; i++) 
    {
	attach_a[i] = disp;
	attach_b[i] = disp;
	attach_c[i] = disp;
	attach_d[i] = disp;
    }
    lb_mptr = "yes";
    nrzi_mptr = "no";
    txc_mptr = "baud";
    rxc_mptr = "baud";
    ssm->sm_txclock = TXC_IS_BAUD;
    ssm->sm_rxclock = TXC_IS_BAUD;
    ssm->sm_loopback = 1;
    ssm->sm_nrzi = 0;

    TRACE_OUT
}

test_ports(ports_open)
int ports_open;			/* indicates if ports already open */
				/* port to port only */
{

    func_name = "test_ports"; 
    TRACE_IN 
    if (!ports_open) 
    {
    	using_device = xmit_device;
    	open_ports();
    	if (port_to_port) 
        {
	    using_device = rec_device;
	    open_ports();
       	}
    }

    strcpy(device, rec_device->device);	/* set name for error report */
    device_name = device;

    if (!status_dcp)
        exercise_dcp();

    if (status_dcp || (debug && !ttyb_test)) 
    {
	using_device = xmit_device;
	statistics();
	if (port_to_port) 
        {
	    using_device = rec_device;
	    statistics();
	}
    }

    if ((port_to_port && ports_open) || !port_to_port) 
    {
    	if (xmit_device->s)
	    close(xmit_device->s);
	send_message(0, DEBUG, "Close transmit port '%s'.\n", 
		xmit_device->device);

    	if (xmit_device->fid) 
        {
	    flock(xmit_device->fid, LOCK_UN);	/* unlock file */
	    close(xmit_device->fid);
        }

    	if (rec_device->s)
            close(rec_device->s);
	send_message(0, DEBUG,"Close receive port '%s'.\n", rec_device->device);
 
    	if (rec_device->fid) 
   	{
	    flock(rec_device->fid, LOCK_UN);	/* unlock file */
            close(rec_device->fid);
    	}
    }
    TRACE_OUT
}

open_ports()
{
    int i;
    int open_ret;

    func_name = "open_ports"; 
    TRACE_IN 
    if (using_device->dcp || simulate_error == DEV_NOT_OPEN) 
    {
	sprintf(tmpbuf, "/dev/%s", 
                using_device->dcp ? using_device->device : "dev.invalid");

	if ((using_device->fid = open(tmpbuf, O_RDWR, 0)) == -1) 
	{
	    perror(perror_msg);
	    sprintf(msg, "Couldn't open file '%s'.", tmpbuf);
            if (run_on_error && (verbose || single_pass))		
		send_message(0, WARNING, msg);
	    else
		send_message(-DEV_NOT_OPEN, ERROR, msg);
	    return_code = DEV_NOT_OPEN;
	    TRACE_OUT
	    return(FALSE);
	} 
    } 
    else 
    {
	for (i=0; i < MAX_IFD; i++) 
 	{
            sprintf(tmpbuf, "/dev/ifd%d", i);   /* ifd name for MCP or HSI */
 
	    send_message(0, DEBUG, "opening %s\n", tmpbuf);
            if ((using_device->fid = open(tmpbuf, O_RDWR|O_EXCL, 0)) != -1) 
	    {
     						/* locked? */
                if (flock(using_device->fid, LOCK_EX|LOCK_NB) != -1)
                    break;
		else
		    close(using_device->fid);
	    }
        }
	if (i >= MAX_IFD)                       /* no available ifd file */
 	{
            strcpy(msg, "No /dev/ifd files available.");
		
            if (run_on_error && (verbose || single_pass))
                send_message(0, WARNING, msg);
            else
                send_message(-DEV_NOT_AVAIL, ERROR, msg);

	    return_code = DEV_NOT_AVAIL;
	    TRACE_OUT
	    return FALSE;
	}
	sprintf(using_device->ifdname, "ifd%d", i);
        layer(SIOCUPPER, using_device->device, using_device->ifdname);
	layer(SIOCLOWER, using_device->ifdname, using_device->device);
    }
    TRACE_OUT
}

routine_usage()
{
    func_name = "proc_sund_ports"; 
    TRACE_IN 
    printf("Routine specific arguments [defaults]:\n");
    printf("device : SunLink port(s) to test [none]\n");
    printf("   Supported devices:\n");
    printf("         mcp0 -  mcp15\n");
    printf("         dcpa0 - dcpd3\n");
    printf("p= Data pattern [r]\n");
    printf("    c : character 0x55\n");
    printf("    i : incrementing\n");
    printf("    d : decrementing\n");
    printf("    d : random\n");
    printf("i : internal loopback [external]\n");
    printf("k : load dcp kernal [no load]\n");
    printf("st : display sunlink status only\n\n");
}

/**** dummy code to satisfy libtest.a *******/
clean_up()
{
}
