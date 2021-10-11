/*	@(#)types.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef _NSE_TYPES_H
#define _NSE_TYPES_H

/* Including <sys/types.h> to get time_t and unsigned types typedefs. */
#include <sys/types.h>

/* 
 * Including <rpc/types.h> to get bool_t, TRUE, and FALSE definitions.
 * The window system header files also define TRUE and FALSE and
 * rpc/types.h does not check for them already being defined so we
 * undefined them here just to be sure.
 */
#ifndef bool_t
#undef TRUE
#undef FALSE
#include <rpc/types.h>
#endif

typedef	char		*Nse_opaque;
typedef	int		(*Nse_intfunc)();
typedef bool_t		(*Nse_boolfunc)();
typedef void		(*Nse_voidfunc)();
typedef	Nse_opaque	(*Nse_opaquefunc)();

#endif _NSE_TYPES_H
