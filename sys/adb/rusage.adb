#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

rusage
./"utime"16t" "16t"stime"n{ru_utime,2X}{ru_stime,2X}
+/"maxrss"16t"ixrss"16t"idrss"16t"isrss"n{ru_maxrss,D}{ru_ixrss,D}{ru_idrss,D}{ru_isrss,D}
+/"minflt"16t"majflt"16t"nswap"n{ru_minflt,D}{ru_majflt,D}{ru_nswap,D}
+/"inblock"16t"oublock"16t"msgsnd"16t"msgrcv"n{ru_inblock,D}{ru_oublock,D}{ru_msgsnd,D}{ru_msgrcv,D}
+/"nsignals"16t"nvcsw"16t"nivcsw"n{ru_nsignals,D}{ru_nvcsw,D}{ru_nivcsw,D}
+,<9-1$<rusage
