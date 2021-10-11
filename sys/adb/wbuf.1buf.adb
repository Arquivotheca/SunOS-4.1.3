#include <sys/param.h>
#include <sys/user.h>

user
#ifdef sparc
<u+{EXPR,&((struct user *)0)->u_pcb.pcb_spbuf[0]}+((<c-<o-1)*{EXPR, sizeof ((struct user *)0)->u_pcb.pcb_spbuf[0]})/"sp="8tX
<u+{EXPR,&((struct user *)0)->u_pcb.pcb_wbuf[0]}+((<c-<o-1)*{EXPR, sizeof ((struct user *)0)->u_pcb.pcb_wbuf[0]})/16Xn
<o+1>o
,(<c-<o)$<wbuf.1buf
#endif /*sparc*/
