#ifndef lint
static  char sccsid[] = "@(#)swapgeneric.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987-1990 by Sun Microsystems, Inc.
 *
 * XXX - This stuff is no longer generic or swap related
 * and should probably be moved to autoconf.c.
 */

/* XXX: These must be kept in sync with scsi config flags. */
#define	DISK	0	/* Scsi disk */
#define	TAPE	1	/* Scsi tape */
#define	CDROM	2	/* Scsi cdrom */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/mtio.h>
#undef NFSCLIENT
#include <sys/mount.h>
#include <sys/socket.h>
#include <net/if.h>

#include <machine/pte.h>
#include <sundev/mbvar.h>
#ifdef	OPENPROMS
#include <mon/obpdefs.h>
#else	OPENPROMS
#include <mon/sunromvec.h>
#endif	OPENPROMS
#include <sun/autoconf.h>

#ifdef sun4c
#include <machine/auxio.h>
#define	Auxio (*(u_char *)AUXIO_REG)
#endif sun4c

#ifdef OPENPROMS
/*
 * Undefine DPRINTF to remove compiled code.
 * #define DPRINTF		if (bootpath_debug) printf
 */
#ifdef	DPRINTF
static int bootpath_debug = 1;		/* Non-zero for debug messages */
#endif	DPRINTF
#endif

/*
 * Generic configuration;  all in one
 */
dev_t	rootdev;
struct vnode *rootvp;

/*
 * Flags in bdevlist/cdevlist.
 * This stuff moved outside the "ifdef OPENPROM" for building
 * VME dev_info structs, and booting from VME.
 */
#define	SG_EXISTS	1	/* device exists */
#define	SG_HEXUNIT	2	/* unit number is hex */
#define	SG_TAPE		4	/* tape minor number */

#define	SG_NHEX		3	/* number of hex digits required & allowed */

static int	sg_parse_name();
static dev_t	sg_form_dev();

#ifndef	OPENPROMS

#include "id.h"
#if NID > 0
extern  struct mb_driver idcdriver;
#endif

#include "xd.h"
#if NXD > 0
extern	struct mb_driver xdcdriver;
#endif

#include "xy.h"
#if NXY > 0
extern	struct mb_driver xycdriver;
#endif

# include "xt.h"
#if NXT > 0
extern	struct mb_driver xtcdriver;
#endif

# include "mt.h"
#if NMT > 0
extern	struct mb_driver tmdriver;
#endif

#ifdef	OLDSCSI
#include "sc.h"
#if NSC > 0
extern	struct mb_driver scdriver;
#endif

#include "si.h"
#if NSI > 0
extern	struct mb_driver sidriver;
#endif

#include "se.h"
#if NSE > 0
extern	struct mb_driver sedriver;
#endif

#include "sw.h"
#if NSW > 0
extern	struct mb_driver swdriver;
#endif

#include "sm.h"
#if NSM > 0
extern  struct mb_driver smdriver;
#endif

#include "wds.h"
#if NWDS > 0
extern	struct mb_driver wdsdriver;
#endif
#else	OLDSCSI

#include "sd.h"
#if NSD > 0
extern	struct mb_driver sddriver;
#endif

#endif	OLDSCSI

#include "ns.h"
#if NNS > 0
extern	struct mb_driver nsdriver;
#endif

#include "rd.h"
#if NRD > 0
extern	struct mb_driver rddriver;
#endif

#include "fd.h"
#if NFD > 0
extern	struct mb_driver fdcdriver;
#endif

#include "hd.h"
#if NHD > 0
extern	struct mb_driver hdcdriver;
#endif

#include "ft.h"
#if NFT > 0
extern	struct mb_driver ftdriver;
#endif

/*
 * Flags in bdevlist/cdevlist.
 */
#define	SG_EXISTS	1	/* device exists */
#define	SG_HEXUNIT	2	/* unit number is hex */
#define	SG_TAPE		4	/* tape minor number */

#define	SG_NHEX		3	/* number of hex digits required & allowed */

struct	bdevlist {
	char	*bl_name;
	struct mb_driver *bl_driver;
	dev_t	bl_root;
	int	bl_flags;
} bdevlist[] = {

#if NID > 0
	{"id", &idcdriver, makedev(22, 0), SG_HEXUNIT},
#endif

#if NXD > 0
	{"xd", &xdcdriver, makedev(10, 0)},
#endif

#if NXY > 0
	{"xy", &xycdriver, makedev(3, 0)},
#endif

#if NSC > 0
	{"sd", &scdriver, makedev(7, 0)},
#endif

#ifdef	OLDSCSI
#if NSI > 0
	{"sd", &sidriver, makedev(7, 0)},
#endif

#if NSE > 0
	{"sd", &sedriver, makedev(7, 0)},
#endif

#if NSW > 0
	{"sd", &swdriver, makedev(7, 0)},
#endif

#if NSM > 0
	{"sd", &smdriver, makedev(7, 0)},
#endif

#if NWDS > 0
	{"sd", &wdsdriver, makedev(7, 0)},
#endif
#else	OLDSCSI
#if NSD > 0
	{"sd", &sddriver, makedev(7, 0)},
#endif
#endif	OLDSCSI

#if NRD > 0
	{"rd", &rddriver, makedev(13, 0)},
#endif

#if NNS > 0
	{"ns", &nsdriver, makedev(12, 0)},
#endif

#if NHD > 0
	{"hd", &hdcdriver, makedev(15, 0)},
#endif

#ifdef ROOT_ON_FLOPPY
#if NFD > 0
	{"fd", &fdcdriver, makedev(16, 0)},
#endif
#endif ROOT_ON_FLOPPY

#ifdef ROOT_ON_TAPE
#include "st.h"
#ifdef	OLDSCSI
#if NST > 0
#   if NSC > 0
	{"st", &scdriver, makedev(11, MT_NOREWIND)},
#   endif
#   if NSI > 0
	{"st", &sidriver, makedev(11, MT_NOREWIND)},
#   endif
#   if NSE > 0
	{"st", &sedriver, makedev(11, MT_NOREWIND)},
#   endif
#   if NSW > 0
	{"st", &swdriver, makedev(11, MT_NOREWIND)},
#   endif

#   if NSM > 0
	{"st", &smdriver, makedev(11, MT_NOREWIND)},
#   endif

#   if NWDS > 0
	{"st", &wdsdriver, makedev(11, MT_NOREWIND)},
#   endif
#endif NST > 0
#else	OLDSCSI
#if NST > 0
	{"st", &stdriver, makedev(11, MT_NOREWIND)},
#endif
#endif	OLDSCSI

#if NXT > 0
	{"xt", &xtcdriver, makedev(8, MT_NOREWIND)},
#endif

#if NMT > 0
	{"mt", &tmdriver, makedev(1, MT_NOREWIND)},
#endif
#endif ROOT_ON_TAPE
	{0}
};

#include "rd.h"
#if NRD > 0
/*
 * If the ram disk is intialized from tape, it needs to
 * use the character device to skip to the right file.
 */
