#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)sel_svc.c 1.1 92/07/30 SMI";
#endif
#endif

/*
 * Copyright 1985, 1988, 1989 Sun Microsystems Inc. 
 */

#include <suntool/selection_impl.h>
#include <sunwindow/rect.h>		/* LINT_CAST definition */
#include <sys/stat.h>
#include <sys/file.h>
#include <net/if.h>
#include <sys/ioctl.h>

static Seln_holder   holder[SELN_RANKS];
static int	     held_file[SELN_RANKS];	/*  fds of held files	*/

static int	     full_notifications,
		     trace_acquire,
		     trace_done,
		     trace_hold_file,
		     trace_inform,
		     trace_inquire,
		     trace_stop,
		     trace_yield;

static void	     seln_svc_acquire(),
		     seln_svc_dispatch(),
		     seln_svc_done(),
		     seln_svc_functions_all(),
		     seln_svc_function_one(),
		     seln_svc_give_holder(),
		     seln_svc_hold_file(),
		     seln_svc_inform(),
		     seln_svc_init_access(),
		     seln_svc_inquire(),
		     seln_svc_inquire_all(),
		     seln_svc_clear_functions(),
		     seln_svc_do_function();

static Seln_result   seln_svc_do_reply(),
		     seln_svc_do_yield(),
		     seln_send_msg();

static Seln_function_buffer
		     seln_execute_fn();

static char	    *seln_svc_debug(),
		    *seln_svc_notify_suppression();

static Notify_func   old_func;

/*
 *	Start the service
 */
void
seln_init_service(debugging)
    int	debugging;
{
    int	i;

    for (i = 0; i < SELN_RANKS; i++) {
	bzero((char *)(LINT_CAST(&holder[i])), sizeof (Seln_holder));
	holder[i].state = SELN_NONE;
	holder[i].rank = (Seln_rank) i;
    }
    if (debugging) {seln_svc_program |= SELN_SVC_TEST;
    }
    seln_svc_init_access(&seln_my_access);
    clients.client_num = 1;
    clients.ops.do_function = seln_svc_do_function;
    clients.ops.do_request = seln_svc_do_reply;
    clients.access = seln_my_access;
    clients.access.client = (char *)&clients;
    clients.client_data = (char *) &seln_my_access;
    
    seln_std_timeout.tv_sec = 0;
    seln_std_timeout.tv_usec = 0;
    return;
}

/*
 * Destroy portmap mapping
 */
int
seln_svc_unmap()
{
    return pmap_unset(seln_svc_program, SELN_VERSION);
}

/*
 * Stop the service
 */
void
seln_svc_stop()
{
    (void) seln_svc_unmap();
    (void)notify_remove_input_func((char *) seln_my_udp_transport,
			     seln_listener, seln_my_udp_socket);
    svc_destroy(seln_my_udp_transport);
    (void)close(seln_my_udp_socket);
    (void)notify_remove_input_func((char *) seln_my_tcp_transport,
			     seln_listener, seln_my_tcp_socket);
    svc_destroy(seln_my_tcp_transport);
    (void)close(seln_my_tcp_socket);
    (void)fprintf(stderr, "Selection service terminated normally\n");
}

/*
 *	dispatch is called by rpc dispatcher, and in turn calls
 *	the appropriate response function below;
 */
