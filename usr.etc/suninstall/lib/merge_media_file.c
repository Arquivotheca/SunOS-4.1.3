#ifndef lint
#ifdef SunB1
#ident			"@(#)merge_media_file.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)merge_media_file.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		merge_media_file.c
 *
 *	Description:	This file contains the routines necessary for
 *		merging the appl_media_file and media_files into
 *		a media_file structure.
 */

#include <stdio.h>
#include "install.h"
#include "menu.h"


static void 		peruse_media_lists();
static media_file *	find_media_module();


/*
 *	Name:		merge_media_file()
 *
 *	Description:	merge the named media files into the contents of
 *		'soft_p', so we can get the values of mf_loaded and
 *		mf_select correct.
 *
 *	Return Value : 	 1 : if everything is okay
 *			-1 : if there was an error.
 */

int
merge_media_file(aprid, arid, soft_p)
	char *		aprid;
	char *		arid;
	soft_info *	soft_p;
{
	media_file *	new_p;			/* ptr to impl. media files */
	media_file *	ar_p;			/* ptr to appl. media files */
	soft_info	ar;			/* scratch software info */


	/*
	 *	Software info structure must be initialized before the
	 *	first call to read_software_info() since it allocates
	 *	and frees its own run-time memory.
	 */
	bzero((char *) &ar, sizeof(ar));
    
	if (read_media_file(aprid, soft_p) == 1 &&
	    read_media_file(arid, &ar) == 1)  {
		for (ar_p = ar.media_files;
		     ar_p < &ar.media_files[ar.media_count] ; ar_p++ ) {

			/*
			 *	This media file is not interesting since
			 *	it is implementation architecture module.
			 */
			if (strcmp(ar_p->mf_loadpt, "impl") == 0)
				continue;

			/*
			 *	This media file is not interesting since
			 *	it is root module.
			 */
			if (strcmp(ar_p->mf_loadpt, "root") == 0)
				continue;

			/*
			 *	Find the media file in the new list and
			 *	mark it as the one in appl_media_file.
			 *      Silently ignores a media file that is in 
			 *	the appl. list, but not in the new list.
			 */
			for (new_p = soft_p->media_files;
			     new_p < &soft_p->media_files[soft_p->media_count];
			     new_p++) {
				if (strcmp(ar_p->mf_name, new_p->mf_name))
					continue;

				new_p->mf_loaded = ar_p->mf_loaded;
				new_p->mf_select = ar_p->mf_select;
			}
		}
	}
	else
		return(-1);

	free_media_file(&ar);

	return(1);
} /* end merge_media_file() */


/*
 *	Name:		split_media_file()
 *
 *	Description:	split the named media files into appl_media_file and
 *		impl_media_file, overwriting the information in the old
 *		appl_media_file. This is only useful after add_services,
 *		when the software module data is marked as loaded and not
 *		selected.
 *
 *	Return Value : 	 1 : if everything is okay
 *			-1 : if there was an error.
 */

int
split_media_file(aprid, arid, soft_p)
	char *		aprid;
	char *		arid;
	soft_info *	soft_p;
{
	if (save_media_file(aprid, soft_p) != 1)
		return(-1);
	if (save_media_file(arid, soft_p) != 1)
		return(-1);
	return(1);
} /* end split_media_file() */




/*
 *	Name:		info_split_media_file()
 *
 *	Description:	split the named media files into appl_media_file and
 *		media_file, while not loosing the info already in the
 *		appl_media_file. This is only called in config_ok() in
 *		soft_form.c (from the software form).  Because the
 *		selections are made during the looping of the
 *		software_modules, some modules that had previously been
 *		selected become deselected. Therefore, we read all the
 *		media_files for that application arch and remake the
 *		appl_media_file so as not to loose the selections already
 *		chosen.
 *
 *	Return Value : 	 1 : if everything is okay
 *			-1 : if there was an error.
 */
int
info_split_media_file(aprid, arid, soft_p)
	char *		aprid;
	char *		arid;
	soft_info *	soft_p;
{
	soft_info	appl_arch;

	bzero((char *) &appl_arch, sizeof(appl_arch));

	/*
	 *	Save the current selections for this aprid
 	 */	

	if (save_media_file(aprid, soft_p) != 1)
		return(-1);
	

	/*
	 *	Read the appl_media_file for this appl arch
	 */

	if (read_media_file(arid, &appl_arch) != 1)
		return(-1);


	/*
	 *	Now we must make sure the appl_media_file is up to date. We
	 *	read it in and reset the select values, to make sure nothing
	 *	is selected that shouldn't be.
	 *
	 */

	/*
	 *	Set select fields to "no", so that later we reset them
	 *	correctly. 
	 */
	reset_selected_media(&appl_arch);

	/*
	 *	set up appl_media_file with the values from the
	 *	media_files.aprid's
	 */
	if (reset_appl_media_file(soft_p, &appl_arch) != 1)
		return(-1);


	/*
	 *	Now let's save the new corrected data in appl_media_file.
	 */
	if (save_media_file(arid, &appl_arch) != 1)
		return(-1);
	
	return(1);
} /* end info_split_media_file() */


/*
 *	Name:		reset_appl_media_file()
 *
 *	Description:	Read in all the architectures and pass them
 *		appropriately to peruse_media_lists(), which will actually
 *		reset the load and select values correctly for the
 *		appl_media_file list.
 *
 *	Return Value: 	 1 : if all is ok.
 *			-1 : if all is not ok.
 */
