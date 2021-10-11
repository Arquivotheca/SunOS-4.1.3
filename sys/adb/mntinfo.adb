#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <nfs/nfs.h>
#include <netinet/in.h>
#include <nfs/nfs_clnt.h>

mntinfo
+/"mi_rootvp"n{mi_rootvp,X}
+/"mi_hard mi_printed mi_int mi_down mi_noac = "x
+/"mi_refct"8t"mi_tsize"8t"mi_stsize"8t"mi_bsize"n{mi_refct,D}{mi_tsize,D}{mi_stsize,D}{mi_bsize,D}
+/"mi_mntno"8t"mi_timeo"8t"mi_retrans"n{mi_mntno,D}{mi_timeo,D}{mi_retrans,D}
+/"mi_hostname"{mi_hostname,32C}
+/"mi_netname"8t"mi_netnamelen"8t"mi_authflavor"n{mi_netname,X}{mi_netnamelen,X}{mi_authflavor,D}
+/"mi_acregmin"8t"mi_acregmax"n{mi_acregmin,D}{mi_acregmax,D}
+/"mi_acdirmin"8t"mi_acdirmax"n{mi_acdirmin,D}{mi_acdirmax,D}