static void
seln_svc_dispatch(request, transport)
    struct svc_req       *request;
    SVCXPRT              *transport;
{
    struct sockaddr_in *who;

    if(request->rq_proc == SELN_SVC_NULLPROC) {
      if (!svc_sendreply(transport, xdr_void, (char *)0)) {
	clnt_perror((CLIENT *)(LINT_CAST(transport)),
		    "Couldn't reply to null RPC call");
      }
      return;
    }
    who = svc_getcaller(transport);

    if (chklocal(ntohl(who->sin_addr)) == FALSE) {
      /* NOT a local call - ERROR */
      svcerr_auth(transport,AUTH_REJECTEDCRED);
      return;
    }

    switch (request->rq_proc) {
      case SELN_SVC_ACQUIRE:
	seln_svc_acquire(transport);
	return;
      case SELN_SVC_DONE:
	seln_svc_done(transport);
	return;
      case SELN_SVC_HOLD_FILE:
	seln_svc_hold_file(transport);
	return;
      case SELN_SVC_INFORM:
	seln_svc_inform(transport);
	return;
      case SELN_SVC_INQUIRE:
	seln_svc_inquire(transport);
	return;
      case SELN_SVC_INQUIRE_ALL:
	seln_svc_inquire_all(transport);
	return;
      case SELN_SVC_CLEAR_FUNCTIONS:
	seln_svc_clear_functions(transport);
	return;
      case SELN_SVC_GET_HOLDER:
	seln_svc_give_holder(transport);
	return;
      case SELN_SVC_STOP:
	(void)svc_sendreply(transport, xdr_void, (char *)0);
	if (trace_stop) {
	    (void)fprintf(stderr, "Selection services halted by remote request.\n");
	}
	seln_svc_stop();
	return;
      case SELN_SVC_FUNCTION_ONE:
	seln_svc_function_one(transport);
	return;
      case SELN_SVC_FUNCTIONS_ALL:
	seln_svc_functions_all(transport);
	return;
      case SELN_CLNT_DO_FUNCTION:
	seln_do_function(transport);
	return;
      case SELN_CLNT_REQUEST:
	seln_do_reply(transport);
	return;
      default:
	svcerr_noproc(transport);
	return;
    }
}

/*
 *	Caller will become holder of a selection.
 */
static void
seln_svc_acquire(transport)
    SVCXPRT              *transport;
{
    Seln_holder           input;
    Seln_result           result;

    if (!svc_getargs(transport, xdr_seln_holder, &input)) {
	svcerr_decode(transport);
	(void) svc_sendreply(transport, xdr_enum, 
		(char *)(LINT_CAST(SELN_FAILED)));
	return;
    }
    if (trace_acquire) {
	(void)fprintf(stderr, "Service entered at Acquire with holder:\n");
	seln_dump_holder(stderr, &input);
    }
    if (ord(input.rank) >= ord(SELN_CARET) &&
	ord(input.rank) <= ord(SELN_SHELF)) {
	if (held_file[ord(input.rank)] != 0) {
	    (void)close(held_file[ord(input.rank)]);
	    held_file[ord(input.rank)] = 0;
	}
	holder[ord(input.rank)] = input;
	result = SELN_SUCCESS;
    } else {
	result = SELN_FAILED;
    }
    (void) svc_sendreply(transport, xdr_enum, 
    	(char *)(LINT_CAST(&result)));

    if (trace_acquire) {
	(void)fprintf(stderr, "Acquire returned ");
	seln_dump_result(stderr, &result);
	(void)fprintf(stderr, "\n");
    }
}

/*
 *	Tell service to hold a selection stored in a file.
 */
static void
seln_svc_hold_file(transport)
    SVCXPRT              *transport;
{
    Seln_file_info        input;
    int                   fd;
    Seln_result           result;

    input.pathname = NULL;
    if (!svc_getargs(transport, xdr_seln_file_info, &input)) {
	svcerr_decode(transport);
	result = SELN_FAILED;
	(void) svc_sendreply(transport, xdr_enum, 
		(char *)(LINT_CAST(&result)));
	return;
    }
    if (trace_hold_file) {
	(void)fprintf(stderr, "Service entered at Hold File with args:\n");
	seln_dump_file_args(stderr, &input);
    }
    if (ord(input.rank) < ord(SELN_PRIMARY) ||
	ord(input.rank) > ord(SELN_SHELF)) {
	complain("Selection service can't hold file");
	(void)fprintf(stderr, "selection # %d\n", ord(input.rank));
	result = SELN_FAILED;
	(void) svc_sendreply(transport, xdr_enum, 
		(char *)(LINT_CAST(&result)));
    }
    if ((fd = open(input.pathname, O_RDONLY, 0)) == -1) {
	perror("Selection service couldn't open selection file");
	(void)fprintf(stderr, "filename: \"%s\"\n", input.pathname);
	result = SELN_FAILED;
	(void) svc_sendreply(transport, xdr_enum,
		(char *)(LINT_CAST(&result)));
	return;
    }
    if (held_file[ord(input.rank)] != 0) {
	(void)close(held_file[ord(input.rank)]);
	held_file[ord(input.rank)] = 0;
    } else {
	if (holder[ord(input.rank)].state == SELN_EXISTS) {
	    (void)seln_send_yield(input.rank, &holder[ord(input.rank)], TRUE);
	    holder[ord(input.rank)].state = SELN_NONE;
	}
    }
    held_file[ord(input.rank)] = fd;
    holder[ord(input.rank)].state = SELN_FILE;
    holder[ord(input.rank)].access = clients.access;
    result = SELN_SUCCESS;
    (void) svc_sendreply(transport, xdr_enum, 
    	(char *)(LINT_CAST(&result)));

    if (trace_hold_file) {
	(void)fprintf(stderr, "Hold File returned ");
	seln_dump_result(stderr, &result);
	(void)fprintf(stderr, "\n");
    }
}

