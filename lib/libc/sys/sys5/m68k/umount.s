/* @(#)umount.s 1.1 92/07/30 SMI */

#include "SYS.h"

#define SYS_umount	22
SYSCALL(umount)
	RET
