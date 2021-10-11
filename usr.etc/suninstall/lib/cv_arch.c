#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)cv_arch.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)cv_arch.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		cv_arch.c
 *
 *	Description:	Routines for manipulating architectures.
 */

#include "install.h"


/*
 *	Local types:
 */

/*
 *	Definition of an architecture type:
 *
 *		arch_code	- the architecture
 *		arch_major	- the major architecture
 *		arch_minor	- the minor architecture
 */
typedef struct arch_type_t {
	int		arch_code;
	int		arch_major;
	int		arch_minor;
} arch_type;




/*
 *	Local variables:
 */

static	conv		arch_list[] = {		/* architecture conv. map */
#ifndef SunB1
	"sun2",		ARCH_SUN2,
#endif
	"sun3",		ARCH_SUN3,
	"sun3x",	ARCH_SUN3X,
	"sun4",		ARCH_SUN4,
	"sun386",	ARCH_SUN386,
	"sun4c",	ARCH_SUN4C,
	"sun4m",	ARCH_SUN4M,

	(char *) 0,	0
};

static	arch_type	type_list[] = {		/* architecture # map */
#ifndef SunB1
	ARCH_SUN2,	2,	1,
#endif
	ARCH_SUN3,	3,	1,
	ARCH_SUN3X,	3,	2,
	ARCH_SUN4,	4,	1,
	ARCH_SUN386,	386,	1,
	ARCH_SUN4C,	4,	2,
	ARCH_SUN4M,	4,	3,

	0,		0,	0
};




/*
 *	Name:		arch_major()
 *
 *	Description:	Return the major architecture number given an
 *		architecture code.  Returns 0 if the architecture code
 *		cannot be converted.
 */

int
arch_major(arch_code)
	int		arch_code;
{
	arch_type *	at;			/* ptr to architecture type */


	for (at = type_list; at->arch_code; at++)
		if (at->arch_code == arch_code)
			return(at->arch_major);
	return(0);
} /* end arch_major() */




/*
 *	Name:		arch_minor()
 *
 *	Description:	Return a minor architecture number given an
 *		architecture code.  Return 0 if the architecture code
 *		cannot be converted.
 */

int
arch_minor(arch_code)
	int		arch_code;
{
	arch_type *	at;			/* ptr to architecture type */


	for (at = type_list; at->arch_code; at++)
		if (at->arch_code == arch_code)
			return(at->arch_minor);
	return(0);
} /* end arch_minor() */




/*
 *	Name:		cv_arch_to_str()
 *
 *	Description:	Convert an architecture code into a string.  Returnss
 *		NULL if the architecture code cannot be converted.
 */

char *
cv_arch_to_str(arch_p)
	int *		arch_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = arch_list; cv->conv_text; cv++)
		if (cv->conv_value == *arch_p)
			return(cv->conv_text);
	return(NULL);
} /* end cv_arch_to_str() */




/*
 *	Name:		cv_str_to_arch()
 *
 *	Description:	Convert a string into an architecture code.
 *		Returns 1 if string was converted and 0 otherwise.
 */

int
cv_str_to_arch(str, data_p)
	char *		str;
	int *		data_p;
{
	conv *		cv;			/* conversion pointer */


	for (cv = arch_list; cv->conv_text; cv++)
		if (strcmp(cv->conv_text, str) == 0) {
			*data_p = cv->conv_value;
			return(1);
		}

	*data_p = 0;
	return(0);
} /* end cv_str_to_arch() */