/*
 *	Caller is yielding control of the indicated selection
 */
static void
seln_svc_done(transport)
    SVCXPRT            *transport;
{
    Seln_holder         input;
    Seln_result         result;

    if (!svc_getargs(transport, xdr_seln_holder, &input)) {
	svcerr_decode(transport);
	(void) svc_sendreply(transport, xdr_enum, 
		(char *)(LINT_CAST(SELN_FAILED)));
	return;
    }
    if (trace_done) {
	(void)fprintf(stderr, "Service entered at Done with args:\n");
	seln_dump_holder(stderr, &input);
    }
    if (ord(input.rank) >= ord(SELN_CARET) &&
	ord(input.rank) <= ord(SELN_SHELF) &&
	seln_equal_access(&holder[ord(input.rank)].access,
			  &input.access)) {
	if (holder[ord(input.rank)].state == SELN_FILE)  {
	    (void)close(held_file[ord(input.rank)]);
	    held_file[ord(input.rank)] = 0;
	}
	/* preserve the fact that this selection was made
	 * by resetting state, but not access
	 */
	holder[ord(input.rank)].state = SELN_NONE; 
	result = SELN_SUCCESS;
    } else {
	result = SELN_FAILED;
    }
    (void) svc_sendreply(transport, xdr_enum, 
    	(char *)(LINT_CAST(&result)));

    if (trace_done) {
	(void)fprintf(stderr, "Done returned ");
	seln_dump_result(stderr, &result);
	(void)fprintf(stderr, "\n");
    }
}

/*
 *	Caller has seen a function-key transition which it believes 
 *	to be of interest to the selection manager.
 */
static void
seln_svc_inform(transport)
    SVCXPRT              *transport;
{
    Seln_inform_args      input;
    Seln_function_buffer  result;

    if (!svc_getargs(transport, xdr_seln_inform_args, &input)) {
	svcerr_decode(transport);
	(void) svc_sendreply(transport, xdr_enum, 
		(char *)(LINT_CAST(SELN_FAILED)));
	return;
    }
    if (trace_inform) {
	(void)fprintf(stderr, "Service entered at Inform with args:\n");
	seln_dump_inform_args(stderr, &input);
    }
    if (input.down) {		       /* button-down: note status	 */
	if (!seln_function_pending()) {
	    if (holder[ord(SELN_SECONDARY)].state == SELN_EXISTS) {
#ifdef SEND_YIELD
		(void)seln_send_yield(SELN_SECONDARY, &holder[ord(SELN_SECONDARY)], TRUE);
#endif
		holder[ord(SELN_SECONDARY)].state = SELN_NONE;
	    }
	    holder[ord(SELN_SECONDARY)].access = seln_null_holder.access;
	}
	seln_function_set(input.function);
	result = seln_null_function;
    } else {			       /* button-up	 */
	seln_function_clear(input.function);
	if (!seln_function_pending()) {/* invoke transfer	 */
	    result = seln_execute_fn(&input);
	} else {
	    result = seln_null_function;
	}
    }
    (void) svc_sendreply(transport, xdr_seln_function, 
    	(char *)(LINT_CAST(&result)));

    if (trace_inform) {
	(void)fprintf(stderr, "Inform returned ");
	seln_dump_function_buffer(stderr, &result);
	(void)fprintf(stderr, "\n");
    }
}

/*
 *  Caller wants to know how to contact the holder of the indicated selection.
 *	A valid indication is SELN_UNSPECIFIED, which means "you figure out
 *	whether I mean primary or secondary according to the state of the
 *	function keys, and tell me."
 */
