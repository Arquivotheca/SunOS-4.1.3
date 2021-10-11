#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)exit.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

#include <sys/types.h>

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

struct handlers {
	void	(*handler)();
	caddr_t	arg;
	struct	handlers *next;
};

extern	void _cleanup();

/* the list of handlers and their arguments */
struct	handlers *_exit_handlers;

/*
 * exit -- do termination processing, then evaporate process
 */
void
exit(code)
	int code;
{
	register struct handlers *h;

	while (h = _exit_handlers) {
		_exit_handlers = h->next;
		(*h->handler)(code, h->arg);
	}
	_cleanup();
	_exit(code);
}
