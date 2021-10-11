#include <sys/types.h>
#include <sys/stream.h>

queue
./"qinfo"16t"first"16t"last"16t"next"n{q_qinfo,X}{q_first,X}{q_last,X}{q_next,X}n"link"16t"ptr"16t"count"8t"flag"n{q_link,X}{q_ptr,X}{q_count,u}{q_flag,x}n"minpsz"8t"maxpsz"8t"hiwat"8t"lowat"n{q_minpsz,d}{q_maxpsz,d}{q_hiwat,u}{q_lowat,u}