static void
seln_svc_inquire(transport)
    SVCXPRT            *transport;
{
    Seln_rank           input;
    Seln_holder         result;

    if (!svc_getargs(transport, xdr_enum, &input)) {
	svcerr_decode(transport);
	(void) svc_sendreply(transport, xdr_seln_holder, 
		(char *)(LINT_CAST(&seln_null_holder)));
	return;
    }
    if (trace_inquire) {
	(void)fprintf(stderr, "Service entered at Inquire with rank: ");
	seln_dump_rank(stderr, &input);
	(void)fprintf(stderr, "\n");
    }
    switch (input) {
      case SELN_UNSPECIFIED:
	if (seln_function_pending()) {
	    input = SELN_SECONDARY;
	} else {
	    input = SELN_PRIMARY;
	}			   /* FALL THROUGH	 */
      case SELN_CARET:
      case SELN_PRIMARY:
      case SELN_SECONDARY:
      case SELN_SHELF:
	result = holder[ord(input)];
	break;
      default:
	result = seln_null_holder;
    }
    (void) svc_sendreply(transport, xdr_seln_holder, 
    	(char *)(LINT_CAST(&result)));

    if (trace_inquire) {
	(void)fprintf(stderr, "Inquire returned: ");
	seln_dump_holder(stderr, &result);
    }
}

/*
 *  Caller wants to know holders of all selections
 */
static void
seln_svc_inquire_all(transport)
    SVCXPRT            *transport;
{
    Seln_holders_all    result;

    if (trace_inquire) {
	(void)fprintf(stderr, "Service entered at Inquire_all\n");
    }
    bcopy((char *) &holder[ord(SELN_CARET)],
	  (char *) &result, sizeof (Seln_holder) * (SELN_RANKS - 1));
    (void) svc_sendreply(transport, xdr_seln_holders_all, 
    	(char *)(LINT_CAST(&result)));

    if (trace_inquire) {
	(void)fprintf(stderr, "Inquire_all returned:\n");
	seln_dump_holder(stderr, &result.caret);
	seln_dump_holder(stderr, &result.primary);
	seln_dump_holder(stderr, &result.secondary);
	seln_dump_holder(stderr, &result.shelf);
    }
}

/*
 *  Caller is telling service all function keys are up
 */
static void
seln_svc_clear_functions(transport)
    SVCXPRT            *transport;
{
    seln_function_reset();
    if (holder[ord(SELN_SECONDARY)].state == SELN_EXISTS) {
	(void)seln_send_yield(SELN_SECONDARY, &holder[ord(SELN_SECONDARY)], TRUE);
	holder[ord(SELN_SECONDARY)].state = SELN_NONE;
    }
    (void) svc_sendreply(transport, xdr_void, (char *)0);
}

/*
 *  	Caller wants to know the state of a function key
 */
static void
seln_svc_function_one(transport)
    SVCXPRT            *transport;
{
    Seln_function	 input;
    u_int		 result;

    if (!svc_getargs(transport, xdr_enum, &input)) {
	svcerr_decode(transport);
	(void) svc_sendreply(transport, xdr_seln_holder, 
		(char *)(LINT_CAST(&seln_null_holder)));
	return;
    }
    result = seln_function_state(input);
    (void) svc_sendreply(transport, xdr_u_int, 
    	(char *)(LINT_CAST(&result)));
}


/*
 *  	Caller wants to know the state of all function keys
 */
static void
seln_svc_functions_all(transport)
    SVCXPRT            *transport;
{
    Seln_functions_state result;
    result = seln_functions_all();
    (void) svc_sendreply(transport, xdr_seln_functions_state, 
    	(char *)(LINT_CAST(&result)));
}

/*
 *	Give_holder: clients ask for a holder for the service as they start up
 */
static void
seln_svc_give_holder(transport)
     SVCXPRT            *transport;
 {
     Seln_holder	 hold;

     hold.rank = SELN_UNKNOWN;
     hold.state = SELN_NONE;
     hold.access = clients.access;
     (void) svc_sendreply(transport, xdr_seln_holder, 
     	(char *)(LINT_CAST(&hold)));
 }
 
 
/*	Procedures by which the service resonds to requests about
 *	files it's acting as the holder of
 */

