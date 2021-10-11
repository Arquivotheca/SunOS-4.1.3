/*
 * admin_param.c - package of functions for dealing with parameter manipulation
 * 	with the administrative framework.
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint 
static  char sccsid[] = "@(#)admin_param.c 1.1 92/07/30 SMI"; 
#endif

#include <string.h>
#include "admin_amcl.h"

/*
 * admin_get_param(argc, argv, param_name, param_value)
 *
 * Locate the parameter named by param_name in the argv list and return its
 * string value in param_value.  Assumes parameters are name=value pairs.
 */

int
admin_get_param(argc, argv, param_name, param_value, param_size)
	int argc;
	char *argv[];
	char *param_name;
	char *param_value;
	int param_size;
{
	char *cp;
	int i;

	/* Sanity check all arguments */
	if ((argc <= 0) || (argv == NULL) || (param_name == NULL) || 
	    (strlen(param_name) == 0) || (param_value == NULL) ||
	    (param_size <= 0))
		return (-1);

	/* Compare each keyword against the requested one, return value */
	for (i = 0; i < argc; i++) {
		if ((cp = strchr(argv[i], '=')) == NULL)
			continue;
		*cp = '\0';
		if (strcmp(argv[i], param_name) == 0) {
			*cp++ = '=';
			if (strlen(cp) > param_size)
				return (-1);
			else {
				strcpy(param_value, cp);
				return (0);
			}
		}
		*cp = '=';
	}

	/* No match */
	return (-1);
}


/*
 * admin_add_argp(argp, name, type, length, value)
 *
 * General purpose routine to append an argument to an Admin_arg list.  
 */
int
admin_add_argp(argp, name, type, length, value)
	Admin_arg **argp;
	char *name;
	u_int type;
	u_int length;
	caddr_t value;
{
	extern char *malloc();
	Admin_arg *ap, *aap;

	/* allocate a structure and copy args into fields. */
	aap = (Admin_arg *) malloc(sizeof (Admin_arg));
	strcpy(aap->name, name);
	aap->type = type;
	aap->length = length;
	aap->value = value;
	aap->next = NULL;

	/* If no list yet then start one, otherwise tack it on end */
	if (*argp == NULL)
		*argp = aap;
	else {
		for (ap = *argp; ap->next != NULL; ap = ap->next);
		ap->next = aap;
	}
	return (0);
}
