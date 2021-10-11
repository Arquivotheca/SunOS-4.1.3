#ifndef lint
static  char sccsid[] = "@(#)rpcprocs.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */


#include <rpc/rpc.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netdb.h>
#include "sundiag.h"
#include "sundiag_rpcnums.h"

#include <sys/reboot.h>
#include "ats_sundiag.h"
#include "sundiag_ats.h"
#include "amp.h"
#include "tcm_prognum.h"

#include "../../lib/include/probe_sundiag.h"
#include "../../lib/include/dtm_tcm.h"
#include "../../lib/include/tcm_dtm.h"
#include "../../lib/include/libonline.h"

#define DTM_PORT 2023
#define TCM_PORT 2024
#define	RPC_BUFSIZE	1024
#define MESSAGE_SIZE	512

extern char	*sprintf(), *strcpy(), *strtok(), *malloc();
extern char	*remotehost;

static int udp_socket, open_tcps=0;
static SVCXPRT	*s_transp;
static CLIENT	*client;
static int	hostid;

/* forward declarations */
extern write_log();
static void	rpc_dispatch();
static SVCXPRT *init_tester_svc();
static s_adjust_fds();
static Notify_value s_listener();

/******************************************************************************
 * ttysw_print(), prints the messages on the console subwindow of Sundiag     *
 ******************************************************************************/
ttysw_print(buf, len)
char	*buf;
int	len;
{
  static char	crlf[4]="\r\n";
  char	*ptr, *begin;

  if (sundiag_console == (Tty)0)	/* console window is not up yet */
    (void)write(tty_fd, buf, len);
  else					/* change LF to LF+CR */
  {
    ptr = buf;
    begin = ptr;
    while (*ptr != '\0')
    {
      if (*ptr == '\n')
      {
	*ptr = '\0';			/* NULL-terminated it */
        (void)ttysw_output(sundiag_console, begin, strlen(begin));
	(void)ttysw_output(sundiag_console, crlf, 2);
	*ptr++ = '\n';			/* restore it */
	begin = ptr;
      }
      else
	++ptr;
    }
    if (begin != ptr)			/* more to be printed */
	(void)ttysw_output(sundiag_console, begin, strlen(begin));
  }
}

/******************************************************************************
 * logmsg(msg, which, errcode) decides where to put sundiag-generated messages*
 * Input: msg: the message to be logged.                                      * 
 *        which:                                                              *  
 *        if which <= 0, it logs to INFO file.                                * 
 *        if which = 0, it logs to both INFO and ERROR files.                 *  
 *        if which < 0 OR if which > 1, it will be printed on the console.    * 
 *        errcode: an interger error code.                                    *  
 ******************************************************************************/
logmsg(msg, which, errcode)
char *msg;
int   which;
int   errcode;
{
  static char	sundiag_host[82]="\0";
  char line [RPC_BUFSIZE-1];
  char line_buf [RPC_BUFSIZE-1];
  struct timeval tv;
  register struct tm *tp;

        (void)gettimeofday(&tv, (struct timezone *)0);
        tp = localtime(&tv.tv_sec);

	if (sundiag_host[0] == '\0') {
	  if (remotehost == NULL)
	    (void)gethostname(sundiag_host, 80); /* get the sundiag host name */
	  else
			/* put remote host name in log files */
	    (void)strcpy(sundiag_host, remotehost);
	}
  	error_base = (which>1)? 9000 : 7000;
        (void)sprintf(line_buf, 
	"%03d.%02d.%03d.%04d %02d/%02d/%02d %02d:%02d:%02d %s sundiag %s: %s\n",
		test_id, version_id, subtest_id,error_base+errcode,
		(tp->tm_mon + 1), tp->tm_mday, tp->tm_year,
		tp->tm_hour, tp->tm_min, tp->tm_sec,
		sundiag_host,
		which>1?"ERROR":"INFO", msg);

	if (which <= 0)			/* INFO message to info. file only */
	  write_log(line_buf, info_fp);
	else
	{
	  write_log(line_buf, info_fp);
	  write_log(line_buf, error_fp);
	}

	if (which > 1 || which < 0)	/* write it to console too */
	{
	  if (!tty_mode)		/* but remove the first 16 chars */
	  {
	    ttysw_print(line_buf+16, strlen(line_buf)-16);
	  }
	  else
	    console_message(line_buf+16);
	}
}

