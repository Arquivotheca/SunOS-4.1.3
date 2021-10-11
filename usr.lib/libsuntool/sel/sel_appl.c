#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)sel_appl.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

#include <suntool/selection_impl.h>
#include <sunwindow/attr.h>
#include <sunwindow/rect.h>
#include <varargs.h>

static void	seln_null_terminate_buffer();
static void	seln_init_request_buffer();
extern		char *malloc();

/*
 *	Generic request to another holder
 */
/* VARARGS1 */
Seln_request *
seln_ask(holder, va_alist)
Seln_holder          *holder;
va_dcl
{
    static Seln_request  *buffer;
    va_list	valist;

    if (buffer == (Seln_request *) NULL) {
	buffer = (Seln_request *) (LINT_CAST(
		malloc ((unsigned)(sizeof(Seln_request)))));
	if (buffer == (Seln_request *) NULL) {
	    (void)fprintf(stderr,
		    "Couldn't malloc request buffer (no swap space?)\n");
	    return &seln_null_request;
	}
    }
    if (holder->state == SELN_NONE) {
	return &seln_null_request;
    }
    va_start(valist);
    if (attr_make((char **) (LINT_CAST(buffer->data)),
		  sizeof (buffer->data) /sizeof (char *),
		  valist) == (char **) 0) {
	complain("Selection request too big -- not sent");
	va_end(valist);
	return &seln_null_request;
    }
    va_end(valist);
    seln_init_request_buffer(buffer, holder);
    if (seln_request(holder, buffer) == SELN_SUCCESS) {
	return buffer;
    } else {
	return &seln_null_request;
    }
}

/* VARARGS2 */
void
seln_init_request(buffer, holder, va_alist)
Seln_request         *buffer;
Seln_holder          *holder;
va_dcl
{

    va_list	valist;
    
    va_start(valist);
    if (attr_make((char **) (LINT_CAST(buffer->data)),
		  sizeof(buffer->data) / sizeof(char *),
		  valist) == (char **) 0) {
	complain("Selection request too big -- not sent");
	va_end(valist);
	return;
    }
    va_end(valist);
    seln_init_request_buffer(buffer, holder);
}

/* VARARGS3 */
Seln_result
seln_query(holder, reader, context, va_alist)
Seln_holder          *holder;
Seln_result         (*reader)();
char                 *context;
va_dcl
{
    static Seln_request  *buffer;
    va_list	valist;

    if (buffer == (Seln_request *) NULL) {
	buffer = (Seln_request *) (LINT_CAST(
		malloc ((unsigned)(sizeof(Seln_request)))));
	if (buffer == (Seln_request *) NULL) {
	    (void)fprintf(stderr,
		    "Couldn't malloc request buffer (no swap space?)\n");
	    return SELN_FAILED;
	}
    }
    if (holder->state == SELN_NONE) {
	return SELN_FAILED;
    }
    va_start(valist);
    if (attr_make((char **) (LINT_CAST(buffer->data)),
		  sizeof(buffer->data) / sizeof(char *),
		  valist) == (char **) 0) {
	complain("Selection request too big -- not sent");
	va_end(valist);
	return SELN_FAILED;
    }
    va_end(valist)
    seln_init_request_buffer(buffer, holder);
    buffer->requester.consume = reader;
    buffer->requester.context = context;
    return seln_request(holder, buffer);
}

static void
seln_init_request_buffer(buffer, holder)
    Seln_request	 *buffer;
    Seln_holder          *holder;
{
    buffer->buf_size = attr_count((char **)(LINT_CAST(buffer->data))) * 
    	sizeof(char *);
    buffer->rank = holder->rank;
    buffer->addressee = holder->access.client;
    buffer->replier = 0;
    buffer->requester.consume = 0;
    buffer->requester.context = 0;
}
