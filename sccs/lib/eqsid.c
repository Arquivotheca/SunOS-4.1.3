# include	"../hdr/defines.h"

SCCSID(@(#)eqsid.c 1.1 92/07/30 SMI); /* from System III 5.1 */

eqsid(s1, s2)
register struct sid *s1, *s2;
{
	if (s1->s_rel == s2->s_rel &&
		s1->s_lev == s2->s_lev &&
		s1->s_br == s2->s_br &&
		s1->s_seq == s2->s_seq)
			return(1);
	else
		return(0);
}
