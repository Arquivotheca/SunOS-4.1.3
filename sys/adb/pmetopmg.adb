#include <sys/types.h>
#include <machine/vm_hat.h>
#include <machine/mmu.h>

pmgrp
#if !defined(sun3x) && !defined(sun386) && !defined (sun4m)
((.-*pments)%{EXPR,NPMENTPERPMGRP * sizeof(struct pment)}*{SIZEOF})+(*pmgrps)$<pmgrp
#endif
