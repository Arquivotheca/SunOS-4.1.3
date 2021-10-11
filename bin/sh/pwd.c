/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)pwd.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.14 */
#endif

/* 
*	UNIX shell
 */

#include		"mac.h"
#include		<sys/param.h>
#include		<sys/types.h>
#include		<sys/stat.h>


#define	DOT		'.'
#define	NULL	0
#define	SLASH	'/'

static char cwdname[MAXPATHLEN];
static int 	didpwd = FALSE;

cwd(dir)
	register char *dir;
{
	register char *pcwd;
	register char *pdir;

	/* First remove extra /'s */

	rmslash(dir);

	/* Now remove any .'s */

	pdir = dir;
	if(*dir == SLASH)
		pdir++;
	while(*pdir) 			/* remove /./ by itself */
	{
		if((*pdir==DOT) && (*(pdir+1)==SLASH))
		{
			movstr(pdir+2, pdir);
			continue;
		}
		pdir++;
		while ((*pdir) && (*pdir != SLASH)) 
			pdir++;
		if (*pdir) 
			pdir++;
	}
	/* take care of trailing /. */
	if(*(--pdir)==DOT && pdir > dir && *(--pdir)==SLASH) {
		if(pdir > dir) {
			*pdir = NULL;
		} else {
			*(pdir+1) = NULL;
		}
	
	}
	
	/* Remove extra /'s */

	rmslash(dir);

	/* Now that the dir is canonicalized, process it */

	if(*dir==DOT && *(dir+1)==NULL)
	{
		return;
	}

	if(*dir==SLASH)
	{
		/* Absolute path */

		pcwd = cwdname;
	 	if (pcwd >= &cwdname[MAXPATHLEN])
		{
			didpwd=FALSE;
			return;
		}
		*pcwd++ = *dir++;
		didpwd = TRUE;
	}
	else
	{
		/* Relative path */

		if (didpwd == FALSE) 
			return;
			
		pcwd = cwdname + length(cwdname) - 1;
		if(pcwd != cwdname+1)
		{
			*pcwd++ = SLASH;
		}
	}
	while(*dir)
	{
		if(*dir==DOT && 
		   *(dir+1)==DOT &&
		   (*(dir+2)==SLASH || *(dir+2)==NULL))
		{
			/*
			 * Parent directory.  I could be crossing a
			 * symbolic link so rather than try to figure
			 * out if I did and go through the work of
			 * getting the correct cwdname here, I will
			 * defer to cwdprint().
			 */
			didpwd=FALSE;
			return;
		}
	 	if (pcwd >= &cwdname[MAXPATHLEN])
		{
			didpwd=FALSE;
			return;
		}
		*pcwd++ = *dir++;
		while((*dir) && (*dir != SLASH))
		{  
	 		if (pcwd >= &cwdname[MAXPATHLEN])
			{
				didpwd=FALSE;
				return;
			}
			*pcwd++ = *dir++;
		} 
		if (*dir) 
		{
	 		if (pcwd >= &cwdname[MAXPATHLEN])
			{
			didpwd=FALSE;
			return;
			}
			*pcwd++ = *dir++;
		}
	}
	if (pcwd >= &cwdname[MAXPATHLEN])
	{
		didpwd=FALSE;
		return;
	}
	*pcwd = NULL;

	--pcwd;
	if(pcwd>cwdname && *pcwd==SLASH)
	{
		/* Remove trailing / */

		*pcwd = NULL;
	}
	return;
}

/*
 *	Print the current working directory.
 */

cwdprint()
{
	if (didpwd == FALSE)
		pwd();

	prs_buff(cwdname);
	prc_buff(NL);
	return;
}

/*
 *	This routine will remove repeated slashes from string.
 */

static
rmslash(string)
	char *string;
{
	register char *pstring;

	pstring = string;
	while(*pstring)
	{
		if(*pstring==SLASH && *(pstring+1)==SLASH)
		{
			/* Remove repeated SLASH's */

			movstr(pstring+1, pstring);
			continue;
		}
		pstring++;
	}

	--pstring;
	if(pstring>string && *pstring==SLASH)
	{
		/* Remove trailing / */

		*pstring = NULL;
	}
	return;
}

/*
 *	Find the current directory the hard way.
 */

extern char	*getwd();

static
pwd()
{
	if (getwd(cwdname) == NULL) {
		error(cwdname);
		cwdname[0] = '\0';
	}
}
