#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)getidprom.c 1.1 92/07/30 SMI";
#endif

/*
 * Note: Obsoleted and replaced by prom_idprom.c.
 * No longer used.
 */

#include <sys/types.h>
#include <machine/param.h>
#include <mon/idprom.h>
#include <mon/sunromvec.h>
#include <sun/openprom.h>

#define	nextnode		(dnode_t)prom_nextnode
#define	rootnode()		nextnode((dnode_t)0)
#define	getproplen		prom_getproplen
#define	getprop			prom_getprop

#define	NO_FORMAT	((char)(IDFORM_1 - 1))

static int idpromlength;
static struct idprom idprom;

/*
 * Get idprom from "idprom" property of root node.
 */

char
getidprom(addr, size)
	caddr_t addr;
	int size;
{
	int length;
	dnode_t rootid;
	caddr_t p = addr;

	if (idpromlength == 0)  {
		if ((rootid = rootnode()) == OBP_BADNODE)  {
			printf("getidprom: can't get root node!\n");
			return (NO_FORMAT);
		}

		length = getproplen(rootid, OBP_IDPROM);
		if (length == -1)  {
			printf("Missing idprom property.\n");
			return (NO_FORMAT):
		}

		if (length > sizeof (idprom))  {
			printf("getidprom: property size <%d> too large!\n",
				length);
			return (NO_FORMAT);
		}

		idpromlength = length;
		(void)getprop(rootid, OBP_IDPROM, (char *)idprom);
	}
	bcopy((char *)idprom, addr, size);
	return (*addr);
}
