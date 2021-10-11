#include <sys/types.h>
#include <lwp/lwp.h>
#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/monitor.h>

monitor_t
./"head"16t"tail"n{mon_waiting.hq_head,X}{mon_waiting.hq_tail,X}
+/"owner"8t"previous_mon"8t"nest_level"8t"mon_lock"n{mon_owner,X}{mon_set,X} {mon_nestlevel,X}{mon_lock,X}{END}
