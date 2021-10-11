/*      @(#)a.out.h 1.1 92/07/30 SMI; from UCB 4.1 83/05/03       */

#ifndef _a_out_h
#define _a_out_h

#if !defined(_CROSS_TARGET_ARCH)

   /* The usual, native case.  Usage:
    *      #include <a.out.h>
    */

#include <machine/a.out.h>

#else /*defined(_CROSS_TARGET_ARCH)*/

   /* Used when building a cross-tool, with the target system architecture
    * determined by the _CROSS_TARGET_ARCH preprocessor variable at compile
    * time.  Usage:
    *      #include <a.out.h>
    * ...plus compilation with command (e.g. for Sun-4 target architecture):
    *      cc  -DSUN2=2 -DSUN3=3 -DSUN3X=31 -DSUN4=4 \
    *		-D_CROSS_TARGET_ARCH=SUN4  ...
    * Note: this may go away in a future release.
    */
#  if   _CROSS_TARGET_ARCH == SUN2
#    include "sun2/a.out.h"
#  elif _CROSS_TARGET_ARCH == SUN3
#    include "sun3/a.out.h"
#  elif _CROSS_TARGET_ARCH == SUN3X
#    include "sun3x/a.out.h"
#  elif _CROSS_TARGET_ARCH == SUN4
#    include "sun4/a.out.h"
#  elif _CROSS_TARGET_ARCH == VAX
#    include "vax/a.out.h"
#  endif

#endif /*defined(_CROSS_TARGET_ARCH)*/

/*
 * Usage when building a cross-tool with a fixed target system architecture
 * (Sun-4 in this example), bypassing this file:
 *      #include <sun4/a.out.h>
 */

#endif /*!_a_out_h*/
