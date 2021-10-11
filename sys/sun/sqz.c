#ifndef lint
static	char sccsid[] = "@(#)sqz.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * SQueeZe driver -- enable restricting the amount of main memory
 * available without having to patch physmem and reboot; hopefully,
 * this can become the System Query driver.
 *
 * NB: This is only an approximation, as things like lotsfree and
 * desfree are *not* recalculated, and the pagedaemon will still look at
 * the allocated pages, adding to system overhead.
 */

/*
 * XXX:
 *	should permit can't wait mode/flag
 *	would be nice if it did auto minor allocation
 *	since rm_allocpage() does atomic allocation, if you ask for too many
 *	    pages (for example, more than lotsfree), you can sleep forever
 */

/* XXXGIANTDATA is used to avoid the gigantic data section bug */

#include "sqzd.h"
#if NSQZD > 0
#include <sys/param.h>
#include <sys/errno.h>
#include <sun/sqz.h>
#include <vm/page.h>
#include <vm/rm.h>
#include <vm/seg.h>

void	sqzdrop(), page_splice();

struct	sqz_info {
	struct page *plist;		/* list of pages held */
	int count;			/* count of pages in the list */
	int open;			/* flag for exclusive open */
}	sqz_info[NSQZD];

int	sqz_total;			/* total size held (in pages) */

/* exclusive open the device, and initialize the associated info */
/*ARGSUSED*/
int
sqzopen(dev, flags)
	dev_t dev;
	int flags;
{
	register struct sqz_info *sqz;

	if (minor(dev) >= NSQZD)
		return (ENXIO);
	sqz = &sqz_info[minor(dev)];
	if (sqz->open)
		return (EBUSY);

	sqz->open++;
	sqz->count = 0;
	sqz->plist = NULL;

	return (0);
}

/* close the device and release the associated storage */
/*ARGSUSED*/
int
sqzclose(dev, flags)
	dev_t dev;
	int flags;
{
	register struct sqz_info *sqz = &sqz_info[minor(dev)];

	sqzdrop(sqz, sqz->count);
	if (sqz->plist)
		panic("sqzclose");
	sqz->open = 0;

	return (0);
}

/* process the ioctl requests */
/*ARGSUSED*/
int
sqzioctl(dev, cmd, data, flags)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flags;
{
	register struct sqz_info *sqz = &sqz_info[minor(dev)];
	int count, delta;
	int result = 0;
	struct sqz_status *ss;

	switch (cmd) {
	case SQZSET:
		count = *(int *)data;
		if (count < 0) {
			result = EINVAL;
			break;
		}
		delta = count - sqz->count;
		if (delta < 0)
			sqzdrop(sqz, -delta);
		else if (delta > 0)
			result = sqzadd(sqz, delta);
		break;

	case SQZGET:
		ss = (struct sqz_status *)data;
		ss->count = sqz->count;
		ss->total = sqz_total;
		break;

	default:
		result = EINVAL;
		break;
	}

	return (result);
}

/* acquire count pages and hold them */
int
sqzadd(sqz, count)
	struct sqz_info *sqz;
	int count;
{
	int result = 0;
#ifdef XXXGIANTDATA
	struct page *pp;
#else
	struct page *pp, **ppp;
#endif

	pp = rm_allocpage((struct seg *)0, (addr_t)0, (u_int)ptob(count), 1);
	if (pp == NULL)
		result = ENOMEM;
	else {
#ifdef XXXGIANTDATA
		page_splice(&sqz->plist, pp);
#else
		ppp = &sqz->plist;
		page_splice(ppp, pp);
#endif
		sqz->count += count;
		sqz_total += count;
	}

	return (result);
}

/* release count pages */
void
sqzdrop(sqz, count)
	struct sqz_info *sqz;
	int count;
{
	struct page **ppp, *pp;

	sqz->count -= count;
	sqz_total -= count;

	ppp = &sqz->plist;
	while (count--) {
		pp = *ppp;
#ifdef XXXGIANTDATA
		/* verify pristine page which page_rele will free */
		if (pp->p_keepcnt != 1 || pp->p_vnode != NULL)
			panic("sqzdrop");
#endif XXXGIANTDATA
		page_sub(ppp, pp);
		page_rele(pp);
	}
}

/* splice the second linked list onto the *end* of the first */
void
page_splice(ppp, pp)
	struct page **ppp, *pp;
{
	struct page *head1, *tail1, *head2, *tail2;

	if (*ppp == NULL)
		*ppp = pp;
	else if (pp == NULL)
		;	/* nothing to do */
	else {
		head1 = *ppp;
		head2 = pp;
		tail1 = head1->p_prev;
		tail2 = head2->p_prev;

		tail1->p_next = head2;
		head2->p_prev = tail1;

		tail2->p_next = head1;
		head1->p_prev = tail2;
	}
}
#endif NSQZD > 0
