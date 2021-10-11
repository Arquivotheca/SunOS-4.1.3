#include <sys/param.h>
#include <sys/user.h>

fpu
#ifdef sparc
./"floats"n{fpu_regs,32f}
+/"doubles"n{fpu_dregs,16F}
+/"fsr"n{fpu_fsr,X}
+/"flags"16t"extra"16t"qcnt"n{fpu_flags,X}{fpu_extra,X}{fpu_qcnt,D}
+/"fpq"n{fpu_q}
.,{EXPR,FQ_DEPTH}/Xi{OFFSETOK}{END}
#endif sparc