struct	cdevlist {
	char	*cl_name;
	struct mb_driver *cl_driver;
	dev_t	cl_croot;
	dev_t	cl_broot;
	int	cl_flags;
} cdevlist[] = {

#include "st.h"
#ifdef	OLDSCSI
#if NST > 0
#  if NSC > 0
	{"st", &scdriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#  if NSI > 0
	{"st", &sidriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#  if NSE > 0
	{"st", &sedriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#  if NSM > 0
	{"st", &smdriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#  if NSW > 0
	{"st", &swdriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#  if NWDS > 0
	{"st", &wdsdriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#  endif
#endif NST > 0
#else	OLDSCSI
#if NST > 0
	{"st", &stdriver, makedev(18, MT_NOREWIND), makedev(11, MT_NOREWIND)},
#endif
#endif	OLDSCSI

#include "sr.h"
#ifdef	OLDSCSI
#if NSR > 0
#  if NSC > 0
	{"sr", &scdriver, makedev(58, 0), makedev(18, 0)},
#  endif
#  if NSI > 0
	{"sr", &sidriver, makedev(58, 0), makedev(18, 0)},
#  endif
#  if NSE > 0
	{"sr", &sedriver, makedev(58, 0), makedev(18, 0)},
#  endif
#  if NSM > 0
	{"sr", &smdriver, makedev(58, 0), makedev(18, 0)},
#  endif
#  if NSW > 0
	{"sr", &swdriver, makedev(58, 0), makedev(18, 0)},
#  endif
#  if NWDS > 0
	{"sr", &wdsdriver, makedev(58, 0), makedev(18, 0)},
#  endif
#endif NSR > 0
#else	OLDSCSI
#if NSR > 0
	{"sr", &srdriver, makedev(58, 0), makedev(18, 0)},
#endif
#endif	OLDSCSI

#if NXT > 0
	{"xt", &xtcdriver, makedev(30, MT_NOREWIND), makedev(8, MT_NOREWIND)},
#endif

#if NMT > 0
	{"mt", &tmdriver, makedev(5, MT_NOREWIND), makedev(1, MT_NOREWIND)},
#endif

#if NID > 0
	{"id", &idcdriver, makedev(72, 0), makedev(22, 0), SG_HEXUNIT},
#endif

#if NXD > 0
	{"xd", &xdcdriver, makedev(42, 0), makedev(10, 0)},
#endif

#if NXY > 0
	{"xy", &xycdriver, makedev(9, 0), makedev(3, 0)},
#endif

#ifdef	OLDSCSI
#if NSC > 0
	{"sd", &scdriver, makedev(17, 0), makedev(7, 0)},
#endif

#if NSI > 0
	{"sd", &sidriver, makedev(17, 0), makedev(7, 0)},
#endif

#if NSE > 0
	{"sd", &sedriver, makedev(17, 0), makedev(7, 0)},
#endif

#if NSW > 0
	{"sd", &swdriver, makedev(17, 0), makedev(7, 0)},
#endif

#if NSM > 0
	{"sd", &smdriver, makedev(17, 0), makedev(7, 0)},
#endif

#if NWDS > 0
	{"sd", &wdsdriver, makedev(17, 0), makedev(7, 0)},
#endif
#else	OLDSCSI
#if NSD > 0
	{"sd", &sddriver, makedev(17, 0), makedev(7, 0)},
#endif
#endif	OLDSCSI

#if NFT > 0
	{"ft", &ftdriver, -1, makedev(14, 0)},
#endif

	{0}
};
#endif NRD > 0

static int	sg_parse_name();
static dev_t	sg_form_dev();

#else	!OPENPROMS

/*
 * slightly nonportable C- bdevlist will point to a  kmem_alloc'ed
 * area of memory which will then be an array of bdevlist structures.
 * The terminator for this list will be signalled by a null bl_dev pointer
 *
 * There will only be one entry per device kind- that is, the bl_dev
 * will point to the devinfo structure for the first valid and attached
 * device found.
 */
static struct bdevlist {
	struct	dev_info *bl_dev;
	dev_t	bl_root;
	int	bl_exists;		/*
					 * bl_exists contains a bit pattern
					 * for valid units for this device.
					 */

	struct	bdevlist *bl_next;	/* successor */
	int	bl_flags;		/* flags for IPI from psra/pie */
} *bdevlist = NULL;

#include "rd.h"
#if	NRD > 0

static struct dev_ops pseudo_ops = { 0 };


/*
 * slightly nonportable C- cdevlist will point to a  kmem_alloc'ed
 * area of memory which will then be an array of cdevlist structures.
 * The terminator for this list will be signalled by a null cl_dev pointer
 *
 */
static struct cdevlist {
	struct	dev_info *cl_dev;
	dev_t	cl_croot;
	dev_t	cl_broot;
	int	cl_exists;		/* bit pattern of attached units */
	struct	cdevlist *cl_next;	/* successor */
} *cdevlist = NULL;

#endif	NRD

/*
 * XXX FIX ME FIX ME FIX ME
 * XXX THIS IS GROSS
 * XXX THERE MUST BE A BETTER WAY TO DO THIS
 * XXX PERHAPS SOMETHING IN sun/conf.c THAT TIES NAMESPACE TO {b, c}devsw
 * XXX ENTRIES. THIS COULD ALSO REDUCE THE LOAD IN sun/wscons.c
 *
 * XXX Also, shouldn't the [bc]maptabs be in REVERSE order of preference,
 * XXX as the [bc]devlists are built in LIFO order?
 */

static struct bmaptab {
	char	*m_name;	/* device name */
	dev_t 	m_bmajor;	/* major device number */
} bmaptab[] = {

#ifdef	sun4m
#include "id.h"
#include "xd.h"			/* keeping xd included for now, just in case */
#else
#define	NID 0
#define	NXD 0
#endif
#if	NID > 0
	{ "id", makedev(22, 0) },
#endif	NID > 0

#if	NXD > 0
	{ "xd", makedev(10, 0) },
#endif	NXD > 0

#include "sd.h"
#if	NSD > 0
	{ "sd", makedev(7, 0) },
#endif	NSD > 0

#include "fd.h"
#if	NFD > 0
	{ "fd", makedev(16, 0)	},
#endif	NFD

/* rd.h included above */
#if	NRD > 0
	{ "rd", makedev(13, 0)	},
#endif	NRD

#include "ns.h"
#if	NNS > 0
	{ "ns", makedev(12, 0)	},
#endif	NNS

#ifdef	ROOT_ON_TAPE
#include "st.h"
#if	NST > 0
	{ "st", makedev(11, MT_NOREWIND) },
#endif	NST > 0
#endif	ROOT_ON_TAPE

#include "sr.h"
#if	NSR > 0
	{ "sr", makedev(18, 0) },
#endif	NSR > 0

	{ 0 },
};

#if	NRD > 0
static struct cmaptab
{
	char	*m_name;	/* device name */
	dev_t 	m_bmajor;	/* major device number - block */
	dev_t 	m_cmajor;	/* major device number - char */
} cmaptab[] = {

#if	NSD > 0
	{ "sd", makedev(7, 0), makedev(17, 0) },
#endif	NSD > 0

#if	NFD > 0
	{ "fd", makedev(16, 0),	makedev(54, 0)	},
#endif	NFD

#if	NST > 0
	{ "st", makedev(11, MT_NOREWIND), makedev(18, MT_NOREWIND)	},
#endif	NST > 0

#include "ft.h"
#if	NFT > 0
	{ "ft", makedev(14, 0),	-1		},
#endif	NFT

#if	NSR > 0
	{ "sr", makedev(18, 0), makedev(58, 0) },
#endif	NSR > 0

#if	NID > 0
	{ "id", makedev(22, 0), makedev(72, 0) },
#endif

	{ 0 },
};
#endif	NRD
#endif	!OPENPROMS

struct vfssw *
getfstype(askfor, fsname)
	char *askfor;
	char *fsname;
{
	int fstype;
#ifdef	OPENPROMS
	char	devname[OBP_MAXDEVNAME];
#else	OPENPROMS
	struct	bootparam	*bp;
	char	devname[32];
#endif	OPENPROMS
	short	unit;
	extern char *strcpy(), *strncpy();

	if (boothowto & RB_ASKNAME) {

		for (fsname[0] = '\0'; fsname[0] == '\0'; /* empty */) {
			printf("%s filesystem type (", askfor);
			for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
				if (!vfssw[fstype].vsw_name)
					continue;
				if (!strcmp("swap", askfor) &&
				    !strcmp("4.2", vfssw[fstype].vsw_name))
					continue;	/* "swap" can't be on "4.2" */
				if (!strcmp("root", askfor) &&
				    !strcmp("spec", vfssw[fstype].vsw_name))
					continue;	/* "root" can't be on "spec" */
				printf("%s ", vfssw[fstype].vsw_name);
			}
			printf("): ");
			gets(fsname);
			for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
				if (vfssw[fstype].vsw_name == 0)
					continue;	/* ignore fstypes without names */
				if (!strcmp("swap", askfor) &&
				    !strcmp("4.2", vfssw[fstype].vsw_name))
					continue;	/* "swap" can't be on "4.2" */
				if (!strcmp("root", askfor) &&
				    !strcmp("spec", vfssw[fstype].vsw_name))
					continue;	/* "root" can't be on "spec" */
				if (*fsname == '\0') {
							/* default, grab the first */
					(void) strcpy(fsname,
					    vfssw[fstype].vsw_name);
					return (&vfssw[fstype]);
				}
				if (strcmp(vfssw[fstype].vsw_name, fsname)==0) {
					return (&vfssw[fstype]);
				}
			}
			printf("Unknown filesystem type '%s'\n", fsname);
			*fsname = '\0';
		}
	} else if (*fsname) {
		for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
			if (vfssw[fstype].vsw_name == 0) {
				continue;
			}
			if (strcmp(vfssw[fstype].vsw_name, fsname) == 0) {
				return (&vfssw[fstype]);
			}
		}
	} else {        /* last try before giving up */
		/*
		 * See if the device specified on the boot line
		 * is an initialized network interface.
		 * If so, then select "nfs".
		 */
#ifdef OPENPROMS
#ifdef DPRINTF
		char *path;
#endif
		extern char *prom_bootpath();

#ifdef DPRINTF
		path = prom_bootpath();
#else
                (void) prom_bootpath();
#endif
#ifdef	DPRINTF
		DPRINTF("getfstype: path <%s> --> ", path ? path : "NONE!");
#endif	DPRINTF

		devname[0] = (char)0;	/* In case of error getting devname */
		if (prom_get_boot_dev_name(devname, sizeof devname) != 0)  {
#ifdef	DPRINTF
			DPRINTF("getfstype: Error getting devname");
#endif	DPRINTF
			return ((struct vfssw *)0);
		}

		unit = (short)prom_get_boot_dev_unit();
#ifdef	DPRINTF
		DPRINTF("device <%s>, unit <%x>\n",
		    devname, (int)unit);
#endif	DPRINTF

#else	OPENPROMS
		bp = *romp->v_bootparam;
		if (bp) {
			devname[0] = bp->bp_dev[0];
			devname[1] = bp->bp_dev[1];
			devname[2] = '\0';
			unit = bp->bp_ctlr;
		}
#endif	OPENPROMS
		if (ifname(devname, unit)) { /* interface found */
			for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
				if (vfssw[fstype].vsw_name == 0)
					continue;
				if (strcmp(vfssw[fstype].vsw_name, "nfs") == 0)
					return (&vfssw[fstype]);
			}
		}
	}

	return ((struct vfssw *)0);
}

/*
 * If booted with ASKNAME, prompt on the console for a filesystem
 * name and return it.
 */
getfsname(askfor, name)
	char *askfor;
	char *name;
{

	if (boothowto & RB_ASKNAME) {
		printf("%s name: ", askfor);
		gets(name);
	}
}

#ifndef	OPENPROMS
/*
 * Return the device number and name of the first
 * block device in the bdevlist that can be opened.
 * If booted with the -a flag, ask user for device name.
 * Name must be at least 128 bytes long.
 * askfor is one of "root" or "swap".
 */
dev_t
getblockdev(askfor, name)
	char *askfor;
	char *name;
{
	register struct bdevlist *bl;
	struct bdevlist	lbl;
	int ctlr, unit, part, i, def_part;
	static int first_time = 0;
	dev_t	dev;

	if (strcmp(askfor, "swap") == 0)
		part = def_part = 1;	/* default swap partition is 'b' */
	else
		part = def_part = 0;	/* default root partition is 'a' */

	if (boothowto & RB_ASKNAME) {
		for (bl = bdevlist; bl->bl_name; bl++) {
			if (first_time)
				break;
			if (chkroot(bl) > 0) {
				bl->bl_flags |= SG_EXISTS;
				continue;
			}
			/* We don't check for extra tape units.  */
			if ((bdevsw[major(bl->bl_root)].d_flags & B_TAPE))
				continue;
			/*
			 * Controller present; unit 0 is not.  Check for
			 * additional units 1-6.
			 */
			for (i = 1; i <= 6; i++) {
				register int root;

				root = bl->bl_root;
				bl->bl_root = makedev(major(bl->bl_root), i<<3);
				if (chkroot(bl) > 0)
					bl->bl_flags |= SG_EXISTS;
				bl->bl_root = root;
				if (bl->bl_flags & SG_EXISTS)
					break;
			}
		}
		first_time = 1;

		for (unit = -1; unit == -1; /* empty */) {
			/*
			 * SCSI makes things complicated since disks, tapes,
			 * and other devices are hung off the same controller.
			 * Thus, we have to use the bdev table name.
			 */
			printf("%s device ( ", askfor);
			for (bl = bdevlist; bl->bl_name; bl++) {
				if (bl->bl_flags & SG_EXISTS)
					if (bl->bl_flags & SG_HEXUNIT)
						printf("%s%%3x[a-h] ",
						    bl->bl_name);
					else
						printf("%s%%d[a-h] ",
						    bl->bl_name);
			}
			printf("): ");
			gets(name);
			if (*name == '\0') {
				bl = bdevlist;
				unit = 0;
				part = def_part;
				break;
			}
			for (bl = bdevlist; bl->bl_name; bl++) {
				if (bl->bl_name[0] == name[0] &&
				    bl->bl_name[1] == name[1])
					break;
			}
			if (!bl->bl_name) {
				printf("unknown device name %c%c\n",
				    name[0], name[1]);
				continue;
			}
sdloop:
			unit = 0;		/* default unit number */
			part = def_part;	/* default partition */
			/*
			 * Parse name to get unit number and partition.
			 */
			if (sg_parse_name(name, &unit, &part, bl->bl_flags)) {
				/*
				 * See if we can open it.  If not, reprompt.
				 */
				dev = sg_form_dev(bl->bl_root, &name[2],
					unit, part, bl->bl_flags);
				lbl = *bl;		/* local struct copy */
				lbl.bl_root = dev;
				if (chkroot(&lbl) > 0)
					return (dev);
				/*
				 * does scsi make things complicated !!!.
				 * There are different
				 * kinds of scsis. look through all of them.
				 */
				if (name[0] == 's' && name[1] == 'd' &&
				    (++bl)) {
					for (; bl->bl_name; bl++) {
						if (bl->bl_name[0] == name[0] &&
						    bl->bl_name[1] == name[1])
							goto sdloop;
					}
				}
				printf("cannot open %s - re-enter.\n", name);
				unit = -1;	/* stay in loop */
			}
		}
	} else {
		unit = 0;

		/*
		 * If a name was given, find that block device and use it.
		 */
		if (*name != '\0') {
			for (bl = bdevlist; bl->bl_name; bl++) {
				if (bl->bl_name[0] == name[0] &&
				    bl->bl_name[1] == name[1]) {
					if (chkroot(bl) >= 0 &&
					    sg_parse_name(name, &unit,
					    &part, bl->bl_flags))
						goto found;
				}
			}
			return ((dev_t)0);
		}
		/*
		 * Look for device we booted from and use it if found.
		 */
		for (bl = bdevlist; bl->bl_name; bl++) {
			register struct bootparam *bp = (*romp->v_bootparam);

			if (!((*bl->bl_name == bp->bp_dev[0]) &&
				(*(bl->bl_name+1) == bp->bp_dev[1])))
				continue;

			name[0] = bp->bp_dev[0];
			name[1] = bp->bp_dev[1];
			ctlr = bp->bp_ctlr;
			part = bp->bp_part;
			unit = bp->bp_unit;

			/* If scsi disk (or CDROM), get logical unit. */
			if (name[0] == 's' && name[1] == 'd') {
				register struct bdevlist *bbl;

				if ((unit = lkupbdev(bl, ctlr, unit, DISK)) == -1) {

					/* maybe it's a CDROM */
					bbl = bl;
					for (bl = bdevlist; bl->bl_name; bl++) {
						if (!((*bl->bl_name == 's') &&
						    (*(bl->bl_name+1) == 'r') &&
						    (bl->bl_flags & SG_EXISTS)))
							continue;
						/*
						 * Note, use bp_unit as unit is
						 * modified.
						 */
						if ((unit = lkupbdev(bl, ctlr,
						    bp->bp_unit, CDROM)) != -1)
							break;
					}
					if (unit == -1) {
						bl = bbl;
						continue;
					}
				}

			/* If scsi CDROM, get logical unit. */
			} else if (name[0] == 's' && name[1] == 'r') {
				if ((unit = lkupbdev(bl, ctlr, unit, CDROM)) == -1)
					continue;

			/* IPI device, get logical unit. */
			} else if (bl->bl_flags & SG_HEXUNIT) {
				unit = (ctlr << 4) | unit;
			}

			/* Got unit, now let's open it and see if it's there */
			lbl = *bl;		/* copy struct */
			lbl.bl_root = sg_form_dev(bl->bl_root, &name[2],
				unit, part, bl->bl_flags);
			if (chkroot(&lbl) > 0)
				return (lbl.bl_root);
		}

		/*
		 * Last chance, use the first device that can be opened.
		 */
		unit = 0;
		part = def_part;
		for (bl = bdevlist; bl->bl_name; bl++) {
			if (chkroot(bl) > 0)
				goto found;
		}
		return ((dev_t)0);
	}

found:
	name[0] = bl->bl_name[0];
	name[1] = bl->bl_name[1];
	return (sg_form_dev(bl->bl_root, &name[2], unit, part, bl->bl_flags));
}

#else	!OPENPROMS

/*
 * Return the device number and name of the first
 * block device in the bdevlist that can be opened.
 * If booted with the -a flag, ask user for device name.
 * Name must be at least 128 bytes long.
 * askfor is one of "root" or "swap".
 */
dev_t
getblockdev(askfor, name)
	char *askfor;
	char *name;
{
	register struct bdevlist *bl;
	struct bdevlist lbl;
	int unit, part, def_part, askme = (boothowto & RB_ASKNAME);
	dev_t dev, chk;

	char devname[OBP_MAXDEVNAME];
#ifdef DPRINTF
	char *path;
#endif
	extern char *prom_bootpath();

	if (strcmp(askfor, "swap") == 0)
		part = def_part = 1;    /* default swap partition is 'b' */
	else
		part = def_part = 0;    /* default root partition is 'a' */

	if (!bdevlist) {
		if (build_bdevlist() == 0) {
			panic("getblockdev: No Devices to boot off of?");
			/*NOTREACHED*/
		}
	}
tryagain:

	if (askme) {
		for (unit = -1; unit == -1; /* empty */) {
			printf("%s device (", askfor);
			for (bl = bdevlist; bl != NULL; bl = bl->bl_next) {
				if (bl->bl_exists) {
					if (bl->bl_flags & SG_HEXUNIT)
						printf("%s%%3x[a-h] ",
						    bl->bl_dev->devi_name);
					else
						printf("%s%%d[a-h] ",
						    bl->bl_dev->devi_name);
				}
			}
			printf("): ");
			gets(name);
			if (*name == '\0') {
				bl = bdevlist;
				unit = 0;
				part = def_part;
				break;
			}
			for (bl = bdevlist; bl->bl_next; bl = bl->bl_next) {
				if (strncmp(
				    bl->bl_dev->devi_name, name, 2) == 0)
					break;
			}
			if (!bl) {
				printf("Unknown device name '%c%c'\n",
				    name[0], name[1]);
				continue;
			}
			unit = 0; /* default unit number */
			part = def_part;        /* default partition */
			/*
			 * Parse name to get unit number and partition.
			 */
			if (sg_parse_name(name, &unit, &part, bl->bl_flags)) {
				/*
				 * See if we can open it.  If not, reprompt.
				 */
				dev = sg_form_dev(bl->bl_root, &name[2],
					unit, part, bl->bl_flags);
				lbl = *bl;              /* local struct copy */
				lbl.bl_root = dev;
				if (lbl.bl_exists)
					return (dev);
			}
		}
	} else {
		unit = 0;

		/*
		 * If we are looking for swap, assume
		 * that the second partition is needed.
		 */
		if (strcmp (askfor, "swap") == 0)
			part = 1;
		else
			part = 0;
		/*
		 * If a name was given, find that block device
		 */
		if (*name != '\0') {
			for (bl = bdevlist; bl != NULL; bl = bl->bl_next) {
				if (strncmp(bl->bl_dev->devi_name, name, 2))
					continue;
				if ((bl->bl_exists) &&
				    sg_parse_name(name, &unit,
					&part, bl->bl_flags))
					goto found;
			}
			return ((dev_t)0);
		}
		/*
		 * Look for device we booted from
		 */

#ifdef	DPRINTF
		path = prom_bootpath();
		DPRINTF("getblockdev: path <%s> --> ", path ? path : "NONE!");
#else
        (void) prom_bootpath();
#endif	DPRINTF

		if (prom_get_boot_dev_name(devname, sizeof devname) != 0)  {
#ifdef	DPRINTF
		    DPRINTF("getblockdev: Error getting devname (usefirst)\n");
#endif	DPRINTF
		    goto usefirst;
		}

		unit = prom_get_boot_dev_unit();
		part = prom_get_boot_dev_part();

#ifdef	DPRINTF
		DPRINTF("device <%s> unit <%x> part <%d>\n",
		    devname, unit, part);
#endif	DPRINTF

		for (bl = bdevlist; bl != NULL; bl = bl->bl_next) {
		    if (strcmp(bl->bl_dev->devi_name, devname) != 0)
			continue;
		    if (bl->bl_exists & (1 << unit))
			goto found;
		}

		/*
		 * Use the first device that can be opened.
		 * If we are looking for swap, assume
		 * that the second partition is needed.
		 */
usefirst:
		if (strcmp (askfor, "swap") == 0) part = 1;
		for (bl = bdevlist; bl != NULL; bl = bl->bl_next) {
			if (bl->bl_exists == 0)
				continue;
			/*
			 * if some user has a floppy in their drive, and they
			 * are booting a GENERIC kernel over the net, they
			 * expect to NOT swap on the floppy.  Since there is
			 * nothing intrinsically "illegal" about it - it is
			 * just too small for normal operations, we put in
			 * a special HACK here, to disallow swapping on "fd"
			 */
			if ((bl->bl_dev->devi_name[0] == 'f') &&
			    (bl->bl_dev->devi_name[1] == 'd') &&
			    (strcmp (askfor, "swap") == 0))
				continue;
			while (unit < 8*sizeof (bl->bl_exists)) {
				if (bl->bl_exists&(1<<unit)) {
					if (chkopenable(makedev(major(bl->bl_root),
								(unit * 8 + part))))
						goto found;
				}
				unit++;
			}
		}
		return ((dev_t)0);
	}

found:
	name[0] = bl->bl_dev->devi_name[0];
	name[1] = bl->bl_dev->devi_name[1];
	chk = (sg_form_dev(bl->bl_root, &name[2], unit, part, bl->bl_flags));

	/*
	 * Make sure that you can open the device
	 * the user ended up specifying (unless it's rd.c)
	 */
	if (strncmp("rd0", name, 3) != 0) {
		if (chkopenable(chk)) {
			return (chk);
		} else {
			printf("Cannot open '%s'\n", name);
			askme = 1;
			goto tryagain;
		}
	} else
	return (chk);
}

/*
 * check if dev is openable, 0 no, 1 yes
 */
static
chkopenable(dev)
	dev_t	dev;
{
	short maj = major(dev);

	if ((*bdevsw[maj].d_open)(dev, FREAD) == 0) {
		(void)(*bdevsw[maj].d_close)(dev, FREAD);
		return (1);
	} else
		return (0);
}

/*
 * Check to see whether name is already in bdevlist
 */

static struct bdevlist *
chkdup_b(name)
register char *name;
{
	register struct bdevlist *tp = bdevlist;

	while (tp) {
		if (strcmp(name, tp->bl_dev->devi_name) == 0)
			break;
		else
			tp = tp->bl_next;
	}
	return (tp);
}

/*
 * If this dev structure refers to this bmaptab entry,
 * and an equivalent name (i.e., another unit) is not
 * already in the bdevlist, add it in.
 */

static
add_to_blist(dev, op)
register struct dev_info *dev;
char *op;
{
	register struct bmaptab *bmp = (struct bmaptab *) op;

	if (strcmp(dev->devi_name, bmp->m_name) == 0 && dev->devi_driver) {
		register struct bdevlist *tp;
		/*
		 * If there is a devlist entry allready in the list,
		 * then only add a notation to it that there is another
		 * unit available.
		 */
		if (tp = chkdup_b(dev->devi_name)) {
			tp->bl_exists |= (1<<dev->devi_unit);
		} else {
			tp = bdevlist;
			bdevlist = (struct bdevlist *)new_kmem_zalloc(
				sizeof (*tp), KMEM_NOSLEEP);
			if (!bdevlist)
				panic("add_to_blist: no memory");
			bdevlist->bl_dev = dev;
			bdevlist->bl_next = tp;
			bdevlist->bl_root = bmp->m_bmajor;
			bdevlist->bl_exists |= (1<<dev->devi_unit);
			/* have to set the flag for IPI devices */
			if (strcmp(bmp->m_name, "id") == 0)
				bdevlist->bl_flags = SG_HEXUNIT;
		}
	}
}

static int
build_bdevlist()
{
	extern struct dev_info *top_devinfo;
	register struct bmaptab *bmp;

	for (bmp = bmaptab; bmp->m_name; bmp++) {
#if	NRD > 0
		if (strcmp("rd", bmp->m_name) == 0 ||
		    strcmp("ns", bmp->m_name) == 0 ||
		    strcmp("ft", bmp->m_name) == 0) {
			register struct dev_info *dev =
				(struct dev_info *)new_kmem_zalloc(
					sizeof (struct dev_info), KMEM_NOSLEEP);
			if (!dev)
				panic("build_bdevlist: no memory");
			dev->devi_name = bmp->m_name;
			dev->devi_driver = &pseudo_ops;
			add_to_blist(dev, (char *) bmp);
		} else
#endif	NRD > 0
		walk_devs(top_devinfo, add_to_blist, (char *) bmp);
	}
	{
/* move "sr" devices to the end of the list. */
		register struct bdevlist *srl, *otl, *scn, *nxt;
		srl = NULL;
		otl = NULL;
		nxt = bdevlist;
		while (scn = nxt) {
			nxt = scn->bl_next;
			if ((scn->bl_dev->devi_name[0] == 's') &&
			    (scn->bl_dev->devi_name[1] == 'r')) {
				scn->bl_next = srl;
				srl = scn;
			} else {
				scn->bl_next = otl;
				otl = scn;
			}
			scn = nxt;
		}
		while (srl != NULL) {
			nxt = srl->bl_next;
			srl->bl_next = scn;
			scn = srl;
			srl = nxt;
		}
		while (otl != NULL) {
			nxt = otl->bl_next;
			otl->bl_next = scn;
			scn = otl;
			otl = nxt;
		}
		bdevlist = scn;
	}
	return (bdevlist != NULL);
}

#if	NRD > 0
/*
 * Check to see whether name is already in cdevlist
 */
static struct cdevlist *
chkdup_c(name)
register char *name;
{
	register struct cdevlist *tp = cdevlist;

	while (tp) {
		if (strcmp(name, tp->cl_dev->devi_name) == 0)
			break;
		else
			tp = tp->cl_next;
	}
	return (tp);
}

/*
 * If this dev structure refers to this cmaptab entry,
 * and an equivalent name (i.e., another unit) is not
 * already in the cdevlist, add it in.
 */
static
add_to_clist(dev, op)
register struct dev_info *dev;
char *op;
{
	register struct cmaptab *cmp = (struct cmaptab *) op;

	if (strcmp(dev->devi_name, cmp->m_name) == 0 &&	dev->devi_driver) {
		register struct cdevlist *tp;

		/*
		 * If there is a devlist entry allready in the list,
		 * then only add a notation to it that there is another
		 * unit available.
		 */
		if (tp = chkdup_c(dev->devi_name)) {
			tp->cl_exists |= (1<<dev->devi_unit);
		} else {
			tp = cdevlist;
			cdevlist = (struct cdevlist *)
				new_kmem_zalloc(sizeof (*tp), KMEM_NOSLEEP);
			if (!cdevlist)
				panic("add_to_clist: no memory");
			cdevlist->cl_dev = dev;
			cdevlist->cl_next = tp;
			cdevlist->cl_croot = cmp->m_cmajor;
			cdevlist->cl_broot = cmp->m_bmajor;
			cdevlist->cl_exists |= (1<<dev->devi_unit);
		}
	}
}

static
build_cdevlist()
{
	extern struct dev_info *top_devinfo;
	register struct cmaptab *cmp;

	for (cmp = cmaptab; cmp->m_name; cmp++) {
		if (strcmp("rd", cmp->m_name) == 0 ||
		    strcmp("ns", cmp->m_name) == 0 ||
		    strcmp("ft", cmp->m_name) == 0) {
			register struct dev_info *dev =
				(struct dev_info *)new_kmem_zalloc(
				sizeof (struct dev_info), KMEM_NOSLEEP);
			if (!dev)
				panic("build_cdevlist: no memory");
			dev->devi_driver = &pseudo_ops;
			dev->devi_name = cmp->m_name;
			add_to_clist(dev, (char *) cmp);
		} else
		walk_devs(top_devinfo, add_to_clist, (char *) cmp);
	}
	return (cdevlist != NULL);
}
#endif	NRD
#endif	!OPENPROMS

#if NRD > 0

/*
 * for ease of use - we can patch the kernel with where to get the munixfs.
 * yes, I *know* this is a HORRIBLE HACK - the alternative is to build
 * 4 different (1/4", 1/2", floppy, net) MUNIX kernels.
 * if you make changes here, be so kind as to fixup things in ...src/sundist
 * XXX - toss the string by making getchardev smarter and just parse the
 * 	boot args
 */
char	loadramdiskfrom[16] = { 0 };

/*
 * loadramdiskfile is used for both tape and cdrom; for tape is file
 * number, for cdrom is offset in bytes from start of boot partition
 * to munixfs image.
 * NOTE: this is patched via adb in the various scripts in ...src/sundist
 */
int	loadramdiskfile = -1;

#ifndef	OPENPROMS
/*
 * The ram disk driver is initialized from another device
 * that contains the image of a 4.2 file system.
 *
 * Return the device number and name of the first
 * char device in the cdevlist that can be opened.
 * Name must be at least 128 bytes long.
 */
int
getchardev(askfor, name, fileno, cdev, bdev)
	char	*askfor, *name;
	int	*fileno;	/* -1 or file number on tape */
	dev_t	*cdev, *bdev;
{
	register struct cdevlist *cl;
	int	ctlr, unit, part;
	int	i;

	if (boothowto & RB_ASKNAME) {
		for (cl = cdevlist; cl->cl_name; cl++) {
			if ((i = chkcdev(cl)) > 0) {
				cl->cl_flags |= SG_EXISTS;
				continue;		/* Found it */
			} else if (i < 0) {
				continue;		/* no contrler found */
			}
			/*
			 * If st0 failed, check second host adapter for st4.
			 * Note, we use the block device so we don't confuse
			 * the ram disk driver.
			 */
			if (cl->cl_name[0] == 's' && cl->cl_name[1] == 't') {
				register int root;

				root = cl->cl_croot;
				cl->cl_croot = makedev(major(cl->cl_croot),
				    MTMINOR(4) + MT_NOREWIND);
				if (chkcdev(cl) > 0)
					cl->cl_flags |= SG_EXISTS;
				cl->cl_croot = root;
			}
		}
retry:
		fubar();		/* XXX sun4 compiler workaround */
		for (unit = -1; unit == -1; /* empty */) {
			printf("%s device ( ", askfor);
			for (cl = cdevlist; cl->cl_name; cl++)
				if (cl->cl_flags & SG_EXISTS) {
					if (cl->cl_flags & SG_HEXUNIT)
						printf("%s%%3x[a-h] ",
						    cl->cl_name);
					else
						printf("%s%%d[a-h] ",
						    cl->cl_name);
				}
			printf("): ");
			gets(name);
			if (*name == '\0') {
				cl = cdevlist;
				unit = 0;
				part = 0;
				break;
			}
			for (cl = cdevlist; cl->cl_name; cl++) {
				if (cl->cl_name[0] == name[0] &&
				    cl->cl_name[1] == name[1])
					break;
			}
			if (!cl->cl_name) {
				printf("unknown device name %c%c\n",
				    name[0], name[1]);
				continue;
			}
			unit = part = 0;		/* set defaults */
			(void) sg_parse_name(name, &unit, &part, cl->cl_flags);
		}
	} else {
		ctlr = unit = part = 0;
		for (cl = cdevlist; cl->cl_name; cl++) {
			if ((i = chkcdev(cl)) > 0) {
				cl->cl_flags |= SG_EXISTS;
				continue;		/* Found it */
			} else if (i < 0) {
				continue;		/* no controler found */
			}
			/*
			 * If st0 failed, check second host adapter for st4.
			 * Note, we use the block device so we don't confuse
			 * the ram disk driver.
			 */
			if (cl->cl_name[0] == 's' && cl->cl_name[1] == 't') {
				register int root;

				root = cl->cl_croot;
				cl->cl_croot = makedev(major(cl->cl_croot),
				    MTMINOR(4) + MT_NOREWIND);
				if (chkcdev(cl) > 0)
					cl->cl_flags |= SG_EXISTS;
				cl->cl_croot = root;
			}
		}
		/*
		 * Look for device we booted from and use it if found.
		 * Note, scsi requires some special processing to get unit.
		 */
		for (cl = cdevlist; cl->cl_name; cl++) {
			register struct bootparam *bp = (*romp->v_bootparam);
			if (!((*cl->cl_name == bp->bp_dev[0]) &&
			    (*(cl->cl_name+1) == bp->bp_dev[1]) &&
			    (cl->cl_flags & SG_EXISTS)))
				continue;

			name[0] = cl->cl_name[0];
			name[1] = cl->cl_name[1];
			ctlr = bp->bp_ctlr;
			unit = bp->bp_unit;
			part = bp->bp_part;

			/* If scsi tape, get logical unit. */
			if (name[0] == 's' && name[1] == 't') {
				if ((unit = lkupcdev(cl, ctlr, unit, TAPE)) != -1)
					goto found;

			/* If scsi disk (or CDROM), get logical unit. */
			} else if (name[0] == 's' && name[1] == 'd') {
				register struct cdevlist *ccl;

				if ((unit = lkupcdev(cl, ctlr, unit, DISK)) != -1)
					goto found;

				/* maybe it's a CDROM */
				ccl = cl;
				for (cl = cdevlist; cl->cl_name; cl++) {
					if (!((*cl->cl_name == 's') &&
					    (*(cl->cl_name+1) == 'r') &&
					    (cl->cl_flags & SG_EXISTS)))
						continue;

					/* Note, use bp_unit as unit is modified */
					if ((unit = lkupcdev(cl, ctlr, bp->bp_unit,
						CDROM)) != -1)
						goto found;
				}
				cl = ccl;

			/* If scsi CDROM, get logical unit. */
			} else if ((name[0] == 's') && (name[1] == 'r')) {
				if ((unit = lkupcdev(cl, ctlr, unit, CDROM)) != -1)
					goto found;

			/* Non-scsi device, we're done. */
			} else {
				goto found;
			}
		}
		goto retry;	/* didn't find it: must be an error? */
	}

found:
	name[0] = cl->cl_name[0];
	name[1] = cl->cl_name[1];
	*fileno = -1;

	/*
	 * If the device is a tape, prompt for the file number
	 * If RB_ASKNAME was set prompt the user.  Otherwise default
	 * to the filenumber stamped into the kernel.
	 */
	if ((bdevsw[major(cl->cl_broot)].d_flags & B_TAPE) != 0) {
		char	buf[40];
		register char	*p = buf;
		register int	num;

		cl->cl_flags |= SG_TAPE;
		if (!(boothowto & RB_ASKNAME) && (loadramdiskfile != -1)) {
			*fileno = loadramdiskfile;
		} else {
			p = buf;
			do {
				printf("Tape file number? ");
				gets(p);
			} while (!*p || *p < '0' || *p > '9');

			for (num = 0; *p && *p >= '0' && *p <= '9'; ++p)
				num = (num * 10) + *p - '0';

			*fileno = num;
		}
	}
	*bdev = sg_form_dev(cl->cl_broot, &name[2], unit, part, cl->cl_flags);
	*cdev = sg_form_dev(cl->cl_croot, &name[2], unit, part, cl->cl_flags);
}

/*
 * Lookup unit number for scsi.  Translates physical unit number
 * (really target/lun number) into kernel logical unit number.
 * Returns logical unit number.  Otherwise, returns -1.
 * XXX: There ought to be a way to combine this with lkupbdev and
 * save some space.
 */
lkupcdev(cl, ctlr, unit, flags)
	register struct cdevlist *cl;
	int ctlr, unit, flags;
{
	register struct mb_device *md = mbdinit;
	int root;
	short maj, minr;

	/* Find entry in table */
	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_alive == 0  ||  md->md_driver != cl->cl_driver  ||
		    md->md_flags != flags  ||  md->md_ctlr != ctlr  ||
	 	    md->md_slave != unit)
			continue;

		/* Fix up minor unit number to point to right unit. */
		if (flags == TAPE)
			minr =  MTMINOR(md->md_unit) + MT_NOREWIND;
		else
			minr = md->md_unit <<3;

		/* Open device and see if it's there. */
		root = cl->cl_croot;
		cl->cl_croot = makedev(major(root), minr);
		maj = major(cl->cl_croot);
		if ((*cdevsw[maj].d_open)(cl->cl_croot, FREAD) == 0) {
			(void)(*cdevsw[maj].d_close)(cl->cl_croot, FREAD);
			cl->cl_croot = root;
			return (md->md_unit);
		}
		cl->cl_croot = root;
	}
	return (-1);
}

