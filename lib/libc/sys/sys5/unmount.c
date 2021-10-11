#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)unmount.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

unmount(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_unmount, s);
}
