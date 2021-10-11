#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chmod.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

chmod(s, m)
    char           *s;
{
    CHK(s);
    return syscall(SYS_chmod, s, m);
}