chkcdev(cl)
	register struct cdevlist *cl;
{
	register struct mb_device *md;
	register int status = -1;

	for (md = mbdinit; md->md_driver; md++) {
		short	maj;

		/* looking for correct driver and unit number */
		if (md->md_alive == 0 || md->md_driver != cl->cl_driver)
			continue;
#ifdef		not
		/*
		 * If want things exact; but slow...
		 * Then reverse this ifdef.
		 */
		status = 0;
		maj = major(cl->cl_croot);
		if ((*cdevsw[maj].d_open)(cl->cl_croot, FREAD) == 0) {
			(void)(*cdevsw[maj].d_close)(cl->cl_croot, FREAD);
			return (1);
		}
#else		not
		return (1);
#endif		not
	}
	/*
	 * The device was not in the system maintained list of mainbus devices.
	 * Check for a pseudo device that is usable as a root or swap device.
	 */
	if ((cl->cl_driver->mdr_flags & MDR_PSEUDO) != 0)
		return (1);
	return (status);
}


#else	!OPENPROMS

/*
 * The ram disk driver is initialized from another device
 * that contains the image of a 4.2 file system.
 *
 * Return the device number and name of the first
 * char device in the cdevlist that can be opened.
 * Name must be at least 128 bytes long.
 */
int
getchardev(askfor, name, fileno, cdev, bdev)
	char	*askfor,
		*name;
	int	*fileno;	/* -1 or file number on tape */
	dev_t	*cdev, *bdev;
{
	register struct cdevlist *cl;
	int	unit,
		part;
	struct dev_info *dip;

	char devname[OBP_MAXDEVNAME];
	char *path;
	extern char *prom_bootpath();

	if (!cdevlist) {
		if (build_cdevlist() == 0) {
			panic("getchardev: No Devices to boot off of?");
			/*NOTREACHED*/
		}
	}

	/*
	 * see if a default is defined or if we can just use the bootargs
	 */

	if (!(boothowto & RB_ASKNAME)) {
	    if (loadramdiskfrom[0] == '\0') {

		path = prom_bootpath();
#ifdef	DPRINTF
		DPRINTF("getchardev: path <%s> --> ", path ? path : "NONE!");
#endif	DPRINTF

		if (prom_get_boot_dev_name( devname, sizeof devname ) != 0)  {
#ifdef	DPRINTF
		    DPRINTF("getchardev: Error getting devname (tryagain)");
#endif	DPRINTF
		    goto tryagain;
		}

		unit = prom_get_boot_dev_unit();
		part = prom_get_boot_dev_part();

#ifdef	DPRINTF
		DPRINTF("device <%s> unit <%x> part <%d>\n",
		    devname, unit, part);
#endif	DPRINTF

		loadramdiskfrom[0] = devname[0];
		loadramdiskfrom[1] = devname[1];
		loadramdiskfrom[2] = (char)0;

		/*
		 * XXX bad cause IPI has lots of units!
		 */
                loadramdiskfrom[2] = '0' + unit;
                loadramdiskfrom[3] = 'a' + part;
                loadramdiskfrom[4] = '\0';
            }
            (void)strncpy(name, loadramdiskfrom, 16);
 
            for (cl = cdevlist; cl != NULL; cl = cl->cl_next) {
                    if (!(cl->cl_exists & (1 << unit)))
                            continue;
                    if (strncmp(cl->cl_dev->devi_name, name, 2) == 0) {
                            printf("Loading ram disk from %s\n", name);
                            goto got_name;
                    }
            }
	    /*
	     * maybe, if it was "sd" and the unit didn't exist, it
	     * was really "sr" (we boot "sr" as "sd")
	     */
	    if ((name[0] == 's') && (name[1] == 'd')) {
		for (cl = cdevlist; cl != NULL; cl = cl->cl_next) {
		    if (strncmp(cl->cl_dev->devi_name, "sr", 2))
			continue;
		    /* we found "sr", does it exist? */
		    /* XXX NOTE: ASSUMES only 1 cdrom! */
		    if (!(cl->cl_exists))
			continue;
		    /* it lives, so fix things up */
		    name[1] = 'r';
		    unit = 0;	/* XXX ASSUMES only 1 CDROM */
		    /*
		     * we leave partition as is, for rd.c rd_offset
		     * rd.c will strip out before doing open
		     */
		    printf("Loading ram disk from %s\n", name);
			goto got_name;
		    }
		}
	}
tryagain:

	for (unit = -1; unit == -1; /* empty */) {
		printf("%s device (", askfor);
		for (cl = cdevlist; cl != NULL; cl = cl->cl_next)
			if (cl->cl_exists)
				printf("%s%%d[a-h] ", cl->cl_dev->devi_name);
		printf("): ");
		gets(name);
		if (*name == '\0') {
			cl = cdevlist;
			unit = 0;
			part = 0;
			break;
		}

		for (cl = cdevlist; cl != NULL; cl = cl->cl_next) {
			if (strncmp(cl->cl_dev->devi_name, name, 2) == 0)
				break;
		}
		if (!cl) {
			printf(
			    "Unknown device name '%c%c'\n", name[0], name[1]);
			continue;
		}
		if (name[2] < '0' || name[2] > '7') {
			printf("bad/missing unit number\n");
			continue;
		}
		unit = name[2] - '0';
		if (name[3] == '\0') {
			part = 0;
		} else if (name[3] >= 'a' && name[3] <= 'h') {
			part = name[3] - 'a';
		} else {
			printf("bad partition (use a-h)\n");
			unit = -1;
		}
	}

	name[0] = cl->cl_dev->devi_name[0];
	name[1] = cl->cl_dev->devi_name[1];
	name[2] = '0' + unit;
	name[3] = 'a' + part;
	name[4] = '\0';

got_name:

	/* If the device is a tape, prompt for the file number */
	if ((bdevsw[major(cl->cl_broot)].d_flags & B_TAPE) != 0) {
		char	buf[40];
		register char	*p = buf;
		register int	num;

		if (!(boothowto & RB_ASKNAME) && (loadramdiskfile != -1)) {
			*fileno = loadramdiskfile;
		} else {
			p = buf;
			do {
				printf("Tape file number? ");
				gets(p);
			} while (!*p || *p < '0' || *p > '9');

			for (num = 0; *p && *p >= '0' && *p <= '9'; ++p)
				num = (num * 10) + *p - '0';

			*fileno = num;
		}
		*bdev = makedev(major(cl->cl_broot),
			minor(cl->cl_broot)|unit|part);
		*cdev = makedev(major(cl->cl_croot),
			minor(cl->cl_croot)|unit|part);
	} else {
		*fileno = -1;

		*bdev = makedev(major(cl->cl_broot), (unit * 8 + part));
		*cdev = makedev(major(cl->cl_croot), (unit * 8 + part));
	}


	/*
	 * This should be also be pertinent for
	 * sun3x/80 or other ejectable floppy machines.
	 *
	 *
	 * if an auto ejectable floppy, we must eject the one we booted
	 * off of so the filesystem can be loaded
	 * NOTE: we can't open/do ioctls to the device - it isn't mapped in yet.
	 */
	/*
	 * FIXME FIX ME XXX should really do either...
	 * if (cmajor(cdev) == 54 || cmajor(cdev) == Maj#of_sf)
	 */
	if (name[0] == 'f' && name[1] == 'd') {
		/* turn on drive select and (later eject) */
#ifdef sun4c
		Auxio |= (AUX_MBO | AUX_EJECT); /* pre-set eject ?proms broke?*/
		Auxio |= (AUX_MBO | AUX_DRVSELECT);
		DELAY(100);
		Auxio = (Auxio | AUX_MBO) & ~AUX_EJECT; /* eject is low true */
		DELAY(100);
		/* now turn off both eject and select, MBO = Must Be Ones! */
		Auxio = (Auxio | AUX_MBO | AUX_EJECT) & ~AUX_DRVSELECT;
		printf(
		    "please insert diskette \"B\", press any key to continue:");
#else sun4c
		printf("please insert next diskette, type any key to continue");
#endif sun4c
		if (getchar() != '\n')
			printf("\n");
	}
}
#endif	!OPENPROMS
#endif NRD > 0