static int
reset_appl_media_file(current_soft_p, appl_soft_p)
	soft_info *	current_soft_p;
	soft_info *	appl_soft_p;
{

	arch_info *	arch_list;
    	arch_info *	ap;
	soft_info 	arch_soft;
	char		buf[MEDIUM_STR];
	char		buf1[MEDIUM_STR];
	char		pathname[MAXPATHLEN];

	bzero((char *) &arch_soft, sizeof(arch_soft));


	if (read_arch_info(ARCH_INFO, &arch_list) != 1)	{
		menu_log("Cannot read arch_list file.");
		return(-1);
	}
	for (ap = arch_list; ap ; ap = ap->next)    {

		/*
		 *	If application/rel/real dont' match, ignore it
		 */
		if (strcmp(aprid_to_arid(ap->arch_str, buf),
			   aprid_to_arid(appl_soft_p->arch_str, buf1)) != 0)
			continue;

		(void) sprintf(pathname, "%s.%s", MEDIA_FILE, ap->arch_str);
		if (read_media_file(pathname, &arch_soft) != 1)
			return(-1);

		/*
		 *	Correct the data
		 */
		peruse_media_lists(&arch_soft, current_soft_p, appl_soft_p);


	}
	free_arch_info(arch_list);
	return(1);

} /* end reset_appl_media_file() */



/*
 *	Name:		peruse_media_lists()
 *
 *	Description:	merge the appl_media_files.sunX.* into on
 *		appl_media_file to use as the standard by which to load all
 *		media.
 *
 *	Return Value: 	none
 */
static void
peruse_media_lists(arch_soft_p, current_soft_p, appl_soft_p)
	soft_info *	arch_soft_p;	/* loop arch soft_info */
	soft_info *	current_soft_p;	/* the one we are currently adding */
	soft_info *	appl_soft_p;	/* the appl_arch media file */
{
	media_file *	arch_media_p;
	media_file *	current_media_p;
	media_file *	appl_media_p;
	
	for (appl_media_p = appl_soft_p->media_files;
	     appl_media_p <
	     	&appl_soft_p->media_files[appl_soft_p->media_count];
	     appl_media_p++) {

		/*
		 * 	forget about implementation dependant files
		 */
		if (strcmp(appl_media_p->mf_loadpt, "impl") == 0)
			continue;
		/*
		 * 	forget about implementation dependant files
		 */
		if (strcmp(appl_media_p->mf_loadpt, "root") == 0)
			continue;
		
		/*
		 *	Find the current media list module (the one we are
 		 *	adding now)
		 */
		current_media_p = find_media_module(appl_media_p->mf_name,
						    current_soft_p); 
		if (current_media_p == (media_file *)NULL)
			continue;
		
		/*
		 *	Find the arch media list module (the one we are
 		 *	looping through in the arch list)
		 */
		arch_media_p = find_media_module(appl_media_p->mf_name,
						 arch_soft_p); 
		if (arch_media_p == (media_file *)NULL)
			continue;
		
		/*
		 *	This is the hard part, figuring out what the correct
 		 *	values  of select and loaded should be in the
 		 *	appl_media_file. Pay attention to the inner
		 *	comments and that will help. 
		 */

		if (arch_media_p->mf_loaded == ANS_YES) {

			/*
			 *	the looping arch module is loaded, so the
		 	 *	appl_arch_media module must be loaded, so we
		 	 *	set it to yes.
			 */
			appl_media_p->mf_loaded = ANS_YES;

			/*
			 *	And it should not be selected, unless, the
		 	 *	current media files says it should be. (so
		 	 *	we don't double select it by accident and
		 	 *	overwrite what was loaded, but only when
		 	 *	forced)  
		 	 */	
			if (current_media_p->mf_select == ANS_YES)
				appl_media_p->mf_select = ANS_YES;
			else
				appl_media_p->mf_select = ANS_NO;
		} else {
			/*
			 *	remember, appl_media_p->mf_select is
			 *	initially set to ANS_NO, so if it was not
		 	 *	loaded, it should only be marked selected if
		 	 *	one of the appl_archs selected it.
			 */
			if (appl_media_p->mf_loaded != ANS_YES)  {
				if (arch_media_p->mf_select == ANS_YES)
					appl_media_p->mf_select = ANS_YES;
			}
			
			/*
			 *	Only if the current software says you want
			 *	to load it  again, then select it here, even
			 *	if it is already loaded.
			 *
			 *	This can shoot yourself in the foot, but the
		 	 *	software screen asks the user specifically
		 	 *	if that's what he wants to do (i.e. re-load
		 	 *	the software).
			 */
			if (current_media_p->mf_select == ANS_YES)
				appl_media_p->mf_select = ANS_YES;
		}
	}
} /* end peruse_media_lists() */



/*
 *	Name:		find_media_module()
 *
 *	Description:	find the software module named mod_name in the
 *			media_list that is pointed to by soft_p->media_files
 *
 *	Return Value: 	pointer to media_file structure if found.
 *			NULL pointer if not found
 */

static media_file *
find_media_module(mod_name, soft_p)
	char *		mod_name;
	soft_info *	soft_p;
{
	static media_file *	new_p;

	for (new_p = soft_p->media_files;
	     new_p < &soft_p->media_files[soft_p->media_count];
	     new_p++) {
		if (strcmp(mod_name, new_p->mf_name) == 0)
			return(new_p);
	
	}
	/* if the module can't be found, which should never happen */
	return((media_file *)NULL); 

} /* end find_media_module() */









