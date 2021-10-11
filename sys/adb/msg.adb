#include	<sys/types.h>
#include	<lwp/common.h>
#include	<lwp/queue.h>
#include	<lwp/mch.h>
#include	<lwp/lwperror.h>
#include	<lwp/message.h>
#include	<lwp/process.h>

msg_t
./"msg_next"16t"msg_sender"n{msg_next,X}{msg_sender,X}
+$<<thread{OFFSETOK}
+/"arg size"16t"arg buf"16t"res size"16t"res buf"n{msg_argsize,D}{msg_argbuf,X}{msg_ressize,D}{msg_resbuf,X}{END}
