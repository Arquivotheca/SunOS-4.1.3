/* copy s into t, return the location of the next free character in s */
char *
copstr(s, t)
	register char	*s,
	                *t;
{
	if (t == 0)
		return (s);
	while (*t)
		*s++ = *t++;
	*s = '\0';
	return (s);
}
