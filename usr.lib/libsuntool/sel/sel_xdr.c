#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)sel_xdr.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

#include <suntool/selection_impl.h>
#include <sunwindow/rect.h>		/* LINT_CAST definition */
#include <sunwindow/notify.h>
#include <signal.h>

static	int	    sigpipe_occurred;
static Notify_value sigpipe_handler();
static Notify_value (*oldsigpipe_handler)();

/*
 *	sel_xdr.c:  XDR routines to support the selection service
 */


int
xdr_seln_access(xdrsp, argsp)
    XDR                *xdrsp;
    Seln_access        *argsp;
{

    return (xdr_int(xdrsp, (int *)(LINT_CAST(&argsp->pid)))	&&
	    xdr_int(xdrsp, &argsp->program)		&&
	    xdr_sockaddr_in(xdrsp, &argsp->tcp_address)	&&
	    xdr_sockaddr_in(xdrsp, &argsp->udp_address)	&&
	    xdr_opaque(xdrsp, (char *)(LINT_CAST(&argsp->client)), 
	    		sizeof (char *)));
}

int
xdr_seln_file_info(xdrsp, argsp)
    XDR             *xdrsp;
    Seln_file_info  *argsp;
{
    return (xdr_int(xdrsp, (int *)(LINT_CAST(&argsp->rank)))    &&
	    xdr_string(xdrsp, &argsp->pathname, SELN_MAX_PATHNAME));
}

int
xdr_seln_function(xdrsp, argsp)
    XDR                  *xdrsp;
    Seln_function_buffer *argsp;
{
    return (xdr_enum(xdrsp, &argsp->function)		&&
	    xdr_enum(xdrsp, &argsp->addressee_rank)	&&
	    xdr_seln_holder(xdrsp, &argsp->caret)	&&
	    xdr_seln_holder(xdrsp, &argsp->primary)	&&
	    xdr_seln_holder(xdrsp, &argsp->secondary)	&&
	    xdr_seln_holder(xdrsp, &argsp->shelf));
}

int
xdr_seln_functions_state(xdrsp, argsp)
    XDR                  *xdrsp;
    Seln_functions_state *argsp;
{
    register u_int	  i;

    for  (i = 0; i < SELN_FUNCTION_WORD_COUNT; i++) {
	if (!xdr_u_int(xdrsp, argsp->data + i))
	   return FALSE;
    }
    return TRUE;
}

int
xdr_seln_holder(xdrsp, argsp)
    XDR                *xdrsp;
    Seln_holder        *argsp;
{
    return (xdr_int(xdrsp, (int *)(LINT_CAST(&argsp->rank)))	&&
	    xdr_enum(xdrsp, &argsp->state)	&&
	    xdr_seln_access(xdrsp, &argsp->access));
}

int
xdr_seln_holders_all(xdrsp, argsp)
    XDR                *xdrsp;
    Seln_holders_all   *argsp;
{
    return (xdr_seln_holder(xdrsp, &argsp->caret)	&&
	    xdr_seln_holder(xdrsp, &argsp->primary)	&&
	    xdr_seln_holder(xdrsp, &argsp->secondary)	&&
	    xdr_seln_holder(xdrsp, &argsp->shelf));
}

int
xdr_seln_inform_args(xdrsp, argsp)
    XDR                *xdrsp;
    Seln_inform_args   *argsp;
{
    return (xdr_seln_holder(xdrsp, &argsp->holder)	&&
	    xdr_enum(xdrsp, &argsp->function)		&&
	    xdr_int(xdrsp, &argsp->down));
}

int
xdr_seln_reply(xdrsp, argsp)
    XDR                  *xdrsp;
    Seln_request         *argsp;
{
    int			  get_ok = TRUE;

    switch (xdrsp->x_op) {
      case XDR_ENCODE:
	do {
	    if (seln_get_reply_buffer(argsp) != SELN_SUCCESS)
		get_ok =  FALSE;
	    if (!xdr_seln_request(xdrsp, argsp))
		return FALSE;
	} while (get_ok && argsp->status == SELN_CONTINUED);
	return TRUE;
      case XDR_DECODE:
	for (;;) {
	    if (!xdr_seln_request(xdrsp, argsp)) {
		return FALSE;
	    }
	    if (argsp->requester.consume != 0) {
		if (argsp->requester.consume(argsp) != SELN_SUCCESS) {
		    return TRUE;
		}
		if (argsp->status != SELN_CONTINUED) {
		    return TRUE;
		}
	    } else {
		return TRUE;
	    }
	}
      case XDR_FREE:
	return TRUE;
    }
    /*NOTREACHED*/
}


/* bug# 1018992 -- trapped SIGPIPE since the receiver can close the pipe
 * at any time.  Recovered by clearing functions.
 */

int
xdr_seln_request(xdrsp, argsp)
    XDR                  *xdrsp;
    Seln_request         *argsp;
{
    char                 *data = argsp->data;
    int			 error;

    sigpipe_occurred = 0;
    oldsigpipe_handler = notify_set_signal_func(SIGPIPE, sigpipe_handler,
                                                SIGPIPE, NOTIFY_ASYNC);

    error = xdr_opaque(xdrsp, (char *)(LINT_CAST(&argsp->replier)), sizeof (char *))
	    && xdr_seln_requester(xdrsp, &argsp->requester)
	    && xdr_opaque(xdrsp, (char *)(LINT_CAST(&argsp->addressee)), sizeof (char *))
            && xdr_enum(xdrsp, &argsp->rank)
	    && xdr_enum(xdrsp, &argsp->status)
	    && xdr_u_int(xdrsp, &argsp->buf_size)
	    && xdr_bytes(xdrsp, &data, &argsp->buf_size, SELN_BUFSIZE);

    notify_set_signal_func(SIGPIPE, *oldsigpipe_handler, SIGPIPE, NOTIFY_ASYNC);

    return (error && ! sigpipe_occurred);
}


static Notify_value
sigpipe_handler(me, signal, when)
Notify_client me;
int signal;
Notify_signal_mode when;
{
    sigpipe_occurred = 1;
    seln_clear_functions();
    return(NOTIFY_DONE);
}


int
xdr_seln_requester(xdrsp, argsp)
    XDR                 *xdrsp;
    Seln_requester	*argsp;
{

    return (xdr_opaque(xdrsp, (char *)(LINT_CAST(&argsp->consume)), 
    		sizeof (char *))	    &&
	    xdr_opaque(xdrsp, (char *)(LINT_CAST(&argsp->context)), 
	    	sizeof (char *)));
}

int
xdr_sockaddr_in(xdrsp, argsp)
    XDR                  *xdrsp;
    struct sockaddr_in   *argsp;
{
    unsigned              length;

    length = sizeof (*argsp);
    return (xdr_bytes(xdrsp, (char **) &argsp, &length, length));
}
