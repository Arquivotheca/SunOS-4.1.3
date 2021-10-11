#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>

proc
{*p_pid,<f}="pid "D
,#(({*p_pid,<f})-<4)$<
<l-1>l
<f+{SIZEOF}>f
,#<l$<
$<findproc.nxt
