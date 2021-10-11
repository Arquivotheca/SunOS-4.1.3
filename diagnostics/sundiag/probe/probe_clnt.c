#ifndef lint
static  char sccsid[] = "@(#)probe_clnt.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/time.h>
#include <strings.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <sys/time.h>
#include "sundiag_rpcnums.h"
#include "probe.h"
#include "probe_sundiag.h"

extern char *sprintf();

#define MAXHOSTNAMELEN  64
#define RETRY_SEC		30
#define TOTAL_TIMEOUT_SEC       70
#define TOTAL_TIMEOUT_USEC      0

static char *progname, *my_hostname;
static int sock_type = RPC_ANYSOCK;
static CLIENT *client;

static	struct	sockaddr_in *get_sockaddr();
static	CLIENT	*init_udp_client();

/*
 * init_clnt() - gets the hostname and sockaddr_in for the target host, 
 * and initializes the RPC client handle
 */
init_clnt(hostname, name)
    char *hostname;
    char *name;
{
    struct sockaddr_in *server;
    progname = name;
    my_hostname = get_hostname();

    if (!debug)
    {
      server = get_sockaddr(hostname, AF_INET, 0);
      client = init_udp_client(server, (u_long) SUNDIAG_PROG,
                                 (u_long) SUNDIAG_VERS);
    }
}

/*
 * send_log_msg(msg_type, msg) - sends a string-type message to Sundiag,
 * used to send error and info type messages
 */
send_log_msg(msg_type, msg)
    int msg_type;
    char *msg;
{
    int  procnum, i;
    char fmt_msg_buffer[BUFSIZ - 1];
    char *fmt_msg = fmt_msg_buffer;
    char *msg_type_id;
    struct timeval	tv;
    register struct tm	*tp;

    switch (msg_type) {
    case INFO:
        procnum = INFO_MSG;
        msg_type_id = "INFO";
        break;
    case WARNING:
        procnum = WARNING_MSG;
        msg_type_id = "WARNING";
        break;
    case FATAL:
        procnum = FATAL_MSG;
        msg_type_id = "FATAL";
        break;
    case ERROR:
        procnum = ERROR_MSG;
        msg_type_id = "ERROR";
        break;
    default:
        procnum = DEFAULT_MSG;
        msg_type_id = "DEFAULT";
        break;
    }

    (void)gettimeofday(&tv, (struct timezone *)0);
    tp = localtime(&tv.tv_sec);		/* get current time & date */

    (void)sprintf(fmt_msg, "%02d/%02d/%02d %02d:%02d:%02d %s %s %s: %s",
	(tp->tm_mon + 1), tp->tm_mday, tp->tm_year,
	tp->tm_hour, tp->tm_min, tp->tm_sec,
	my_hostname, progname,
	msg_type_id, msg);

    if (!debug)
    {
      if (call_client(client, (u_long) procnum, xdr_wrapstring, 
	    (char *)&fmt_msg, xdr_void, (char *)NULL))
	return;
      else
      {
	(void) fprintf(stderr, "%s: RPC msg not sent.\n", progname);
	exit(1);
      }
    }
    else
	(void)fprintf(stderr, "%s", fmt_msg);
}

/*
 * send_probe_msg(f_dev) - sends the hardware probing struct f_dev to Sundiag,
 * used to send the probing info, is the main RPC routine for probe/Sundiag
 */
send_probe_msg(f_dev)
    struct f_devs *f_dev;
{
    int i;
    struct found_dev *pf;

    if (debug) {
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
    else if (call_client(client, (u_long) PROBE_DEVS, xdr_f_devs, 
	    (caddr_t)f_dev, xdr_void, (char *)NULL))
            return;
    else
    {
	(void) fprintf(stderr, "%s: RPC msg not sent.\n", progname);
	exit(1);
    }
}

/*
 * end_clnt() - destroys the client and closes the socket
 */
end_clnt()
{
    free(my_hostname);
    clnt_destroy(client);
}

/*
 * get_sockaddr(name, family, port) returns a pointer to a sockaddr_in
 * for the given hostname, family and port
 */
static	struct sockaddr_in *
get_sockaddr(hostname, family, port)
    char           *hostname;
    short           family;
    u_short         port;
{
    struct hostent *hp;
    static struct sockaddr_in server_addr;
 
    if ((hp = gethostbyname(hostname)) == NULL) {
        (void) fprintf(stderr,
                       "Sorry, host %s not in hosts database\n", hostname);
        exit(1);
    }

    bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
    server_addr.sin_family = family;
    server_addr.sin_port = port;

    return (&server_addr);
}

/*
 * init_udp_client(raddr, prognum, versnum) returns a pointer to an RPC 
 * client for the given sockaddr_in, program prognum, version versnum.
 */
static	CLIENT *init_udp_client(raddr, prognum, versnum)
    struct sockaddr_in *raddr;
    u_long          prognum, versnum;
{
    CLIENT         *client = NULL;
    struct timeval  retry_timeout;

    retry_timeout.tv_sec = RETRY_SEC;
    retry_timeout.tv_usec = 0;

    if ((client = clntudp_create(raddr, prognum, versnum, retry_timeout,
                                 &sock_type)) == NULL) {
        clnt_pcreateerror("clntudp_create");
        exit(1);
    }
    return (client);
}

/*
 * call_client calls the remote procedure procnum associated with the client
 * handle client
 */
call_client(client, procnum, inproc, in, outproc, out)
    CLIENT         *client;
    u_long          procnum;
    char           *in, *out;
    xdrproc_t       inproc, outproc;
{
    struct timeval  total_timeout;
    enum clnt_stat  clnt_stat;

    if (debug)
        (void) printf("sending sundiag message number %ld\n", procnum);

    total_timeout.tv_sec = TOTAL_TIMEOUT_SEC;
    total_timeout.tv_usec = TOTAL_TIMEOUT_USEC;
    clnt_stat = clnt_call(client, procnum, inproc, in,
                          outproc, out, total_timeout);
    if (clnt_stat != RPC_SUCCESS) {
        clnt_perror(client, "clnt_call");
        return(0);
    }
    return(1);
}

/*
 * get_hostname calls the gethostname function and returns the hostname
 */
char *
get_hostname()
{
    static char *hostname = NULL;

    if (hostname != NULL)
	return(hostname);
    hostname = mem_alloc(MAXHOSTNAMELEN);
    if (hostname == NULL) {
        (void) fprintf(stderr, "get_hostname: out of memory\n");
        exit(1);
    }
    if ((gethostname(hostname, MAXHOSTNAMELEN - 1)) == -1) {
        (void) fprintf(stderr, "no hostname\n", TRUE);
        exit(1);
    }
    if (debug)
    	(void) printf("hostname = %s\n", hostname);
    return (hostname);
}

/*
 * errmsg(errnum) returns the string of the system error message, based on the
 * errnum, to your program, courtesy of Guy Harris
 */
char *
errmsg(errnum)
        int errnum;
{
        extern int sys_nerr;
        extern char *sys_errlist[];
        static char errmsgbuf[6+10+1];

        if (errnum >= 0 && errnum < sys_nerr)
                return (sys_errlist[errnum]);
        else {
                (void) sprintf(errmsgbuf, "Error %d", errnum);
                return (errmsgbuf);
        }
}
