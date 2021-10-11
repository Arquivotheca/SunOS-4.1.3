/*	@(#)mtab.h 1.1 92/07/30 SMI; from UCB 4.4 83/05/28	*/

/*
 * Mounted device accounting file.
 */

#ifndef _mtab_h
#define _mtab_h

struct mtab {
	char	m_path[32];		/* mounted on pathname */
	char	m_dname[32];		/* block device pathname */
	char	m_type[4];		/* read-only, quotas */
};

#endif /*!_mtab_h*/
