#if !defined(lint) && defined(SCCSIDS)
static char     sccsid[] = "@(#)ieee_globals.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc. 
 */

/*
 * contains definitions for variables for IEEE floating-point arithmetic
 * modes; IEEE floating-point arithmetic exception handling; 
 */

#include <floatingpoint.h>

enum fp_direction_type fp_direction;
/*
 * Current rounding direction. Updated by ieee_flags. 
 */

enum fp_precision_type fp_precision;
/*
 * Current rounding precision. Updated by ieee_flags. 
 */

sigfpe_handler_type ieee_handlers[N_IEEE_EXCEPTION];
/*
 * Array of pointers to functions to handle SIGFPE's corresponding to IEEE
 * fp_exceptions. sigfpe_default means do not generate SIGFPE. An invalid
 * address such as sigfpe_abort will cause abort on that SIGFPE. Updated by
 * ieee_handler. 
 */
fp_exception_field_type fp_accrued_exceptions;
/*
 * Sticky accumulated exceptions, updated by ieee_flags. In hardware
 * implementations this variable is not automatically updated as the hardware
 * changes and should therefore not be relied on directly. 
 */
