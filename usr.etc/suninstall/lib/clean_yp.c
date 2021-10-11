#ifndef lint
#ifdef SunB1
#ident			"@(#)clean_yp.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)clean_yp.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 * Copyright (c) 1990 by Sun Microsystems,Inc
 */


/***************************************************************************
** Function : (void) clean_yp()
**
** Return Value : none
**
** Description : This function 
**
****************************************************************************
*/

#include "install.h"

void
clean_yp(root_path, yp_type)
	char	*root_path;
	int	yp_type;
{
	char	cmd[MAXPATHLEN + 15];

	/*
	 *	If this client is not running NIS, then delete the NIS stuff or
	 *	if it's a client or slave, remove the Makefile and updaters
	 */

	switch(yp_type) {
	case YP_NONE :
		(void) sprintf(cmd, "rm -rf %s%s/var/yp", dir_prefix(),
			       root_path);
		x_system(cmd);
		break;
	case YP_CLIENT :
	case YP_SLAVE :
		(void) sprintf(cmd, "%s%s/var/yp/Makefile", dir_prefix(),
			       root_path);
		(void) unlink(cmd);
		(void) sprintf(cmd, "%s%s/var/yp/updaters", dir_prefix(),
			       root_path);
		(void) unlink(cmd);
		break;
	}


}

