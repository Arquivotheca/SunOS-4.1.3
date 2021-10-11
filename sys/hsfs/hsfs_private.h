/*      @(#)hsfs_private.h 1.1 92/07/30 SMI */

/*
 * Private definitions for High Sierra filesystem
 * Copyright (c) 1989 by Sun Microsystem, Inc.
 */

/*
 * global routines.
 */
extern int hs_readsector();		/* read a sector */

extern struct vnode *hs_makenode();	/* lookup/construct an hsnode/vnode */
extern int hs_remakenode();		/* make hsnode from directory lbn/off */
extern int hs_dirlook();		/* lookup name in directory */
extern struct vnode *hs_findhash();	/* find an hsnode in the hash list */
extern void hs_freenode();		/* destroy an hsnode */
extern void hs_freehstbl();		/* destroy the incore hnode table */

extern int hs_parsedir();		/* parse a directory entry */
extern int hs_namecopy();		/* convert d-characters */
extern void hs_filldirent();		/* destroy the incore hnode table */

extern int hs_access();			/* check vnode protection */
extern int hs_read();			/* vnode read routine */

extern void hs_synchash();

#define UCRED			u.u_cred
