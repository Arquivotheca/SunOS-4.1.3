#include <sys/param.h>
#include <vm/hat.h>
#include <vm/as.h>

as
#if defined (sun3x) || defined (sun4m)
./"flags"8t"keepcnt"16t"segs"16t"seglast"nx{OFFSETOK}{a_keepcnt,x}{a_segs,X}{a_seglast,X}
+/"rss"16t"valid/l1pfn"16t"l2pts"8t"l3pts"n{a_rss,X}{a_hat,X}{a_hat.hat_l2pts,x}{a_hat.hat_l3pts,x}
#else
./"flags"8t"keepcnt"16t"segs"16t"seglast"nx{OFFSETOK}{a_keepcnt,x}{a_segs,X}{a_seglast,X}
+/"rss"16t"ctx"16t"pmgrp"n{a_rss,X}{a_hat.hat_ctx,X}{a_hat.hat_pmgrps,X}
#endif
