#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)mkdir.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

mkdir(p, m)
    char           *p;
{
    CHK(p);
    return syscall(SYS_mkdir, p, m);
}
