#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)truncate.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

truncate(p, l)
    char           *p;
    off_t           l;
{
    CHK(p);
    return syscall(SYS_truncate, p, l);
}
