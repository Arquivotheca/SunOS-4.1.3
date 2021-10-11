#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <specfs/snode.h>

snode
./"next"n{s_next,X}
+$<<vnode{OFFSETOK}
+/"realvp"16t"devvp"16t"flag"8t"maj"8t"min"n{s_realvp,X}{s_bdevvp,X}{s_flag,x}{s_dev,2b}
+/"nextr"16t"size"n{s_nextr,D}{s_size,D}
+/"atime "{s_atime,Y}n"mtime "{s_mtime,Y}n"ctime "{s_ctime,Y}
+/"count"16t"owner"16t"lckcount"n{s_count,X}{s_owner,X}{s_lckcnt,X}{END}
+,<9-1$<snode
