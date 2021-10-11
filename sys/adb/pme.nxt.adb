#include <sys/param.h>
#include <machine/vm_hat.h>

pment
#if !defined(sun386) && !defined(sun3x) && !defined(sun4m)
#if defined(sun4) || defined(sun4c)
./{pme_page,X}X{OFFSETOK}{END}
#else
./{pme_page,X}{pme_next,x}x{OFFSETOK}{END}
#endif
+,<9-1$<pme.nxt
#endif
