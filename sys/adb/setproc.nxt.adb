#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>

proc
,#(({*p_pid,<f})-<4)$<setproc.done
<f+{SIZEOF}>f
,#(<f-<e)$<setproc.nop
$<setproc.nxt
