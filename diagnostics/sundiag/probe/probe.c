#ifndef lint
static  char sccsid[] = "@(#)probe.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <stdio.h>
#include <rpc/rpc.h>  /* also included rpc/types.h and sys/time.h */
#include <sys/socket.h>
#include "sundiag_rpcnums.h"
#include "probe.h"
#include "sdrtns.h"             /* sundiag standard header file */
#include "../../lib/include/probe_sundiag.h"
#include "../../lib/include/libonline.h"

#define MAXHOSTNAMELEN  64
#define TIMEOUT_SEC     70
#define TIMEOUT_USEC     0

extern char *hostname;
extern char *get_hostname();
extern struct sockaddr_in *get_sockaddr();
extern CLIENT *init_udp_client();
/*
 * VERY IMPORTANT: 1) THIS IS FOR >= 3.5.1, < 4.0, FOR SUN2'S AND SUN3'S
 *		   2) > SYS4-3.2, < 4.0, FOR SUN4'S
 *		   3) OR USE JOM KAHN'S NEW SCSI DRIVER
 *
 * parameters: -m - will make device files if missing
 *             -d - turns on debug mode
 *	       -v - turns on verbose mode
 *	       -t - turns on trace mode
 *             -h <hostname> - send RPC messages to <hostname>, otherwise
 *		    host is self
 */
main(argc, argv)
    int     argc;		/* LINT MESSAGE */
    char    **argv;
{
    char **args;
    char *progname;
    int makedevs = 0;
    struct f_devs f_dev;
    extern char *testname_list[];

    func_name = "main";
    TRACE_IN
    versionid = "1.1";
    test_name = argv[0];
    test_id = get_test_id(test_name, testname_list);
    version_id = get_sccs_version (versionid);
    exec_by_sundiag = TRUE;

    if (argv) {
	progname = *argv;
        for (args = ++argv;*args;args++) {
            if ((strcmp(*args, "-m") == 0))
                makedevs = 1;
	    else if ((strcmp(*args, "-d") == 0))
                debug = TRUE;
	    else if ((strcmp(*args, "-v") == 0))
		verbose = TRUE;
	    else if ((strcmp(*args, "-t") == 0))
		trace = TRUE;
            else if ((strcmp(*args, "-h") == 0) && *(args+1)) {
                hostname = *(args+1);
		args++;
	    }
        }
    }

    if(debug || verbose || trace) 
	exec_by_sundiag = FALSE;

    if(hostname == (char *)NULL)
    	hostname = get_hostname();

    device_name = hostname;

    sun_probe(NULL, &f_dev, makedevs);
    send_probe_msg(&f_dev, progname, hostname);
    TRACE_OUT
}

/*
 * send_probe_msg(f_dev) - sends the hardware probing struct f_dev to Sundiag,
 * used to send the probing info, is the main RPC routine for probe/Sundiag
 */
send_probe_msg(f_dev, progname, hostname)
    struct f_devs *f_dev;
    char   *progname, *hostname;
{
    int i;
    struct found_dev *pf;

    func_name = "send_probe_msg";
    TRACE_IN
    if (verbose) {
    	(void) printf("f_dev->cpuname = %s\n", f_dev->cpuname);
    	(void) printf("f_dev->num = %d\n", f_dev->num);
        pf = f_dev->found_dev;
        for (i = 0; i < f_dev->num; i++) {
            (void) printf("found[%d].device_name = %s\n", i, pf->device_name);
            (void) printf("found[%d].unit = %d\n", i, pf->unit);
            (void) printf("found[%d].u_tag.utype = %d\n", i, pf->u_tag.utype);
            switch(pf->u_tag.utype) {
            case GENERAL_DEV:
                (void) printf("found[%d].u_tag.uval.devinfo.status = %d\n", i,
                    pf->u_tag.uval.devinfo.status);
            	break;
            case MEM_DEV:
                (void) printf("found[%d].u_tag.uval.meminfo.amt = %d\n", i,
                    pf->u_tag.uval.meminfo.amt);
            	break;
            case TAPE_DEV:
                (void) printf("found[%d].u_tag.uval.tapeinfo.status = %d\n", i,
                    pf->u_tag.uval.tapeinfo.status);
                (void) printf("found[%d].u_tag.uval.tapeinfo.ctlr = %s\n", i,
                    pf->u_tag.uval.tapeinfo.ctlr);
                (void) printf("found[%d].u_tag.uval.tapeinfo.ctlr_num = %d\n", 
		    i, pf->u_tag.uval.tapeinfo.ctlr_num);
                (void) printf("found[%d].u_tag.uval.tapeinfo.t_type = %d\n", i,
                    pf->u_tag.uval.tapeinfo.t_type);
                break;
            case DISK_DEV:
                (void) printf("found[%d].u_tag.uval.diskinfo.amt = %d\n", i,
                    pf->u_tag.uval.diskinfo.amt);
            	(void) printf("found[%d].u_tag.uval.diskinfo.status = %d\n", i,
                    pf->u_tag.uval.diskinfo.status);
                (void) printf("found[%d].u_tag.uval.diskinfo.ctlr = %s\n", i,
                    pf->u_tag.uval.diskinfo.ctlr);
                (void) printf("found[%d].u_tag.uval.diskinfo.ctlr_num = %d\n",
		    i, pf->u_tag.uval.diskinfo.ctlr_num);
                break;
            }
        *pf++;
	}
    }    
    if (exec_by_sundiag && !verbose)
	if (send_dev_info(f_dev, hostname)) {
	 	TRACE_OUT
		return;
	}
	else {
	     (void) fprintf(stderr, "%s: RPC msg not sent.\n", progname);
	     exit(1);
        }
  
    TRACE_OUT
}

/******************************************************************************
 * handles client rpc for probe device array data.                            *
 *   return 1 for success, 0 for failure.
 *****************************************************************************/
static  int send_dev_info(f_dev, hostname)
struct f_devs *f_dev;
char   *hostname;
{
    static struct sockaddr_in *server=NULL;
    struct timeval timeout;
    enum clnt_stat clnt_stat;
    register CLIENT *client;

    func_name = "send_dev_info";
    TRACE_IN

    if (server == NULL)
      server = get_sockaddr(hostname, AF_INET, 0);

    if ((client=init_udp_client(server, (u_long) SUNDIAG_PROG,
                                 (u_long) SUNDIAG_VERS)) == NULL)
    {
      TRACE_OUT
      return (0);
    }


    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;

    clnt_stat = clnt_call(client, (u_long)PROBE_DEVS, xdr_f_devs,
                (char *)f_dev, xdr_void, (char *)NULL, timeout);

    if (clnt_stat != RPC_SUCCESS) {
        clnt_perror (client, "clnt_call");
	TRACE_OUT
        return(0);
    }

    clnt_destroy(client);
    TRACE_OUT
    return(1);
}

/* dummy routine to to satisfy sundiag test code requirement. */
int
clean_up()
{
    func_name = "clean_up";
    TRACE_IN
    TRACE_OUT
}
