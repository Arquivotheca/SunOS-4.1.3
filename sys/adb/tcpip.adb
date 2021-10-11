#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcpip.h>

tcpiphdr
.>1
<1/"next"16t"prev"16t"pr"8t"len"n{ti_next,X}{ti_prev,X}{ti_pr,b}{ti_len,x}n"src"16t"dst"16t"sport"8t"dport"n{ti_src,X}{ti_dst,X}{ti_sport,x}{ti_dport,x}n"seq"16t"ack"16t"fl|off"8t"win"8t"sum"8t"urp"n{ti_seq,X}{ti_ack,X}x{ti_win,x}{ti_sum,x}{ti_urp,x}
*<1,*<1$<tcpip
