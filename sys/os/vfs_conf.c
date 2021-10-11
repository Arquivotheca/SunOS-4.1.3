/*	@(#)vfs_conf.c 1.1 92/07/30 SMI	*/

#include <sys/param.h>
#include <sys/vfs.h>

#ifdef UFS
extern	struct vfsops ufs_vfsops;
#endif

#ifdef NFSCLIENT
extern	struct vfsops nfs_vfsops;
#endif

#ifdef PCFS
extern	struct vfsops pcfs_vfsops;
#endif

#ifdef LOFS
extern	struct vfsops lo_vfsops;
#endif

#ifdef RFS
extern	struct vfsops rfs_vfsops;
#endif

#ifdef TFS
extern	struct vfsops tfs_vfsops;
#endif

#ifdef TMPFS
extern  struct vfsops tmp_vfsops;
#endif

#ifdef HSFS
extern  struct vfsops hsfs_vfsops;
#endif

extern	struct vfsops spec_vfsops;

/*
 * WARNING: THE POSITIONS OF FILESYSTEM TYPES IN THIS TABLE SHOULD NOT
 * BE CHANGED. These positions are used in generating fsids and fhandles.
 * Thus, changing positions will cause a server to change the fhandle it
 * gives out for a file.
 */

struct vfssw vfssw[] = {
	"spec", &spec_vfsops,		/* SPEC */
#ifdef UFS
	"4.2", &ufs_vfsops,		/* UFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef NFSCLIENT
	"nfs", &nfs_vfsops,		/* NFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef PCFS
	"pcfs", &pcfs_vfsops,		/* PCFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef LOFS
	"lo", &lo_vfsops,		/* LOopback */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef RFS
	"rfs", &rfs_vfsops,		/* RFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef TFS
	"tfs", &tfs_vfsops,		/* TFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef TMPFS
	"tmp", &tmp_vfsops,		/* TMPFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
#ifdef HSFS
	"hsfs", &hsfs_vfsops,		/* HSFS */
#else
	(char *)0, (struct vfsops *)0,
#endif
};

#define	NVFS	(sizeof vfssw / sizeof vfssw[0])

struct vfssw *vfsNVFS = &vfssw[NVFS];
