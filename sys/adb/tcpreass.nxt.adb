#include <sys/types.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcpip.h>

tcpiphdr
./{ti_next,X}{ti_len,d}{ti_seq,X}
{*ti_next,.},(({*ti_next,.})-<h)$<tcpreass.nxt
