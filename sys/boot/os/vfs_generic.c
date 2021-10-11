
#ifndef lint
static  char sccsid[] = "@(#)vfs_generic.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif


/*
 * Mostly stolen from estale:/usr.MC68020/estale/sys/sys/sun/swapgeneric.c
 */

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/vm.h>
#include "boot/systm.h"
#include <sys/reboot.h>
#include <sys/file.h>
#include <sys/vfs.h>
#undef KERNEL
#undef NFS
#include <sys/mount.h>

#include <machine/pte.h>
#include <sundev/mbvar.h>
#include <mon/sunromvec.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include "boot/protosw.h"
#include <net/if.h>
#include <stand/saio.h>
#include "boot/conf.h"

#undef MOUNT_MAXTYPE
#define	MOUNT_MAXTYPE  2


extern struct bdevlist bdevlist[];
#ifdef OPENPROMS
extern struct ndevlist ndevlist[];
#endif

int	dump_debug = 10;

/*
 * What it does:
 */

struct vfssw *
getfstype(askfor, fsname)
	char *askfor;
	char *fsname;
{
	int fstype;

	if (boothowto & RB_ASKNAME) {
		for (fsname[0] = '\0'; fsname[0] == '\0'; ) {
			printf("%s filesystem type ( ", askfor);
			for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
				if (vfssw[fstype].vsw_name)
					printf("%s ", vfssw[fstype].vsw_name);
			}
			printf(" ): ");
			(void) gets(fsname);
			for (fstype = 0; fstype < MOUNT_MAXTYPE; fstype++) {
				if (vfssw[fstype].vsw_name == 0) {
					continue;
				}
				if (*fsname == '\0') {
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
	}
	return ((struct vfssw *)0);
}

/*
 * If booted with ASKNAME prompt on the console for a filesystem
 * name and return it.
 */
getfsname(askfor, name)
	char *askfor;
	char *name;
{

	if (boothowto & RB_ASKNAME) {
		printf("%s name: ", askfor);
		(void) gets(name);
	}
}

/*
 * Return the device number and name of the frist block
 * block device in the bdevlist that cna be opened.
 * If booted with the -a flag ask user for device name.
 * Name must be at least 128 bytes long.
 * askfor is one of "root" or "swap".
 */
dev_t
getblockdev(askfor, name)
	char *askfor;
	char *name;
{
	register struct bdevlist *bl;
	register int i;
	register char *cp;
	int unit;
	int part;
	int ctlr = 0;
	dev_t	dev;

#ifdef OPENPROMS
	extern struct bootparam *prom_bootparam();
	dev_t	obp_getblockdev();

	if (prom_getversion() > 0)
		return (obp_getblockdev(askfor, name));
#endif

#ifndef sun4m
	if (boothowto & RB_ASKNAME) {
		for (unit = -1; unit == -1; ) {
			printf("%s device ( ", askfor);
			for (bl = bdevlist; bl->bl_name; bl++)
			    if (strcmp(bl->bl_name, "") != 0)
				if (bl->bl_flags == 0)
				    printf("%s%cd[a-h] ", bl->bl_name, '%');
				else
				    printf("%s%c3x[a-h] ", bl->bl_name, '%');
			printf("): ");
			(void) gets(name);
			if (*name == '\0') {
			    bl = bdevlist;
			    unit = 0;
			    part = 0;
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

			/* Get unit from name string */
			if (bl->bl_flags == 0) {
			    if (name[2] < '0' || name[2] > '7') {
				printf("bad/missing unit number\n");
				continue;
			    }
			} else {
			    if (name[2] != '0' || name[3] != '0' ||
				(!((name[4] >= '0' && name[4] <= '9') ||
				(name[4] >= 'a' && name[4] <= 'f'))))  {
				printf("bad/missing unit number\n");
				continue;
			    }
			}
			/*
			 * FIXME: We need something like a conf table
			 * (e.g. GENERIC) to make the device name into
			 * the REAL device info.  This HACK fixes
			 * up SCSI for the time being.  Note, the
			 * controller is always 0.
			 */
			if (bl->bl_flags == 0)
			    unit = name[2] - '0';
			else {
			    if (name[4] >= '0' || name[4] <= '9')
				unit = name[4] - '0';
			    else
				unit = 10 + name[4] - 'a';
			    ctlr = name[3] - '0';
			}
#ifndef OPENPROMS
			if (name[0] == 's' && name[1] == 'd')
				unit = (unit & 0xFE) <<2 | (unit & 0x01);
#endif
			/* Get partition from name string */
			if (bl->bl_flags ==0) {
			    if (name[3] == '\0') {
				part = 0;
			    } else if (name[3] >= 'a' && name[3] <= 'h') {
				part = name[3] - 'a';
			    } else {
				printf("bad partition (use a-h)\n");
				unit = -1;
			    }
			} else {
			    if (name[5] == '\0') {
				part = 0;
			    } else if (name[5] >= 'a' && name[5] <= 'h') {
				part = name[5] - 'a';
			    } else {
				printf("bad partition (use a-h)\n");
				unit = -1;
			    }
			}
		}
	} else {
		unit = 0;
		part = 0;
		/*
		 * If a name was given find that block device
		 * If rootfs is not initialized, this doesn't happen
		 */
		if (*name != '\0') {
			for (bl = bdevlist; bl->bl_name; bl++) {
				if (bl->bl_name[0] == name[0] &&
				    bl->bl_name[1] == name[1]) {
					if (chkroot(bl)) {
						if (name[2] != '\0')
							unit = name[2] - '0';
						if (name[3] != '\0')
							part = name[3] - 'a';
						goto found;
					}
				}
			}
			printf("%c%c%c%c not configured in boot\n",
				name[0], name[1], name[2], name[3]);
			return ((dev_t)0);
		}
		/*
		 * Look for device we booted from
		 */
		for (bl = bdevlist; bl->bl_name; bl++) {
#ifdef	OPENPROMS
			register struct bootparam *bp = prom_bootparam();
#else	OPENPROMS
			register struct bootparam *bp = (*romp->v_bootparam);
#endif	OPENPROMS

			if (!((*bl->bl_name == bp->bp_dev[0]) &&
				(*(bl->bl_name+1) == bp->bp_dev[1])))
				continue;
			/*
			 * This is a cheat.   Since we don't support
			 * the full apparatus of the mb_device list,
			 * we assume the device we were booted from
			 * is openable if we located it in the
			 * bdevlist..
			 */
			ctlr = bp->bp_ctlr;
			unit = bp->bp_unit;
			part = bp->bp_part;
			goto found;
		}

		/*
		 * Use first device that can be opened
		 */
		for (bl = bdevlist; bl->bl_name; bl++) {
			if (chkroot(bl)) {
				goto found;
			}
		}
		return ((dev_t)0);
	}

found:
	name[0] = bl->bl_name[0];
	name[1] = bl->bl_name[1];
	cp = name + 2;
#ifndef OPENPROMS
	/*
	 * FIXME: We need something like a conf table
	 * to track kernel conf file changes.
	 */
	if (name[0] == 's' && name[1] == 'd')
		unit = ((unit & 0xF8) >>2 | (unit & 0x01));
#endif OPENPROMS
	if (bl->bl_flags) {
		*cp++ = '0';
		*cp++ = '0' + ctlr;
		*cp++ = '0' + unit;
	} else {
		*cp++ = '0' + unit;
	}
	cp[0] = 'a' + part;
	cp[1] = '\0';
	dev = makedev(major(bl->bl_root), (ctlr << 6 | unit << 3 | part));
	return dev;
#endif !sun4m
}

#ifdef OPENPROMS
/*
 * Return the device number and name of the frist block
 * block device in the bdevlist that cna be opened.
 * If booted with the -a flag ask user for device name.
 * Name must be at least 128 bytes long.
 * askfor is one of "root" or "swap".
 */

#define	ASKBUFSIZE	(256)

dev_t
obp_getblockdev(askfor, name)
	char *askfor;
	char *name;
{
	register struct bdevlist *bl;
	register struct boottab  *dp;
	register int unit;
	register char *cp, *p;
	extern char *prom_bootpath();
	register char *bpath = prom_bootpath();
	char *rootname = name;
	char *kmem_alloc();
	static char *prompt = "%s (block) device ('*' for list) : ";
	extern int Mustask;
	void prom_interpret();

	bl = &bdevlist[1];
	dp = bl->bl_driver;
	strncpy(dp->b_dev, "bl", 2);
	if (boothowto & RB_ASKNAME) {
		cp = kmem_alloc(ASKBUFSIZE);
		while (1)  {
			printf("Default %s device: %s\n", askfor, bpath);
			printf(prompt, askfor);
			(void) gets(cp);
			if (*cp == (char)0)  {	/* Default? */
				strcpy(cp, bpath);
				printf("%s (block) device : %s\n",
				    askfor, bpath);
				break;
			}
			if (strcmp("*", cp) != 0)
				break;
			printf("\nDevice tree info:\n");
			prom_interpret("show-devs");
			printf("\nAvailable device aliases:\n");
			prom_interpret("devalias");
		}
		name = bl->bl_name;
		(void)strcpy(name, cp);

		/*
		 * Allow user to specify partition as an option character.
		 */

		if (*(p = name) != (char)0)  {
			while (*p++ != (char)0)	/* Partition specified? */
				;
			p -= 3;		/* Back up over '\0', 'a', : */
		}

		if (*name && (*name == '/') && (*p != ':')) {
			unit = -1;

			do {
				printf("Partition ('a' - 'h') : ");
				(void)gets(cp);
				if (*cp && (*cp < 'a' || *cp > 'h'))  {
					printf("Invalid partition number.\n");
				} else
					unit = 0;
			} while (unit != 0);
			if (*cp) {
				for (; *name; name++);
				*name++ = ':';
				*name++ = *cp;
				*name = '\0';
			}
		}

		/*
		 * Try to update bootpath ... PROM devalias expansion needed
		 */

		if (*(bl->bl_name) == '/')
			(void)strcpy(bpath, bl->bl_name);
		else
			Mustask++;

		kmem_free(cp, ASKBUFSIZE);
	} else {
		/*
		 * If a name was given find that block device
		 * If rootfs is not initialized, this doesn't happen
		 */
		if (*name != '\0') {
			/*
			 * chkroot() always return 0 anyway, no point
			 * to go through the checking
			 */
			printf("%s not configured in boot\n", name);
			return ((dev_t)0);
		}
#ifdef  DUMP_DEBUG
		dprint(dump_debug, 10, "bpath = %s\n", bpath);
#endif  /* DUMP_DEBUG */
		/*
		 * use the boot device
		 */
		strcpy(bl->bl_name, bpath);
	}

found:
	strcpy(rootname, bl->bl_name);
	return (makedev(major(bl->bl_root), 0));
}

/*
 * Return the device number and name of the frist net
 * device in the ndevlist that can be opened.
 * If booted with the -a flag ask user for device name.
 * Name must be at least 128 bytes long.
 * askfor is "ether" now.
 */

dev_t
getnetdev(askfor)
	char *askfor;
{
	register struct ndevlist *nl;
	register struct boottab  *dp;
	extern char *prom_bootpath();
	register char *bpath = prom_bootpath();
	static char *prompt = "%s device ('*' for list) : ";
	extern int Mustask;
	extern void prom_interpret();

	nl = &ndevlist[0];
	dp = nl->nl_driver;
	strncpy(dp->b_dev, "nl", 2);
	if (boothowto & RB_ASKNAME) {
		while (1)  {
			printf("Default %s device: %s\n", askfor, bpath);
			printf(prompt, askfor);
			(void) gets(nl->nl_name);
			if (*(nl->nl_name) == (char)0)  {	/* default? */
				strcpy(nl->nl_name, bpath);
				printf("%s device : %s\n", askfor, bpath);
				break;
			}
			if (strcmp("*", nl->nl_name) != 0)  {

				/*
				 * Try to update bootpath ...
				 * PROM devalias expansion needed!
				 */

				if (*(nl->nl_name) == '/')
					strcpy(bpath, nl->nl_name);
				else
					Mustask++;
				break;
			}

			printf("\nDevice tree info:\n");
			prom_interpret("show-devs");
			printf("\nAvailable device aliases:\n");
			prom_interpret("devalias");
		}
	} else
		strcpy(nl->nl_name, bpath);

found:
	return (nl->nl_root);
}
#endif	OPENPROMS

/*ARGSUSED*/
chkroot(bl)
	register struct bdevlist *bl;
{
#ifdef lint
	bl = bl;
#endif lint
	return (0);
}

#ifdef OPENPROMS
#define	millitime()	prom_gettime()
#else
#define	millitime()	(*romp->v_nmiclock)
#endif !OPENPROMS
#define	SLEEPTIME	5	/* in seconds */
sleep (addr, prio)
{
	register int time, i;
#ifdef  DUMP_DEBUG
	dprint(dump_debug, 10,
		"sleep (addr 0x%x, prio 0x%x)\n",
		addr, prio);
#endif  /* DUMP_DEBUG */
	/*
	 * this used to be a panic, but let's just spin a while
	 * and see what happens
	 */
	for (i = 0, time = millitime(); i < 10000000; i++)
	    if (millitime() > (time + SLEEPTIME*1000))
		break;

}

/*
 * Arrange that (*fun)(arg) is called in t/hz seconds.
 */
timeout(fun, arg, t)
	int (*fun)();
	caddr_t arg;
	register int t;
{
#ifdef lint
	t = t;
#endif lint
	{
	int	i;

		for (i = 1; i > 0; i--) {
			if (get_udp() != 0) return;
		}
	}

	(*fun)(arg);
	return;
}

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	register caddr_t chan;
{
#ifdef  DUMP_DEBUG
	dprint(dump_debug, 0,
		"wakeup(chan 0x%x)\n",
		chan);
#endif  /* DUMP_DEBUG */

}
