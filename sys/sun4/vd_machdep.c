/*
 * @(#)vd_machdep.c 1.1 92/07/30 SMI
 *
 * Machine-dependent functions for loadable modules
 *
 */

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/vm.h>
#include <machine/psl.h>
#include <machine/scb.h>
#include <sundev/mbvar.h>

extern int spurious();

/*
 * Check if an interrupt vector is already in use.
 *
 * Returns 0 if free, 1 if in use.
 */

/*ARGSUSED*/
int
vd_checkirq(vec, pri, space)
	struct vec *vec;
	int pri;
	u_int space;
{
	int irq;

	if (pri == 0)
		return 0;	/* no interrupt */

	if (pri < 0 || pri > 7) {
		printf("invalid interrupt priority %d\n", pri);
		return 1;
	}

	irq = vec->v_vec;
	if (irq < VEC_MIN || irq > VEC_MAX) {
		printf("invalid interrupt vector %x\n", irq);
		return 1;
	}

	if (vme_vector[irq - VEC_MIN].func != spurious) {
		printf("interrupt vector %x is already in use\n", irq);
		return 1;
	}
	return 0;
}

/*
 * Reset an interrupt vector to initial value, i.e., spurious.
 */

/*ARGSUSED*/
void
vd_resetirq(vec, pri, space)
	struct vec *vec;
	int pri;
	u_int space;
{
	int irq;

	if (pri == 0)
		return;	/* no interrupt */

	irq = vec->v_vec;
	if (irq < VEC_MIN || irq > VEC_MAX) {
		printf("vd_resetirq: bad vector %x\n", irq);
		return;
	}
	vme_vector[irq - VEC_MIN].func = spurious;
	vme_vector[irq - VEC_MIN].arg = NULL;
}
