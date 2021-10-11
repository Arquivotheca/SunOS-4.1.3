#include <stdio.h>
#include <sys/param.h>

char	*malloc(),
	*strcat(),
	*strncpy(),
	*strcpy();
void	setname();

char **
split(path)
	char	*path;
{
	register char	*p;
	int count;
	char	**namearray;
	char	**np;
	
	if( *path && *path == '/' )
		++path;

	for( p = path, count = 1; *p; ++p )
		if( *p == '/' )
			++count;
	++count;	/* for null pointer at end of list */

	np = namearray = (char **)malloc((unsigned)(count * sizeof(char **)));

	while( *path )
	{
		register int	len;

		p = path;
		while( *p && *p != '/' )
			++p;
		len = p-path+1;
		*np = malloc((unsigned)len);
		(void)strncpy(*np, path, len);
		(*np)[len-1] = '\0';

		if( *p == '/' )
			++p;
		path = p;
		++np;
	}
	*np = NULL;
	return(namearray);
}

char **
pathopt(pathv)
	char	**pathv;
{
	register char	**p,
			**n;
	int	count;
	register char	**newv;

	for( p = pathv, count = 0; *p; ++p, ++count)
		;
	newv = (char **)malloc((unsigned)((count+1) * sizeof(char **)));
	for( p = pathv, n = newv; *p; ++p )
	{
		if( !**p )
			;
		else if( strcmp(*p, ".") == 0 )
			;
		else if( strcmp(*p, "..") == 0 && n > newv )
		{
			if( strcmp(*--n, "..") == 0 )
				++n;
			else
			{
				free(*n);
				*n = NULL;
			}

		}
		else
		{
			*n = malloc((unsigned)(strlen(*p)+1));
			(void)strcpy(*n, *p);
			++n;
		}
		free(*p);
		*p = NULL;
	}
	*n = (char *)NULL;
	free((char *)pathv);

	return(newv);
}

char *
sccsname(prefix, path, sccsflag)
	char	*prefix,
		*path;
	int	sccsflag;
{
	register char	**pathv;
	register char	**pv;
	static char	newpath[MAXPATHLEN];
	int	i, j, k;
	int	sccsidx = -1;

	newpath[0] = '\0';

	if( prefix )
	{
		(void)strcpy(newpath, prefix );
		(void)strcat(newpath, "/");
	}
	(void)strcat(newpath, path);

	pv = pathv = pathopt(split(newpath));

	newpath[1] = '\0';
	if( newpath[0] != '/' )
		newpath[0] = '\0';

	i = -1;
	while( *pv )
	{
		++i;
		if( strcmp("SCCS", *pv) == 0 )
			sccsidx = i;
		++pv;
	}
	pv = pathv;

	if( sccsidx >= 0 )
		j = sccsidx;
	else
		j = i;

	k = 0;
	while( k < j )
	{
		(void)strcat(newpath, pathv[k++]);
		(void)strcat(newpath, "/");
	}

	if( sccsflag )
		(void)strcat(newpath, "SCCS/");
	if( strncmp("s.", pathv[i], 2) != 0 )
		(void)strcat(newpath, "s.");
	(void)strcat(newpath, pathv[i]);
	freev(pathv);
	return(newpath);
}

freev(v)
	char	**v;
{
	register char	**vp = v;

	while( *vp )
	{
		free(*vp);
		*vp++ = NULL;
	}
	free((char *)v);
}
