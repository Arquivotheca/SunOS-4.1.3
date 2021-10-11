#include	<sys/types.h>
#include	<lwp/common.h>
#include	<lwp/queue.h>
#include	<lwp/mch.h>
#include	<lwp/lwperror.h>
#include	<lwp/process.h>

machstate_t
#ifdef mc68000
./"D0"16t"D1"16t"D2"16t"D3"n{mc_temps[0],X}{mc_temps[1],X}{mc_generals[0],X}{mc_generals[1],X}
+/"D4"16t"D5"16t"D6"16t"D7"n{mc_generals[2],X}{mc_generals[3],X}{mc_generals[4],X}{mc_generals[5],X}
+/"A0"16t"A1"16t"A2"16t"A3"n{mc_temps[2],X}{mc_temps[3],X}{mc_generals[6],X}{mc_generals[7],X}
+/"A4"16t"A5"16t"A6"16t"A7"n{mc_generals[8],X}{mc_generals[9],X}{mc_generals[10],X}{mc_generals[11],X}
+/"PC"16t"PS"n{mc_temps[4],X}{mc_temps[5],X}{END}
#endif 
#ifdef sparc
./"g0"16t"g1"16t"g2"16t"g3"n{sparc_globals.sparc_gsregs[0],X}{sparc_globals.sparc_gsregs[1],X}{sparc_globals.sparc_gsregs[2],X}{sparc_globals.sparc_gsregs[3],X}
+/"g4"16t"g5"16t"g6"16t"g7"n{sparc_globals.sparc_gsregs[4],X}{sparc_globals.sparc_gsregs[5],X}{sparc_globals.sparc_gsregs[6],X}{sparc_globals.sparc_gsregs[7],X}
+/"o0"16t"o1"16t"o2"16t"o3"n{sparc_oregs.sparc_osregs[0],X}{sparc_oregs.sparc_osregs[1],X}{sparc_oregs.sparc_osregs[2],X}{sparc_oregs.sparc_osregs[3],X}
+/"o4"16t"o5"16t"o6"16t"o7"n{sparc_oregs.sparc_osregs[4],X}{sparc_oregs.sparc_osregs[5],X}{sparc_oregs.sparc_osregs[6],X}{sparc_oregs.sparc_osregs[7],X}
+/"pc"16t"npc"16t"psr"16t"y"n{sparc_pc,X}{sparc_npc,X}{sparc_psr,X}{sparc_y,X}
#endif sparc
