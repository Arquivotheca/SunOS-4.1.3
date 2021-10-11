#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>

proc
./"link"16t"rlink"16t"nxt"16t"prev"n{p_link,X}{p_rlink,X}{p_nxt,X}{p_prev,X}
+/"as"16t"segu"16t"stack"16t"uarea"n{p_as,X}{p_segu,X}{p_stack,X}{p_uarea,X}
+/"upri"8t"pri"8t"cpu"8t"stat"8t"time"8t"nice"n{p_usrpri,b}{p_pri,b}{p_cpu,b}{p_stat,b}{p_time,b}{p_nice,b}
+/"slp"8t"cursig"16t"sig"n{p_slptime,b}{p_cursig,b}{p_sig,X}
+/"mask"16t"ignore"16t"catch"n{p_sigmask,X}{p_sigignore,X}{p_sigcatch,X}
+/"flag"16t"uid"8t"suid"8t"pgrp"n{p_flag,X}{p_uid,d}{p_suid,d}{p_pgrp,d}
+/"pid"8t"ppid"8t"xstat"8t"ticks"n{p_pid,d}{p_ppid,d}{p_xstat,x}{p_cpticks,d}
+/"cred"16t"ru"16t"tsize"n{p_cred,X}{p_ru,X}{p_tsize,X}
+/"dsize"16t"ssize"16t"rssize"n{p_dsize,X}{p_ssize,X}{p_rssize,X}
+/"maxrss"16t"swrss"16t"wchan"n{p_maxrss,X}{p_swrss,X}{p_wchan,X}
+/"%cpu"16t"pptr"16t"tptr"n{p_pctcpu,X}{p_pptr,X}{p_tptr,X}
+/"real itimer"n{p_realtimer,4D}
+/"idhash"16t"swlocks"n{p_idhash,d}{p_swlocks,d}
+/"aio forw"16t"aio back"8t"aio count"8t"threadcnt"n{p_aio_forw,X}{p_aio_back,X}{p_aio_count,X}{p_threadcnt,X}
+,<9-1$<proc
