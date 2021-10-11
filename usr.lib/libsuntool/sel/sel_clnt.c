#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)sel_clnt.c 1.1 92/07/30";
#endif
#endif

#include <varargs.h>
#include <sunwindow/rect.h>	/* LINT_CAST definition */
#include <suntool/selection_impl.h>

/*
 *	Sel_clnt:	client (selection-holder) routines for talking
 *				to the selection service, and to other holders.
 *
 *	There may be several selections active at a time, distinguished by
 *	their "rank" (primary, secondary, shelf).
 *
 *	Terminology:  The old service / client distinction doesn't work too
 *	well here, since the real service does very little, and the "clients"
 *	may both make and respond to calls symmetrically.  So:
 *	The clearinghouse process that helps application programs get in
 *	touch with each other, and maintains the state of the user interface,
 *		is the "service".
 *	The remainder of this process (outside the selection routines)
 *	is the "client."
 *	A process (this or another) that currently holds a selection
 *		is a "holder";
 *	a process that wants to communicate with a holder about the selection
 *		is a "requester."
 */

typedef enum {
    Seln_seized_self, Seln_seized_ok, Seln_seized_blind
}	Seln_seize_result;


/*
 * How we get in touch with the server: 
 */

static CLIENT      *service;
static int          service_socket;
static Seln_holder  service_holder;

/*
 * Procedures 
 */

static int          seln_init_holder_process();

static void 	    seln_client_remove(),
		    seln_dispatch(),
		    seln_update_service();

static Seln_result  seln_get_access(),
		    seln_get_service_holder(),
		    seln_local_request();

static Seln_holder  seln_holder_of_rank();

static Seln_seize_result
		    seln_seize();

static CLIENT      *seln_get_service();

/*
 *	External procedures (called by our client)
 *
 *
 * Create:  generate & store client identification; initialize per-process
 * information (socket, service transport handle, ...) if this is the first
 * client. 
 */
char *
seln_create(function_proc, request_proc, client_data)
    void              (*function_proc)();
    Seln_result       (*request_proc)();
    char               *client_data;
{
    register Seln_client_node *handle;
    extern char *malloc();
    static unsigned num_of_clients; /* incremented each time a new client calls
    				     * seln_create. So each client has a unique
				     * id for varification of existence.
				     */

    if (clients.next == (Seln_client_node *) NULL) {
	if (!seln_init_holder_process()) {
	    return (char *) NULL;
	}
    }
    if ((handle = (Seln_client_node *) (LINT_CAST(
    		malloc((unsigned)(sizeof(Seln_client_node))))))
	== (Seln_client_node *) NULL) {
	return (char *) NULL;
    }
    handle->client_num = ++num_of_clients;
    handle->ops.do_function = function_proc;
    handle->ops.do_request = request_proc;
    handle->access = seln_my_access;
    handle->access.client = (char *)handle;
    handle->client_data = client_data;
    handle->next = clients.next;
    clients.next = handle;
    
    return (char *) handle;
}

/*  Destroy:	destroy a client instance; if that was the last client in
 *		this process, shut down everything else.
 */
void
seln_destroy(client)
    char    *client;
{
    int      i;

    if (client ==  (char *)NULL) {
	(void)fprintf(stderr, "Selection library asked to destroy a 0 client.\n");
	return;
    }
    for (i = 1; i < SELN_RANKS; i++) {
        (void) seln_done(client, (Seln_rank) i);
    }
    seln_client_remove((Seln_client_node *)(LINT_CAST(client)));
    if (clients.next == NULL) {
	(void) notify_set_input_func((Notify_client) seln_my_udp_transport,
                                     NOTIFY_FUNC_NULL, seln_my_udp_socket);
        svc_destroy(seln_my_udp_transport);
	(void)close(seln_my_udp_socket);

	(void) notify_set_input_func((Notify_client) seln_my_tcp_transport,
				     NOTIFY_FUNC_NULL, seln_my_tcp_socket);
	svc_destroy(seln_my_tcp_transport);
	(void)close(seln_my_tcp_socket);

	clnt_destroy(service);
	(void)close(service_socket);
    }
}

/*	Next batch are calls to the selection service
 *
 *      Acquire:	acquire the indicated selection.
 *	(The client may pass in SELN_UNSPECIFIED; this means acquire the
 *	secondary selection if a function key is down, else the primary.)
 *	Return the socket on which to listen for RPC requests; NULL on failure. 
 */
