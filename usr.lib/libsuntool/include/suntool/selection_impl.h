/*	@(#)selection_impl.h 1.1 92/07/30 */

#ifndef	suntool_selection_impl_DEFINED
#define	suntool_selection_impl_DEFINED

/*
 *	Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <sunwindow/notify.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>

extern int          errno;

#define SELN_SVC_PROGRAM    100015	/* SELNSVCPROG in <rpcsvc/??.h>	*/
#define SELN_LIB_PROGRAM    100025	/* SELNLIBPROG in <rpcsvc/??.h>	*/
#define SELN_SVC_TEST	0x20000000	/* flag ORed into program number  */
#define SELN_VERSION             6	/*  seln_get_function_state	  */
#define SELN_VERSION_ORIGINAL    1

/* Procedure IDs for server-module procedures	 */

#define SELN_SVC_NULLPROC	 0
#define SELN_SVC_ACQUIRE	 1
#define SELN_SVC_CLEAR_FUNCTIONS 2
#define SELN_SVC_DONE		 3
#define SELN_SVC_GET_HOLDER	 4
#define SELN_SVC_HOLD_FILE	 5
#define SELN_SVC_INFORM		 6
#define SELN_SVC_INQUIRE	 7
#define SELN_SVC_INQUIRE_ALL	 8
#define SELN_SVC_STOP		 9
#define SELN_SVC_FUNCTION_ONE	10
#define SELN_SVC_FUNCTIONS_ALL	11


/*
 * Procedure IDs for client-module procedures 
 */

#define SELN_CLNT_REQUEST	17
#define SELN_CLNT_DO_FUNCTION	18

/* Random lengths and limits	 */

#define SELN_HOSTNAME_LENGTH   256	/* see socket(2)  */
#define SELN_STD_TIMEOUT_SEC 	10
#define SELN_STD_TIMEOUT_USEC 	 0	/* 10 sec timeout on connections  */

#define SELN_MAX_PATHNAME     1024	/* MAXPATHLEN in <sys/param.h>
					 * (which includes a whole bunch						 * of other stuff I've already got)
					 */

#define NO_PMAP	0	/* svc_register skips its pmap_set if protocol == 0  */

/*	initializers		*/

#define SELN_NULL_ACCESS { 0, 0, {0}, {0}, 0}
#define SELN_NULL_HOLDER { SELN_UNKNOWN, SELN_NONE, SELN_NULL_ACCESS}

#define	SELN_RANKS	((u_int)SELN_UNSPECIFIED)

#define complain(str)	\
	(void)fprintf(stderr, "Selection library internal error:\n%s\n",str)

#define ord(enum_val)	\
	((int)enum_val)	/*  work around botch in language	*/


typedef struct {
    void	    (*do_function)();
    Seln_result	    (*do_request)();
}	Seln_client_ops;


typedef struct client_node    {
    Seln_client_ops	 ops;  /* How we pass requests to client  */
    char		*client_data;
    Seln_access		 access;
    struct client_node	*next;
    unsigned		client_num; /* this client is the (client_num)th
    				     * client for this selection library
				     */
}	Seln_client_node;

/*	functions for dumping & tracing; in debug.c	*/

void	seln_dump_file_args(),
	seln_dump_function_buffer(),
	seln_dump_holder(),
	seln_dump_inform_args(),
	seln_dump_rank(),
	seln_dump_result(),
	seln_dump_state();

Seln_result
	seln_dump_service();

/*	routines to translate date in rpc			*/

int	xdr_seln_access(),
	xdr_seln_holder(),
	xdr_seln_holders_all(),
	xdr_seln_file_info(),
	xdr_seln_function(),
	xdr_seln_functions_state(),
	xdr_seln_inform_args(),
	xdr_seln_reply(),
	xdr_seln_request(),
	xdr_sockaddr_in();

/*	routines to manipulate the function-key state		*/

void	seln_function_clear(),
	seln_function_set(),
	seln_function_reset();

int	seln_function_state(),
	seln_function_pending();

Seln_functions_state
	seln_functions_all();

/*	common routines						*/

int	seln_equal_access(),
	seln_equal_addresses();

void	seln_do_function(),
	seln_do_reply();

Seln_result
	seln_get_reply_buffer(),
	seln_init_reply(),
	seln_get_tcp(),
	seln_send_yield();

Notify_value
	seln_listener();



/*	common data defined in sel_common.c	*/

extern Seln_access	 seln_my_access;
extern int		 seln_my_tcp_socket;
extern int		 seln_my_udp_socket;
extern SVCXPRT		*seln_my_tcp_transport;
extern SVCXPRT		*seln_my_udp_transport;
extern SVCXPRT		*seln_req_transport;

extern Seln_client_node	 clients;

extern struct timeval	 seln_std_timeout;
extern int               seln_svc_program;

#endif
