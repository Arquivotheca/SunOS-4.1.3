# include	"../hdr/defines.h"

SCCSID(@(#)chksid.c 1.1 92/07/30 SMI); /* from System III 5.1 */

chksid(p,sp)
char *p;
register struct sid *sp;
{
	if (*p || sp == 0 ||
		(sp->s_rel == 0 && sp->s_lev) ||
		(sp->s_lev == 0 && sp->s_br) ||
		(sp->s_br == 0 && sp->s_seq))
			fatal("invalid sid (co8)");
}
