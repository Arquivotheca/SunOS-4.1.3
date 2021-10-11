#include <sys/param.h>
#include <machine/vm_hat.h>

ptbl
#if defined  (sun3x)
"keepcnt"16t"validcnt"n{ptbl_keepcnt,x}{ptbl_validcnt,X}
+/"asptr"16t"base"16t"next"8t"prev"8t"n{ptbl_as,X}{ptbl_base,X}{ptbl_next,x}{ptbl_prev,x}
#endif
#if defined (sun4m)
"lockcnt"n{ptbl_lockcnt,x}
+/"asptr"16t"base"16t"next"8t"prev"8t"n{ptbl_as,X}{ptbl_base,X}{ptbl_next,x}{ptbl_prev,x}
#endif
