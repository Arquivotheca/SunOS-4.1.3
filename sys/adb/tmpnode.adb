#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <tmpfs/tmpnode.h>

tmpnode
./"flags"16t"count"16tn{tn_flags,X}{tn_count,X}
+/"back"16t"forw"16t"owner"16t"dir"16tn{tn_back,X}{tn_forw,X}{tn_owner,X}{tn_dir,X}
+$<<vnode{OFFSETOK}
+/"amp"16tn{tn_amapp,X}