Seln_rank
seln_acquire(seln_client, asked)
    Seln_client	        seln_client;
    Seln_rank           asked;
{
    Seln_rank           given;
    Seln_result         result;
    Seln_holder         buffer;
    enum clnt_stat	status;
    Seln_client_node	*client;

    client = (Seln_client_node *)(LINT_CAST(seln_client));
    if (client == (Seln_client_node *)NULL)  {
	complain("Acquire for a null client");
	return SELN_UNKNOWN;
    }
    /* Determine which if unspecified; take it from current holder if any  */
    if (seln_seize(client->client_data, asked, &given) == Seln_seized_self)  {
	return given;
    }

    /* Tell the service we're it now	 */
    buffer.rank = given;
    buffer.state = SELN_EXISTS;
    buffer.access = client->access;
    status = clnt_call(service, SELN_SVC_ACQUIRE, xdr_seln_holder, &buffer,
		  xdr_enum, &result, seln_std_timeout);
    if (status != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't inform service we hold selection");
	return SELN_UNKNOWN;
    }
    if (result != SELN_SUCCESS) {
	complain("Service wouldn't let us acquire selection");
	(void)fprintf(stderr, "requested selection: %d; result: %d\n",
		given, result);
	return SELN_UNKNOWN;
    }
    return given;
}

/*
 *	Tell the service we're giving up the selection (e.g. on termination)
 */
Seln_result
seln_done(seln_client, rank)
    Seln_client   	seln_client;
    Seln_rank           rank;
{
    Seln_holder         my_descriptor;
    Seln_result         result;
    Seln_client_node	*client;

    client = (Seln_client_node *)(LINT_CAST(seln_client));
    if (client == (Seln_client_node *)NULL)  {
	complain("Done for a null client");
	return SELN_FAILED;
    }

    my_descriptor.rank = rank;
    my_descriptor.access = client->access;
    if (clnt_call(service, SELN_SVC_DONE, xdr_seln_holder, &my_descriptor,
		  xdr_enum, &result, seln_std_timeout)  !=  RPC_SUCCESS) {
	clnt_perror(service, "Couldn't reach service to yield selection");
	return SELN_FAILED;
    }
    return result;
}

/*
 *	We want the service to take over as holder of a selection
 *	whose contents are stored in a file.
 */
Seln_result
seln_hold_file(rank, path)
    Seln_rank		 rank;
    char		*path;
{
    Seln_file_info	 buffer;
    Seln_result		 result;

    if (service == (CLIENT *) NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return SELN_FAILED;
	}
    }
    buffer.rank = rank;
    buffer.pathname = path;
    if (clnt_call(service, SELN_SVC_HOLD_FILE, xdr_seln_file_info,
		  &buffer, xdr_enum, &result, seln_std_timeout)
	!=  RPC_SUCCESS) {
	clnt_perror(service, "Couldn't ask service to hold a file for us");
	return SELN_FAILED;
    }
    return result;
}

/*
 *  Get rid of any selections this process holds
 */
void
seln_yield_all()
{
    register int          i;
    union {
	Seln_holder           array[SELN_RANKS];
	struct {
	    Seln_holder           first;
	    Seln_holders_all      rest;
	}                     rec;
    }                     holders;

    holders.rec.rest = seln_inquire_all();
    for (i = ord(SELN_CARET); i <= ord(SELN_SHELF); i++) {
	if (holders.array[i].state == SELN_EXISTS &&
	    seln_holder_same_process(&holders.array[i])) {
	    (void)seln_send_yield((Seln_rank) i, &holders.array[i], TRUE);
	    seln_update_service((Seln_rank) i);
	}
    }
}

/*
 *	We saw a function key change state; tell the service.
 */
Seln_function_buffer
seln_inform(seln_client, which, down)
    Seln_client    	seln_client;
    Seln_function       which;
    int                 down;
{
    Seln_inform_args    buffer;
    Seln_function_buffer result;
    Seln_client_node	*client;

    client = (Seln_client_node *)(LINT_CAST(seln_client));
    if (service == (CLIENT *) NULL) {
	return seln_null_function;
    }
    buffer.holder.rank = SELN_UNKNOWN;
    buffer.holder.state = SELN_NONE;
    if (client == (Seln_client_node *) NULL) {
	bzero((char *)(LINT_CAST(&buffer.holder.access)), sizeof(Seln_access));
    } else {
	buffer.holder.access = client->access;
    }
    buffer.function = which;
    buffer.down = down;
    result.addressee_rank = SELN_UNKNOWN;
    if (clnt_call(service, SELN_SVC_INFORM, xdr_seln_inform_args, &buffer,
		  xdr_seln_function, &result, seln_std_timeout)
	!= RPC_SUCCESS) {
	clnt_perror(service, "Couldn't tell service function key has changed");
    }
    return result;
}