/*	seln_svc_do_function: respond to a function-key release when
 *		service holds a file as a selection or shelf.
 */
/*ARGSUSED*/
static void
seln_svc_do_function(private, buffer)
    caddr_t               private;
    Seln_function_buffer *buffer;
{
    Seln_holder          **hold = NULL;

    if (seln_figure_response(buffer, hold) != SELN_IGNORE) {
	complain(
"Service asked to to perform meaningless operation on a file-selection?"
		 );
    }
}


/*	seln_svc_do_reply: respond to a request concerning a selection
 *		for which selection service holds a file.
 */
static Seln_result
seln_svc_do_reply(item, context, len)
    Seln_attribute        item;
    Seln_replier_data    *context;
    int                   len;
{
    struct stat           stat_buf;
    int                   fd, count, size;
    char                 *destp;
    extern long lseek();

    if ((int) item >= ord(SELN_TRACE_ACQUIRE) &&
	(int) item <= ord(SELN_TRACE_DUMP)) {
	*context->response_pointer++ =
	    (char *) seln_svc_debug(item, (u_int)(LINT_CAST(
	    	*context->request_pointer)));
	return SELN_SUCCESS;
    }
    if ((int) item == SELN_SET_NOTIFY_SUPPRESSION
     ||	(int) item == SELN_GET_NOTIFY_SUPPRESSION) {
	*context->response_pointer++ =
	    seln_svc_notify_suppression(item,
			    (u_int)(LINT_CAST(*context->request_pointer)));
	return SELN_SUCCESS;
    }
    if (holder[ord(context->rank)].state != SELN_FILE) {
	return SELN_DIDNT_HAVE;
    }
    fd = held_file[ord(context->rank)];
    if (fstat(fd, &stat_buf) != 0) {
	perror("Selection service couldn't reply about a file");
	return SELN_FAILED;
    }
    if (context->context == 0) {
	if (lseek(fd, 0L, 0) == (long)-1) {
	    perror("Selection service couldn't reset to start of file");
	    return SELN_FAILED;
	}
    }
    switch (item) {
      case SELN_REQ_BYTESIZE:
	*context->response_pointer++ = (char *) stat_buf.st_size;
	return SELN_SUCCESS;
      case SELN_REQ_CONTENTS_ASCII:
	size = stat_buf.st_size - (int) context->context;
	if (size > len) {
	    count = read(fd,(char *)(LINT_CAST(context->response_pointer)), len);
	    if (count != len) {
		goto terminate_buffer;
	    }
	    context->response_pointer =  (char **) (LINT_CAST(
		((char *) (LINT_CAST(context->response_pointer)) + count)));
	    context->context += count;
	    return SELN_CONTINUED;
	} else {
	    count = read(fd,(char*)(LINT_CAST(context->response_pointer)), size);
	terminate_buffer:
	    destp = (char *) context->response_pointer;
	    destp += count;
	    while ((int) destp % 4 != 0) {
		*destp++ = '\0';
	    }
	    context->response_pointer = (char **) (LINT_CAST(destp));
	    *context->response_pointer++ = (char *) 0;
	    return SELN_SUCCESS;
	}
      case SELN_REQ_YIELD:
	*context->response_pointer++ =
	    (char *) seln_svc_do_yield(context->rank);
	return SELN_SUCCESS;
      case SELN_REQ_END_REQUEST:
	return SELN_SUCCESS;
      default:
	return SELN_UNRECOGNIZED;
    }
}


/*	seln_svc_do_yield: respond to a yield request when
 *		service holds a file as a selection or shelf.
 */
static Seln_result
seln_svc_do_yield(input)
    Seln_rank             input;
{
    Seln_result		  result;

    if (trace_yield) {
	(void)fprintf(stderr, "Service told to yield ");
	seln_dump_rank(stderr, &input);
	(void)fprintf(stderr, " selection.\n");
    }
    if (ord(input) < ord(SELN_PRIMARY) || ord(input) > ord(SELN_SHELF) ||
	    holder[ord(input)].state != SELN_FILE) {
	    result = SELN_DIDNT_HAVE;
    } else if (seln_function_pending && ord(input) == ord(SELN_PRIMARY)) {
	    result = SELN_WRONG_RANK;
    } else {    /* reset state, but not access, to preserve the fact that
		 * this selection was made */
	holder[ord(input)].state = SELN_NONE;
	(void)close(held_file[ord(input)]);
	held_file[ord(input)] = 0;
	    result = SELN_SUCCESS;
    }
    if (trace_yield) {
	(void)fprintf(stderr, "Service yield returns ");
	seln_dump_result(stderr, &result);
	(void)fprintf(stderr, "\n");
    }
    return result;
}


