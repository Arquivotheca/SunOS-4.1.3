#ifndef lint
#ifdef SunB1
static char    sccsid[] = 	"@(#)cd_tar.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static char    sccsid[] = 	"@(#)cd_tar.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *      Copyright (c) 1990 Sun Microsystems, Inc.
 */

/****************************************************************************
 *
 *      Name:           cd_tar.c
 *
 *      Description:    This file contains all the routines necessary for
 *			creating a CDROM directory structure for placing
 *			software categories appropriately 
 *
 *			The format is basically the same as the /export
 *			structure in the normal file system.  The
 *			std_cdrom_path() function, used in suninstall, is
 *			used to create the path names, so we  are ensured
 *			to always use the correct names.
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "install.h"


/*
 *	Entry type definitions
 */
#define	DIRECTORY_T	1
#define	FILE_T		2


/*
 *	User interface constants.
 *
 *	-d debug is undocumented
 *		(it will skip copying the modules listed install_tar_files() 
 *		 modules to speed things up for testing)
 *
 */
#define NUM_ARGS  4
#define USAGE \
"\
usage: cd_tar [-v][-d] tar_directory proto_directory XDRTOC_path\
 MINIROOT_path\n\
  where :\n\
    -v              prints verbose output on copying.\n\
    -d              debug mode (will skip copying some software categories.\n\
			including 'usr', 'Demo', 'Games'...)\n\
    tar_directory   is the directory for release Categories in tar format\n\
    proto_directory is the directory to set up Categories in a directory\n\
	     	    	structure suitable for the CDROM image\n\
    XDRTOC_path     is the full path of the XDRTOC file for the current\n\
		        architecture\n\
    MINIROOT_path   is the path to the pre-built MINIROOT image\n"
		

/*
 *	Local functions
 */
int		chk_ent();			/* check if a  dir. or file */
int		copy_file();			/* do the actual cp */
int		install_tar_files();		/* ins. software Categories */
int		install_other_files();		/* ins. XDRTOC and MINIROOT */
int		read_toc();			/* read the XDRTOC */
void		usage();			/* shows USAGE macro to user */

/*
 *	Local variables
 */
short		vflag = 0;			/* verbose flag */
short		debug = 0;			/* debug flag */
char		cmd[MAXPATHLEN * 3];		/* generic temp buffer */
char *		progname;			/* name of program */ 

/*
 *	Pointers to command line arguments and sizes
 */
char *		tar_directory;			/* location of tar files */
char *		proto_directory;		/* location of dir struct */
char *		XDRTOC_path;			/* XDRTOC file */
long		XDRTOC_size;			/* size of XDRTOC */
char *		MINIROOT_path;			/* location of MINIROOT */
long		MINIROOT_size;			/* size of MINIROOT */

long		temp_size;			/* temp size var */

main(argc, argv)
	int	argc;
	char **	argv;
{
	soft_info 	soft;			/* holds TOC info */
	char		pathname[MAXPATHLEN];	/* pathname buffer */

	bzero((char *)&soft, sizeof(soft));

	progname = basename(argv[0]);

	(void) umask(UMASK);

	/*
	 *	Only superuser can do damage with this command
	 */
	if (suser() != 1) {
		(void) fprintf(stderr, "%s: must be superuser\n", progname);
		exit(1);
	}

	argv++;
	argc--;

	
	/*
	 *	To be verbose or not be verbose, that is the question
	 */
	while ((argc >= 1) && (**argv == '-')) {
		switch (*(*argv + 1)) {
		case 'v' :
			vflag++;
			break;
		case 'd' :
			debug++;
			break;
		}
		argc--;
		argv++;
	}
	
	/*
	 *	All command line arguments are required, except for the
	 *	-v and -d flag. 
	 */
	if (argc != NUM_ARGS)
		usage();


	/*
	 *	Check for the tar directory
	 */
	if (chk_ent(*argv, DIRECTORY_T) != 0)
		exit(1);
	tar_directory = *argv;

	/*
	 *	Check for the proto directory
	 */
	if (chk_ent(*(++argv), DIRECTORY_T) != 0)
		exit(1);
	proto_directory = *argv;

	/*
	 *	Check for the XDRTOC image file
	 */
	if (chk_ent(*(++argv), FILE_T) != 0)
		exit(1);
	XDRTOC_path = *argv;
	XDRTOC_size = temp_size;

	/*
	 *	Check for the MINIROOT image file
	 */
	if (chk_ent(*(++argv), FILE_T) != 0)
		exit(1);
	MINIROOT_path = *argv;
	MINIROOT_size = temp_size;

	
	/*
	 *	read the XDRTOC file for list of tar files.
	 */
	if (read_toc(&soft) != 1)
		exit(1);

	/*
	 *	All is honky dory by here.
	 */
	
	if (install_tar_files(&soft) != 1)
		exit(1);

	if (install_other_files(&soft) != 1)
		exit(1);

	/*
	 *	Now let's update AVAIL_ARCHES file
	 */
	(void)sprintf(pathname, "%s/%s", proto_directory,
		      basename(AVAIL_ARCHES));

	add_key_entry(soft.arch_str, "", pathname, KEY_ONLY);

	/*
	 *	Let's clean up memory
	 */
	free_media_file(&soft);

	exit(0);

	/* NOTREACHED */
}

	
/****************************************************************************
 *
 *	Function: 	(int) read_toc()
 *
 *	Description: 	read the XDRTOC file
 *
 *	Return Value: 	 1 : if all goes well
 *			-1 : if there was an error.
 *
 ****************************************************************************
 */
int
read_toc(soft_p)
	soft_info *	soft_p;
{
	sys_info 	sys;
	int		ret_code;

	bzero((char *)&sys, sizeof(sys));
	
	/*
	 *	we know XDRTOC already exists
	 */

	ret_code = toc_xlat(&sys, XDRTOC_path, soft_p);

	if (ret_code != 1)
		(void) fprintf(stderr, "%s: could not read file : %s\n",
			       progname, XDRTOC_path);

	return(ret_code);
}


/****************************************************************************
 *
 *	Function: 	(int) install_tar_files()
 *
 *	Description: 	Install the software Category tar files
 *
 *	Return Value:	 1 : if all is well and good
 *			-1 : if all is not good
 *
 ****************************************************************************
 */
int
install_tar_files(soft_p)
	soft_info *	soft_p;
{
	media_file *	mf_p;			/* tmp media ptr */
	char 		pathname[MAXPATHLEN];	/* temp path buffer */

	/*
	 *	We now have the list in soft_p->media_files
	 */

	for (mf_p = soft_p->media_files; mf_p &&
	     mf_p < &soft_p->media_files[soft_p->media_count]; mf_p++) {
		
		if (debug) {
			if ((strcmp("usr", mf_p->mf_name) == 0)              ||
			    (strcmp("Versatec", mf_p->mf_name) == 0)         ||
			    (strcmp("Demo", mf_p->mf_name) == 0)             ||
			    (strcmp("System_V", mf_p->mf_name) == 0)         ||
			    (strcmp("User_Diag", mf_p->mf_name) == 0)        ||
			    (strcmp("Debugging", mf_p->mf_name) == 0)        ||
			    (strcmp("Security", mf_p->mf_name) == 0)         ||
			    (strcmp("SunView_Programmers",mf_p->mf_name)== 0)||
			    (strcmp("SunView_Demo",mf_p->mf_name) == 0)      ||
			    (strcmp("Games", mf_p->mf_name) == 0)) {
				(void) printf(
				   "%s: debug mode : skipping tar file %s\n",
					      progname, mf_p->mf_name);
				continue;
			}
		}
		
		(void) sprintf(pathname, "%s/%s", tar_directory,mf_p->mf_name);

		if (copy_file(pathname, mf_p->mf_loadpt, soft_p->arch_str,
			      mf_p->mf_size) != 0)
			return(-1);
		
	} /* end for */

	return(1);

} /* end install_tar_files */



/****************************************************************************
 *
 *	Function: 	(int) install_other_files()
 *
 *	Description: 	Install the MINIROOT and XDRTOC files
 *
 *	Return Value:	 1 : if all is well and good
 *			-1 : if all is not good
 *
 ****************************************************************************
 */
int
install_other_files(soft_p)
	soft_info *	soft_p;
{
	/*
	 *	Finally, copy the XDRTOC to the implementation specific
	 *	location
	 */
	if (copy_file(XDRTOC_path, "impl", soft_p->arch_str,
		      XDRTOC_size) != 0)
		return(-1);
	
	/*
	 *	Finally, copy the MINIROOT image to the implementation
	 *	specific location
	 */
	if (copy_file(MINIROOT_path, "impl", soft_p->arch_str,
		      MINIROOT_size) != 0)
		return(-1);

	return(1);
}


/****************************************************************************
 *
 *	Function: 	(int) chk_ent()
 *
 *	Description: 	test if "entry" exists and is a directory or file,
 *			depending on the entry_type.
 *
 *	Return Value:	 0 : if is a directory and exists
 *			-1 : otherwise
 *
 ****************************************************************************
 */
int
chk_ent(entry, entry_type)
	char *	entry;			/* name of entry */
	int	entry_type;		/* type of entry (FILE or DIRECTORY) */
{
	struct stat	buf;
	char		pathname[MAXPATHLEN + MAXNAMLEN]; /* tmp path buf */
	
	/*
	 *	If stat fails, something is definitly amiss
	 */
	if (stat(entry, &buf) == 0) {
		switch(entry_type) {
		case DIRECTORY_T :
			/*
			 *	If it's not a directory, so  spit it out.
			 */
			if (S_ISDIR(buf.st_mode))
				return(0);
			else {
				(void) fprintf(stderr,
					       "%s: %s : not a directory\n",
					       progname, entry);
			}
			
			break;
		case FILE_T :
			/*
			 *	If it's not a file, so spit it out.
			 */
			if (S_ISREG(buf.st_mode)) {
				temp_size = (long)buf.st_size;
				return(0);
			} else {
				(void) fprintf(stderr, "%s: %s : not a file\n",
					       progname, entry);
			}
			break;
		} /* end switch */
	} else {
		/*
		 *	Stat failed
		 */
		(void) sprintf(pathname, "%s: %s ", progname, entry);
		perror(pathname);
	}  /* end if stat() */
	

	/*
	 *	If we reached here, it's an error
	 */
	return(-1);
	
} /* end chk_ent() */


/****************************************************************************
 *
 *	Function: 	(int) copy_file()
 *
 *	Description: 	copy file to the proto_directory. The filename must
 *			be the a full path to the file
 *
 *	Return Value:	 0 : if all is ok
 *			-1 : if an error occurs
 *
 ****************************************************************************
 */
int
copy_file(filename, loadpt, arch_str, size)
	char *		filename;		/* full path of file to cp */
	char *		loadpt;			/* kind of path */
	char *		arch_str;		/* arch of  files*/
	long		size;			/* size in bytes of file */
{
	char *		cd_path;		/* tmp char * */
	char *		cp;			/* tmp char * */
	char		pathname[MAXPATHLEN];	/* generic temp buffer */


	/*
	 *	Get the basic proto path
	 */
	cd_path = std_cd_path(loadpt, arch_str, basename(filename));
	
	/*
	 *	Make sure we find a '/'
	 */
	if ((cp = strrchr(cd_path, '/')) == (char *)NULL) {
		(void)fprintf(stderr, "%s: not '/' in %s\n", progname,
			      cd_path);
		return(-1);
	}

	*cp = '\0';		/* strip filename to leave pathname */
	
	(void) sprintf(pathname, "%s%s", proto_directory, cd_path);
	
	mkdir_path(pathname);	/* if it exists, so what */
	
	*cp = '/';		/* restore to full path and filename */
	
	(void) strcat(pathname, cp);

	/*
	 *	To avoid over working, skip files that have already
	 *	been copied over
	 */
	if (access(pathname, F_OK) == 0) {
		(void) printf("%s: %s exists already, skipping copy\n",
			      progname, pathname);
		return(0);
	}
	
	(void)sprintf(cmd, "cp %s %s", filename, pathname);
	
	if (vflag)
		(void) printf("%s: %s, size = %1.1lf Meg\n", progname,
			      cmd, (size * 1.0)/MEGABYTE );
	
	if (system(cmd) != 0) {
		(void) fprintf(stderr, "%s: copy failed : %s\n",
			       progname, cmd);
		return(-1);
	}

	return(0);

} /* end copy_file() */


/****************************************************************************
 *
 *	Function: 	(void) usage()
 *
 *	Description: 	display a usage message and exit.
 *
 *	Return Value:	none
 *
 ****************************************************************************
 */
void
usage()
{
	puts(USAGE);
	exit(1);
}
