# include	"../hdr/defines.h"

SCCSID(@(#)flushto.c 1.1 92/07/30 SMI); /* from System III 5.1 */

flushto(pkt,ch,put)
register struct packet *pkt;
register char ch;
int put;
{
	register char *p;
	extern char *getline();
	while ((p = getline(pkt)) != NULL && !(*p++ == CTLCHAR && *p == ch))
		pkt->p_wrttn = put;

	if (p == NULL)
		fmterr(pkt);

	putline(pkt,0);
}
