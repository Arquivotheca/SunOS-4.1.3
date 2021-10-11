#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <net/raw_cb.h>

rawcb
./"next"16t"prev"16t"socket"n{rcb_next,X}{rcb_prev,X}{rcb_socket,X}n"family"8t"fport"8t"faddr"16t"family"8t"lport"8t"laddr"n{rcb_faddr.sa_family,x}{rcb_faddr.sa_data,xX}{rcb_laddr.sa_family,x}{rcb_laddr.sa_data,xX}"pcb"16t"flags"n{rcb_pcb,X}{rcb_flags,x}
+,<9-1$<rawcb
