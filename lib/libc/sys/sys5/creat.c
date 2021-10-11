#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)creat.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

creat(s, m)
    char           *s;
{
    CHK(s);
    return syscall(SYS_creat, s, m);
}
