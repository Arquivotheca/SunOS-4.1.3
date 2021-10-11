#include <sys/param.h>
#include <machine/vm_hat.h>

pmgrp
#if !defined (sun3x) && !defined (sun4m)
./"num"8t"keepcnt"nx{OFFSETOK}{pmg_keepcnt,x}
+/"as"16t"base"n{pmg_as,X}{pmg_base,X}
+/"next"16t"prev"16t"pme"n{pmg_next,X}{pmg_prev,X}{pmg_pme,X}{END}
+,<9-1$<pmgrp
#endif
