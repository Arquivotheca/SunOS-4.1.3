#ifndef lint
static	char sccsid[] = "@(#)_Q_utoq.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

#define	FUNC	utoq

QUAD
_Q_/**/FUNC/**/(x)
	unsigned x;
{
	unpacked	px;
	QUAD		q,c;
	int 		*pc =(int*)&c;
	pc[0] = 0x401e0000; pc[1]=pc[2]=pc[3]=0;	/* pc = 2^31 */
	if((x&0x80000000)!=0) {
		x ^= 0x80000000;
		_fp_unpack(&px,&x,fp_op_integer);
		_fp_pack(&px,&q,fp_op_extended);
		q = _Q_add(q,c);
	} else {
		_fp_unpack(&px,&x,fp_op_integer);
		_fp_pack(&px,&q,fp_op_extended);
	}
	return q;
}
