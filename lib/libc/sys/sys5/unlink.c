#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)unlink.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

unlink(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_unlink, s);
}
