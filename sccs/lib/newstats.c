# include	"../hdr/defines.h"

SCCSID(@(#)newstats.c 1.1 92/07/30 SMI); /* from System III 5.1 */

newstats(pkt,strp,ch)
register struct packet *pkt;
register char *strp;
register char *ch;
{
	char fivech[6];
	repeat(fivech,ch,5);
	sprintf(strp,"%c%c %s/%s/%s\n",CTLCHAR,STATS,fivech,fivech,fivech);
	putline(pkt,strp,0);
}
