#include <machine/pte.h>
#include <sys/param.h>

pte
(.-Sysbase)%{EXPR, MMU_PAGESIZE}*{SIZEOF}+Sysmap,<9/X
