/*	@(#)bootconf.h 1.1 92/07/30 SMI	*/

/*
 * Boot time configuration information objects
 */

#ifndef _sys_bootconf_h
#define _sys_bootconf_h

#define	MAXFSNAME	16
#define	MAXOBJNAME	128
/*
 * Boot configuration information
 */
struct bootobj {
	char	bo_fstype[MAXFSNAME];	/* filesystem type name (e.g. nfs) */
	char	bo_name[MAXOBJNAME];	/* name of object */
	int	bo_flags;		/* flags, see below */
	int	bo_size;		/* number of blocks */
	struct vnode *bo_vp;		/* vnode of object */
};

/*
 * flags
 */
#define	BO_VALID	0x01		/* all information in object is valid */
#define	BO_BUSY		0x02		/* object is busy */

extern struct bootobj rootfs;
extern struct bootobj dumpfile;
extern struct bootobj swapfile;

#endif /*!_sys_bootconf_h*/
