#ifndef lint
static  char sccsid[] = "@(#)rpcrtns.c	1.1  7/30/92 Copyright Sun Micro";
#endif

/******************************************************************************
 * rpcrtns.c					
 *							
 * Purpose:
 *    Variables and routines to be used by Sundiag interface routines.
 *    Most of thess routines are utilizing RPC service to handle Sundiag
 *    messages.
 *
 * 								      
 * Routines:  
	      int     send_msg(procnum, msg)
 *	      CLIENT *init_udp_client(raddr, prognum, versnum)
 *	      char   *get_hostname()
 *	      struct  sockaddr_in *get_sockaddr(hostname, family, port)
 *	      int     call_client(client, procnum, inproc, in, 
 *					outproc, out)
 *	      int     send_rpc_msg(msg_type, msg)
 *
 ******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <sys/socket.h>
#include <rpc/rpc.h>
#ifdef   SVR4
#include <rpc/clnt_soc.h>
#endif
#include "sdrtns.h"
#include "rpcrtns.h"
#include "../../lib/include/libonline.h"

#ifdef SVR4
extern CLIENT *clntudp_create();
extern char   *malloc() ;
#endif

/* forward declarations and good only for rpcrtns.c */
int     send_msg();
int     call_client();
char    *get_hostname();
struct  sockaddr_in *get_sockaddr();
CLIENT  *init_udp_client();

/* external functions  for this routine rpcrtns.c */
#ifndef SVR4
extern	char	*sprintf();
#endif
extern	char	*strcpy();

/******************************************************************************
 * handles client rpc.							      *
 ******************************************************************************/
int send_msg(procnum, msg)
int	procnum;
char	*msg;
{
    static struct sockaddr_in *server=NULL;
    static CLIENT *client=NULL;

    if (hostname == NULL)
      hostname = get_hostname();

    if (server == NULL)
      server = get_sockaddr(hostname, AF_INET, 0);

    if (client == NULL) {
       if ((client=init_udp_client(server, (u_long) SUNDIAG_PROG,
                                 (u_long) SUNDIAG_VERS)) == NULL)
           return (0);
    }

    if (!call_client(client, (u_long)procnum, xdr_wrapstring, (char *)&msg,
							xdr_void, (char *)0))
      return(0);

    return(1);
}

/*
 * init_udp_client(raddr, prognum, versnum) returns a pointer to an RPC 
 * client for the given sockaddr_in, program prognum, version versnum.
 */
CLIENT *init_udp_client(raddr, prognum, versnum)
    struct sockaddr_in *raddr;
    u_long          prognum, versnum;
{
#   ifndef SVR4
    int	sock_type = RPC_ANYSOCK;
    CLIENT         *client = NULL;
    struct timeval  retry_timeout;

    retry_timeout.tv_sec = RETRY_SEC;
    retry_timeout.tv_usec = 0;

    if ((client = clntudp_create(raddr, prognum, versnum, retry_timeout,
                                 &sock_type)) == (CLIENT *)NULL) {
        clnt_pcreateerror("clntudp_create");
        exit(RPCINIT_RETURN);
    }
    return (client);
#else SVR4
	char    host[256], *p = host;
	int     result = 0, len;
	CLIENT  *clnt = NULL;
	struct timeval  wait;
	int     sock = RPC_ANYSOCK;
	struct sockaddr_in svc_addr;
	struct hostent  *hp;
				 
	 strcpy(p, hostname);
	 hp = gethostbyname(host);
	 (void) strncpy((caddr_t)&svc_addr.sin_addr,hp->h_addr,hp->h_length);
	 svc_addr.sin_family = AF_INET;
	 svc_addr.sin_port = 0;
	 wait.tv_sec = 5;
	 wait.tv_usec = 0;
	 clnt = clntudp_create(&svc_addr, prognum, versnum, wait, &sock);
	 if (clnt == NULL) {
		 clnt_pcreateerror("clntudp_create");
		 exit(RPCINIT_RETURN);
	 }
	 return(clnt);
#endif SVR4
}

/******************************************************************************
 * sends the test message.						      *
 ******************************************************************************/
