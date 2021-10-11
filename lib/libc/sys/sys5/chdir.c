#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)chdir.c 1.1 92/07/30 SMI";
#endif

# include "../common/chkpath.h"

chdir(s)
    char           *s;
{
    CHK(s);
    return syscall(SYS_chdir, s);
}