#ifndef	OPENPROMS
/*
 * Lookup unit number for scsi.  Translates physical unit number
 * (really target/lun number) into kernel logical unit number.
 * Returns logical unit number.  Otherwise, returns -1.
 * XXX: There ought to be a way to combine this with lkupcdev and
 * save some space.
 */
lkupbdev(bl, ctlr, unit, flags)
	register struct bdevlist *bl;
	int ctlr, unit, flags;
{
	register struct mb_device *md = mbdinit;
	int root;
	short maj, minr;

	/* Find entry in table */
	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_alive == 0  ||  md->md_driver != bl->bl_driver  ||
		    md->md_flags != flags  ||  md->md_ctlr != ctlr  ||
	 	    md->md_slave != unit)
			continue;

		/* Fix up minor unit number to point to right unit. */
		if (flags == TAPE)
			minr =  MTMINOR(md->md_unit) + MT_NOREWIND;
		else
			minr = md->md_unit <<3;

		/* Open device and see if it's there. */
		root = bl->bl_root;
		bl->bl_root = makedev(major(root), minr);
		maj = major(bl->bl_root);
		if ((*bdevsw[maj].d_open)(bl->bl_root, FREAD) == 0) {
			(void)(*bdevsw[maj].d_close)(bl->bl_root, FREAD);
			bl->bl_root = root;
			return (md->md_unit);
		}
		bl->bl_root = root;
	}
	return (-1);
}

