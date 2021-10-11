#ifndef lint
static	char sccsid[] = "@(#)_Q_cmp.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

enum fcc_type
_Q_cmp(x,y)
	QUAD x,y;
{
	unpacked	px,py,pz;
	enum fcc_type	fcc;
	_fp_current_exceptions = 0;
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_unpack(&py,&y,fp_op_extended);
	fcc = _fp_compare(&px,&py,0);	/* quiet NaN unexceptional */
	_Q_set_exception(_fp_current_exceptions);
	return fcc;
}
