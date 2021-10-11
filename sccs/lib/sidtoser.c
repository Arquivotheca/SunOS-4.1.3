# include	"../hdr/defines.h"

SCCSID(@(#)sidtoser.c 1.1 92/07/30 SMI); /* from System III 5.1 */

sidtoser(sp,pkt)
register struct sid *sp;
struct packet *pkt;
{
	register int n;
	register struct idel *rdp;

	for (n = maxser(pkt); n; n--) {
		rdp = &pkt->p_idel[n];
		if (rdp->i_sid.s_rel == sp->s_rel &&
			rdp->i_sid.s_lev == sp->s_lev &&
			rdp->i_sid.s_br == sp->s_br &&
			rdp->i_sid.s_seq == sp->s_seq)
				break;
	}
	return(n);
}
