#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)apr.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)apr.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
*
*		Suninstall Library  for the 
*		Support of Multiple Architectures, Multiple
*		Releases and Multiple Realizations
*
*/

#include "install.h"
#include <string.h>


/*
 *	Local global variables
 */
static char 	pathname[MAXPATHLEN];	/* for generic  storage of pathnames */


/*
*	same_appl_arch
*
*	returns a boolean of whether the application architecture 
*	of two os's exactly the same
*/
boolean 
same_appl_arch(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( strcmp(os_1->appl_arch, os_2->appl_arch) == 0 );
}
	

/*
*	same_impl_arch
*
*	returns a boolean of whether the implement architecture
*	of two os's exactly the same
*/
boolean 
same_impl_arch(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( strcmp(os_1->impl_arch, os_2->impl_arch) == 0 );
}


/*
*	same_release
*
*	returns a boolean of whether the release of two os's is 
*	exactly the same
*/

boolean 
same_release(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( strcmp(os_1->release, os_2->release) == 0 );
}

/*
 *	same_realization
 *
 *	returns a boolean of whether the realization of two os's is
 *	exactly the same
 */
boolean 
same_realization(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( strcmp(os_1->realization, os_2->realization) == 0 );
}

/*
*	same_arch_pair
*
*	returns a boolean of whether the architecture pair
*	of two os's exactly the same
*/
boolean 
same_arch_pair(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( same_impl_arch(os_1, os_2) && same_appl_arch(os_1, os_2) ); 
}



/*
*	same_os
*
*	returns a boolean of whether the two os's are 
*	exactly the same (same arch_pair, release, and PSR)
*/
boolean 
same_os(os_1, os_2)
	Os_ident *os_1;
	Os_ident *os_2;
{
	return( same_arch_pair(os_1, os_2) &&
		same_release(os_1, os_2)  &&
		same_realization(os_1, os_2) );
}

/*
*	appl_arch
*
*	returns an application architecture from
*	a given impl_arch string, and put it into buf
*/
char *
appl_arch(impl_arch, buf)
	char *impl_arch;
	char *buf;
{
	int type;
	(void) sscanf(impl_arch, "sun%d", &type);
	(void) sprintf(buf, "sun%d", type);
	return(buf);
}

/*
*	os_aprid
*
*	given an Os_ident struct, return a pointer to a
*	long form name (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
*	The name is stored in buf.
*/
char *
os_aprid(os, buf)
	Os_ident *os;
	char *buf;
{
	(void) sprintf(buf, "%s.%s.%s.%s%s", os->appl_arch, os->impl_arch, 
		    os->os_name, os->release, os->realization);
	return(buf);
}


/*
*	os_arid
*
*	given an Os_ident struct, return a pointer to an
*	application release id (e.g. sun3.sunos.4.0.3)
*	Note that an arid does not contain an implementation
*	architecture, nor does it have a realization string.
*
*	This is generally used to describe sharable /usr
*/
char *
os_arid(os, buf)
	Os_ident *os;
	char *buf;
{
	(void) sprintf(buf, "%s.%s.%s", os->appl_arch, os->os_name, 
		os->release);
	return(buf);
}


/*
*	os_irid
*
*	given an Os_ident struct, return a pointer to a
*	long form name (e.g. sun3x.sunos.4.0.3_PSR_Z)
*/
char *
os_irid(os, buf)
	Os_ident *os;
	char *buf;
{
	(void) sprintf(buf, "%s.%s.%s%s", os->impl_arch, os->os_name, 
		os->release, os->realization);
	return(buf);
}


/*
*	aprid_to_aid
*
*	given an aprid, (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
*	return an application arch.id  sun3. 
*/
char *
aprid_to_aid(aprid, buf)
	char 	*aprid;
	char	*buf;
{
	Os_ident os;

	*buf = '\0';
	if (fill_os_ident(&os, aprid) >= 0)   {
		(void) strcpy(buf, os.appl_arch);
	}
	return(buf);
}



/*
*	aprid_to_iid
*
*	given an aprid, (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
*	return an implementation arch.id sun3x. 
*/
char *
aprid_to_iid(aprid, buf)
	char 	*aprid;
	char	*buf;
{
	Os_ident os;

	*buf = '\0';
	if (fill_os_ident(&os, aprid) >= 0)   {
		(void) strcpy(buf, os.impl_arch);
	}
	return(buf);
}



/*
*	aprid_to_rid
*
*	given an aprid, (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
*	return a pointer to an application release id sunos.4.0.3_PSR_Z 
*	The name is stored in buf.
*/
char *
aprid_to_rid(aprid, buf)
	char *aprid;
	char *buf;
{
	Os_ident os;

	*buf = '\0';
	if (fill_os_ident(&os, aprid) > 0)   {
		(void) sprintf(buf, "%s.%s%s", os.os_name, 
			       os.release, os.realization);
	}
	return(buf);	
}


