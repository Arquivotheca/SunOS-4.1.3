#ifndef lint
#ifdef SunB1
#ident			"@(#)is_server.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)is_server.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include "install.h"


/****************************************************************************
** Function : (int) is_server()
** 
** Return values :  1 : if the host is a server of the architecture
** 		    0 : if the host is not a server of the architecture
**
** Description : this function checks if the host machine is a server of the
**		 architecture that was passed to it by string.  We
**		 check if any clients have been created using that
**		 architecture.  If they have, we return a true (1), if not,
**		 we return a false (0).
**
*****************************************************************************
*/
int
is_server(arch)
	char		*arch;		/* full aprid string */
{
	char		pathname[MAXPATHLEN];
	char		buf[MEDIUM_STR];

	(void) sprintf(pathname, "/tftpboot/boot.%s",
		       aprid_to_irid(arch, buf));

	if (access(pathname, 0) == 0)
		return(1);
	else
		return(0);
	
	
}



