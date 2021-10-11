#ifndef lint
static	char sccsid[] = "@(#)_Q_div.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	div

QUAD 
_Q_/**/FUNC/**/(x,y)
	QUAD x,y;
{
	unpacked	px,py,pz;
	QUAD		z;
	_fp_current_exceptions = 0;
	_Q_get_rp_rd();
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_unpack(&py,&y,fp_op_extended);
	_fp_/**/FUNC/**/(&px,&py,&pz);
	_fp_pack(&pz,&z,fp_op_extended);
	_Q_set_exception(_fp_current_exceptions);
	return z;
}
