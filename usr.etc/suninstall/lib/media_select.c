#ifndef lint
#ifdef SunB1
#ident			"@(#)media_select.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)media_select.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

#include "install.h"

/*
 *	Name:		reset_selected_media()
 *
 *	Description:	to reset the "selected" field to all "ANS_NO"'s
 *
 *	Return Value: 	none
 */

void
reset_selected_media(soft_p)
	soft_info *	soft_p;
{
	media_file *	mf_p;

	for (mf_p = soft_p->media_files;
	     mf_p < &soft_p->media_files[soft_p->media_count] ;
	     mf_p++ )
		mf_p->mf_select = ANS_NO;

} /* end reset_selected_media() */



 
/*
 *	Name:		none_selected()
 *
 *	Description:	test selections
 *
 *	Return Value: 	1 : if no selection is made
 *			0 : if selections have been made.
 */

int
none_selected(soft_p)
	soft_info *	soft_p;
{
	media_file *	mf_p;
	int		selected_media = 0;
	
	for (mf_p = soft_p->media_files;
	     mf_p < &soft_p->media_files[soft_p->media_count] ;
	     mf_p++ ) {
		if (mf_p->mf_select == ANS_YES)
			selected_media++;
	}

	if (selected_media > 0)
		return(0);
	else
		return(1);
} /* end reset_selected_media() */



