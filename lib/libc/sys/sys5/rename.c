#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)rename.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

rename(f, t)
    char           *f, *t;
{
    CHK(f);
    CHK(t);
    return syscall(SYS_rename, f, t);
}
