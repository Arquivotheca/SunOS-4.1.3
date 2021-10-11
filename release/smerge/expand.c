#include <stdio.h>
#include "smerge.h"
#include "ssum.h"

extern char	*myname,
		*getword(),
		*copys(),
		*malloc(),
		*getenv(),
		*getname(),
		*sprintf(),
		*strcpy();
extern int debug;

/* expand - do variable substitution on a string returning a pointer to
 * a static buffer containing new string
 *

 */
struct var {
	char	*name,
		*value;
} vars[] = {
	{VARTABLE, NULL},	/* name of decision table */
	{VARSTATE1, NULL},	/* state of first file */
	{VARSTATE2, NULL},	/* state of second file */
	{VARBASE1, NULL},	/* common base sid, "AMBIGUOUS", or "NONE" */
	{VARBASE2, NULL},	/* common base sid, "AMBIGUOUS", or "NONE" */
	{VARNSID1, NULL},	/* new sid's for file 1 */
	{VARNSID2, NULL},	/* new sid's for file 2 */
	{VARTREE1, NULL},	/* source 1 tree */
	{VARTREE2, NULL},	/* source2 tree */
	{VARTREE3, NULL},	/* output tree */
	{VARDIR, NULL},		/* directory part of path from the base of
				 * $tree[1-3] up to but not includeing $file
				 */
	{VARFILE, NULL},		/* last path component of filename */
	{VARTOP1, NULL},		/* sid of top sccs delta of file 1 */
	{VARTOP2, NULL},		/* sid of top sccs delta of file 2 */

	{NULL, NULL}
};

void
setname(name, value)
	char	*name,
		*value;
{
	struct var	*v = vars;

	if( debug )
		(void)fprintf(stderr, "setname(%s,%s)\n", name, value);

	while( v->name )
	{
		if( strcmp(v->name, name) == 0 )
			break;
		++v;
	}
	if( !v->name )
	{
		(void)fprintf(stderr, "%s: internal error setname(%s,%s)\n",
			myname, name, value);
		exit(-1);
	}

	if( v->value )
		free(v->value);

	if( value )
		v->value = copys(value);
	else
		v->value = NULL;
}

setsidname(name, array, delta)
	char	*name;
	delta_t	**array;
	int	delta;
{
	char	buf[80];
	sid_t	*sid;

	sid = &array[delta]->sid;
	if( sid->s_br != 0 )
		(void)sprintf(buf, "%d.%d.%d.%d",
			sid->s_rel, sid->s_lev, sid->s_br, sid->s_seq);
	else
		(void)sprintf(buf, "%d.%d", sid->s_rel, sid->s_lev);
	setname(name, buf);
}

clearnames()
{
	setname(VARBASE1, (char *)NULL);/* common base sid, "skew", or "none" */
	setname(VARBASE2, (char *)NULL);/* common base sid, "skew", or "none" */
	setname(VARDIR, (char *)NULL);	/* dir part of path from tree to file */
	setname(VARFILE, (char *)NULL);	/* last path component of filename */
	setname(VARTOP1, (char *)NULL);	/* sid of top sccs delta of file 1 */
	setname(VARTOP2, (char *)NULL);	/* sid of top sccs delta of file 2 */
	setname(VARNSID1, (char *)NULL);/* new sid's in file 1 */
	setname(VARNSID2, (char *)NULL);/* new sid's in file 2 */
}

char *
getname(word, required)
	char	*word;
	int	required;
{
	struct var	*v = vars;
	char		*p;

	if( debug )
		(void)fprintf(stderr, "getname(%s,%s) = ",
			word, required ? "REQ":"!REQ");

	while( v->name )
	{
		if( strcmp(v->name, word) == 0 && v->value )
		{
			if( v->value )
			{
				if( debug )
					(void)fprintf(stderr, "%s\n", v->value);
				return(v->value);
			}
			else
				break;
		}
		++v;
	}
	if( (p = getenv(word)) != NULL )
	{
		if( debug )
			(void)fprintf(stderr, "%s\n", p);
		return(p);
	}

	if( required )
	{
		(void)fprintf(stderr, "%s: %s not set\n",
			myname, word);
		exit(-1);
	}
	return(NULL);
}

char *
expand(arg)
	char	*arg;
{
	static char	buf[4096];
	register char	*p,
			*bufp,
			*q;
	char		word[80];

	bufp = buf;
	p = arg;

	while( *p )
	{
		if( *p == '\\' )
		{
			++p;
			*bufp++ = *p++;
		}
		else if( *p == '$' )
		{
			++p;
			if( *p == '(' )
			{
				p = getword(word, ++p);
				if( *p++ != ')' )
				{
					(void)fprintf(stderr,
						"%s: missing ')' in %s\n",
						myname,
						getname(VARTABLE, REQUIRED));
					exit(-1);
				}
			}
			else
				p = getword(word, p);
			
			q = getname(word, REQUIRED);
			while( *q )
				*bufp++ = *q++;
		}
		else if( (*bufp++ = *p++) == '\n' && *p == '\0' )
			*--bufp = '\0'; /* zap unescaped newline at eol */
	}
	*bufp = '\0';
	return(buf);
}
