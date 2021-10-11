#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)access.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

access(p, m)
    char           *p;
{
    CHK(p);
    return syscall(SYS_access, p, m);
}
