#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)strtod.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

#include <errno.h>
#include <stdio.h>
#include <values.h>
#include <floatingpoint.h>

double
strtod(cp, ptr)
	char           *cp;
	char          **ptr;
{
	double          x;
	decimal_mode    mr;
	decimal_record  dr;
	fp_exception_field_type fs;
	enum decimal_string_form form;
	char           *pechar;

	string_to_decimal(&cp, MAXINT, 0, &dr, &form, &pechar);
	if (ptr != (char **) NULL)
		*ptr = cp;
	if (form == invalid_form)
		return 0.0;	/* Shameful kluge for SVID's sake. */
	mr.rd = fp_direction;
	decimal_to_double(&x, &mr, &dr, &fs);
	if (fs & (1 << fp_overflow)) {	/* Overflow. */
		errno = ERANGE;
	}
	if (fs & (1 << fp_underflow)) {	/* underflow */
		errno = ERANGE;
	}
	return x;
}
