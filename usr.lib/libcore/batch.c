/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)batch.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/*
 * Denote the beginning of a batch of changes to the picture.
 */
begin_batch_of_updates()
{
    char *funcname;
    int errnum;

    funcname = "begin_batch_of_updates";

    if (_core_batchupd) {
	errnum = 17;
	_core_errhand(funcname, errnum);
	return (1);
    }
    _core_batchupd = TRUE;

    return (0);
}

/*
 * Denote the end of a batch of changes to the picture.
 */
end_batch_of_updates()
{
    char *funcname;
    int errnum;

    funcname = "end_batch_of_updates";

    if (!_core_batchupd) {
	errnum = 18;
	_core_errhand(funcname, errnum);
	return (1);
    }
    _core_repaint(FALSE); /* newframe() all appropriate surfaces */


 /* Eventually should add a drain buffer command to DD when no batch. */

    _core_batchupd = FALSE;
    return (0);
}
