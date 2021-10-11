#include	<sys/types.h>
#include	<lwp/lwp.h>
#include	<sys/asynch.h>

aiodone
./"next"16t"result"n{aiod_next,X}{aiod_result,X}
+/"thread_id"8t"thread_key"n{aiod_thread.thread_id,X}{aiod_thread.thread_key,X}
+/"state"16t"fd"n{aiod_state,D}{aiod_fd,D}{END}
