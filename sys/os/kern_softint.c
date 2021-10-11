#ifndef lint
static	char sccsid[] = "@(#)kern_softint.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include <sys/types.h>

/*
 * Handle software interrupts through 'softcall' mechanism
 */

typedef int (*func_t)();
/*
 * Until the Hydra floppy driver uses dual-level interrupts--,
 * the Sun3X level needs to be 7, not 6
 */
#ifdef SUN3X_80
#define	spl_softint	spl7
#else
#define	spl_softint	spl6
#endif SUN3X_80

#define	NSOFTCALLS	50

struct softcall {
	func_t	sc_func;		/* function to call */
	caddr_t	sc_arg;			/* arg to pass to func */
	struct softcall *sc_next;	/* next in list */
} softcalls[NSOFTCALLS];

struct softcall *softhead, *softtail, *softfree;

/*
 * XXX - FOR SOME REASON, LEVEL ONE SOFT-INTS ARE BEING LOST.
 * THIS IS NOT ACCEPTABLE. HOWEVER, TURNING ON THIS FLAG
 * CAUSES THEM TO BE REISSUEED. ARE ANY OTHER LEVEL OF
 * SOFT INTERRUPTS BEING LOST?
 */
int	limes_paranoia = 1;

/*
 * Call function func with argument arg
 * at some later time at software interrupt priority
 */
softcall(func, arg)
	register func_t func;
	register caddr_t arg;
{
	register struct softcall *sc;
	static int first = 1;
	int s;

	s = spl_softint();
	if (first) {
		for (sc = softcalls; sc < &softcalls[NSOFTCALLS]; sc++) {
			sc->sc_next = softfree;
			softfree = sc;
		}
		first = 0;
	}
	/* coalesce identical softcalls */
	for (sc = softhead; sc != 0; sc = sc->sc_next)
		if (sc->sc_func == func && sc->sc_arg == arg)
			goto out;
	if ((sc = softfree) == 0)
		panic("too many softcalls");
	softfree = sc->sc_next;
	sc->sc_func = func;
	sc->sc_arg = arg;
	sc->sc_next = 0;
	if (softhead) {
		softtail->sc_next = sc;
		softtail = sc;
	} else {
		softhead = softtail = sc;
		siron();
	}
out:
	if (limes_paranoia)
		siron();
	(void) splx(s);
}

/*
 * Called to process software interrupts
 * take one off queue, call it, repeat
 * Note queue may change during call
 */
softint()
{
	register func_t func;
	register caddr_t arg;
	register struct softcall *sc;
	int s;

	if (softhead == 0)
		return (0);
	s = spl_softint();
	while (sc = softhead) {
		softhead = sc->sc_next;
		func = sc->sc_func;
		arg = sc->sc_arg;
		sc->sc_next = softfree;
		softfree = sc;
		(void)splx(s);
		(*func)(arg);
		s = spl_softint();
	}
	(void) splx(s);
	return (1);
}
