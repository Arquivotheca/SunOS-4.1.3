#ifndef lint
static	char sccsid[] = "@(#)_Q_qtou.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_get_rp_rd(), _Q_set_exception();

#define	FUNC	qtou

unsigned
_Q_/**/FUNC/**/(x)
	QUAD x;
{
	unpacked	px;
	QUAD		c;
	unsigned	u,*pc = (unsigned*)&c,r;
	pc[0] = 0x401e0000; pc[1]=pc[2]=pc[3]=0;	/* c = 2^31 */
	r = 0;
	u = *(int*)&x;	/* high part of x */
	if(u>=0x401e0000&&u<0x401f0000) {
		r = 0x80000000;
		x = _Q_sub(x,c);
	}

	_fp_current_exceptions = 0;
	fp_direction = fp_tozero;
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_pack(&px,&u,fp_op_integer);
	_Q_set_exception(_fp_current_exceptions);
	return (u|r);
}
