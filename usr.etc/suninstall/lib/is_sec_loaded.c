#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)is_sec_loaded.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)is_sec_loaded.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		is_sec_loaded.c
 *
 *	Description:	Is Security loaded on the system?  Returns 1 if
 *		it is and 0 if it is not.
 */

#include "install.h"
#include "menu.h"




/*
 *	External functions:
 */
extern	char *		sprintf();


int
is_sec_loaded(arch_str)
	char*		arch_str;
{
#ifdef SunB1
#ifdef lint
	arch_str = arch_str;
#endif /* lint */

	return(1);
#else
#ifndef CONFIG_SEC
#ifdef lint
	arch_str = arch_str;
#endif /* lint */
	/*
	 *	If we are not supposed to configure security, then just
	 *	lie ans say it isn't loaded.
	 */
	return(0);
#else
	int		loaded = 0;		/* is Security loaded? */
	media_file *	mf_p;			/* ptr to a media file */
	char		pathname[MAXPATHLEN];	/* path to media_file.<arch> */
	char		arid[MEDIUM_STR];	/* appl. arch/release id */
	soft_info	tmp;			/* scratch soft_info */


	bzero((char *) &tmp, sizeof(tmp));
	(void) sprintf(pathname, "%s.%s", MEDIA_FILE, 
			aprid_to_arid(arch_str, arid));

	switch (read_media_file(pathname, &tmp)) {
	case 0:
		menu_log("%s: %s: cannot read file.", progname, pathname);
		/*
		 *	Fall through here
		 */
	case -1:
		menu_abort(1);

	case 1:
		break;
	}

	/*
	 *	Search the media list for "Security"
	 */
	for (mf_p = tmp.media_files; mf_p &&
	     mf_p < &tmp.media_files[tmp.media_count]; mf_p++) {

		if (strcmp(mf_p->mf_name, "Security") == 0) {
			if (mf_p->mf_select == ANS_YES ||
			    mf_p->mf_loaded == ANS_YES)
				loaded = 1;

			break;
		}
	}

	free_media_file(&tmp);

	return(loaded);
#endif /* CONFIG_SEC */
#endif /* SunB1 */
} /* end is_sec_loaded() */
