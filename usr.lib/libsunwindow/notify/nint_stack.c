#ifndef	lint
#ifdef sccs
static	char sccsid[] = "@(#)nint_stack.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Nint_stack.c - Implement the interposer stack primitives private interface.
 */

#include <sunwindow/ntfy.h>
#include <sunwindow/ndet.h>
#include <sunwindow/nint.h>

pkg_private_data NTFY_CONDITION *nint_stack = 0;
pkg_private_data int	nint_stack_size = 0;
pkg_private_data int	nint_stack_next = 0;

pkg_private Notify_func
nint_push_callout(client, cond)
	NTFY_CLIENT *client;
	NTFY_CONDITION *cond;
{
	Notify_func func;
	Notify_func *functions;
	register NTFY_CONDITION *stack_cond;

	/* Make sure that stack is large enough */
	if (nint_stack_next >= nint_stack_size &&
	    nint_alloc_stack() != NOTIFY_OK)
		return(NOTIFY_FUNC_NULL);
	/* Allocate function list if appropriate */
	if (cond->func_count > 1)
		if ((functions = ntfy_alloc_functions()) == NTFY_FUNC_PTR_NULL)
			return(NOTIFY_FUNC_NULL);
	/* Place condition on stack */
	stack_cond = &nint_stack[nint_stack_next];
	*stack_cond = *cond;
	/*
	 * Fixup stack condition
	 */
	/* Copy function list, if appropriate */
	if (cond->func_count > 1) {
		bcopy((caddr_t)cond->callout.functions, (caddr_t)functions,
		    sizeof(NTFY_NODE));
		stack_cond->callout.functions = functions;
		func = functions[0];
	} else
		func = stack_cond->callout.function;
	/* Set function index */
	stack_cond->func_next = 1;
	/*
	 * Null out fields that are managed by others (for neatness).
	 * However, jam client->nclient into the data field
	 * in order to do some consistency checking later.
	 */
	stack_cond->data.an_u_int = (u_int) client->nclient;
	stack_cond->next = NTFY_CONDITION_NULL;
	/* Bump stack pointer */
	nint_stack_next++;
	return(func);
}

pkg_private void
nint_pop_callout()
{
	NTFY_BEGIN_CRITICAL;
	nint_unprotected_pop_callout();
	NTFY_END_CRITICAL;
}

pkg_private void
nint_unprotected_pop_callout()
{
	register NTFY_CONDITION *stack_cond;

	ntfy_assert(nint_stack_next > 0, "stack underflow");
	/* Decrement stack pointer */
	nint_stack_next--;
	stack_cond = &nint_stack[nint_stack_next];
	/* Free functions list, if allocated before */
	if (stack_cond->func_count > 1)
		ntfy_free_functions(stack_cond->callout.functions);
}

pkg_private Notify_error
nint_alloc_stack()
{
	/* Make sure that stack is large enough */
	if (nint_stack_next >= nint_stack_size) {
		register NTFY_CONDITION *old_stack = nint_stack;
		int old_size = nint_stack_size;

		/* Allocate new stack */
		nint_stack_size += NTFY_FUNCS_MAX*2;
		if ((nint_stack = (NTFY_CONDITION *)
		  ntfy_malloc((u_int)nint_stack_size*sizeof(NTFY_CONDITION))) ==
		    NTFY_CONDITION_NULL)
			return(notify_errno);
		/* Copy old to new stack */
		if (old_stack)
			bcopy((caddr_t)old_stack, (caddr_t)nint_stack,
			    old_size*sizeof(NTFY_CONDITION));
		/* Free previous stack */
		if (old_stack)
			ntfy_free_malloc((NTFY_DATA)old_stack);
	}
	return(NOTIFY_OK);
}

