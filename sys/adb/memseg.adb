#include <sys/types.h>
#include <vm/page.h>

memseg
./"page"16t"epage"n{pages,X}{epages,X}
+/"pfn"16t"epfn"n{pages_base,X}{pages_end,X}
+/"next"n{next,X}
*.,<9-1$<memseg
