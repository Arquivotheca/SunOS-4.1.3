static
umatch(s, p)
	register char	*s,
	                *p;
{
	register        c;

	switch (*p)
	{
	case 0:
		return (1);

	case '[':
	case '?':
	case '*':
		while (*s)
			if (amatch(s++, p))
				return (1);
		break;

	default:
		c = *p++;
		while (*s)
			if (*s++ == c && amatch(s, p))
				return (1);
		break;
	}
	return (0);
}

/* stolen from glob through find */

amatch(s, p)
	register char	*s,
	                *p;
{
	register int    cc,
	                scc,
	                k;
	int             c,
	                lc;

top:
	scc = *s;
	lc = 077777;
	switch (c = *p)
	{

	case '[':
		k = 0;
		while (cc = *++p)
		{
			switch (cc)
			{

			case ']':
				if (k)
					return (amatch(++s, ++p));
				else
					return (0);

			case '-':
				k |= lc <= scc && scc <= (cc = p[1]);
			}
			if (scc == (lc = cc))
				k++;
		}
		return (0);

	case '?':
		if (scc == 0)
			return (0);
		p++, s++;
		goto top;

	case '*':
		return (umatch(s, ++p));
	case 0:
		return (!scc);
	}
	if (c != scc)
		return (0);
	p++, s++;
	goto top;
}
