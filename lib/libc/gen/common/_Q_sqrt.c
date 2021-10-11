#ifndef lint
static	char sccsid[] = "@(#)_Q_sqrt.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	sqrt

QUAD 
_Q_/**/FUNC/**/(x)
	QUAD x;
{
	unpacked	px,pz;
	QUAD		z;
	_fp_current_exceptions = 0;
	_Q_get_rp_rd();
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_/**/FUNC/**/(&px,&pz);
	_fp_pack(&pz,&z,fp_op_extended);
	_Q_set_exception(_fp_current_exceptions);
	return z;
}