/*
 *	aprid_to_arid
 *
 *	given an aprid, (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
 *	return a pointer to an arid (sun3.sunos.4.0.3)
 *
 *	Note : An arid has no implementation arch or realization string.
 *	The name is stored in buf.
 *
 */
char *
aprid_to_arid(aprid, buf)
	char *aprid;
	char *buf;
{
	Os_ident os;

	*buf = '\0';
	if (fill_os_ident(&os, aprid) > 0)
		return(os_arid(&os, buf));	
	else
		return(buf);
}


/*
*	aprid_to_irid
*
*	given an aprid, (e.g. sun3.sun3x.sunos.4.0.3_PSR_Z).
*	return a pointer to an irid (sun3x.sunos.4.0.3_PSR_Z) 
*	The name is stored in buf.
*/
char *
aprid_to_irid(aprid, buf)
	char *aprid;
	char *buf;
{
	Os_ident os;

	*buf = '\0';
	if (fill_os_ident(&os, aprid) > 0)
		return(os_irid(&os, buf));	
	else
		return(buf);
}


/*
*	irid_to_aprid
*
*	given an irid, (e.g. sun3x.sunos.4.0.3_PSR_Z).
*	return a pointer to an aprid (sun3.sun3x.sunos.4.0.3_PSR_Z) 
*	The name is stored in buf.
*/
char *
irid_to_aprid(irid, buf)
	char *irid;
	char *buf;
{
	int	type;

	*buf = '\0';
	if (sscanf(irid, "sun%d", &type) == 1)	{
		(void) sprintf(buf, "sun%d.%s", type, irid);
	}
	return(buf);
}


/*
*	os_ident_token
*
*	given an Os_ident struct, return a pointer to a
*	long form name (e.g. sun3x.4.0.3_PSR_Z) for radio panel
*	uses only.  The name is stored in buf.
*/
char *
os_ident_token(os, buf)
	Os_ident *os;
	char *buf;
{
	(void) sprintf(buf, "%s.%s%s", os->impl_arch, os->release, 
		os->realization);
	return(buf);
}

/*
*	extract_str
*
*	extract a token from a given string terminated
*	by counting the appearances of the terminator
*	return a pointer to next char after the terminator.
*	or (char *)NULL if it is end of string.
*	Notice that the line feed is treated as end of string too.
*/
char * 
extract_str(src, dst, terminator, times)
	char 	*src, *dst;
	char 	terminator;
	int 	times;
{
	char *cp;
	int count;
	for ( count = 0, cp = src; *cp && *cp != '\n' ; *dst++ = *cp++ ) {
		if (*cp == terminator)
			count++ ;
		if (count >= times)
			break;
	}
	*dst = '\0';
	if (*cp == '\0' || *cp == '\n')  
		return ((char *)NULL);
	return (cp+1);
}


/*
*	fill_os_ident
*	
*	fills an Os_ident struct with a aprid
* 
*	Return 1:   if aprid is a complete one;
*	       0:   if the aprid is arch-pair (sun3.sun3x) only.
*	      -1:   otherwise.
*/
int 
fill_os_ident(os, aprid)
	Os_ident *os;
	char *aprid;
{
	char *cp;

	bzero((char *) os, sizeof(Os_ident));

	if ((cp = extract_str(aprid, os->appl_arch, '.', 1)) == (char *)NULL)
		return(-1);
	if ((cp = extract_str(cp, os->impl_arch, '.', 1)) == (char *)NULL)
		return(0);
	if ((cp = extract_str(cp, os->os_name, '.', 1)) == (char *)NULL)
		return(-1);
	/* Copy the rest to release, and pull realization out of that */
	(void) extract_str(cp, os->release, '\n', 1);
	cp = strstr(os->release, PSR_IDENT);
	if (cp != NULL) {
		strcpy(os->realization, cp);
		*cp = '\0';
	}
	return(1);
}

    
/*
 *	std_execpath
 *
 *	returns the full pathname of the standard place
 *	where the execs should reside
 *	(i.e. /export/exec/sun3.sunos.4.1.0)
 */
char *
std_execpath(os, buf)
	Os_ident *os;
	char *buf;
{
	char buff[MEDIUM_STR];
	
	(void) sprintf(buf, "%s/%s", STD_EXEC_PATH, os_arid(os, buff));
	return(buf);
}


/*
 *	Function : (char  *) proto_root_path()
 *
 *	Returns the full pathname of how proto.root should be named and
 *	where it should  reside.
 *
 *	Be forwarned, this sprintf's to a static buffer and the buffer will
 *	be overwitten each time it is called.
 *
 */
char *
proto_root_path(aprid)
	char *		aprid;		/* the arch_str of the architech. */
{
	char		buf[MEDIUM_STR];
	
	(void) sprintf(pathname, "%s%s.%s",  STD_EXEC_PATH, PROTO_ROOT,
		       aprid_to_rid(aprid, buf));
	return(pathname);
}



