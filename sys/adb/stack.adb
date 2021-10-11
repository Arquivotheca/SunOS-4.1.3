#include <machine/frame.h>

frame
.>f
#ifdef sparc
<f/16Xn
<f/16pn
#endif
{*fr_savpc,<f}/in
{*fr_savfp,<f},<9-1$<stack
