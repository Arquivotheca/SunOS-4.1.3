#include <sys/param.h>
#include <vm/page.h>

page
./"flags"8t"keepcnt"nx{OFFSETOK}{p_keepcnt,x}
+/"vnode"16t"offset"16t"hash"n{p_vnode,X}{p_offset,X}{p_hash,X}
+/"next"16t"prev"16t"vpnext"16t"vpprev"n{p_next,X}{p_prev,X}{p_vpnext,X}{p_vpprev,X}
+/"pme"16t"lckcnt"n{p_mapping,X}{p_lckcnt,x}{END}
