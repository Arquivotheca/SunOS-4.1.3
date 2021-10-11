#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <rpc/types.h>
#include <nfs/nfs.h>
#include <nfs/rnode.h>

rnode
./"r_freef"16t"r_freeb"16t"r_hash"n{r_freef,X}{r_freeb,X}{r_hash,X}n
+/"r_vnode"
+$<<vnode{OFFSETOK}
+/"r_fh"n{r_fh,xxxxxxxxxxxxxxxx}
+/"r_flags"8t"r_error"8t"r_nextr/r_lastcookie"n{r_flags,x}{r_error,x}{r_nextr,X}
+/"r_owner"16t"r_count"n{r_owner,X}{r_count,D}
+/"r_cred"16t"r_unlcred"16t"r_unlname"16t"r_unldvp"n{r_cred,X}{r_unlcred,X}{r_unlname,X}{r_unldvp,X}
+/"r_size"16t{r_size,X}
+/"r_attr"
+$<<vattr{OFFSETOK}
+/"r_attrtime"16t{r_attrtime.tv_sec,Y}n{END}
