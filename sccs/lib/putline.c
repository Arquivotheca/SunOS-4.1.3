# include	"../hdr/defines.h"

SCCSID(@(#)putline.c 1.7 86/08/26 SMI); /* from System III 5.1 */
/*
	Routine to write out either the current line in the packet
	(if newline is zero) or the line specified by newline.
	A line is actually written (and the x-file is only
	opened) if pkt->p_upd is non-zero.  When the current line from 
	the packet is written, pkt->p_wrttn is set non-zero, and
	further attempts to write it are ignored.  When a line is
	read into the packet, pkt->p_wrttn must be turned off.
*/

#define MAX_LINES	99999	/* Max # of lines that can fit in the */
				/* stats field.  Larger #s will overflow */
				/* and corrupt the file */

int	Xcreate;
FILE	*Xiop;


putline(pkt,newline)
register struct packet *pkt;
char *newline;
{
	static char obf[BUFSIZ];
	char *xf;
	register char *p;
	extern char *auxf();
	FILE *fdfopen();

	if(pkt->p_upd == 0) return;

	xf = auxf(pkt->p_file, 'x');

	if(!Xcreate) {
		/*
		 * Stash away gid and uid from the stat,
		 * as Xfcreat will trash Statbuf.
		 */
		int	gid, uid;

		stat(pkt->p_file,&Statbuf);
		gid = Statbuf.st_gid;
		uid = Statbuf.st_uid;
		Xiop = xfcreat(xf,Statbuf.st_mode);
		setbuf(Xiop,obf);
		chown(xf, uid, gid);
	}
	if (newline)
		p = newline;
	else {
		if(!pkt->p_wrttn++)
			p = pkt->p_line;
		else
			p = 0;
	}
	if (p) {
		if (fputs(p, Xiop) == EOF)
			xmsg(xf, "putline");
		if (Xcreate)
			while (*p)
				pkt->p_nhash += *p++;
	}
	Xcreate = 1;
}

flushline(pkt,stats)
register struct packet *pkt;
register struct stats *stats;
{
	register char *p;
	char *xf;
	char ins[6], del[6], unc[6], hash[6];

	if (pkt->p_upd == 0)
		return;
	putline(pkt,0);
	rewind(Xiop);

	if (stats) {
		if (stats->s_ins > MAX_LINES) {
			stats->s_ins = MAX_LINES;
		}
		if (stats->s_del > MAX_LINES) {
			stats->s_del = MAX_LINES;
		}
		if (stats->s_unc > MAX_LINES) {
			stats->s_unc = MAX_LINES;
		}
		sprintf(ins,"%05u",stats->s_ins);
		sprintf(del,"%05u",stats->s_del);
		sprintf(unc,"%05u",stats->s_unc);
		for (p = ins; *p; p++)
			pkt->p_nhash += (*p - '0');
		for (p = del; *p; p++)
			pkt->p_nhash += (*p - '0');
		for (p = unc; *p; p++)
			pkt->p_nhash += (*p - '0');
	}

	sprintf(hash,"%5u",pkt->p_nhash&0xFFFF);
	zeropad(hash);

	xf = auxf(pkt->p_file, 'x');

	if (fprintf(Xiop, "%c%c%s\n", CTLCHAR, HEAD, hash) == EOF)
		xmsg(xf, "flushline");
	if (stats)
		if (fprintf(Xiop, "%c%c %s/%s/%s\n",
				CTLCHAR, STATS, ins, del, unc) == EOF)
			xmsg(xf, "flushline");
	if (fflush(Xiop) == EOF)
		xmsg(xf, "flushline");

	/*
	 * Lots of paranoia here, to try to catch
	 * delayed failure information from NFS.
	 */
	if (fsync(fileno(Xiop)) < 0)
		xmsg(xf, "flushline");
	if (fclose(Xiop) == EOF)
		xmsg(xf, "flushline");
}

xrm(pkt)
struct packet *pkt;
{
	if (Xiop)
		fclose(Xiop);
	Xcreate = 0;
	Xiop = (FILE *)0;
}
