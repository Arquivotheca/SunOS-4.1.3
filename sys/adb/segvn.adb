#include <sys/param.h>
#include <vm/seg_vn.h>

segvn_data
./"pgprot"8t"prot"8t"maxprot"8t"type"n{pageprot,b}{prot,b}{maxprot,b}{type,b}
+/"vnode"16t"offset"16t"anon_index"16t"anon_map"n{vp,X}{offset,X}{anon_index,X}{amp,X}
+/"vpage"16t"cred"16t"swresv"n{vpage,X}{cred,X}{swresv,X}
