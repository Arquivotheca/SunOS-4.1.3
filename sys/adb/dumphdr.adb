#include <sys/time.h>
#include <sys/dumphdr.h>

dumphdr
./"magic"16t"version"16t"flags"16t"pagesize"n{dump_magic,X}{dump_version,X}{dump_flags,X}{dump_pagesize,D}
+/"chunksize"16t"mapsize"16t"nchunks"16t"dumpsize"n{dump_chunksize,D}{dump_bitmapsize,D}{dump_nchunks,D}{dump_dumpsize,D}
+/"crashtime"n{dump_crashtime.tv_sec,Y}"."{dump_crashtime.tv_usec,D}
+/"versionoff"16t"panicoff"16t"headersize"n{dump_versionoff,X}{dump_panicstringoff,X}{dump_headersize,X}
+/"version"n{END}s
+/"panicstr"ns
