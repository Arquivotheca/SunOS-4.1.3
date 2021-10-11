/*	@(#)vfs_conf.c 1.1 92/07/30 SMI	*/

#include <sys/param.h>
#include <sys/vfs.h>
#include <sys/bootconf.h>

#ifdef UFS
extern	struct vfsops ufs_vfsops;	/* XXX Should be ifdefed */
#endif

#ifdef NFS
extern	struct vfsops nfs_vfsops;
#endif

#ifdef PCFS
extern	struct vfsops pcfs_vfsops;
#endif

struct vfssw vfssw[] = {
#ifdef UFS
	"4.2", &ufs_vfsops,	/* 0 = MOUNT_UFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef NFS
	"nfs", &nfs_vfsops,	/* 1 = MOUNT_NFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef PCFS
	"pc", &pcfs_vfsops,	/* 2 = MOUNT_PC */
#else
	(char *)0, (struct vfsops *)0,
#endif
};

/*
 * Stolen from ../sun/bootconf.c
 */

struct bootobj rootfs;
