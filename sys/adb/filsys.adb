#include <sys/param.h>
#include <ufs/fs.h>

fs
./"link"16t"rlink"16t"time"16tn{fs_link,X}{fs_rlink,X}{fs_time,Y}
+/"size"16t"dsize"16t"ncg"n{fs_size,D}{fs_dsize,D}{fs_ncg,D}
+/"bsize"16t"fsize"16t"frag"n{fs_bsize,X}{fs_fsize,X}{fs_frag,X}
+/"mod"8t"clean"8t"ronly"8t"mnt"n{fs_fmod,b}{fs_clean,b}{fs_ronly,b}{fs_fsmnt,12C}
+/"rotor"8t{fs_cgrotor,D}