/*
 *	functions_state: ask service for state of all function-keys
 */
Seln_result
seln_functions_state(result)
    Seln_functions_state *result;
{
    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return SELN_FAILED;
	}
    }
    if (clnt_call(service, SELN_SVC_FUNCTIONS_ALL, xdr_void, 0,
		  xdr_seln_functions_state, result, seln_std_timeout)
	    != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't ask service who holds selection");
	return SELN_FAILED;
    }
    return SELN_SUCCESS;
}

/*
 *	get_function_state:  ask the service whether a function key is down
 */
seln_get_function_state(func)
    Seln_function       func;
{
    u_int		result;

    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return 0;
	}
    }
    if (clnt_call(service, SELN_SVC_FUNCTION_ONE, xdr_enum, &func,
		  xdr_int, &result, seln_std_timeout) != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't ask service who holds selection");
	return 0;
    }
    return result;
}


/*
 *	Inquire:  ask the service for access info for the holder of
 *		  the indicated selection.  (Again, UNSPECIFIED means
 *		  PRIMARY or SECONDARY as appropriate.)
 */
Seln_holder
seln_inquire(which)
    Seln_rank           which;
{
    Seln_holder         result;

    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return seln_null_holder;
	}
    }
    if (clnt_call(service, SELN_SVC_INQUIRE, xdr_enum, &which,
		  xdr_seln_holder, &result, seln_std_timeout) != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't ask service who holds selection");
	return seln_null_holder;
    }
    return result;
}

/*
 *	Inquire_all:  ask the service for access info for the holders of
 *			all selections.
 */
Seln_holders_all
seln_inquire_all()
{
    Seln_holders_all      result;

    if (service == (CLIENT *) NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *) NULL) {
	    result.caret = result.primary = result.secondary = result.shelf =
		seln_null_holder;
	    return result;
	}
    }
    if (clnt_call(service, SELN_SVC_INQUIRE_ALL, xdr_void, 0,
		  xdr_seln_holders_all, &result, seln_std_timeout)
	!= RPC_SUCCESS) {
	clnt_perror(service, "Couldn't ask service for selection holders");
	result.caret = result.primary = result.secondary = result.shelf =
	    seln_null_holder;
	return result;
    }
    return result;
}

/* VARARGS0 */
Seln_result
seln_debug(va_alist)
va_dcl
{
    Seln_request          buffer;
    va_list		valist;
    char                **requestp;

    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *) NULL) {
	    return SELN_FAILED;
	}
    }
    buffer.replier = 0;
    buffer.requester.consume = 0;
    buffer.requester.context = 0;
    buffer.addressee = service_holder.access.client;
    buffer.rank = SELN_UNKNOWN;
    buffer.status = SELN_SUCCESS;
    va_start(valist);
    if (attr_make_count((char **)(LINT_CAST(buffer.data)), 
    	SELN_BUFSIZE / sizeof(char *), valist,
	(int *)(LINT_CAST(&(buffer.buf_size)))) == NULL)  {
		complain("Debug request wouldn't fit in buffer");
		return SELN_FAILED;
    }
    va_end(valist);
    if (clnt_call(service, SELN_CLNT_REQUEST, xdr_seln_request,
			   &buffer, xdr_seln_reply, &buffer, seln_std_timeout)
	!= RPC_SUCCESS) {
	return SELN_FAILED;
    }
    requestp = (char **) (LINT_CAST(buffer.data));
    if (*requestp++ != (char *) SELN_REQ_YIELD) {
	return SELN_FAILED;
    }
    return (Seln_result) *requestp;
}

void
seln_use_test_service()
{
    seln_svc_program |= SELN_SVC_TEST;
}

void
seln_use_timeout(secs)
    int                   secs;
{
    seln_std_timeout.tv_sec = secs;
}

/*
 *	Clear_functions:  tell the service all function keys are up
 */
void
seln_clear_functions()
{
    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return;
	}
    }
    if (clnt_call(service, SELN_SVC_CLEAR_FUNCTIONS, xdr_void, 0,
		  xdr_void, 0, seln_std_timeout) != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't tell service to clear functions");

    }
}

Seln_result
seln_stop(auth)
    int                 auth;
{
    if (service == (CLIENT *)NULL) {
	if ((service = seln_get_service(&service_socket)) == (CLIENT *)NULL) {
	    return SELN_FAILED;
	}
    }
    if (clnt_call(service, SELN_SVC_STOP, xdr_enum, &auth,
		  xdr_void, 0, seln_std_timeout)
	!= RPC_SUCCESS) {
	clnt_perror(service, "Couldn't tell service to stop");
	return SELN_FAILED;
    } else {
	return SELN_SUCCESS;
    }
}


