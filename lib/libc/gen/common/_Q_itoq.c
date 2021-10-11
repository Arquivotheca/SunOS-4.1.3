#ifndef lint
static	char sccsid[] = "@(#)_Q_itoq.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

#define	FUNC	itoq

QUAD
_Q_/**/FUNC/**/(x)
	int x;
{
	unpacked	px;
	QUAD		q;
	_fp_unpack(&px,&x,fp_op_integer);
	_fp_pack(&px,&q,fp_op_extended);
	return q;
}
