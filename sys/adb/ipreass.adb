#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>

ipq
./"next"8t"ttl"8t"proto"8t"seq-id"8t"frag"8t"src"8t"dst"
{*next,.},(({*next,.})-<h)$<ipreass.nxt
