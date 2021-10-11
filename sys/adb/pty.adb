#include <sys/param.h>
#include <sys/stream.h>
#include <sys/ttycom.h>
#include <sys/tty.h>
#include <sys/ptyvar.h>

pty
.>1
./n"flags"16t"stuff1st"8t"stufflst"8t"stufflen"n{pt_flags,X}{pt_stuffqfirst,X}{pt_stuffqlast,X}{pt_stuffqlen,D}
+$<<tty{OFFSETOK}
+/n"wbufcid"16t"selr"16t"selw"16t"sele"n{pt_wbufcid,X}{pt_selr,X}{pt_selw,X}{pt_sele,X}"stream"16t"pgrp"8t"send"8t"ucntl"n{pt_stream,X}{pt_pgrp,d}{pt_send,b}{pt_ucntl,b}
+,<9-1$<pty