/*
 * Check for block device presence.  Returns -1 (no controller),
 * 0 (controller present but no unit), or 1 (controller and unit present).
 */
chkroot(bl)
        register struct bdevlist *bl;
{
        register struct mb_device *md;
        register int status = -1;

        for (md = mbdinit; md->md_driver; md++) {
                short   maj;

                if (md->md_alive == 0 || md->md_driver != bl->bl_driver)
                        continue;
                status = 0;
                maj = major(bl->bl_root);
                if ((*bdevsw[maj].d_open)(bl->bl_root, FREAD) == 0) {
                        (void)(*bdevsw[maj].d_close)(bl->bl_root, FREAD);
                        return (1);
                }
        }
        /*
         * The device was not in the system maintained list of mainbus devices.
         * Check for a pseudo device that is usable as a root or swap device.
         */
        if ((bl->bl_driver->mdr_flags & MDR_PSEUDO) != 0)
                return (1);
        return (status);
}
#endif	!OPENPROMS

/*
 * These routines moved outside the "ifndef OPENPROMS" as they
 * are needed for VME disk boot.
 */

/*
 * Parse device name into unit and partition number.
 * The name is of one of two forms:
 *      decimal:   2 char name, 1-2 digit unit, 1 char partition.
 *      hex:       2 char name, 3 hex unit, 1 char partition.
 * Returns non-zero if name in good form.
 *      Sets unit number pointed to by unitp to -1 if badly parsed.
 *      Leaves unit number and partition at old value if not entered.
 */
