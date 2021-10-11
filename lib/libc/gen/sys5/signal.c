#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)signal.c 1.1 92/07/30 SMI";
#endif

/*
 * Backwards compatible signal.
 */
#include <signal.h>

void (*
signal(s, a))()
	int s;
	void (*a)();
{
	struct sigvec osv, sv;

	sv.sv_handler = a;
	sv.sv_mask = 0;
	sv.sv_flags = SV_INTERRUPT|SV_RESETHAND;
	if (sigvec(s, &sv, &osv) < 0)
		return (BADSIG);
	return (osv.sv_handler);
}