/******************************************************************************
 * Writes the passed message to the specified log file, and flushes it.	      *
 * Input: msg, the text to be written.					      *
 *	  fp, the file pointer of the file to be written to.		      *
 ******************************************************************************/
write_log(msg, fp)	/* write the passed message to log file */
char	*msg;
FILE	*fp;
{
  if (fprintf(fp, "%s", msg) == EOF)
    perror("Writing log file");

  (void)fflush(fp);
}

/******************************************************************************
 * Initializes RPC server.						      *
 ******************************************************************************/
init_rpc()
{
  static  SVCXPRT *init_udp_svc();	/* forward declaration */

  if ((udp_socket=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
  {
    perror("socket");
    sundiag_exit(1);
  }

  if ((s_transp=init_udp_svc(udp_socket, DTM_PORT, SUNDIAG_PROG, SUNDIAG_VERS,
		rpc_dispatch)) == NULL)
    sundiag_exit(1);

  (void)notify_set_input_func((char *)s_transp, s_listener, udp_socket);
	/* s_transp was used as client handle */
}

/******************************************************************************
 * send periodical message(heart beat) message to ATS(once a minute).	      *
 ******************************************************************************/
send_pong()
{
  S_AMP_MSG_CB	msgbuf;

  msgbuf.message_num=SUNDIAG_ATS_PONG;
  msgbuf.data=NULL;
  msgbuf.data_size=0;
  msgbuf.action=NULL;
  msgbuf.max_retransmit=4;
  msgbuf.max_wack=60;
  amp_rpc_send(client, &hostid, sizeof(int), xdr_int, &msgbuf);
}

/******************************************************************************
 * get_sockaddr(), initialize the "server_addr" structure.		      *
 ******************************************************************************/
static struct sockaddr_in *get_sockaddr(hostname, family, port)
char	*hostname;
short	family;
u_short	port;
{
  struct hostent *hp;
  static struct sockaddr_in server_addr;

    if ((hp=gethostbyname(hostname)) == NULL)
    {
      (void)fprintf(stderr, "gethostbyname: invalid host name '%s'\n",
								hostname);
      sundiag_exit(1);
    }

    bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
    server_addr.sin_family = family;
    server_addr.sin_port = port;
    return(&server_addr);
}

/******************************************************************************
 * Initializes RPC client.						      *
 ******************************************************************************/
init_client(hostname)
char	*hostname;
{
  static struct sockaddr_in *server=NULL;
  int	sock_type = RPC_ANYSOCK;
  S_AMP_MSG_CB	msgbuf;
  struct timeval  retry_timeout;

  hostid = gethostid();
  server = get_sockaddr(hostname, AF_INET, TCM_PORT);

  retry_timeout.tv_sec = 60;
  retry_timeout.tv_usec = 0;

  if ((client = clntudp_create(server, (u_long)TCM_PROG, (u_long)TCM_VERS,
		retry_timeout, &sock_type)) == NULL) {
        clnt_pcreateerror("init_client: clntudp_create");
        sundiag_exit(1);
  }

  amp_initialize();
  msgbuf.message_num=SUNDIAG_ATS_UP;
  msgbuf.data=NULL;
  msgbuf.data_size=0;
  msgbuf.action=NULL;
  msgbuf.max_retransmit=4;
  msgbuf.max_wack=60;
  amp_rpc_send(client, &hostid, sizeof(int), xdr_int, &msgbuf);
}

/******************************************************************************
 * Called by init_rpc().						      *
 ******************************************************************************/
static	SVCXPRT *init_udp_svc(sock, port, prognum, versnum, dispatcher)
int	sock;
int	port;
u_long	prognum, versnum;
void	(*dispatcher)();    
{
  struct	sockaddr_in addr;
  SVCXPRT	*transp;

  pmap_unset(prognum, versnum);

  bzero((char *)&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = port;
  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return((SVCXPRT *)0);
  }

  if ((transp=svcudp_create(sock)) == NULL)
  {
    perror("svcudp_create");
    return((SVCXPRT *)0);
  }

  if (!svc_register(transp, prognum, versnum, dispatcher, IPPROTO_UDP))
  {
    perror("svc_register");
    return((SVCXPRT *)0);
  }

  return(transp);
}

/******************************************************************************
 * s_adjust_fds(saved) was lifted from suntools to do the appropriate fd magic*
 * so that notify_start will work correctly, and dispatching is unnecessary.  *
 ******************************************************************************/
static s_adjust_fds(saved)
int	saved;
{
    register int        opened;
    register int        closed;
    register int        fd;

    opened = svc_fds & ~saved;
    fd = 0;
    while (opened != 0) {
        if (opened & 1) {
            (void)notify_set_input_func((char *)s_transp,
                                         s_listener, fd);
            open_tcps |= 1 << fd;
        }
        fd++;
        opened >>= 1;
    }
    closed = saved & ~svc_fds;
    fd = 0;
    while (closed != 0) {
        if (closed & 1) {
            (void)notify_set_input_func((char *)s_transp,
                                         NOTIFY_FUNC_NULL, fd);
            open_tcps &= ~(1 << fd);
        }
        fd++;
        closed >>= 1;
    }
}

/****************************************************************************** 
 * s_listener(clnt, fd) was lifted from suntools to call svc_getreq and	      *
 * s_adjust_fds, so that notify_start will work correctly, and dispatching    *
 * is unnecessary.							      *
 ******************************************************************************/
/*ARGSUSED*/
static Notify_value s_listener(clnt, fd)
int     *clnt;
int     fd;
{
        int             saved_fds;

        saved_fds = svc_fds;
        svc_getreq(1 << fd);
        if (fd == udp_socket || open_tcps & (1 << fd))
                s_adjust_fds(saved_fds);
        return (NOTIFY_DONE);
}

/******************************************************************************
 * RPC message dispatcher, will be executed when an RPC message arrived from  *
 * the clients.								      *
 ******************************************************************************/
static void	rpc_dispatch(t_rqstp, t_transp)
struct svc_req *t_rqstp;
SVCXPRT *t_transp;
{
  struct f_devs f_dev;
  int  rpc_pass;	/* flag for succeeding in svc_sendreply() or not */
  char *msg = NULL;
  FILE *fopen();

  struct strings str;
  struct option_file optfile;
  char	*param[21];		/* max. to 20 tokens(plus one to store NULL) */
  int	 mode, i;
  int	 seq_num;

  rpc_pass = 0;			/* initialize the flag */

	switch(t_rqstp->rq_proc)
	{
	  case ATS_SUNDIAG_ACK:
		amp_pack_msg(t_transp);
		return;

          case NULLPROC:
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

                return;			/* do not log */

          case INFO_MSG:
          case WARNING_MSG:
                if (!svc_getargs(t_transp, xdr_wrapstring, &msg)) {
                        svcerr_decode(t_transp);
                        return;
                }
 
                write_log(msg, info_fp);  /* for log file only */
 
                if (!ats_nohost)
                  send_host(msg, 1);    /* test is still running */
 
                if (!tty_mode)          /* write to console */
		{
                  ttysw_print(msg+16,strlen(msg)-16);
		}
                else
                  console_message(msg+16);
 
                svc_freeargs(t_transp, xdr_wrapstring, &msg);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
                  rpc_pass = 1;
 
                break;

          case DEFAULT_MSG:
          case LOGFILE_MSG:
                if (!svc_getargs(t_transp, xdr_wrapstring, &msg)) {
			svcerr_decode(t_transp);
			return;
		}

		write_log(msg, info_fp);

		if (!tty_mode)		/* write to console */
		  ttysw_print(msg, strlen(msg));
		else
		  console_message(msg);

		svc_freeargs(t_transp, xdr_wrapstring, &msg);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		  rpc_pass = 1;

                break;

          case FATAL_MSG:
          case ERROR_MSG:
                if (!svc_getargs(t_transp, xdr_wrapstring, &msg)) {
			svcerr_decode(t_transp);
			return;
		}

		write_log(msg, info_fp);	/* write to info. log */
		write_log(msg, error_fp);	/* write to error log */

		if (!ats_nohost)
		  send_host(msg, 0);/* not running anymore */

		if (!tty_mode)			/* write to console */
		{
		  ttysw_print(msg+16, strlen(msg)-16);
		}
		else
		  console_message(msg+16);

		svc_freeargs(t_transp, xdr_wrapstring, &msg);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		  rpc_pass = 1;

		if (send_email==1 || send_email==3)
		  sd_send_mail();

		break;

	  case CONSOLE_MSG:
                if (!svc_getargs(t_transp, xdr_wrapstring, &msg)) {
			svcerr_decode(t_transp);
			return;
		}

		if (!tty_mode)		/* write to console only */
		  ttysw_print(msg, strlen(msg));
		else
		  console_message(msg);

		svc_freeargs(t_transp, xdr_wrapstring, &msg);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		  rpc_pass = 1;

                break;
		
	  case PROBE_DEVS:
		f_dev.cpuname = NULL;
        	f_dev.found_dev = NULL;
                if (!svc_getargs(t_transp, xdr_f_devs, &f_dev)) {
                        svcerr_decode(t_transp);
                        exit(1);
		}

		init_tests(&f_dev);		/* in tests.c */

                svc_freeargs(t_transp, xdr_f_devs, &f_dev);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

                return;

	  case ATS_SUNDIAG_START_TESTS:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
		if (running == IDLE)		/* should return status too */
		  start_proc();

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_STOP_TESTS:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
		if (running == GO)		/* should return status too */
		  stop_proc();

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_SELECT_TEST:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_strings, &str)) {
                        svcerr_decode(t_transp);
                        return;
                }

		ats_start_test(str.op[0], str.op[1]);

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_DESELECT_TEST:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_strings, &str)) {
                        svcerr_decode(t_transp);
                        return;
                }

		ats_stop_test(str.op[0], str.op[1]);

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_HALT:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		reboot(RB_HALT);

		return;

	  case ATS_SUNDIAG_QUIT:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		(void)kill_proc();		/* never return */

	  case ATS_SUNDIAG_INTERVEN:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_int, &mode)) {
                        svcerr_decode(t_transp);
                        return;
                }

		interven_proc((Panel_item)NULL, mode, (Event *)NULL);

		svc_freeargs(t_transp, xdr_int, &mode);
		if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_OPT_COREFILE:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_int, &mode)) {
                        svcerr_decode(t_transp);
                        return;
                }

		if (running == IDLE)
		    core_file = mode;	/* ==1, enabled; ==0, disabled */

		svc_freeargs(t_transp, xdr_int, &mode);
		if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_RESET:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		if (running == IDLE)
		  reset_proc();

		return;

	  case ATS_SUNDIAG_OPTION_FILE:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_option, &optfile)) {
                        svcerr_decode(t_transp);
                        return;
                }

		if (optfile.action == 1)
		  (void)load_option(optfile.fname, 2);
		else
		  (void)store_option(optfile.fname);

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;

	  case ATS_SUNDIAG_OPTION:
		if (!svc_getargs (t_transp, xdr_int, &seq_num)) {
			svcerr_decode(t_transp);
			return;
  		}
		amp_send_ack(client,SUNDIAG_ATS_ACK,t_rqstp->rq_proc,seq_num);
                if (!svc_getargs(t_transp, xdr_strings, &str)) {
                        svcerr_decode(t_transp);
                        return;
                }

	        if (str.num >= 3)		/* check format first */
		{
		  for (i=0; i < str.num; ++i)
		    param[i] = str.op[i];
		  param[i] = NULL;

		  if (strcmp(param[0], "option")==0)
				/* this is an entry for system option */
		    load_system_options(param);	/* load system-wide options */
		  else
		    load_test_options(param);
				/* otherwise, individual test options */

		  if (!tty_mode)
		  {
		    (void)panel_set(select_item, PANEL_FEEDBACK,
							PANEL_NONE, 0);

		    if (option_frame != NULL)
				/* destroy the popup test option frame */
		      frame_destroy_proc(option_frame);
		  }

		  if (tty_mode)
		    tty_int_sel();
		  optfile_update();	/* update the screen if needed */
		}

                if (!svc_sendreply(t_transp, xdr_void, (caddr_t)NULL))
		{
		  rpc_pass = 1;
		  break;
		}

		return;
	}

	if (rpc_pass == 1)  /* print the message if svc_send_reply() failed */
	{
          (void)printf("\nrpc error with svc_send_reply.\n");
	  return;
	}
}

