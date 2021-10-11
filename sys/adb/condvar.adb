#include <sys/types.h>
#include <sys/param.h>
#include <lwp/lwp.h>
#include <lwp/common.h>
#include <lwp/queue.h>
#include <lwp/condvar.h>

condvar_t
./"forw"16t"back"n{cv_waiting.hq_head,X}{cv_waiting.hq_tail,X}
+/"mon"16t"lock"16t"cm"n{cv_mon,X}{cv_lock,D}{cv_cm,X}{END}
