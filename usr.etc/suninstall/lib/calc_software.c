#ifndef lint
#ifdef SunB1
#ident			"@(#)calc_software.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)calc_software.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		calc_software()
 *
 *	Description:	Calculate the amount of disk space used by software
 *		for the architecture 'arch_str'.  Reads the media file for
 *		the architecture and calls update_bytes() to do the disk
 *		space management.
 *
 *		Returns 1 if everything is okay, 0 if there is insufficent
 *		disk space and -1 if there was an error.
 */

#include "install.h"
#include "menu.h"



/*
 *	External references:
 */
extern	char *		sprintf();


int
calc_software(arch_p, arch_str, sys_p, loadpt)
	arch_info *	arch_p;
	char *		arch_str;
	sys_info *	sys_p;
	char *		loadpt;
{
	media_file *	mf;			/* ptr to media file entry */
	char *		part_p;			/* ptr to partition name */
	char		pathname[MAXPATHLEN];	/* scratch pathname */
	int		ret_code;		/* return code */
	soft_info	soft;			/* soft information */


	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &soft, sizeof(soft));

	(void) sprintf(pathname, "%s.%s", SOFT_INFO, arch_p->arch_str);
	ret_code = read_soft_info(pathname, &soft);
	/*
	 *	Only a return code of 1 is okay here.  Must add error
	 *	message for a return code of 0.  Non-fatal errors are
	 *	not okay here.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 pathname);
		return(-1);
	}

	if (strcmp(loadpt, "impl") == 0)
                (void) sprintf(pathname, "%s.%s", MEDIA_FILE, arch_str);
	else
	        (void) sprintf(pathname, "%s.%s", APPL_MEDIA_FILE, arch_str);
	ret_code = read_media_file(pathname, &soft);

	/*
	 *	Only a return code of 1 is okay here.  Must add error
	 *	message for a return code of 0.  Non-fatal errors are
	 *	not okay here.
	 */
	if (ret_code != 1) {
		if (ret_code == 0)
			menu_log("%s: %s: cannot read file.", progname,
				 pathname);
		return(-1);
	}

	for (mf = soft.media_files;
	     mf && mf < &soft.media_files[soft.media_count]; mf++) {

		/*
		 *	Skip this media file if it is not selected and
		 *	if it is not loaded.
		 */
		if ((mf->mf_select == ANS_NO) ||
		    strcmp(mf->mf_loadpt, loadpt))
			continue;

		part_p = find_mf_part(&soft, mf, sys_p);

		/*
		 *	Can't find the partition name.  Error message provided
		 *	by find_part().
		 */
		if (part_p == NULL)
			return(-1);

		ret_code = update_bytes(part_p, FUDGE_SIZE(mf->mf_size));
		/*
		 *	Only a return code of 1 is okay here.  update_bytes()
		 *	handles any error messages.  Non-fatal errors are
		 *	okay here so return the actual code.
		 */
		if (ret_code != 1) {
			return(ret_code);
		}
	}

	return(1);
} /* end calc_software() */
