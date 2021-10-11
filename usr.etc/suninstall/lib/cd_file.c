#ifndef lint
#ifdef SunB1
static  char    sccsid[] = 	"@(#)cd_file.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static  char    sccsid[] = 	"@(#)cd_file.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1990 Sun Microsystems, Inc.
 */
 
/*
 *	Name:		cd_file.c
 *
 *	Description:	This file contains the routines necessary for
 *		interfacing with the CDROM naming scheme. 
 */	

#include "install.h"
#include <string.h>
#include <ctype.h>


/*
 *	This SI_ISO_CDROM_STD is to be uncommented, only if the ISO std for
 *	CDROM filenames for suninstall will be used, otherwise ufs is
 *	assumed.
 */
#define SI_ISO_CDROM_STD


/*
 *	Local global variables
 */
static char 	pathname[MAXPATHLEN];	/* for generic  storage of pathnames */
static char 	tmp_buffer[MAXPATHLEN];	/* for generic  storage of pathnames */


/*
 *	Structure for load_pt type pathname conversion array, "stdpaths",
 *	below.  The stdpath array holds pointers to the functions for the
 *	creation of std pathnames as have already been defined in lib/apr.c,
 *	the application pair architecture/multiple OS lib file.
 */

typedef struct file_paths {
	char *	load_pt_type;
	char *	(*path_funct)();
} file_path;

static file_path	stdpaths[] = {
	"root",		proto_root_path,
	"share",	aprid_to_sharepath,
	"appl",		aprid_to_execpath,
	"impl",		aprid_to_kvmpath,
	(char *)NULL,	NULL,
};




/*
 *	Local functions
 */

static char *	form_std_cd_path();
static char *	ISO_translate();


/*
 *	Name:		(static char *)ISO_translate()
 *
 *	Description:	This function is used only if the ISO 9660 file
 *			naming standard is implemented, otherwise it's a
 *			bogus function, just returning its argument.
 *
 *			Its purpose is to :
 *
 *			1. translate all '.' (period) characters into '_'
 *			   (underbars), because the ISO standard only supports
 *			   1 '.' character and 31 letters in a filename.
 *
 *			2. translate all characters to lower case, which is
 *			   the only way that ISO 9660 will display file names.
 *
 *	Return Value : 	(char *)path, the possibly translated buffer pathname.
 *			
 */
static char *
ISO_translate(path)
	char *	path;
{
#ifdef SI_ISO_CDROM_STD
	char *	cp;

	for (cp = path; *cp != '\0'; cp++) {
		if (*cp == '.')
			*cp = '_';
		else {
			if (isupper(*cp))
				*cp = tolower(*cp);
		}
	}
#endif /*  SI_ISO_CDROM_STD */

	return(path);
}


/*
 *	Name:		(char *)std_cd_path()
 *
 *	Description:	This function takes a load_pt type and loops through
 *			the stdpaths[] array and calls form_std_cd_path() to
 *			do the actual translation 
 *
 *	Return Value : 	(char *) to pathname, a static char array, that will
 *			change every time this function is called. pathname
 *			will contain the std pathname.
 *
 *			null string pathname, to avoid loadpts that are not
 *			found. 
 *
 *			
 */
char *
std_cd_path(load_pt, aprid, file_name)
	char	*load_pt;
	char	*aprid;
	char	*file_name;
{
	file_path *	path_p;
	
	/*
	 *	Compare all std types
	 */
	
	for (path_p = stdpaths;
	     path_p->load_pt_type != (char *)NULL;
	     path_p++) {
		if (strcmp(path_p->load_pt_type, load_pt) == 0)
			return(ISO_translate(
				form_std_cd_path(path_p, aprid, file_name)));
	}
	*pathname = '\0';
	return(pathname);
}




/*
 *	Name:		(char *)form_std_cd_path()
 *
 *	Description:	This function takes an aprid, and converts it into a
 *			standard pathname for the load_pt_type, as defined
 *			in the static array cdpaths, defined above.
 *
 *			Because of a slight awkwardness of implementation,
 *			the aprid_to_execpath(), aprid_to_kvmpath(), and
 *			aprid_to_sharepath() routines require an extra buffer
 *			This is why the "root" type is special cased out,
 *			because it does not require the extra buffer.
 *
 *	Return Value : 	(char *) standard path name for cd software modules
 *
 *			
 */
static char *
form_std_cd_path(path_p, aprid, file_name)
	file_path *	path_p;
	char *		aprid;
	char *		file_name;
{
	if (strcmp("root", path_p->load_pt_type) == 0) {
		return(proto_root_path(aprid));
	} else {
		/*
		 *	All functions but the root path have the same
		 *	function call specs, so we don't need to know what
		 *	kind of path is needed here.
		 *
		 *	The aprid had better be valid by this point, which
		 *	is a valid assumption.
		 */
		(void) sprintf(pathname, "%s/%s",
			       (*(path_p->path_funct))(aprid, tmp_buffer),
			       file_name);
		return(pathname);
	}

	/* NOTREACHED */
}

