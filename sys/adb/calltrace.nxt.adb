#include <machine/frame.h>

frame
.>f
#ifdef sparc
<f/16Xn
<f/16pn
#endif
{*fr_savpc,<f}/inn
{*fr_savfp,<f},.$<calltrace.nxt
