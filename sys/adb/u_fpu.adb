#include <sys/param.h>
#include <sys/user.h>

user
#ifdef sparc
{*u_pcb.pcb_fpctxp,.}$<<fpu{OFFSETOK}
#endif sparc
