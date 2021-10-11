#include <sys/types.h>
#include <sys/time.h>
#include <rpc/types.h>
#include <sys/errno.h>
#include <nfs/nfs.h>

nfsfattr
./"na_type "{na_type,D}n
+/"na_mode"16t"na_nlink"16t"na_uid"16t"na_gid"n{na_mode,X}{na_nlink,D}{na_uid,D}{na_gid,D}
+/"na_size"16t"na_blocksize"16t"na_rdev"16t"na_blocks"n{na_size,D}{na_blocksize,D}{na_rdev,X}{na_blocks,D}
+/"na_fsid"16t"na_nodeid"n{na_fsid,D}{na_nodeid,D}n
+/"na_atime"16t{na_atime.tv_sec,Y}n
+/"na_mtime"16t{na_mtime.tv_sec,Y}n
+/"na_ctime"16t{na_ctime.tv_sec,Y}{END}