int send_rpc_msg(exit_code, msg_type, msg)
int     exit_code;
int	msg_type;			/* message types */
char	*msg;				/* the unformatted message */
{
  int	procnum;
  char	*msg_type_id, *tmp;
  char	msg_buf[MESSAGE_SIZE+80];	/* plus 80 to envelop the message */
  struct timeval	tv;
  register struct tm	*tp;

  exit_code = (exit_code<0? -exit_code: exit_code);
					/* make sure positive value */
  switch (msg_type)
  {
    case INFO:				/* informative message */
	procnum = INFO_MSG;
	msg_type_id = "INFO";
	break;
    case WARNING:			/* warning message */
	procnum = WARNING_MSG;
	msg_type_id = "WARNING";
	break;
    case FATAL:				/* fatal error message */
	procnum = FATAL_MSG;
	msg_type_id = "FATAL";
	break;
    case ERROR:				/* error message */
	procnum = ERROR_MSG;
	msg_type_id = "ERROR";
	break;
    case LOGFILE:	/* will not be formatted(to sundiag's info. log only) */
	procnum = LOGFILE_MSG;		/* general message to be logged */
	msg_type_id = "LOGFILE";
	break;
    case VERBOSE:	/* will be formatted if exec_by_sundiag is set */
	msg_type_id = "VERBOSE";
	procnum = CONSOLE_MSG;
	break;
    case DEBUG:		/* will be formatted if exec_by_sundiag is set */
	msg_type_id = "DEBUG";
	procnum = CONSOLE_MSG;
	break;
    case TRACE:		/* will be formatted if exec_by_sundiag is set */
	msg_type_id = "TRACE";
	procnum = CONSOLE_MSG;
	break;
    case CONSOLE:	/* will not be formatted(to sundiag's console only) */
	msg_type_id = "CONSOLE";
	procnum = CONSOLE_MSG;		/* general message to be displayed */
	break;
    default:		/* will be processed as INFO messages */
	procnum = DEFAULT_MSG;		/* for safety sake */
	msg_type_id = "DEFAULT";
	break;
  }

  if (strncmp(device_name, "/dev/", 5) == 0)
    tmp = device_name+5;
  else
    tmp = device_name;

  if (strlen(msg) > MESSAGE_SIZE) 
	*(msg+MESSAGE_SIZE) = '\0';
  		/* truncate the message if it is longer than MESSAGE_SIZE */

  if (msg_type != LOGFILE && msg_type != CONSOLE && 
	!((msg_type == DEBUG || msg_type == VERBOSE || msg_type == TRACE)
		 && !exec_by_sundiag))
  { 		/* Sundiag standard formatted message for ERROR, INFO, WARNING,
		   and FAILURE; also for DEBUG, VERBOSE, and TRACE while
		   exec_by_sundiag is on */
			
    (void)gettimeofday(&tv, (struct timezone *)0);
    tp = localtime(&tv.tv_sec);		/* get current time & date */

    if ((msg_type == ERROR || msg_type == INFO || msg_type == WARNING
	|| msg_type == FATAL) && exec_by_sundiag)
	(void)sprintf(msg_buf,
       	     "%03d.%02d.%03d.%04d %02d/%02d/%02d %02d:%02d:%02d %s %s %s: %s\n",
             test_id, version_id, subtest_id,exit_code+error_base,
             (tp->tm_mon + 1), tp->tm_mday, tp->tm_year,
             tp->tm_hour, tp->tm_min, tp->tm_sec,
             tmp, test_name,
             msg_type_id, msg);
    else {
        (void)sprintf(msg_buf,
             "%02d/%02d/%02d %02d:%02d:%02d %s %s %s: %s\n",
             (tp->tm_mon + 1), tp->tm_mday, tp->tm_year,
             tp->tm_hour, tp->tm_min, tp->tm_sec,
             tmp, test_name,
             msg_type_id, msg);
    }
	
  }
  else if ((msg_type == VERBOSE || msg_type == DEBUG) && !exec_by_sundiag) {
    (void)sprintf(msg_buf, "%s\n", msg);
  }
  else if (msg_type == TRACE && !exec_by_sundiag) {
    (void)sprintf(msg_buf, "@%s %s: %s\n", test_name, device_name, msg);
  }
  else
    (void)strcpy(msg_buf, msg);		/* print the raw message only */

  if (exec_by_sundiag)			/* then send to sundiag for logging */
  {

    if (send_msg(procnum, msg_buf))
	return;
    else
    {
	(void)fprintf(stderr, "%s:%s: send_rpc_msg: RPC msg not sent.\n",
						tmp, test_name);
	exit(RPCFAILED_RETURN);
    }
  }
  else
    (void)fprintf(stderr, "%s", msg_buf);	/* print it on stderr */
}

/******************************************************************************
 * get_hostname(), returns the name of the current host.		      *
 * Note: this function should only be called once.			      *
 ******************************************************************************/
char *get_hostname()
{
  char	*hostname;

    hostname = malloc(MAXHOSTNAMELEN+2);
    if (hostname == NULL)
    {
	(void)fprintf(stderr, "%s:%s: get_hostname: out of memory\n",
						device_name, test_name);
	exit(RPCFAILED_RETURN);
    }
    if ((gethostname(hostname, MAXHOSTNAMELEN)) == -1)
    {
	(void)fprintf(stderr, "%s:%s: get_hostname: no hostname\n",
						device_name, test_name);
	exit(RPCFAILED_RETURN);
    }
    return(hostname);
}

/******************************************************************************
 * get_sockaddr(), initialize the "server_addr" structure.		      *
 ******************************************************************************/
struct sockaddr_in *get_sockaddr(hostname, family, port)
char	*hostname;
short	family;
u_short	port;
{
  struct hostent *hp;
  static struct sockaddr_in server_addr;

    if ((hp=gethostbyname(hostname)) == NULL)
    {
      (void)fprintf(stderr, "%s:%s: get_sockaddr: invalid host name '%s'\n",
					device_name, test_name, hostname);
      exit(RPCFAILED_RETURN);
    }
#   ifdef SVR4
    memcpy((caddr_t)&server_addr.sin_addr, hp->h_addr, hp->h_length);
#   else
    bcopy(hp->h_addr, (caddr_t)&server_addr.sin_addr, hp->h_length);
#   endif
    server_addr.sin_family = family;
    server_addr.sin_port = port;
    return(&server_addr);
}

/******************************************************************************
 * call_client(), send the RPC to the server.				      *
 ******************************************************************************/
int call_client(client, procnum, inproc, in, outproc, out)
CLIENT	*client;
u_long	procnum;
char	*in, *out;
xdrproc_t	inproc, outproc;
{
    struct timeval  total_timeout;

    total_timeout.tv_sec = TOTAL_TIMEOUT_SEC;
    total_timeout.tv_usec = TOTAL_TIMEOUT_USEC;
    clnt_call(client, procnum, inproc, in, outproc, out, total_timeout);

    return(1);
}


