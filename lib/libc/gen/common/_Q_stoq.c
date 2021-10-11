#ifndef lint
static	char sccsid[] = "@(#)_Q_stoq.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	stoq

QUAD
_Q_/**/FUNC/**/(x)
	SINGLE x;
{
	unpacked	px;
	QUAD		q;
	_fp_current_exceptions = 0;
	_fp_unpack(&px,&x,fp_op_single);
	_fp_pack(&px,&q,fp_op_extended);
	_Q_set_exception(_fp_current_exceptions);
	return q;
}