/*
 *	Init_access:  fill in the service_access struct so we can
 *	respond to requests for file-selections.
 */
static void
seln_svc_init_access(access)
    Seln_access		 *access;
{
    if ((seln_my_udp_transport = svcudp_bufcreate(RPC_ANYSOCK,
					SELN_RPC_BUFSIZE, SELN_RPC_BUFSIZE))
	== (SVCXPRT *) NULL) {
	perror("selection_service");
	(void)fprintf(stderr, "Couldn't start selection service.\n");
	return;
    }
    seln_my_udp_socket = seln_my_udp_transport->xp_sock;
    (void) seln_svc_unmap();
    if (!svc_register(seln_my_udp_transport, seln_svc_program, SELN_VERSION,
		      seln_svc_dispatch, IPPROTO_UDP)) {
	perror("selection_service");
	(void)fprintf(stderr, "Couldn't start selection service.\n");
	(void)close(seln_my_udp_socket);
	svc_destroy(seln_my_udp_transport);
	return;
    }
    access->pid = getpid();
    access->program = seln_svc_program;
    get_myaddress(&access->udp_address);
    access->udp_address.sin_port = htons(seln_my_udp_transport->xp_port);
    access->client = (caddr_t) &clients;
    old_func = notify_set_input_func((Notify_client) seln_my_udp_transport,
				     seln_listener, seln_my_udp_socket);
    if (seln_get_tcp(seln_svc_program, seln_svc_dispatch) != SELN_SUCCESS) {
	(void)close(seln_my_udp_socket);
	svc_destroy(seln_my_udp_transport);
	return;
    }
    access->tcp_address = access->udp_address;
    access->tcp_address.sin_port = htons(seln_my_tcp_transport->xp_port);
}


/*
 *	Tell holders of data selections that a function key has gone up,
 *	 & who they should contact about it.  This routine returns the
 *	 buffer (and doesn't send it to the informing client) to prevent
 *	 race where the informer might get a button click between informing
 *	 the service and getting its notification.  This way, it acts on the
 *	 return value from seln_inform, before going back to the notifier.
 */
