#if !defined(lint) && !defined(BOOTPROM)
static char sccsid[] = "@(#)intreg.c 1.1 92/07/30 SMI";
#endif

#include <sys/types.h>
#include <machine/param.h>
#include <machine/intreg.h>
#include <machine/pte.h>
#include <machine/mmu.h>

l14enable()
{
	int pmeg;
	long pte;

	mapin(INTREG_ADDR, OBIO_INTREG_ADDR, &pte, &pmeg);

	*INTREG |= IR_ENA_CLK14;

	setpgmap((caddr_t)INTREG_ADDR, pte);
	setsegmap(INTREG_ADDR, pmeg);
}
