#include <sys/types.h>
#include <machine/reg.h>
#define KERNEL
#include <sys/ptrace.h>

ipc
./"ip_lock"16t"ip_error"n{ip_lock,D}{ip_error,x}n
+/"ip_req"16t"ip_addr"16t"ip_addr2"16t"ip_data"n{ip_req,D}{ip_addr,X}{ip_addr2,X}{ip_data,X}n
+/"ip_nbytes"16t"ip_bigbuf"n{ip_nbytes,D}{ip_bigbuf,X}n
+/"ip_vp"n{ip_vp,X}{END}