static Seln_function_buffer
seln_execute_fn(args)
    Seln_inform_args *args;
{

    static Seln_rank informer;
    Seln_function_buffer buffer;

    buffer.function = args->function;
    buffer.caret = holder[ord(SELN_CARET)];
    buffer.primary = holder[ord(SELN_PRIMARY)];
    buffer.secondary = holder[ord(SELN_SECONDARY)];
    buffer.shelf = holder[ord(SELN_SHELF)];
    informer = SELN_UNKNOWN;

    if (!full_notifications) {	/* New style: 1 notification  */
	register Seln_rank recipient;

	switch (args->function) {
	  case SELN_FN_GET:
	  case SELN_FN_FIND:
	    recipient = SELN_CARET;
	    break;
	  case SELN_FN_PUT:
	  case SELN_FN_DELETE:
	    if (buffer.secondary.state == SELN_EXISTS) {
		recipient = SELN_SECONDARY;
	    } else {
		recipient = SELN_PRIMARY;
	    }
	    break;
	  default:
	    return seln_null_function;
	}
	if (holder[ord(recipient)].state != SELN_EXISTS)
	    return seln_null_function;
	/*  if the informer is the intended recipient, tell him in the
	 *  return value.  Otherwise, send a notification and give the
	 *  informer a no-op return.
	 */
	if (seln_equal_access(&args->holder.access,
			      &holder[ord(recipient)].access)) {
	    buffer.addressee_rank = recipient;
	    return buffer;
	} else {
	    (void) seln_send_msg(recipient, &buffer);
	    return seln_null_function;
	}
    }
     else {			/* Old style: 1 - 4 notifications  */
	if (holder[ord(SELN_CARET)].state != SELN_NONE) {
	    if (seln_equal_access(&args->holder.access, &buffer.caret.access)) {
		informer = SELN_CARET;
	    } else {
		if (seln_send_msg(SELN_CARET, &buffer) != SELN_SUCCESS) {
		    return seln_null_function;
		}
	    }
	}
	if (holder[ord(SELN_PRIMARY)].state != SELN_NONE
	    && !seln_equal_access(&buffer.primary.access,
				  &buffer.caret.access)) {
	    if (seln_equal_access(&args->holder.access,
				  &buffer.primary.access)) {
		informer = SELN_PRIMARY;
	    } else {
		if (seln_send_msg(SELN_PRIMARY, &buffer) != SELN_SUCCESS) {
		    return seln_null_function;
		}
	    }
	}
	if (holder[ord(SELN_SECONDARY)].state != SELN_NONE
	    && !seln_equal_access(&buffer.secondary.access,
				  &buffer.caret.access)
	    && !seln_equal_access(&buffer.secondary.access,
				  &buffer.primary.access)) {
	    if (seln_equal_access(&args->holder.access,
				  &buffer.secondary.access)) {
		informer = SELN_SECONDARY;
	    } else {
		if (seln_send_msg(SELN_SECONDARY, &buffer) != SELN_SUCCESS) {
		    return seln_null_function;
		}
	    }
	}
	if (holder[ord(SELN_SHELF)].state != SELN_NONE
	    && !seln_equal_access(&buffer.shelf.access,
				  &buffer.caret.access)
	    && !seln_equal_access(&buffer.shelf.access,
				  &buffer.primary.access)
	    && !seln_equal_access(&buffer.shelf.access,
				  &buffer.secondary.access)) {
	    if (seln_equal_access(&args->holder.access, &buffer.shelf.access)) {
		informer = SELN_SHELF;
	    } else {
		if (seln_send_msg(SELN_SHELF, &buffer) != SELN_SUCCESS) {
		    return seln_null_function;
		}
	    }
	}
    }
    if (informer == SELN_UNKNOWN) {
	return seln_null_function;
    } else {
	buffer.addressee_rank = informer;
	return buffer;
    }
}

static Seln_result
seln_send_msg(rank, buffer)
    Seln_rank             rank;
    Seln_function_buffer *buffer;
{
    register Seln_holder *recipient;
    register CLIENT      *client;
    struct timeval        timeout;
    enum clnt_stat        result;
    int                   socket = RPC_ANYSOCK;

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    recipient = &holder[ord(rank)];
    buffer->addressee_rank = rank;
    if ((client = clntudp_bufcreate(&recipient->access.udp_address,
				    recipient->access.program, SELN_VERSION,
				    timeout, &socket,
				    SELN_RPC_BUFSIZE, SELN_RPC_BUFSIZE))
	== (CLIENT *) NULL) {
	return SELN_FAILED;
    }
    if (trace_inform) {
	(void)fprintf(stderr, "\tfunction-key notification sent to ");
	seln_dump_rank(stderr, &rank);
	(void)fprintf(stderr, " holder.\n");
    }
    result = clnt_call(client, SELN_CLNT_DO_FUNCTION,
		       xdr_seln_function, buffer, xdr_void, 0, timeout);
    clnt_destroy(client);
    (void)close(socket);	/*  no, clnt_destroy didn't close it....  */
    if (result == RPC_TIMEDOUT || result == RPC_SUCCESS)  {
	if (trace_inform) {
	    (void)fprintf(stderr, "    notification returns Success\n");
	}
	return SELN_SUCCESS;
    } else {
	if (trace_inform) {
	    (void)fprintf(stderr, "    notification returns Failure\n");
	}
	return SELN_FAILED;
    }
}

