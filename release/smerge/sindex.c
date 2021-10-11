sindex(s1, s2)
	char	*s1;
	char	*s2;
{
	register char	*p1;
	register char	*p2;
	register int    flag;
	int             ii;

	p1 = s1;
	p2 = s2;
	flag = -1;
	for (ii = 0;; ii++)
	{
		while (*p1 == *p2)
		{
			if (flag < 0)
				flag = ii;
			if (*p1++ == '\0')
				return (flag);
			p2++;
		}
		if (*p2 == '\0')
			return (flag);
		if (flag >= 0)
		{
			flag = -1;
			p2 = s2;
		}
		if (*s1++ == '\0')
			return (flag);
		p1 = s1;
	}
}