/*	Now calls to other holders
 *
 *	Send a request concerning the selection to its holder
 */
Seln_result
seln_request(holder, buffer)
    Seln_holder          *holder;
    Seln_request         *buffer;
{
    CLIENT               *client;
    Seln_access          *access;
    int                   sock = RPC_ANYSOCK;
    enum clnt_stat        result;

    if (seln_holder_same_process(holder)) {
        return seln_local_request(holder, buffer);
    }
    access = &holder->access;
    if (kill(access->pid, 0) == -1 && errno == ESRCH) {
	return SELN_NON_EXIST;
    }
    client = clnttcp_create(&access->tcp_address, access->program,
			    SELN_VERSION, &sock, sizeof (Seln_request),
			    sizeof (Seln_request));
    if (client == NULL) {
	clnt_pcreateerror("Seln_request");
	complain("Couldn't get client handle for current holder");
	return SELN_FAILED;
    }
    result = clnt_call(client, SELN_CLNT_REQUEST, xdr_seln_request,
		       buffer, xdr_seln_reply, buffer, seln_std_timeout);
    if (result != RPC_SUCCESS) {
	clnt_perror(client, "Request to current holder failed");
	clnt_destroy(client);
	(void)close(sock);
	return SELN_FAILED;
    }
    clnt_destroy(client);
    (void)close(sock);
    return SELN_SUCCESS;
}

/*	short-circuit net if holder of selection is
 *	in the same process as requester
 */
/*ARGSUSED*/
static Seln_result
seln_local_request(holder, buffer)
    Seln_holder          *holder;
    Seln_request         *buffer;
{
    Seln_request          request;
    Seln_replier_data     replier_info;

    request = *buffer;
    (void)seln_init_reply(&request, buffer, &replier_info);
    if (buffer->requester.consume) {

	do {
	    if (seln_get_reply_buffer(buffer) != SELN_SUCCESS) {
		return SELN_FAILED;
	    }
	    if (buffer->requester.consume(buffer) == SELN_CANCEL) {
		*buffer->replier->request_pointer = 
		    (char *) SELN_REQ_END_REQUEST;
		(void) seln_get_reply_buffer(buffer);
		return SELN_SUCCESS;
	    }
	} while (buffer->status == SELN_CONTINUED);
	return SELN_SUCCESS;
    }
    if (seln_get_reply_buffer(buffer) != SELN_SUCCESS) {
	return SELN_FAILED;
    }
    if (buffer->status == SELN_CONTINUED) {
	(void) seln_get_reply_buffer(buffer);
	return SELN_FAILED;
    }
    return SELN_SUCCESS;
}
/*
 *	Procedures which get called remotely
 */

/*	seln_dispatch is called by rpc dispatcher, and in turn calls
 *	the appropriate response function below;
 *	that will (typically) call a client callback proc.
 */
