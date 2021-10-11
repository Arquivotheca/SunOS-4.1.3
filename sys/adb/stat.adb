#include <sys/types.h>
#include <sys/stat.h>

stat
./"dev"n
+/"maj"8t"min"8t"ino"n{st_dev,2b}{st_ino,D}
+/"mode"8t"link"8t"uid"8t"gid"n{st_mode,o}{st_nlink,d}{st_uid,d}{st_gid,d}
+/"rdev"n
+/"maj"8t"min"8t"size"n{st_rdev,2b}{st_size,D}
+/"atime "{st_atime,Y}n"mtime "{st_mtime,Y}n"ctime "{st_ctime,Y}
+/"blksize"n{st_blksize,D}
