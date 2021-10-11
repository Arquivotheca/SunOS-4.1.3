#include <machine/frame.h>

frame
.>f
#ifdef sparc
<f/16Xn
<f/16pnn
#endif
{*fr_savfp,<f},.$<stacktrace.nxt
