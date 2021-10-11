/*	@(#)ftw.h 1.1 92/07/30 SMI; from S5R2 1.1	*/

/*
 *	Codes for the third argument to the user-supplied function
 *	which is passed as the second argument to ftw
 */

#ifndef _ftw_h
#define _ftw_h

#define	FTW_F	0	/* file */
#define	FTW_D	1	/* directory */
#define	FTW_DNR	2	/* directory without read permission */
#define	FTW_NS	3	/* unknown type, stat failed */

#endif /*!_ftw_h*/
