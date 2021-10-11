/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)grpck.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>

#define ERROR1 "Too many/few fields"
#define ERROR2a "No group name"
#define ERROR2b "Bad character(s) in group name"
#define ERROR2c "First char in group name not lower case alpha"
#define ERROR2d "Group name too long"
#define ERROR3  "Invalid GID"
#define ERROR4a "Null login name"
#define ERROR4b "Login name not found in password file"

int eflag, badchar, baddigit,badlognam,colons,len,i;
char buf[512];
char tmpbuf[512];

struct passwd *getpwnam();
char *strchr();
char *nptr;
int setpwent();
char *cptr;
FILE *fptr;
int delim[512];
long gid;
int yellowpages;
int error();
int ngrpnamec;

main (argc,argv)
int argc;
char *argv[];
{
  if ( argc == 1)
    argv[1] = "/etc/group";
  else if ( argc != 2 )
       {
	 fprintf (stderr,"\nusage: %s filename\n\n",*argv);
	 exit(1);
       }

  if ( ( fptr = fopen (argv[1],"r" ) ) == NULL )
  { 
	fprintf (stderr,"\ncannot open file %s\n\n",argv[1]);
	exit(1);
  }

  while(fgets(buf,512,fptr) != NULL )
  {
	if ( buf[0] == '\n' )    /* blank lines are ignored */
          continue;

	for (i=0; buf[i]!=NULL; i++)
	{
	  tmpbuf[i]=buf[i];          /* tmpbuf is a work area */
	  if (tmpbuf[i] == '\n')     /* newline changed to NULL */  
	    tmpbuf[i] = NULL;
	}

	for (; i <= 512; ++i)     /* blanks out rest of tmpbuf */ 
	{
	  tmpbuf[i] = NULL;
	}
	colons=0;
	eflag=0;
	badchar=0;
	baddigit=0;
	badlognam=0;
	gid=0l;
	yellowpages=0;

    /* See if this is a NIS reference */

	if(buf[0] == '+' || buf[0] == '-') {
	  yellowpages++;
	}

    /*	Check number of fields	*/

	for (i=0 ; buf[i]!=NULL ; i++)
	{
	  if (buf[i]==':')
          {
            delim[colons]=i;
            ++colons;
          }
	}
	if (colons != 3 )
	{
	  if(!yellowpages) error(ERROR1);
	  continue;
	}

    /*  Check that first character is alpha and rest alphanumeric  */
 
	i = 0;
	if(yellowpages) {
	  i++;
	  if(buf[1] == '\0' || buf[1] == ':') continue;
	}
	else
	{
	  if ( buf[0] == ':' )
	    error(ERROR2a);
	}

	if(!(islower(buf[i]))) {
	  error(ERROR2c);
	}
	for(ngrpnamec=1, i++; buf[i]!=':'; ngrpnamec++, i++) {
	  if(islower(buf[i]));
	  else if(isdigit(buf[i]));
	  else ++badchar;
	}

	if ( badchar > 0 )
	  error(ERROR2b);

    /*  Check for valid number of characters in groupname  */

		if(ngrpnamec > 8) {
			error(ERROR2d);
		}

    /*	check that GID is numeric and <= 65535	*/

	len = ( delim[2] - delim[1] ) - 1;

	if ( len > 5 || ( !yellowpages && len == 0 ) )
	  error(ERROR3);
	else
	{
	  for ( i=(delim[1]+1); i < delim[2]; i++ )
	  {
	    if ( ! (isdigit(buf[i])))
	      baddigit++;
	    else if ( baddigit == 0 )
		gid=gid * 10 + (buf[i]) - '0';    /* converts ascii */
                                                  /* GID to decimal */
	  }
	  if ( baddigit > 0 )
	    error(ERROR3);
	  else if ( gid > 65535l || gid < 0l )
	      error(ERROR3);
	}

     /*  check that logname appears in the passwd file  */

	nptr = &tmpbuf[delim[2]];
	nptr++;
	if ( *nptr == NULL )
	  continue;	/* empty logname list is OK */
	for (;;)
	{
	  if ( ( cptr = strchr(nptr,',') ) != NULL )
	    *cptr=NULL;
	  if ( *nptr == NULL )
	    error(ERROR4a);
	  else
	  {
	    if (  getpwnam(nptr) == NULL )
	    {
	      badlognam=1;
	      error(ERROR4b);
	    }
	  }
	  if ( cptr == NULL )
	    break;
	  nptr = ++cptr;
	  setpwent();
	}
	
  }
  exit(0);
  /* NOTREACHED */
}

    /*	Error printing routine	*/

error(msg)

char *msg;
{
	if ( eflag==0 )
	{
	  fprintf(stderr,"\n\n%s",buf);
	  eflag=1;
	}

	if ( badchar != 0 )
	{
	  fprintf (stderr,"\t%d %s\n",badchar,msg);
	  badchar=0;
	  return;
	}
	else if ( baddigit != 0 )
	     {
		fprintf (stderr,"\t%s\n",msg);
		baddigit=0;
		return;
	     }
	     else if ( badlognam != 0 )
		  {
		     fprintf (stderr,"\t%s - %s\n",nptr,msg);
		     badlognam=0;
		     return;
		  }
		  else
		  {
		    fprintf (stderr,"\t%s\n",msg);
		    return;
		  }
}
