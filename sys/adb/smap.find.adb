#include <sys/types.h>
#include <vm/seg.h>
#include <vm/seg_map.h>
#include <sys/param.h>

seg
*({*s_data,*segkmap}+{EXPR,&((struct segmap_data *)0)->smd_sm})+((.-{*s_base,*segkmap})%{EXPR,MAXBSIZE}*{EXPR, sizeof(struct smap)})$<smap
