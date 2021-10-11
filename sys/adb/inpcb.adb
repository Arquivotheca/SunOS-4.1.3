#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>

inpcb
.>5
*.>6
<5/"next"16t"prev"16t"head"n{inp_next,X}{inp_prev,X}{inp_head,X}n"faddr"16t"fport"8t"laddr"16t"lport"n{inp_faddr,X}{inp_fport,x}{inp_laddr,X}{inp_lport,x}n"socket"16t"ppcb"n{inp_socket,X}{inp_ppcb,X}
<6,<9-1$<inpcb
