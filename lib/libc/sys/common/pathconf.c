#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)pathconf.c 1.1 92/07/30 SMI";
#endif

#include "../common/chkpath.h"

pathconf(p, what)
    char* p;
{
    CHK(p);
    return syscall(SYS_pathconf, p, what);
}
