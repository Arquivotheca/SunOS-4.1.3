# include	"../hdr/defines.h"

SCCSID(@(#)auxf.c 1.1 92/07/30 SMI); /* from System III 5.1 */

/*
	Figures out names for g-file, l-file, x-file, etc.

	File	Module	g-file	l-file	x-file & rest

	a/s.m	m	m	l.m	a/x.m

	Second argument is letter; 0 means module name is wanted.
*/

char *
auxf(sfile,ch)
register char *sfile;
register char ch;
{
	static char auxfile[FILESIZE];
	register char *snp;

	snp = sname(sfile);

	switch(ch) {

	case 0:
	case 'g':	copy(&snp[2],auxfile);
			break;

	case 'l':	copy(snp,auxfile);
			auxfile[0] = 'l';
			break;

	default:
			copy(sfile,auxfile);
			auxfile[snp-sfile] = ch;
	}
	return(auxfile);
}
