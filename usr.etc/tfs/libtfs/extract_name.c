#ifndef lint
static char sccsid[] = "@(#)extract_name.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * _nse_extract_name(path, file_name) extracts the next file name in
 * the path 'path', and skips over the trailing '/', if any.
 * The file name is returned in 'file_name'.
 * the function value is the new index into the path.
 */
char *
_nse_extract_name(path, buf)
	char		*path;
	char		*buf;
{
	char		*filep;

	filep = buf;
	while (path[0] != '/' &&  path[0] != '\0') {
		filep[0] = path[0];
		filep++;
		path++;
	}
	filep[0] = '\0';
	if (path[0] == '/') {
		path++;
	}
	return(path);
}
