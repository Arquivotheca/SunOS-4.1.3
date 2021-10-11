#include <sys/param.h>
#include <sys/user.h>

user
.>u
#ifdef sparc
<u/{u_pcb.pcb_wbcnt}
+/"wbcnt "X
{*u_pcb.pcb_wbcnt,<u}>c
0>o
,(<c-<o)$<wbuf.1buf
#endif /*sparc*/
