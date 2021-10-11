#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcpip.h>

tcpiphdr
.>h
./"next"16t"len"16t"seq"
{*ti_next,.},(({*ti_next,.})-<h)$<tcpreass.nxt
