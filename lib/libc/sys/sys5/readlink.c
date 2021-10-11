#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)readlink.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

readlink(p, b, s)
    char           *p, *b;
{
    CHK(p);
    return syscall(SYS_readlink, p, b, s);
}
