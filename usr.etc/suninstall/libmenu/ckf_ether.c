#ifndef lint
static	char	sccsid[] = "@(#)ckf_ether.c 1.1 92/07/30";
#endif

/*
 *	Name:		ckf_ether.c
 *
 *	Description:	Determine if a field is an internet address.
 *		If the field is an internet address, then one is returned.
 *		Otherwise, zero is returned.
 *
 *	Call syntax:	ret_code = ckf_ether_aton(arg_p, field_p);
 *
 *	Parameters:	pointer		arg_p;
 *			char *		field_p;
 *
 *	Return value:	int		ret_code;
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include "menu.h"


/*
 *	External functions:
 */
extern	struct ether_addr *	ether_aton();


int
ckf_ether_aton(arg_p, field_p)
	pointer		arg_p;
	char *		field_p;
{
#ifdef lint
	arg_p = arg_p;
#endif

	if (strlen(field_p) == 0 || ether_aton(field_p) == NULL) {
		menu_mesg("Field is not a valid Ethernet address.");
		return(0);
	}

	return(1);
} /* end ckf_ether_aton() */
