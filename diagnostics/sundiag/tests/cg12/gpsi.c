
#ifndef lint
static  char sccsid[] = "@(#)gpsi.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <pixrect/pixrect_hs.h>


/**********************************************************************/
void
gpsi()
/**********************************************************************/
/* executes the GPSI-based test modules. There are two of them:
   the one with the built-in C30 code and the one with the
   standard interface.
*/

{
   void gpsi_diag();
   void gpsi_std();

   if (!open_gp())
       return;

    /*
    gpsi_diag();
    */

    gpsi_std();
}
