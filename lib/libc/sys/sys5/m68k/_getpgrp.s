/*	@(#)_getpgrp.s 1.1 92/07/30 SMI; from UCB 4.1 82/12/04	*/

#include "SYS.h"

BSDSYSCALL(getpgrp)
	RET		/* pgrp = _getpgrp(pid); */
