#if !defined(lint) && !defined(BOOTBLOCK)
static char sccsid[] = "@(#)init_devsw.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1988 Sun Microsystems, Inc.
 *
 * Devswitch support routines for the Open Boot Proms.
 *
 */

#include <sys/param.h>
#include <sun/autoconf.h>
#include <sun/openprom.h>
#include <mon/sunromvec.h>
#include <string.h>
#include "boot/conf.h"

#define	NEXT		prom_nextnode
#define	CHILD		prom_childnode
extern char *getlongprop();

char bdevnames[8][3];		/* 3 chars each (2 letters and Null) */

extern struct boottab *(devsw[]);
extern struct bdevlist bdevlist[];
extern struct boottab blkdriver, tapedriver, ifdriver;
extern struct ndevlist ndevlist[];

/*
 * Initial indices for the devsw[] and bdevlist[]. Note that the bdevlist[]
 * has to start at entry [1]! Device [0] is defined to be NODEV and
 * the open routine fails.
 */
static int dev = 0;
static int bdev = 1;

#ifndef sun4m
static void walk(), check_this_node(), add_to_devsw();
#else
static void add_to_devsw();
#endif !sun4m

/*
 * At run time, walk the dev_info structure in the monitor and find all
 * "block", "byte", and "network" devices. Fill in the devsw and bdevlist
 * tables with their names and driver entry points.
 */

init_devsw()
{
	if (prom_getversion() > 0)
		return (init_devlist());

#ifndef sun4m
	walk(NEXT(0));		/* Get the root node */
#endif !sun4m
}

#ifndef sun4m
/*
 * CS201: Data Structures. MWF 11:00-11:50 Wilmot 214. 4 credits. Grade: B-
 */
static void
walk(id)
	int id;
{
	register int curnode;
	register char *c;
	register char *s;

	if (id)
		check_this_node(id);
	if (curnode = CHILD(id))
		walk(curnode);
	if (curnode = NEXT(id))
		walk(curnode);
}


static void
check_this_node(id)
	int id;
{
	register char *name, *type;
	register char *s1, *s2;
	char	*dupstring();

	type = name = (char *)0;
	name = getlongprop(id, "name");
	if (((int)name == 0) || ((int)name == -1))
		return;
	type = getlongprop(id, "device_type");
	if ((int)type == -1)
		type = (char *)0;
	if (strncmp(type, "block", 5) == 0) {
		add_to_devsw(name, &blkdriver, dev);
		(void)strncpy(bdevnames[bdev], name, 3);
		bdevlist[bdev].bl_name = bdevnames[bdev];
		bdevlist[bdev].bl_root = makedev(bdev, 0);
		bdevlist[bdev++].bl_driver = devsw[dev++];
	} else if (strncmp(type, "byte", 4) == 0) {
		add_to_devsw(name, &tapedriver, dev);
		(void)strncpy(bdevnames[bdev], name, 3);
		bdevlist[bdev].bl_name = bdevnames[bdev];
		bdevlist[bdev].bl_root = makedev(bdev, 0);
		bdevlist[bdev++].bl_driver = devsw[dev++];
	} else if (strncmp(type, "network", 7) == 0) {
		add_to_devsw(name, &ifdriver, dev++);
	}
}
#endif !sun4m

static void
add_to_devsw(name, driver, i)
	char *name;
	struct boottab *driver;
	int i;
{
	register struct boottab *bp;

	bp = (struct boottab *)kmem_alloc(sizeof (struct boottab));
	if (bp == 0)
		_stop("kmem_alloc: couldn't get space");
	*bp = *driver;
	/*
	 * b_dev has only 2 characters for the device name whinch is
	 * not enough space for the OBP full path.  For OBP interface,
	 * tempararily use b_desc field to keep the full path name
	 */
	if (prom_getversion() > 0) {
		bp->b_dev[0] = '\0';
		bp->b_desc = name;
	} else
		(void)strncpy(bp->b_dev, name, 2);
	devsw[i] = bp;
}

init_devlist()
{

	register	char	*name;

	name = (char *)kmem_alloc(128);
	if (name == 0)
		_stop("kmem_alloc: couldn't get space");
	bzero((caddr_t)name, 128);
	add_to_devsw(name, &blkdriver, dev);
	bdevlist[1].bl_name = name;
	bdevlist[1].bl_root = makedev(1, 0);
	bdevlist[1].bl_driver = devsw[dev++];

	name = (char *)kmem_alloc(128);
	if (name == 0)
		_stop("kmem_alloc: couldn't get space");
	bzero((caddr_t)name, 128);
	add_to_devsw(name, &ifdriver, dev);
	ndevlist[0].nl_name = name;
	ndevlist[0].nl_root = 0;
	ndevlist[0].nl_driver = devsw[dev++];
}
