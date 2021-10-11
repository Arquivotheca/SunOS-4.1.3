/*
 * substitute 
 */

/* static  char *sccsid = "@(#)substitute.c 1.1 92/07/30 SMI"; */

#include "colldef.h"
#include "y.tab.h"
#include "extern.h"

/*
 * setup _1_to_m
 */
set_1_to_m(a, b)
	struct type_var *a, *b;
{
	if (no_of_1_to_m >= NUM_1_TO_MANY) {
		/*
		 * If too many, ignore it.
		 */
		warning("too many substitute statement.");
		return(ERROR);
	}
	_1_to_m[no_of_1_to_m].one = a->type_val.str_val[0];
	_1_to_m[no_of_1_to_m].many = strsave(b->type_val.str_val);
	++no_of_1_to_m;
	return(OK);
}
