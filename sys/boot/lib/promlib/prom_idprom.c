/*	@(#)prom_idprom.c 1.1 92/07/30 SMI"	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <promcommon.h>
#include <mon/idprom.h>

/*
 * Get idprom property from root node, return to callers buffer.
 * Returns idprom format byte.
 */

#define	rootnode()	prom_nextnode((dnode_t)0)

#define	NO_FORMAT	((char)(IDFORM_1 - 1))

static int idpromlength;
static struct idprom idprombuf;
extern dnode_t prom_nextnode();

char
prom_getidprom(addr, size)
	caddr_t addr;
	int size;
{
	int length;
	dnode_t rootid;

	if (idpromlength == 0)  {
		if ((rootid = rootnode()) == OBP_BADNODE)  {
			prom_printf("prom_getidprom: can't get root node!\n");
			return (NO_FORMAT);
		}

		length = prom_getproplen(rootid, OBP_IDPROM);
		if (length == -1)  {
			prom_printf("Missing idprom property.\n");
			return (NO_FORMAT);
		}

		if (length > sizeof (idprombuf))  {
		  prom_printf("prom_getidprom: property size <%d> too large!\n",
				length);
			return (NO_FORMAT);
		}

		(void)prom_getprop(rootid, OBP_IDPROM, (char *)(&idprombuf));
		idpromlength = length;
	}
	bcopy((char *)(&idprombuf), addr, (u_int) size);
	return (*addr);
}
