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
static char sccsid[] = "@(#)bwddstub.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#include <sun/fbio.h>
#include "langnames.h"

GENDD(_core_bwdd,bwdd,-1,0)
GENDD(_core_bwdd,bw1dd,FBTYPE_SUN1BW,"/dev/bwone0")
GENDD(_core_bwdd,bw2dd,FBTYPE_SUN2BW,"/dev/bwtwo0")
