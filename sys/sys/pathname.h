/*	@(#)pathname.h 1.1 92/07/30 SMI 	*/

/*
 * Pathname structure.
 * System calls which operate on path names gather the
 * pathname from system call into this structure and reduce
 * it by peeling off translated components.  If a symbolic
 * link is encountered the new pathname to be translated
 * is also assembled in this structure.
 */

#ifndef _sys_pathname_h
#define _sys_pathname_h

struct pathname {
	char	*pn_buf;		/* underlying storage */
	char	*pn_path;		/* remaining pathname */
	u_int	pn_pathlen;		/* remaining length */
};

#define PN_STRIP 0x00		/* Strip next component off pn */
#define PN_PEEK	0x01  		/* Only peek at next pn component */
#define pn_peekcomponent(PNP, COMP) pn_getcomponent(PNP, COMP, PN_PEEK)
#define pn_stripcomponent(PNP, COMP) pn_getcomponent(PNP, COMP, PN_STRIP)

#define	pn_peekchar(PNP) 	((PNP)->pn_pathlen!=0?*((PNP)->pn_path):(char)0)
#define pn_pathleft(PNP)	((PNP)->pn_pathlen)
#define pn_getpath(PNP)		((PNP)->pn_path)
#define pn_copy(PNP1, PNP2)	(pn_set(PNP2, pn_getpath(PNP1)))

extern int	pn_alloc();		/* allocat buffer for pathname */
extern int	pn_get();		/* allocate buf and copy path into it */
#ifdef notneeded
extern int	pn_getchar();		/* get next pathname char */
#endif
extern int	pn_set();		/* set pathname to string */
extern int	pn_combine();		/* combine to pathnames (for symlink) */
extern int	pn_getcomponent();	/* get next component of pathname */
extern void	pn_skipslash();		/* skip over slashes */
extern void	pn_free();		/* free pathname buffer */
extern int	pn_append();		/* Append string to pathname */
extern int	pn_getlast();		/* Get last component of pathname */ 

#endif /*!_sys_pathname_h*/
