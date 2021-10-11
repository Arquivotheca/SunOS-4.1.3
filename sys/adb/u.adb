#include <sys/param.h>
#include <sys/user.h>

user
.{u_pcb}$<<pcb{OFFSETOK}
+/"procp"16t"ar0"16t"comm"n{u_procp,X}{u_ar0,X}{u_comm,10C}
+/"arg0"16t"arg1"16t"arg2"n{u_arg[0],X}{u_arg[1],X}{u_arg[2],X}
+/"uap"16t"qsave"16t" "16t"error"n{u_ap,X}{u_qsave,2X}{u_error,b}
+/"rv1"16t"rv2"16t"eosys"n{u_r.r_val1,X}{u_r.r_val2,X}{u_eosys,b}
+/"signal"n{u_signal,32X}"sigmask"n{u_sigmask,32X}
+/"onstack"16t"sigintr"16t"segreset"n{u_sigonstack,X}{u_sigintr,X}{u_sigreset,X}
+/"oldmask"16t"code"16t"sigstack"n{u_oldmask,X}{u_code,X}{u_sigstack,2X}
+/"ofile"16t"pofile"n{u_ofile,X}{u_pofile,X}
+/"ofile arr"n{u_ofile_arr,64X}"pofile arr"n{u_pofile_arr,64b}
+/"lastfile"n{u_lastfile,D}
+/"cdir"16t"rdir"16t"cmask"n{u_cdir,X}{u_rdir,X}{u_cmask,x}n"ru & cru"n{u_ru}
+,2$<<rusage{OFFSETOK}
+/"real itimer"n{u_timer[0],4D}
+/"virtual itimer"n{u_timer[1],4D}
+/"prof itimer"n{u_timer[2],4D}
