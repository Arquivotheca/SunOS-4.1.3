#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)open.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

open(f, m, stuff)
    char           *f;
{
    CHK(f);
    return syscall(SYS_open, f, m, stuff);
}