/*
*	std_kvmpath
*
*	returns the full pathname of the standard place
*	where the kvms should reside
*	(i.e. /export/exec/kvm/sun3.sunos.4.1.0)
*/
char *
std_kvmpath(os, buf)
	Os_ident *os;
	char *buf;
{
	char buff[MEDIUM_STR];

	(void) sprintf(buf, "%s/%s", STD_KVM_PATH, os_irid(os, buff));
	return(buf);
}


/*
*	std_syspath
*
*	returns the full pathname of the standard place
*	where the kvms should reside
*	(i.e. /export/exec/kvm/sys/sunos.4.1)
*/
char *
std_syspath(os, buf)
	Os_ident *os;
	char *buf;
{
	char buff[MEDIUM_STR];

	(void) sprintf(buf, "%s/%s/sys", STD_KVM_PATH, os_irid(os, buff));
	return(buf);
}

/*
*	aprid_to_execpath
*
*	returns the full pathname of the standard place
*	where the execs should reside
*	(i.e. /export/exec/sun3.sunos.4.1.0)
*/
char *
aprid_to_execpath(aprid, buf)
	char *aprid;
	char *buf;
{
	char id[MEDIUM_STR];

	if ( *(aprid_to_arid(aprid, id)) == '\0')
		return(NULL);
	(void) sprintf(buf, "%s/%s", STD_EXEC_PATH, id);
	return(buf);
}


/*
*	aprid_to_kvmpath
*
*	returns the full pathname of the standard place
*	where the kvms should reside
*	(i.e. /export/exec/kvm/sun3.sunos.4.1.1_PSR_Z)
*/
char *
aprid_to_kvmpath(aprid, buf)
	char *aprid;
	char *buf;
{
	char id[MEDIUM_STR];

	if (*(aprid_to_irid(aprid, id)) == '\0')
		return(NULL);
	(void) sprintf(buf, "%s/%s", STD_KVM_PATH, id);
	return(buf);
}


/*
*	aprid_to_syspath
*
*	returns the full pathname of the standard place
*	where the kvms should reside
*	(i.e. /export/exec/kvm/sys/sunos.4.1)
*/
char *
aprid_to_syspath(aprid, buf)
	char *aprid;
	char *buf;
{
	char id[MEDIUM_STR];
	
	if (*(aprid_to_irid(aprid, id)) == '\0')
		return(NULL);
	(void) sprintf(buf, "%s/%s/sys", STD_KVM_PATH, id);
	return(buf);
}

/*
*	aprid_to_sharepath
*
*	returns the full pathname of the standard place
*	where the share's should reside
*	(i.e. /export/share/sunos.4.1)
* 
*	XXX -	Note that share path should not include a realization string,
*		but SunOS 4.1 did, so we must be broken also, to interoperate
*		properly.
*
*/
char *
aprid_to_sharepath(aprid, buf)
	char *aprid;
	char *buf;
{
	Os_ident os;
	
	if (fill_os_ident(&os, aprid) > 0)  {
#define FIXED_IN_41 1
#ifndef FIXED_IN_41
 		(void) sprintf(buf, "%s/%s.%s%s", STD_SHARE_PATH, os.os_name, 
 			os.release, os.realization );
#else
		(void) sprintf(buf, "%s/%s.%s", STD_SHARE_PATH, os.os_name, 
			os.release );
#endif
		return(buf);
	}
	return(NULL);
}


/*
*	default_release
*
*	return the default release string of a particular release
*	i.e.  /export/exec/sun3 for sun3.sunos.4.1.0
*/
char *
default_release(os, buf)
	Os_ident *os;
	char *buf;
{
	(void) sprintf(buf, "%s/%s", STD_EXEC_PATH, os->appl_arch);
	return(buf);
}



/*
*	custom_default_kvmpath
*
*	returns the full pathname of the default place
*	where the kvm should reside if execpath is a custom one
*	(i.e. exec_path = /foo/bar; kvm_path = /foo/bar/kvm)
*/
char *
custom_default_kvmpath(exec_path, buf)
	char *exec_path;
	char *buf;
{
	(void) sprintf(buf, "%s/kvm", exec_path);
	return(buf);
}


/*
 *	Name:		sys_has_release()
 *
 *	Description:	test to see if the system has a realease yet.
 *
 *	Return Value:	1 : if the system already has a release
 *			0 : if the system does not yet have a release
 *
 */
boolean
sys_has_release(sys_p)
	sys_info *	sys_p;			/* system information */
{
	Os_ident	sys_os;			/* system information */

	/*
	 *	Let's check to see if we have selected an os for this
	 *	architecture for this system already. If not, see how many
	 *	could fit it and only display those, else display all of
	 *	them.  By this point fill_os_ident() better not return a -1.
	 */
	if (fill_os_ident(&sys_os, sys_p->arch_str) <= 0)
		return(0);
	else
		return(1);

} /* end sys_has_release() */