static void
seln_dispatch(request, transport)
    struct svc_req     *request;
    SVCXPRT            *transport;
{

    switch (request->rq_proc) {
      case SELN_SVC_NULLPROC:
	if (!svc_sendreply(transport, xdr_void, (char *)0)) {
	    complain("Couldn't reply to RPC call");
	}
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
 *	Client internal routines
 */
static void
seln_update_service(rank)
    Seln_rank           rank;
{
    Seln_holder         holder;
    Seln_result		result;

    holder = seln_null_holder;
    holder.rank = rank;
    (void) clnt_call(service, SELN_SVC_ACQUIRE, xdr_seln_holder, &holder,
		     xdr_enum, &result, seln_std_timeout);
}

static Seln_seize_result
seln_seize(client_data, asked, given)
    char                 *client_data;
    Seln_rank             asked, *given;
{
    Seln_result           result;
    Seln_holder           holder;

    *given = asked;
    /* Ask the current holder for it	 */
    holder = seln_holder_of_rank(given);
    if (holder.state != SELN_EXISTS) {
	return Seln_seized_ok;
    }
    if (seln_holder_same_client(&holder, client_data)) {
	return Seln_seized_self;
    }
    if (kill(holder.access.pid, 0) == -1 && errno == ESRCH) {
	seln_update_service(holder.rank);
	return Seln_seized_ok;
    }
    switch (result = seln_send_yield(holder.rank, &holder, FALSE)) {
      case SELN_SUCCESS:
	return Seln_seized_ok;
      case SELN_WRONG_RANK:	       /* server's behind; try again	 */
	if (*given == SELN_PRIMARY && asked != SELN_SECONDARY) {
	    *given = SELN_SECONDARY;
	    holder = seln_holder_of_rank(given);
	    if (holder.state != SELN_EXISTS) {
		return Seln_seized_ok;
	    }
	    if (seln_holder_same_client(&holder, client_data)) {
		return Seln_seized_self;
	    }
	    result = seln_send_yield(holder.rank, &holder, TRUE);
	} else {
	    complain("Other holder confused about selection ranks");
	    return Seln_seized_blind;
	}
	if (result == SELN_SUCCESS) {
	    return Seln_seized_ok;
	}			       /* else FALL THROUGH  */
    }
    return Seln_seized_blind;
}

/*
 *	Initialize the per-process communication information
 */
static int
seln_init_holder_process()
{

    if ((service = seln_get_service(&service_socket)) == (CLIENT *) NULL) {
	return FALSE;
    }
    if (seln_get_service_holder(&service_holder) != SELN_SUCCESS) {
	clnt_destroy(service);
	(void)close(service_socket);
	service = (CLIENT *) NULL;
	service_socket = 0;
	return FALSE;
    }
    if ((seln_my_udp_transport = svcudp_bufcreate(RPC_ANYSOCK,
					SELN_RPC_BUFSIZE, SELN_RPC_BUFSIZE))
	== (SVCXPRT *) NULL) {
	complain("Couldn't get a UDP transport for requesters to talk to us");
	return FALSE;
    }
    seln_my_udp_socket = seln_my_udp_transport->xp_sock;
    if (seln_get_access(&seln_my_access) == SELN_FAILED) {
	return FALSE;
    }
    if (!svc_register(seln_my_udp_transport, seln_my_access.program, SELN_VERSION,
		      seln_dispatch, NO_PMAP)) {
	complain("Couldn't register transport for requesters to talk to us");
	svc_destroy(seln_my_udp_transport);
	(void)close(seln_my_udp_socket);
	return FALSE;
    }
    (void) notify_set_input_func((Notify_client) seln_my_udp_transport,
				 seln_listener, seln_my_udp_socket);

    if (seln_get_tcp(seln_my_access.program, seln_dispatch) != SELN_SUCCESS) {
    		complain("Couldn't TCP transport for requesters to talk to us");
	svc_destroy(seln_my_udp_transport);
	(void)close(seln_my_udp_socket);
	return FALSE;
    }
    return TRUE;
}

static CLIENT *
seln_get_service(s)
    int                *s;
{
    struct sockaddr_in  address;

    *s = RPC_ANYSOCK;
    get_myaddress(&address);
    address.sin_port = 0;
    return clntudp_bufcreate(&address, seln_svc_program, SELN_VERSION,
		  seln_std_timeout, s, SELN_RPC_BUFSIZE, SELN_RPC_BUFSIZE);
}

static Seln_result
seln_get_service_holder(holder)
    Seln_holder	*holder;
{
    if (clnt_call(service, SELN_SVC_GET_HOLDER, xdr_void, 0,
		  xdr_seln_holder, holder, seln_std_timeout)
	!= RPC_SUCCESS) {
	clnt_perror(service, "No selection service available");
	return SELN_FAILED;
    } else {
	return SELN_SUCCESS;
    }
}

static Seln_result
seln_get_access(access)
    Seln_access        *access;
{
    struct sockaddr_in *addr;

    access->pid = getpid();
    addr = &access->udp_address;
    get_myaddress(addr);
    addr->sin_port =  htons(seln_my_udp_transport->xp_port);
    access->program = SELN_LIB_PROGRAM;
    return SELN_SUCCESS;
}

static Seln_holder
seln_holder_of_rank(rank)
    Seln_rank          *rank;
{
    enum clnt_stat      result;
    Seln_holder         holder;
    result = clnt_call(service, SELN_SVC_INQUIRE, xdr_enum, rank,
		       xdr_seln_holder, &holder, seln_std_timeout);
    if (result != RPC_SUCCESS) {
	clnt_perror(service, "Couldn't get current holder from service");
	return seln_null_holder;
    }
    *rank = holder.rank;
    return holder;
}

static void
seln_client_remove(target)
    Seln_client_node          *target;
{
    register Seln_client_node *prev;

    for (prev = &clients;
	 prev->next != (Seln_client_node *) NULL;
	 prev = prev->next) {
	if (prev->next == target) {
	    prev->next = target->next;
	    free((char *)(LINT_CAST(target)));
	    return;
	}
    }
    complain("Destroying a non-existent client?");
}
