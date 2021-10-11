#ifndef lint
static	char sccsid[] = "@(#)erase.c 1.1 92/07/30 SMI"; /* from UCB 4.1 6/27/83 */
#endif

#include "con.h"
erase(){
	int i;
		for(i=0; i<11*(VERTRESP/VERTRES); i++)
			spew(DOWN);
		return;
}