static int
sg_parse_name(name, unitp, partp, flags)
        char    *name;
        int     *unitp, *partp;
        int     flags;
{
        int     i, c, unit;
        char    *cp;

        cp = name + 2;
        if (*cp == '\0')                /* no unit number specified - OK */
                return (1);

        if (flags & SG_HEXUNIT) {
                unit = 0;
                for (i = 0; i < SG_NHEX; i++) {
                        c = *cp++;
                        if (c == '\0') {
                                printf(
                                    "unit number too short.  specify 3 hex\n");
                                *unitp = -1;
                                return (0);
                        }
                        unit <<= 4;
                        if (c >= '0' && c <= '9') {
                                unit += c - '0';
                        } else if (c >= 'a' && c <= 'f') {
                                unit += c - 'a';
                        } else if (c >= 'A' && c <= 'F') {
                                unit += c - 'A';
                        } else {
                                printf("bad hex digit '%c'\n", c);
                                *unitp = -1;
                                return (0);
                        }
                }
        } else {
                if ((c = *cp++) < '0' || c > '9') {
                        printf("bad unit number.\n");
                        *unitp = -1;
                        return (0);
                }
                unit = c - '0';
                /*
                 * handle optional second digit.
                 */
                if ((c = *cp) >= '0' && c <= '9') {
                        unit *= 10;
                        unit += c - '0';
                        cp++;
                }
        }
        *unitp = unit;
        if ((c = *cp) == '\0')
                return (1);             /* no partition - allow default */
 
        if (c >= 'a' && c <= 'h') {
                *partp = c - 'a';
        } else {
                printf("bad partition (use a-h)\n");
                *unitp = -1;            /* flag error */
                return (0);
        }
        return (1);
}

