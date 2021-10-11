#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>

ucred
./"ref"8t"uid"8t"gid"n{cr_ref,d}{cr_uid,d}{cr_gid,d}
+/"groups"n{cr_groups,8D}
+/"ruid"8t"rgid"8tn{cr_ruid,d}{cr_rgid,d}
