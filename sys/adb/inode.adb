#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>

inode
./"forw"16t"back"n{i_chain[0],X}{i_chain[1],X}
+$<<vnode{OFFSETOK}
+/"devvp"16t"flag"8t"maj"8t"min"8t"ino"n{i_devvp,X}{i_flag,x}{i_dev,2b}{i_number,D}
+/"fs"16t"dquot"16t"owner"16t"count"n{i_fs,X}{i_dquot,X}{i_owner,X}{i_count,X}
+/"nextr"16t"freef"16t"freeb"n{i_un.if_nextr,D}{i_fr.if_freef,X}{i_fr.if_freeb,X}
+$<<dino
.,<9-1$<inode
