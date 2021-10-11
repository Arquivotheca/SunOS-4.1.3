/* @(#)reboot.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04 */

#include "SYS.h"

SYSCALL(reboot)
#if vax
	halt
#endif
#if sun
	stop	#0
#endif
