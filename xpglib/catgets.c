/* X/OPEN XPG2 messaging interfaces 
 *   dumb interfaces to real routines
 */
#if !defined(lint) && defined(SCCSIDS)
static  char *sccsid = "@(#)catgets.c 1.1 92/07/30 SMI";
#endif

#include <stdio.h>
#include <nl_types.h>
#include <locale.h>
#include <memory.h>

#define	MAXCATS	10


extern char *dgettext();

extern char *_current_domain;

static char _cat2domain[MAXCATS][MAXDOMAIN];


char *
catgets(catd, set_num, msg_num, s)
	nl_catd catd;
	int set_num, msg_num;
	char *s;
{
	char str[MAXDOMAIN];
	register char *res;

	if ((catd>=MAXCATS) || (_cat2domain[catd][0] == 0))
		return s;

	sprintf(str,"%d:%d", set_num, msg_num);
	res = dgettext(_cat2domain[catd], str); 
	
	if (strcmp(res, str) == 0) 
		return s;
	else
  		return res;
}

char *catgetmsg(catd,set_id,msg_id,buf,buflen) 
nl_catd catd; int set_id; int msg_id; char *buf; int buflen;
{
	strncpy (buf, catgets(catd, set_id, msg_id, memset(buf,'\0',buflen)), 
		buflen-1);
	return (buf);
}

/* catopen() fakes up to MAXCATS simultaneous catalogue opens.
 * Note that the real stacking of catalogues is done in gettext, where
 * the domains are cached. This locale stacking allows
 * the (bad style) use of multi catalogues in applications.
 */


nl_catd catopen(name, oflag)
char *name;
int oflag;
{	
	register int i;

	if (name == NULL)
		return((nl_catd) -1);

 	if (_current_domain == NULL) {
                if ((_current_domain = (char *)malloc(MAXDOMAIN)) == NULL)
                        return ((nl_catd) -1);
	}
	if (*name == '\0')
		*name = ' ';

	strcpy(_current_domain, name);

	/* first look for a re-open of same catalogue. */

	for (i=0; i<MAXCATS; i++)  {
		if (strcmp(_cat2domain[i], name) == 0)
			return((nl_catd) i);
	}
	
	/* Now assign first free catd */

	for (i=0; i<MAXCATS; i++)  {
		if (_cat2domain[i][0] == 0) {
			strcpy(_cat2domain[i], name);
			return ((nl_catd) i);
		}

	}

	*_current_domain = 0;
	return ((nl_catd) -1);	
}


int catclose (catd) 
nl_catd catd;

{
	if (catd >= MAXCATS)
		return -1;
	/* Invalidate currently open catalogue */
	if (strcmp(_cat2domain[catd],_current_domain) == 0)
		*_current_domain = 0;
	_cat2domain[catd][0] = 0;
	return 0;
}        
