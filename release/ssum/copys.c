char	*malloc(),
	*strcpy();

char *
copys(string)
	char	*string;
{
	register char	*p;

	if( (p = malloc((unsigned)(strlen(string)+1))) == (char *)0 )
	{
		nomem();
		return((char *)0);
	}
	return(strcpy(p, string));
}