/*
 * form device number and name for unit, partition, and flags.
 */
static dev_t
sg_form_dev(base_dev, str, unit, part, flags)
        dev_t   base_dev;       /* starting device major/minor number */
        char    *str;           /* place to put ascii unit/partition */
        int     unit, part;             /* unit and partition */
        int     flags;
{
        int     majdev = major(base_dev);
        int     mindev = minor(base_dev);
        int     i;
 
        if (flags & SG_HEXUNIT) {
                /* Process hex unit numbers (e.g. IPI devices) */
                for (i = (SG_NHEX-1)*4; i >= 0; i -= 4) /* put 3 hex */
                        *str++ = "0123456789abcdef"[(unit >> i) & 0xf];
                /*
                 * Bit 7 of the unit number must be zero and isn't used in
                 * calculating the major/minor number.  This is so it fits
                 * better into a limited number of major numbers.  This bit
                 * corresponds to the high order bit of the slave number, which
                 * ranges 0-7.
                 */
                unit = (unit & ~0xff) >> 1 | unit & 0x7f;
                unit = unit * 8 + part;
                majdev += (unit >> 8) & 0xff;
                mindev += unit & 0xff;
        } else {
                /* Process decimal unit numbers (default) */
                if (unit >= 10) {
                        *str++ = '0' + (unit / 10);
                        unit %= 10;
                }
                *str++ = '0' + unit;
                if (flags & SG_TAPE) {
                        /* tape minor number */
                        mindev = MTMINOR(unit) + MT_NOREWIND;
                } else {
                        /* disk minor number */
                        mindev |= (unit * 8 + part);
                }
        }
        *str++ = 'a' + part;
        *str = '\0';
        return (makedev(majdev, mindev));
}

getchar()
{
	register c;

	c = cngetc();
	if (c == '\r')
		c = '\n';
#ifdef	sun4c
	else if (c == 0177)
		return (c);
#endif
	cnputc(c);
	return (c);
}

gets(cp)
	char *cp;
{
	register char *lp;
	register c;

	lp = cp;
	while (1) {
		c = getchar() & 0177;
		switch (c) {

		case '\n':
			*lp++ = '\0';
			return;

		case 0177:
			if (lp > cp) {
				cnputc('\b');
			}
		case '\b':
			if (lp > cp) {
				cnputc(' ');
				cnputc('\b');
			}
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;

		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;

		default:
			*lp++ = c;
		}
	}
}
static
fubar()
{
	/* this is really ugly, but it makes cc -O work */
}