static char *
seln_svc_debug(attr, value)
    Seln_attribute        attr;
    u_int                 value;
{
    if (attr == SELN_TRACE_DUMP) {
	return (char *) seln_dump_service(stderr, holder, (Seln_rank)value);
    }
    (void)fprintf(stderr, "Selection service trace %s for ", (value ? "on" : "off"));
    switch (attr) {
      case SELN_TRACE_ACQUIRE:
	(void)fprintf(stderr, "acquire.\n");
	if (value == TRUE) {
	    return (char *) (trace_acquire = TRUE);
	} else {
	    return (char *) (trace_acquire = FALSE);
	}
      case SELN_TRACE_DONE:
	(void)fprintf(stderr, "done.\n");
	if (value == TRUE) {
	    return (char *) (trace_done = TRUE);
	} else {
	    return (char *) (trace_done = FALSE);
	}
      case SELN_TRACE_HOLD_FILE:
	(void)fprintf(stderr, "hold file.\n");
	if (value == TRUE) {
	    return (char *) (trace_hold_file = TRUE);
	} else {
	    return (char *) (trace_hold_file = FALSE);
	}
      case SELN_TRACE_INFORM:
	(void)fprintf(stderr, "inform.\n");
	if (value == TRUE) {
	    return (char *) (trace_inform = TRUE);
	} else {
	    return (char *) (trace_inform = FALSE);
	}
      case SELN_TRACE_INQUIRE:
	(void)fprintf(stderr, "inquire.\n");
	if (value == TRUE) {
	    return (char *) (trace_inquire = TRUE);
	} else {
	    return (char *) (trace_inquire = FALSE);
	}
      case SELN_TRACE_YIELD:
	(void)fprintf(stderr, "yield.\n");
	if (value == TRUE) {
	    return (char *) (trace_yield = TRUE);
	} else {
	    return (char *) (trace_yield = FALSE);
	}
      case SELN_TRACE_STOP:
	(void)fprintf(stderr, "stop.\n");
	if (value == TRUE) {
	    return (char *) (trace_stop = TRUE);
	} else {
	    return (char *) (trace_stop = FALSE);
	}
	/*NOTREACHED*/
    }
    /*NOTREACHED*/
}

/*	return or modify the flag which controls how many notifications
 *	are sent on a function-button up (in response to a SELN_INFORM).
 */
static char *
seln_svc_notify_suppression(item, context, len)
    Seln_attribute        item;
    Seln_replier_data    *context;
    int                   len;
{
    int                   old;

    if (item == SELN_GET_NOTIFY_SUPPRESSION) {
	return (char *)full_notifications;
    } else {
	old = full_notifications;
	full_notifications = (int) *context->response_pointer;
	return (char *)old;
    }
}


/* how many interfaces could there be on a computer? */
#define	MAX_LOCAL 64
static struct in_addr addrs[MAX_LOCAL];

bool_t
chklocal(taddr)
	struct in_addr	taddr;
{
	int		i;
	struct in_addr	iaddr;
	int 		num_local;

	if (taddr.s_addr == INADDR_LOOPBACK)
		return (TRUE);
	num_local = getlocal();
	for (i = 0; i < num_local; i++) {
		iaddr.s_addr = ntohl(addrs[i].s_addr);
		if (bcmp((char *) &taddr, (char *) &(iaddr),
			sizeof (struct in_addr)) == 0)
			return (TRUE);
	}
	return (FALSE);
}

int
getlocal()
{
	struct ifconf	ifc;
	struct ifreq	ifreq, *ifr;
	int		n, j, sock;
	char		buf[UDPMSGSIZE];

	ifc.ifc_len = UDPMSGSIZE;
	ifc.ifc_buf = buf;
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return (FALSE);
	if (ioctl(sock, SIOCGIFCONF, (char *) &ifc) < 0) {
		perror("sel_svc:ioctl SIOCGIFCONF");
		(void) close(sock);
		return (FALSE);
	}
	ifr = ifc.ifc_req;
	j = 0;
	for (n = ifc.ifc_len / sizeof (struct ifreq); n > 0; n--, ifr++) {
		ifreq = *ifr;
		if (ioctl(sock, SIOCGIFFLAGS, (char *) &ifreq) < 0) {
			perror("sel_svc:ioctl SIOCGIFFLAGS");
			continue;
		}
		if ((ifreq.ifr_flags & IFF_UP) &&
			ifr->ifr_addr.sa_family == AF_INET) {
			if (ioctl(sock, SIOCGIFADDR, (char *) &ifreq) < 0) {
				perror("SIOCGIFADDR");
			} else {
				addrs[j] = ((struct sockaddr_in *)
						& ifreq.ifr_addr)->sin_addr;
				j++;
			}
		}
		if (j >= (MAX_LOCAL - 1))
			break;
	}
	(void) close(sock);
	return (j);
}