send_start_stop(msgnum, testname, devname, pass)
int msgnum;
char *testname;
char *devname;
int  pass;
{
  struct test	test;
  S_AMP_MSG_CB	msgbuf;

      strcpy(test.testname,testname);
      strcpy(test.devname,devname);
      test.hostid = hostid;
      test.pass_count = pass;
      msgbuf.message_num = msgnum;
      msgbuf.data=NULL;
      msgbuf.data_size=0;
      msgbuf.action=NULL;
      msgbuf.max_retransmit=4;
      msgbuf.max_wack=60;
      amp_rpc_send(client, &test, sizeof(struct test),
						xdr_test, &msgbuf);
}

send_test_msg(msgnum, teststatus, testname, devname, msg)
int msgnum;
int teststatus;
char *testname;
char *devname;
char *msg;
{
  struct failure	failure;
  S_AMP_MSG_CB	msgbuf;

      strcpy(failure.testname,testname);
      strcpy(failure.devname,devname);
      if (strlen(msg) < 256)
        strcpy(failure.message,msg);
      else
      {
	strncpy(failure.message,msg,255);
	failure.message[255] = '\0';
      }
      failure.hostid = hostid;
      failure.run_status = teststatus;
      msgbuf.message_num = msgnum;
      msgbuf.data=NULL;
      msgbuf.data_size=0;
      msgbuf.action=NULL;
      msgbuf.max_retransmit=4;
      msgbuf.max_wack=60;
      amp_rpc_send(client, &failure, sizeof(struct failure),
						xdr_failure, &msgbuf);
}

