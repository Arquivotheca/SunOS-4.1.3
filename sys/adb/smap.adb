#include <sys/param.h>
#include <vm/seg_map.h>

smap
./"vp"16t"off"16t"refcnt"n{sm_vp,X}{sm_off,X}{sm_refcnt,X}
+/"hash"16t"next"16t"prev"n{sm_hash,X}{sm_next,X}{sm_prev,X}{END}
+,<9-1$<smap
