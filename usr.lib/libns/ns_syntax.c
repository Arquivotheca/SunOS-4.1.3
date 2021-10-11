
/*	@(#)ns_syntax.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*  #ident	"@(#)libns:ns_syntax.c	1.1" */
#include <rfs/nserve.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define INV_RES "/. "
#define RES_MESG "resource name cannot contain '/', '.', ' ', or non-printable characters"

#define INV_MACH ":,.\n"
#define MACH_MESG "name cannot contain ':', ',', '\\n', '.', or non-printable characters"

#define VAL_DOM "-_"
#define DOM_MESG "domain name must contain only alphanumerics, '_', or '-'"

#define INV_MESG "contains invalid characters"

pv_resname(cmd,name,flag)
char	*cmd;
char	*name;
int	flag;
{

	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: resource name is null\n",cmd);
		return(1);
	}

	if (v_resname(name)) {
		fprintf(stderr,"%s: resource name <%s> %s\n",cmd,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s\n",cmd,RES_MESG);
		return(1);
	}

	if (strlen(name) > SZ_RES) {
		fprintf(stderr,"%s: resource name <%s> exceeds <%d> characters\n",cmd,name,SZ_RES);
		return(2);
	}
	return(0);
}

pv_uname(cmd,name,flag,title)
char	*cmd;
char	*name;
int	flag;
char	*title;
{

	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: %sname is null\n",cmd, title);
		return(1);
	}

	if (v_uname(name)) {
		fprintf(stderr,"%s: %sname <%s> %s\n",cmd,title,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s%s\n",cmd,title,MACH_MESG);
		return(1);
	}

	if (strlen(name) > SZ_MACH) {
		fprintf(stderr,"%s: %sname <%s> exceeds <%d> characters\n",cmd,title,name,SZ_MACH);
		return(2);
	}
	return(0);
}

pv_dname(cmd,name,flag)
char	*cmd;
char	*name;
int	flag;
{

	if (name == NULL || *name == '\0') {
		fprintf(stderr,"%s: domain name is null\n",cmd);
		return(1);
	}

	if (v_dname(name)) {
		fprintf(stderr,"%s: domain name <%s> %s\n",cmd,name,INV_MESG);
		if (!flag)
			fprintf(stderr,"%s: %s\n",cmd,DOM_MESG);
		return(1);
	}

	if (strlen(name) > SZ_DELEMENT) {
		fprintf(stderr,"%s: domain name <%s> exceeds <%d> characters\n",cmd,name,SZ_DELEMENT);
		return(2);
	}
	return(0);
}

/*
 *	Function to check the resource name for '/', '.', ' ',
 *	or non-printable characters.
 */

v_resname(name)
register char *name;
{

	while (*name != '\0') {
		if (!isprint(*name))
			return(1);
		else
			if (strchr(INV_RES,*name) != NULL)
				return(1);
		name++;
	}
	return(0);
}

/*
 *	Function to check the nodename for ':', '.', ',', '\\n'
 *	or non-printable characters.
 */

v_uname(name)
register char *name;
{

	while (*name != '\0') {
		if (!isprint(*name))
			return(1);
		else
			if (strchr(INV_MACH,*name) != NULL)
				return(1);
		name++;
	}
	return(0);
}

/*
 *	Function to check the domain name for any character
 *	other than '-', '_' or alphanumerics.
 */

v_dname(name)
register char *name;
{

	while (*name != '\0') {
		if (!isalnum(*name) && strchr(VAL_DOM,*name) == NULL)
			return(1);
		name++;
	}
	return(0);
}