/******************************************************************************
 * Parses the message from the test to get the device name and test name,     *
 * then send the entire message to the host in the format understood by the   *
 * Host.								      *
 * Input: msg, the message from test.					      *
 *	  status, ==1: test is still running; ==0: test is stopped.	      *
 ******************************************************************************/
static	send_host(msg, status)
char	*msg;
int	status;
{
  char	*temp, *buf, *delimit;
  char	*testname;
  char	*devname;

  delimit = " \t";
  testname = "";
  devname = "";
  buf = malloc((unsigned)strlen(msg)+2);
  (void)strcpy(buf, msg);			/* make a copy of it */

  if (strtok(buf, delimit) != NULL)		/* message id */
   if (strtok((char *)NULL, delimit) != NULL)	/* date */
    if (strtok((char *)NULL, delimit) != NULL)	/* time */
      if ((temp=strtok((char *)NULL, delimit)) != NULL)	/* device name */
      {
	devname = temp;
	if ((temp=strtok((char *)NULL, delimit)) != NULL)    /* test name */
	  testname=temp;
      }

  if (strcmp(testname, "probe") != 0)	/* don't send the probe messages */
    send_test_msg(SUNDIAG_ATS_FAILURE, status, testname, devname, msg);

  free(buf);
}

/* to be call by msq.c(amp.c) */
udpcall(clnt, procnum, inproc, in, outproc, out, timeout)
CLIENT *clnt;
u_long procnum;
xdrproc_t inproc, outproc;
char *in, *out;
int timeout;
{
  static struct timeval waittime = {0,0};

  waittime.tv_sec = timeout;
  return(clnt_call(clnt, procnum, inproc, in, outproc, out, waittime));
}
