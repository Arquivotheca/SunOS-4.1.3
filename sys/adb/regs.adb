#include <machine/reg.h>

regs
#ifdef sparc
./"psr"16t"pc"16t"npc"n{r_psr,X}{r_pc,X}{r_npc,X}
+/"y"16t"g1"16t"g2"16t"g3"n{r_y,X}{r_g1,X}{r_g2,X}{r_g3,X}
+/"g4"16t"g5"16t"g6"16t"g7"n{r_g4,X}{r_g5,X}{r_g6,X}{r_g7,X}
+/"o0"16t"o1"16t"o2"16t"o3"n{r_o0,X}{r_o1,X}{r_o2,X}{r_o3,X}
+/"o4"16t"o5"16t"o6"16t"o7"n{r_o4,X}{r_o5,X}{r_o6,X}{r_o7,X}
#endif
