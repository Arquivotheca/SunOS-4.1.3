#include	<sys/types.h>
#include	<lwp/common.h>
#include	<lwp/queue.h>
#include	<lwp/mch.h>
#include	<lwp/lwperror.h>
#include	<lwp/process.h>

lwp_t
./"next"16t"type"n{lwp_next,X}{lwp_type,X}
+$<<lwpmach{OFFSETOK}
+/"cntx hd."16t"cntx tl."n{lwp_ctx.hq_head,X}{lwp_ctx.hq_tail,X}
+/"full"16t"susp"16t"whichQ"16t"errno"n{lwp_full,X}{lwp_suspended,X}{lwp_run,D}{lwp_lwperrno,D}
+/"unreceived mesg."n
+/"mesg_head"16t"mesg_tail"n{lwp_messages.hq_head,X}{lwp_messages.hq_tail,X}
+/"blocked on"8t"mon. held"8t"mon_level"n{lwp_blockedon,X}{lwp_monlevel,X}
+/"mesg sent"n
+$<<msg{OFFSETOK}
+/"exc. hd."16t"exc. tl."n{lwp_exchandles.hq_head,X}{lwp_exchandles.hq_tail,X}
+/"joinq hd."16t"join tl."n{lwp_joinq.hq_head,X}{lwp_joinq.hq_tail,X}
+/"priority"16t"flags"16t"stack"16t"lock"n{lwp_prio,X}{lwp_flags,X}{lwp_stack,X}{lwp_lock,X}
+/"cancel"n{lwp_cancel,X}
