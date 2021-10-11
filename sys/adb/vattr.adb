#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>

vattr
./"va_type "{va_type,D}n
+/"va_mode"8t"va_uid"8t"va_gid"n{va_mode,x}{va_uid,d}{va_gid,d}
+/"va_fsid"16t"va_nodeid"16t"va_nlink"n{va_fsid,D}{va_nodeid,D}{va_nlink,d}
+/"va_size"16t"va_blocksize"16tn{va_size,D}{va_blocksize,D}
+/"va_atime"16t{va_atime.tv_sec,Y}n
+/"va_mtime"16t{va_mtime.tv_sec,Y}n
+/"va_ctime"16t{va_ctime.tv_sec,Y}n
+/"va_rdev"8t"va_blocks"n{va_rdev,x}{va_blocks,D}{END}
