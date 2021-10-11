#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)quotactl.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

quotactl(c, d, u, a)
    char           *d, *a;
{
    CHK(d);
    return syscall(SYS_quotactl, c, d, u, a);
}
