#ifndef lint
#ifdef SunB1
#ident			"@(#)read_file.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)read_file.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		read_file()
 *
 *	Description:	Read file 'f_num' from the media described by
 *		'soft_p' into 'name'.
 *
 *		Returns 1 if everything is okay, and 0 if there was a
 *		non-fatal error.
 */

#include "install.h"


int
read_file(soft_p, f_num, name)
	soft_info *	soft_p;
	int		f_num;
	char *		name;
{
	int		ret_code;		/* return code */

	/*
	 *	Skip to the right file
	 */
	ret_code = media_fsf(soft_p, f_num, NOREDISPLAY);
	if (ret_code != 1)
		return(ret_code);
	
	ret_code = media_read_file(soft_p, name, NOREDISPLAY);
	if (ret_code != 1)
		return(ret_code);

	return(1);
} /* end read_file() */
