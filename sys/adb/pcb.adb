#include <sys/param.h>
#include <sys/dir.h>
#ifdef sun386
#include <machine/tss.h>
#endif
#include <sys/user.h>

pcb
#ifdef mc68000
./"pc"n{pcb_regs.val[0],X}
+/"d2"16t"d3"16t"d4"16t"d5"n{pcb_regs.val[1],X}{pcb_regs.val[2],X}{pcb_regs.val[3],X}{pcb_regs.val[4],X}
+/"d6"16t"d7"n{pcb_regs.val[5],X}{pcb_regs.val[6],X}
+/"a2"16t"a3"16t"a4"16t"a5"n{pcb_regs.val[7],X}{pcb_regs.val[8],X}{pcb_regs.val[9],X}{pcb_regs.val[10],X}
+/"a6"16t"a7"n{pcb_regs.val[11],X}{pcb_regs.val[12],X}
+/"sr"n{pcb_sr,X}
+/"flags"n{pcb_flags,X}{END}
#endif mc68000
#ifdef sparc
./
+/"pc"16t"sp"16t"psr"16t"uwm"n{pcb_pc,X}{pcb_sp,X}{pcb_psr,X}{pcb_uwm,X}
+/
+/"flags"n{pcb_flags,X}{END}
#endif sparc
#ifdef sun386
./"p0br"16t"p0lr"16t"p1br"16t"p1lr"n{pcb_p0br,X}{pcb_p0lr,X}{pcb_p1br,X}{pcb_p1lr,X}
+/"szpt"16t"sswap"n{pcb_szpt,X}{pcb_sswap,X}{END}
#endif sun386
