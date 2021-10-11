/* 
 * @(#)names.c 1.1 92/07/30 SMI
 */
/* A collection of routines used to process sccs path names.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/file.h>

char	*malloc(),
	*strcat(),
	*copys(),
	*strncpy(),
	*getenv(),
	*strcpy();
void	setname();

/* split - Given a string with embedded slashes, return a null terminated
 * 	   vector of pointers to path components.
 *	   The routine freev() can be used to destroy the vector.
 */
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

/* pathopt - Given a pointer to a path component vector, returned from split(),
 *	     pathopt replaces the original vector with a minimal, equivalent
 *	     vector of path components.
 *	     The effects of symbolic links are not considered.
 */
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
			if( strcmp(n[-1], "..") == 0 )
				*n++ = copys(*p);
			else
			{
				free(*--n);
				*n = NULL;
			}
		}
		else
			*n++ = copys(*p);
		free(*p);
		*p = NULL;
	}
	*n = (char *)NULL;
	free((char *)pathv);

	return(newv);
}

/* sccsname - Given a path and an optional path prefix, sccsname returns a
 *	      pointer to a static buffer containing the corresponding SCCS
 *	      file name. Iff sccsflag == 1 then the penultimate path component
 *	      of the returned path will be "SCCS".
 *	      If sccsname() is given an SCCS path it will be have as if it
 *	      were given a non-SCCS path.
 *	      If the path prefix != NULL, then it is prepended to the path
 *	      before any other processing is done.
 */
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

/* FileExistOrGet - Return 1 if the file exists or was created from its SCCS
 *		    file. Return 0 otherwise. If echoflag != 0 the echo the
 *		    sccs get command on stdout before executing it.
 */
int
FileExistOrGet(fname, echoflag)
	char	*fname;
	int	echoflag;
{
	char	*sname,
		*getcmd,
		*getflags,
		cmdbuf[MAXPATHLEN];	/* I'm not going to worry about this
					 * buffer being too small (it is
					 * supposed to hold 3 paths plus
					 * command options and punctuation.
					 */

	if( access(fname, F_OK) == 0 )	
		return(1);
	
	sname = sccsname(NULL, fname, 1);

	if( access(sname, F_OK) != 0 )	
		return(0);

	if( (getcmd = getenv("GET")) == NULL )
		getcmd = "/usr/sccs/get";

	if( (getflags = getenv("GFLAGS")) == NULL )
		getflags = "";

	sprintf(cmdbuf, "%s -G%s %s %s\n", getcmd, fname, getflags, sname);
	if( echoflag )
		fprintf(stdout, "%s", cmdbuf);
	if( system(cmdbuf) != 0 )
		return(0);
	return(1);
}

/* freev - Destroy a vector created by split() or pathopt().
 */
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

#ifndef NOCOPYS
/* copys - Return a copy of the given string, using malloc(3) to allocate 
 *	   storage.
 */
char *
copys(string)
	char	*string;
{
	register char	*p;

	if( !string )
		return((char *)0);

	if( (p = malloc((unsigned)(strlen(string)+1))) == (char *)0 )
	{
		nomem();
		return((char *)0);
	}
	return(strcpy(p, string));
}
#endif NOCOPYS
