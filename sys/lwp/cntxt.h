/* @(#)cntxt.h 1.1 92/07/30 Copyr 1987 Sun Micro */
/* Copyright (C) 1987. Sun Microsystems, Inc. */

/* context manipulation functions */

#ifndef _lwp_cntxt_h
#define _lwp_cntxt_h

typedef struct cntxt_actions_t {
	void (*cntxt_save)();		/* save context info */
	void (*cntxt_restore)();	/* restore context info */
	int cntxt_memtype;		/* memory type for the context */
	bool_t cntxt_optimize;		/* enable optimize on context switch */
} cntxt_actions_t;

/* per-thread context descriptor */
typedef struct context_t {
	__queue_t ctx_next;	/* chain of contexts belonging to thread */
	int ctx_type;		/* cookie bound to context memory */
	caddr_t ctx_mem;	/* context memory */
} context_t;
#define	CONTEXTNULL	((context_t *)0)

extern void __init_ctx();
extern void __freectxt();
extern void __ctx_change();
extern int CtxType;

#define	MAXSWTCHACTS	20

#endif /*!_lwp_cntxt_h*/
