/* @(#) except.h 1.1 92/07/30 */
/* Copyright (C) 1987. Sun Microsystems, Inc. */
#ifndef __EXCEPT_H__
#define	__EXCEPT_H__

/* exception context */
typedef struct exccontext_t {
	struct exccontext_t *exc_next;	/* stack of exception handlers */
	int exc_sp;		/* stack at time of exc_handle */
	int exc_pc;		/* return addr from exc_handle() call */
	int exc_return;		/* normal return addr of handling procedure */
	int *exc_retaddr;	/* location of exc_return (for patching) */
	int exc_refcnt;		/* per-procedure count of handlers set */
	int exc_pattern;	/* pattern bound by exc_handle() */
	caddr_t (*exc_func)();	/* function bound by exc_handle() */
	caddr_t exc_env;	/* argument to the function */
} exccontext_t;
#define	EXCNULL	((exccontext_t *) 0)

/*
 * The CATCHALL pattern will match ANY pattern. If you allocate
 * some resource and must reclaim it despite possible
 * exceptions, use a CATCHALL handler to catch any exceptions.
 * 0 and -1 are not usable as patterns by exc_handle.
 * exc_handle returns:
 *	0 if the handler was established correctly
 *	-1 if there was an error in establishing the handler
 *	else pattern from excraise
 */
#define	EXITPATTERN	0		/* indicates an exit handler */

extern int __exccleanup();
extern void __exceptcleanup(/* corpse */);
extern void __init_exc();

#endif __EXCEPT_H__
