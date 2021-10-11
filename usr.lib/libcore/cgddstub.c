/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)cgddstub.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <sun/fbio.h>
#include "langnames.h"

GENDD(_core_cgdd,cgdd,-1,0)
GENDD(_core_cgdd,cg1dd,FBTYPE_SUN1COLOR,"/dev/cgone0")
GENDD(_core_cgdd,cg2dd,FBTYPE_SUN2COLOR,"/dev/cgtwo0")
GENDD(_core_cgdd,cg3dd,FBTYPE_SUN3COLOR,"/dev/cgthree0")
GENDD(_core_cgdd,cg4dd,FBTYPE_SUN4COLOR,"/dev/cgfour0")
GENDD(_core_cgdd,cg6dd,FBTYPE_SUNFAST_COLOR,"/dev/cgsix0")
