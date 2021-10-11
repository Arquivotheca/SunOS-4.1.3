#include <sys/types.h>
#include <machine/vm_hat.h>
#include <machine/mmu.h>
#include <sys/param.h>

pmgrp
#if !defined(sun386) && !defined(sun3x) && !defined (sun4m)
.>p
((.-*pments)%{EXPR,NPMENTPERPMGRP * sizeof(struct pment)}*{SIZEOF})+(*pmgrps)>g
(<p-{*pmg_pme,<g})%{EXPR, sizeof (struct pment)}*{EXPR,MMU_PAGESIZE}+{*pmg_base,<g}=X
<p+{EXPR, sizeof (struct pment)},<9-1$<pmetov
#endif
