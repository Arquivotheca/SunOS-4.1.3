#include <stdio.h>
#include "var.h"

extern char	*copstr(),
		*strrchr(),
		*calloc();

#define lastslash(a)		strrchr( (a) , '/' )

char	*subst();

static char	*
do_df(b, str, type)
	register char	*b;
	char            str[];
char            type;

{
	register char	*p;
	register int    i;
	char            buf[128];

	*b = '\0';
	for (; (i = getword(str, buf)); str += i)
	{
		if (buf[0] == ' ' || buf[0] == '\t')
		{
			b = copstr(b, buf);
			continue;
		}
		p = lastslash(buf);
		if (p)
		{
			*p = '\0';
			b = copstr(b, type == 'D' ? (buf[0] == '\0' ? "/" : buf) : p + 1);
			*p = '/';
		}
		else
			b = copstr(b, type == 'D' ? "." : buf);
	}
	return (b);
}

/*
 * Do the $(@D) type translations. 
 */

static char	*
dftrans(b, vname)
	register char	*b;
	char	*vname;
{
	register char   c1;
	VARBLOCK        vbp;

	c1 = vname[1];
	vname[1] = '\0';
	vbp = srchvar(vname);
	vname[1] = c1;
	if (vbp != 0 && *vbp->varval != 0)
		b = do_df(b, vbp->varval, c1);
	return (b);
}

static
getword(from, buf)
	register char	*from;
	register char	*buf;
{
	register int    i = 0;

	if (*from == '\t' || *from == ' ')
	{
		while (*from == '\t' || *from == ' ')
		{
			*buf++ = *from++;
			++i;
		}
		goto out;
	}
	while (*from && *from != '\t' && *from != ' ')
	{
		*buf++ = *from++;
		++i;
	}
out:
	*buf = '\0';
	return (i);
}
/*
 * Standard translation, nothing fancy. 
 */

static char	*
straightrans(b, vname)
	register char	*b;
	char            vname[];

{
	register VARBLOCK vbp;
	register CHAIN  pchain;
	register NAMEBLOCK pn;

	vbp = srchvar(vname);
	if (vbp != 0)
	{
		if (vbp->v_aflg == YES && vbp->varval)
		{
			pchain = (CHAIN) vbp->varval;
			for (; pchain; pchain = pchain->nextchain)
			{
				pn = (NAMEBLOCK) pchain->datap;
				if (pn->alias)
					b = copstr(b, pn->alias);
				else
					b = copstr(b, pn->namep);
				*b++ = ' ';
			}
			vbp->used = YES;
		}
		else if (*vbp->varval)
		{
			b = subst(vbp->varval, b);
			vbp->used = YES;
		}
	}
	return (b);
}

/*
 * Translate the $(name:*=*) type things. 
 */

static char	*
do_colon(to, from, trans)
	register char	*to,
	                *from;
	char	*        trans;
{
	register int    i;
	register int    leftlen;
	int             len;
	char            left[30],
	                right[70];
	char            buf[1024];
	char		*pbuf;

	/*
	 * Mark off the name (up to colon), the from expression (up to
	 * '='), and the to expresion (up to '\0'). 
	 */
	i = 0;
	while (*trans != '=')
		left[i++] = *trans++;
	left[i] = '\0';
	leftlen = i;
	i = 0;
	while (*++trans)
		right[i++] = *trans;
	right[i] = '\0';

	/*
	 * Now, tanslate. 
	 */

	for (; len = getword(from, buf); from += len)
	{
		pbuf = buf;
		if ((i = sindex(pbuf, left)) >= 0 && pbuf[i + leftlen] == '\0')
		{
			pbuf[i] = '\0';
			to = copstr(to, pbuf);
			to = copstr(to, right);
		}
		else
			to = copstr(to, pbuf);
	}
	return (to);
}

/*
 * Translate the $(name:*=*) type things. 
 */

char	*
colontrans(b, vname)
	register char	*b;
	char            vname[];

{
	register char	*p;
	register char	*q = 0;
	char            dftype = 0;
	char	*pcolon;
	VARBLOCK        vbp;
	extern void     cfree();

	for (p = &vname[0]; *p && *p != ':'; p++);
	pcolon = p;
	*pcolon = '\0';

	if (any("@*<%?", vname[0]))
	{
		dftype = vname[1];
		vname[1] = '\0';
	}
	if ((vbp = srchvar(vname)) == NULL)
		return (b);
	p = vbp->varval;
	if (dftype)
	{
		if ((q = calloc((unsigned) (strlen(p)) + 2, 1)) == NULL)
			fatal("Cannot alloc mem");
		(void) do_df(q, p, vname[1]);	/* D/F trans gets smaller */
		p = q;
	}
	if (p && *p)
		b = do_colon(b, p, pcolon + 1);
	*pcolon = ':';
	if (dftype)
		vname[1] = dftype;
	if (q)
		cfree(q);
	return (b);
}

/* copy string a into b, substituting for arguments */

char	*
subst(a, b)
	register char	*a,
	                *b;
{
	register char	*s;
	static          depth = 0;
	char            vname[BUFSIZ];
	char            closer;

	if (++depth > 100)
		fatal("infinitely recursive macro?");
	if (a != 0)
	{
		while (*a)
		{
			if (*a != '$')
				*b++ = *a++;
			else if (*++a == '\0' || *a == '$')
				*b++ = *a++;
			else
			{
				s = vname;
				if (*a == '(' || *a == '{')
				{
					closer = (*a == '(' ? ')' : '}');
					++a;
					while (*a == ' ')
						++a;
					while (*a != ' ' &&
					       *a != closer &&
					       *a != '\0')
						*s++ = *a++;
					while (*a != closer && *a != '\0')
						++a;
					if (*a == closer)
						++a;
				}
				else
					*s++ = *a++;

				*s = '\0';
				if (amatch(&vname[0], "*:*=*"))
					b = colontrans(b, vname);
				else if (any("@*<%?", vname[0]) && vname[1])
					b = dftrans(b, vname);
				else
					b = straightrans(b, vname);
				s++;
			}
		}
	}

	*b = '\0';
	--depth;
	return (b);
}
