#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)is_running.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)is_running.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>	

#define GREP_STRING  "ps aux | grep %s | grep -v grep > %s"

/***************************************************************************
**
**	Function:	(int)	is_running()
**
**	Description:	tells if the process passed as a string is running
**			on the system already.  Yes, this is a terrible
**			hack, but for now, it must do.
**
**      Return Value:  1  : if process is running
**		       0  : if process is not running
**		      -1  : if an error occurred
**
****************************************************************************
*/
int
is_running(process)
	char	*process;	/* name of process to check */
{
	char		cmd[MAXPATHLEN * 2 + 30];
	char		*cp;
	struct	stat	stat_buf;

	/*
	**	Do a "ps aux" and see if the process is running
	*/
	cp = tmpnam((char *)NULL);
	(void) sprintf(cmd, GREP_STRING, process, cp);
	(void) system(cmd);

	if (stat(cp, &stat_buf) != 0)
		return(-1);

	(void) unlink(cp);	/* clean up */
	
	/*
	** now that we know that we made a file, let's check the size of it
	*/
	if (stat_buf.st_size > 0)
		return(1); 	/* the process is running */
	else
		return(0);	/* the process is not running */

}
