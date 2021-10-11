#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>

ipq
./{next,X}{ipq_ttl,b}{ipq_p,b}{ipq_id,x}{ipq_next,X}{ipq_src,X}{ipq_dst,X}
{*next,.},(({*next,.})-<h)$<ipreass.nxt
