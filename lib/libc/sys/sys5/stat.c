#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)stat.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

stat(s, b)
    char           *s;
    struct stat    *b;
{
    CHK(s);
    return syscall(SYS_stat, s, b);
}
