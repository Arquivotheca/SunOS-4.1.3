#include <sys/types.h>
#include <sys/mbuf.h>

mbuf
.>1
<1/{m_next,X}{m_off,X}{m_len,d}{m_type,d}{m_act,X}
*<1,*<1$<mbuf.nxt
