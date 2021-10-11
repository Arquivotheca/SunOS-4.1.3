# include	<ctype.h>
# include	"../hdr/defines.h"

SCCSID(@(#)chkid.c 1.1 92/07/30 SMI); /* from System III 5.1 */

chkid(line)
char *line;
{
	register char *lp;
	extern int Did_id;

	if (!Did_id && any('%',line))
		for(lp=line; *lp != 0; lp++) {
			if(lp[0] == '%' && lp[1] != 0 && lp[2] == '%')
				if (isupper(lp[1]))
					switch (lp[1]) {
					case 'J':
						break;
					case 'K':
						break;
					case 'N':
						break;
					case 'O':
						break;
					case 'V':
						break;
					case 'X':
						break;
					default:
						return(Did_id++);
					}
		}
	return(Did_id);
}
