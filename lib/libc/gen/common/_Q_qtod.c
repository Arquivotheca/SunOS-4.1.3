#ifndef lint
static	char sccsid[] = "@(#)_Q_qtod.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	qtod

double 
_Q_/**/FUNC/**/(x)
	QUAD x;
{
	unpacked	px;
	double		d;
	_fp_current_exceptions = 0;
	_Q_get_rp_rd();
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_pack(&px,&d,fp_op_double);
	_Q_set_exception(_fp_current_exceptions);
	return d;
}
