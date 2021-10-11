#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <tmpfs/tmp.h>
#include <tmpfs/tmpnode.h>

tmount
./"next"16tn{tm_next,X}
+/"*vfsp"16tn{tm_vfsp,X}
+/"rootnode"16t"mntno"16tn{tm_rootnode,X}{tm_mntno,X}
+/"direntries"16t"directories"16tn{tm_direntries,X}{tm_directories,X}
+/"files"16t"kmemspace"16t"anonmem"16tn{tm_files,X}{tm_kmemspace,X}{tm_anonmem,X}
+/"mnt"n{tm_mntpath,16C}
