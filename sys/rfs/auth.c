/*	@(#)auth.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/auth.c	10.8" */
#include <sys/param.h>
#include <sys/types.h>
#include <rfs/sema.h>
#include <sys/sysmacros.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/kmem_alloc.h>
#include <rfs/rfs_misc.h>
#include <rfs/comm.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/idtab.h>
#include <rfs/rdebug.h>

#ifndef BUFSIZ
#define	BUFSIZ	1024
#endif
#ifndef NULL
#define	NULL 0
#endif
#ifndef TRUE
#define	TRUE 1
#endif
#ifndef FALSE
#define	FALSE 0
#endif
#undef HEADSIZE
#define	HEADSIZE	(sizeof (struct idhead)/sizeof (struct idtab))
#define	CACHESIZE	5
#define	NBPC		2048
#define	MAXALLOC	(NBPC - (2 * sizeof (union heap)))
#define	MAXNAMES	100	/* maximum # of names for auth list	*/
#define	MAX_MLINKS	2	/* max number of gdp links per machine	*/
#define	H_BUSY	0x01
typedef long ALIGN;
#define	isbusy(x) (((unsigned int) (x)) & H_BUSY)
#define	mkbusy(x) (((ALIGN) (x)) | H_BUSY)
#define	mkfree(x) (((ALIGN) (x)) & ~H_BUSY)
#define	howbig(x) ((mkfree(x->h_next)?mkfree(x->h_next):\
			Pavail) - ((char *)(x+1)-Heap))
#define	ASIZE	sizeof (ALIGN)
#define	otop(o)	((union heap *) (Heap + (((unsigned)(o)) & ~H_BUSY)))
#define	ptoo(p) ((ALIGN) ((char *)(p) - Heap))
#define	bump(s) ((s) + (((s)%ASIZE)?ASIZE-((s)%ASIZE):0))

#define	TRTABPRI	PWAIT

/*
 * check(tabptr, value, return) compares tabptr (c in macro) with value (v)
 *	and if there is a match sets return (r) to the remote value.
 *	It returns, 0 if there is a match, 1 if c is higher in the table
 *	than v, and -1 if c is lower in the table than v.
 */
#define	UF	0xFFFF
#define	check(c, v, r) \
	((c->i_rem == v) ?\
	    ((c->i_loc == UF) ? (r = (c + 1)->i_loc, 0) : (r = c->i_loc, 0)) :\
	((c->i_rem < v) ?\
	    ((c->i_loc == UF && (c + 1)->i_rem >= v) ?\
		(r = (c + 1)->i_loc, 0) : -1) :\
	    (((c-1)->i_loc == UF && (c-1)->i_rem <= v)?\
		(r = c->i_loc, 0) : 1)))

static	char	*Global[2] = {0, 0};	/* global default table ref	*/
static	int	Tabwlock = 0;		/* Table write lock		*/
static	int	Tabrlock = 0;		/* Table read lock		*/
static	int	Want_wr = 0;		/* I want to write		*/
static	int	Want_rd = 0;		/* I want to read		*/
static	void	wr_lock();		/* Lock for writers		*/
static	void	rd_lock();		/* Lock for readers		*/
static	void	wr_unlock();		/* Unlock for writers		*/
static	void	rd_unlock();		/* Unlock for readers		*/
char	Domain[MAXDNAME+1]="";		/* domain name for this machine	*/

union heap {
	ALIGN		h_align;	/* put in to assure alignment	*/
	long	h_next;
};

static char	*Heap=NULL;		/* actual location of heap	*/
static int	Pavail=0;		/* free bytes			*/

int
auth_init()
{
	Heap = NULL;
	Global[0] = NULL;
	Global[1] = NULL;
}
/*
 * allocate a block of size bytes from the heap.
 */
char *
h_alloc(size)
int	size;
{
	register union heap	*hp;
	union heap		*thp;
	char	*cp;
	int	fsize;

	DUPRINT2(DB_MNT_ADV, "h_alloc(0x%x)\n", size);
	wr_lock();
	/* first time	*/
	if (Heap == NULL) {
		Heap = rfheap;
		Pavail = rfsize;
		hp = (union heap *) Heap;
		hp->h_next = 0;
	}
	size = bump(size);	/* set size so next alloc is aligned	*/
	fsize = size + sizeof (union heap);
	if (fsize > MAXALLOC) {
		DUPRINT1(DB_MNT_ADV, "h_alloc failed, ENOMEM (size too big)\n");
		u.u_error = ENOMEM;
		wr_unlock();
		return (NULL);
	}
	    hp = (union heap *) Heap;
	    do {
		if (!isbusy(hp->h_next)) {
			while (mkfree(hp->h_next) && !isbusy(otop(hp->h_next)->h_next))
				hp->h_next = otop(hp->h_next)->h_next;
			if (howbig(hp) >= fsize) {
				cp = (char *)hp;
				thp = (union heap *) (cp + fsize);
				thp->h_next = hp->h_next;
				hp->h_next = (long) mkbusy((char *) thp - Heap);
				wr_unlock();
				DUPRINT2(DB_MNT_ADV, "h_alloc 1 returns 0x%x\n", hp+1);
				return ((char *)(hp+1));
			}
			if (howbig(hp) >= size) {
				hp->h_next = (long) mkbusy(hp->h_next);
				wr_unlock();
				DUPRINT2(DB_MNT_ADV, "h_alloc 2 returns 0x%x\n", hp+1);
				return ((char *)(hp+1));
			}
		}
		hp = otop(hp->h_next);
	    } while (hp != (union heap *) Heap);

	DUPRINT1(DB_MNT_ADV, "h_alloc failed, ENOMEM\n");
	u.u_error = ENOMEM;
	wr_unlock();
	return (0);
}
void
h_free(offset)
char	*offset;
{
	union heap	*hp = (union heap *) offset;

	DUPRINT2(DB_MNT_ADV, "h_free(%x)\n", offset);
	wr_lock();
	hp--;		/* go back to h_next	*/
	ASSERT((char *)hp >= Heap || (char *)hp < Heap + Pavail);
	ASSERT(isbusy(hp->h_next) && (hp->h_next > 0 && hp->h_next < Pavail+1));
	hp->h_next = (long) mkfree(hp->h_next);
	wr_unlock();
	return;
}
/*
 * this is called with an idmap that is in the gdp table.
 * this function only frees the map if there are 1 or 0 references
 * to the map in the gdp structure.  This allows multiple pointers
 * to the same map from the gdp structure.
 */
int
freeidmap(offset)
char	*offset;
{
	register struct gdp	*gp;
	register int		gcnt=0;

	for (gp = gdp; gp < &gdp[maxgdp]; gp++) {
		if (gp->idmap[0] == offset)
			gcnt++;
		if (gp->idmap[1] == offset)
			gcnt++;
	}
	if (gcnt <= 1)
		h_free(offset);
}
/*
 *	This section contains functions to add and remove
 *	authorization information for client machines.
 *
 *	The functions are:
 *
 *	addalist(list)
 *	char	**list;
 *
 *	This function adds a list to the heap and returns a
 *	reference to the list.
 *	The list must be terminated by a NULL character.
 *	The reference is an integer and not a pointer, it cannot
 *	be directly used to reference a list.  Addalist returns 0, and
 *	sets u.u_error if it fails.  Addalist reads the list
 *	from user space.
 *
 *	remalist(ref)
 *	int	ref;
 *
 *	Remalist removes the list referred to by ref.  It
 *	will cause an assertion error if ref is invalid.
 *
 *	checkalist(ref, name)
 *	int	ref;
 *	char	*name;
 *
 *	This function returns TRUE (1) if name is in the
 *	referenced list and FALSE otherwise.  If ref is
 *	invalid, an assertion may occur, or an undefined
 *	result.
 *
 */
char	*
addalist(list)
char	**list;
{
	register int	i;
	register int	j;
	register char	*lptr;
	char	*ref = 0;	/* allocation reference to return	*/
	int	size = 0;	/* size of block to be allocated	*/
	int	count = 0;
	int	upath();
	char	*buffer;
	char	**alist;
	int	*sizelist;
	int	upath();

	DUPRINT2(DB_MNT_ADV, "addalist: list=%x\n", list);
	buffer = new_kmem_alloc(BUFSIZ, KMEM_SLEEP);
	alist = (char **)new_kmem_alloc(MAXNAMES * sizeof (char *), KMEM_SLEEP);
	sizelist = (int *)new_kmem_alloc(MAXNAMES * sizeof (int), KMEM_SLEEP);
	for (i=0; i < MAXNAMES; i++) {
		DUPRINT3(DB_MNT_ADV, "addalist: list[%d]=%x\n", i, list);
		if ((int) (alist[i] = (char *) fuword((caddr_t) list++)) == -1) {
			DUPRINT2(DB_MNT_ADV, "addalist: EFAULT on list[%d]\n", i);
			u.u_error = EFAULT;
			goto out;
		}
		if (alist[i] == 0) {
			count = i;
			break;
		}
	}
	if (i == MAXNAMES) {
		u.u_error = EINVAL;
		goto out;
	}
	for (i = 0; i < count; i++) {
		if ((j = upath(alist[i], buffer, BUFSIZ)) < 0) {
			u.u_error = (j == -1)?EFAULT:EINVAL;
			DUPRINT4(DB_MNT_ADV,
		"addalist: upath failed on alist[%d]=%x, errno=%s\n",
			    i, alist[i], (j== -1)?"EFAULT":"EINVAL");
			goto out;
		}
		DUPRINT3(DB_MNT_ADV, "addalist: name[%d]=%s\n", i, buffer);
		sizelist[i] = ++j;
		size += j;
	}
	size++;
	size = bump(size);
	if ((ref = h_alloc(size)) == NULL) {
		u.u_error = ENOMEM;
		goto out;
	}
	lptr = ref;
	for (i = 0; i < count; i++) {
		*lptr++ = (char) sizelist[i]-1;
		if ((j = upath(alist[i], lptr, BUFSIZ)) < 0) {
			h_free(ref);
			u.u_error = (j == -1)?EFAULT:EINVAL;
			DUPRINT4(DB_MNT_ADV,
		"addalist: 2nd upath failed alist[%d]=%x, errno=%s\n",
			    i, alist[i], (j== -1)?"EFAULT":"EINVAL");
			goto out;
		}
		lptr += j;
	}
	/* now fill up rest with nulls	*/
	for (i = (lptr - ref); i < size; i++)
		*lptr++ = '\0';
out:
	kmem_free((caddr_t) buffer, BUFSIZ);
	kmem_free((caddr_t) alist, MAXNAMES * sizeof (char *));
	kmem_free((caddr_t) sizelist, MAXNAMES * sizeof (int));
	return (ref);
}

void
remalist(ref)
char	*ref;
{
	h_free(ref);
}
/*
 * checkalist compares an incoming name, which is assumed to be
 * a full domain name, with the names in the list that ref refers
 * to.  The following conditions constitute a match:
 *
 * 1. complete match of full names.
 * 2. complete match of domain part of name if list item ends
 *    with a SEPARATOR.  e.g., if name = "a.b.c" and the list
 *    item = "a.b." there would be a match because a.b. implies
 *    a match with any name in the domain a.b.
 * 3. the name has the same dompart as this machine, and the
 *    namepart of name exactly matches an item in the list.
 *
 * NOTE: this code parses the names from left to right, i.e.,
 *	domain.name.
 */
int
checkalist(ref, name)
char	*ref;
char	*name;
{
	register char	*lp;
	register char	*np;
	register char	*nlp=NULL;
	char		*sp;
	char		*npart=NULL;	/* if != NULL, pointer to namepart of name */
	DUPRINT3(DB_MNT_ADV, "checkalist: ref=%x, name=%s\n", ref, name);
	rd_lock();
	for (np=name; *np != '\0'; np++)
		;

	/* first check to see if name is in same domain as machine	*/
	for (np = name, lp = Domain; *lp != '\0'; lp++, np++)
		if (*np != *lp)
			break;

	/* if we match domain totally, and name is at a separator,	*/
	/* we call it a match and call whatever is left the namepart	*/
	if (*np == SEPARATOR && *lp == '\0') {
		npart = np+1;
		DUPRINT3(DB_MNT_ADV, "checkalist: name (%s) is local, npart =%s\n",
			name, npart);
	}

	for (lp = ref; *lp > 0; lp = nlp) {
		nlp = lp + (int) *lp + 1;
		sp = ++lp;	/* go past the count	*/
		/* first check for full name match or domain match	*/
		for (np = name; *np != '\0'; np++, lp++)
			if (*np != *lp)
				break;
		if ((*np == '\0' && nlp == lp) ||
		    (*(np-1) == SEPARATOR && lp == nlp && *(lp-1) == SEPARATOR)) {
			rd_unlock();
			DUPRINT1(DB_MNT_ADV, "checkalist returns TRUE, full match\n");
			return (TRUE);
		}
		/* now check for current domain match	*/
		if (!npart)
			continue;
		for (np = npart, lp = sp; *np != '\0'; np++, lp++)
			if (*np != *lp)
				break;

		if (*np == '\0' && lp == nlp) {
			rd_unlock();
			DUPRINT1(DB_MNT_ADV, "checkalist returns TRUE, local domain\n");
			return (TRUE);
		}
	}
	rd_unlock();
	DUPRINT1(DB_MNT_ADV, "checkalist returns FALSE\n");
	return (FALSE);
}
/*
 *	translation table system call (actually called through rfsys).
 *	The uid, gid translation tables in the kernel are locked
 *	whenever they are updated (e.g., from a user level) or
 *	accessed from the kernel (e.g., through the kernel routines
 *	interface). This is to prevent updating the table during an
 *	access operation.
 *
 * setidmap:
 *  finds the gdp structure for the named machine,
 *  and adds uid or gid maps to the heap for the machine.
 */
setidmap(name, flag, map)
char	*name;
int	flag;
struct idtab	*map;
{
	char		sname[MAXDNAME+1];
	struct gdp	*g;
	int		gcnt = 0;
	struct idtab	header;
	char		*ref = NULL;
	char		**refp[MAX_MLINKS];
	int		i;
	struct idtab	*itp;
	struct idtab	iclear;
	struct idhead	*hp;

	DUPRINT4(DB_RFSYS, "setidmap: uname=%s, flag=%d, map=%x\n",
		name ? name : "", flag, map);

	if (!suser())
		return;

	/* NULL name says to clear all tables			*/
	if (!name) {
		for (g = gdp; g < &gdp[maxgdp]; g++)
			if (g->flag) {
				if (g->idmap[UID_DEV]) {
					freeidmap(g->idmap[UID_DEV]);
					g->idmap[UID_DEV] = 0;
				}
				if (g->idmap[GID_DEV]) {
					freeidmap(g->idmap[GID_DEV]);
					g->idmap[GID_DEV] = 0;
				}
			}
		if (Global[UID_DEV]) {
			h_free(Global[UID_DEV]);
			Global[UID_DEV] = 0;
		}
		if (Global[GID_DEV]) {
			h_free(Global[GID_DEV]);
			Global[GID_DEV] = 0;
		}
		return;
	}
	/* copy in the machine name and initial information	*/
	switch (upath(name, sname, MAXDNAME)) {
	case -2:	/* too long	*/
	case  0:	/* too short	*/
		u.u_error = EINVAL;
		return;
	case -1:	/* bad user address	*/
		u.u_error = EFAULT;
		return;
	}

	/*
	 * if name is GLOBAL_CH, this is global name, else
	 * look for name in gdp structure.
	 */
	if (sname[0] == GLOBAL_CH) {
		refp[0] = (flag==UID_MAP)? &(Global[UID_DEV]): &(Global[GID_DEV]);
		gcnt = 1;
	} else {
		for (g = gdp;  g < &gdp[maxgdp] && gcnt < MAX_MLINKS; g++)
			if (strcmp(sname, g->token.t_uname) == 0)
				refp[gcnt++] = (flag == UID_MAP)?
					&(g->idmap[UID_DEV]):
					&(g->idmap[GID_DEV]);

		if (gcnt == 0) {
			u.u_error = ENOENT;
			return;
		}
	}

	/* if there was a map, set it	*/
	if (map) {
		if (copyin((char *) map, (char *) &header, sizeof (struct idtab))) {
			u.u_error = EFAULT;
			return;
		}
	}

	/* free as late as possible, to save current map if something goes wrong */
	for (i=0; i < gcnt; i++)
		if (*refp[i]) {
			freeidmap(*refp[i]);
			*refp[i] = NULL;
		}

	if (map) {
		if ((ref = h_alloc((int) (sizeof (struct idtab) *
			(header.i_tblsiz + HEADSIZE + CACHESIZE)))) == NULL) {
			u.u_error = ENOMEM;
			return;
		}
		rd_lock();

		/* set up header 	*/
		hp = (struct idhead *) ref;
		hp->i_default = header.i_defval;
		hp->i_size = header.i_tblsiz;
		hp->i_cend = HEADSIZE + CACHESIZE;
		hp->i_next = HEADSIZE;
		hp->i_tries = 0;
		hp->i_hits = 0;

		/* mark cache unused	*/
		itp = (struct idtab *) ref;
		iclear.i_rem = UF;
		iclear.i_loc = UF;
		for (i=HEADSIZE; i < HEADSIZE+CACHESIZE; i++)
			itp[i] = iclear;

		/* copy in the table	*/
		if (copyin((char *) (map+1), (char *) (itp+HEADSIZE+CACHESIZE),
		    sizeof (struct idtab) * header.i_tblsiz)) {
			rd_unlock(); /* must unlock before free	*/
			h_free(ref);
			u.u_error = EFAULT;
			return;
		}
		rd_unlock();
	}

	for (i = 0; i < gcnt; i++)
		*refp[i] = ref;
	return;
}
/*
 *	glid returns a local id given a struct gdp pointer
 *	and remote id.  The argument dev chooses whether
 *	the uid or gid table is searched.
 *
 *	glid uses a binary search to improve the search time.
 *	This requires the table to be sorted.  The table is assumed
 *	to be sorted in ascending order with remote id as the
 *	primary sort key.
 *
 */

ushort
glid(dev, gp, rid)	/* get local id, given sysid & rid */
int	dev;
struct gdp	*gp;
register ushort	rid;
{
	register struct idtab	*low;	/* invariant: points to record <= rid */
	register struct idtab	*high;	/* invariant: points to record >= rid */
	register struct idtab	*middle; /* points to most recent record */
	ushort	size;			/* size of table */
	ushort	defval=OTHERID;		/* default value for local id */
	ushort	ret=OTHERID;		/* return value	*/
	struct idtab	*idt=NULL;
	struct idhead	*hp;

	DUPRINT4(DB_RFSYS, "glid: dev=%d, gp=%x, rid=%u\n", dev, gp, rid);

	rd_lock();

	if (gp && gp->idmap[dev])
		idt = (struct idtab *) gp->idmap[dev];
	else if (Global[dev])
		idt = (struct idtab *) Global[dev];
	else
		goto glid_out; 	/* ret already has default value	*/

	/* at this point should have a good table	*/
	DUPRINT2(DB_RFSYS, "glid: found table=%x\n", idt);

	hp = (struct idhead *) idt;
	size = idt->i_tblsiz;
	defval = idt->i_defval;

	/* check sets ret if it finds a match, if it doesn't, use default */
	ret = (defval)?defval:rid;

	if (size == 0)
		goto glid_out;

	low = idt + hp->i_cend;
	high = idt + size + hp->i_cend - 1;

	if (rid < low->i_rem || rid > high->i_rem)  /* out of range */
		goto glid_out;

	/* check the extremes of range	*/
	if (check(low, rid, ret) == 0)
		goto glid_out;
	if (check(high, rid, ret) == 0)
		goto glid_out;

	while (high - low > 1) {
		middle = (high-low)/2 + low;
		switch (check(middle, rid, ret)) {
		case -1:	/* middle is < rid	*/
			low = middle;
			break;
		case 1:		/* middle is > rid	*/
			high = middle;
			break;
		case 0:		/* found it		*/
			goto glid_out;
		}
	}
glid_out:
	rd_unlock();
	DUPRINT2(DB_RFSYS, "glid: returns %u\n", ret);
	return (ret);
}
static void
rd_lock()
{
	(void) splrf();
	Want_rd++;
	while (Tabrlock)
		(void) sleep((caddr_t) &Tabrlock, TRTABPRI);

	Tabwlock++; /* lock potential writers */
	Want_rd--;
	(void) spl0();
}
static void
rd_unlock()
{
	if (--Tabwlock == 0 && Want_wr)
		wakeup((caddr_t) &Tabwlock);
	else if (Want_rd)  /* This may not be necessary */
		wakeup((caddr_t) &Tabrlock);
}
static void
wr_lock()
{
	(void) splrf();
	Want_wr++;
	while (Tabwlock || Tabrlock)
		(void) sleep((caddr_t) &Tabwlock, TRTABPRI);

	Tabrlock++;	/* lock potential readers & writers */
	Want_wr--;
	(void) spl0();
}
static void
wr_unlock()
{
	Tabrlock--;
	if (Want_wr)
		wakeup((caddr_t) &Tabwlock);
	else if (Want_rd)
		wakeup((caddr_t) &Tabrlock);
}
ushort	rglid();

stat_rmap(gdpp, sptr)
struct gdp	*gdpp;
struct rfs_stat	*sptr;
{
	sptr->st_uid = rglid(UID_DEV, gdpp, (ushort) sptr->st_uid);
	sptr->st_gid = rglid(GID_DEV, gdpp, (ushort) sptr->st_gid);
	return;
}
ushort
rglid(dev, gp, rid)	/* get local id, given sysid & rid */
int	dev;
struct gdp	*gp;
register ushort	rid;
{
	register struct idtab	*low;	/* invariant: points to record <= rid */
	register struct idtab	*high;	/* invariant: points to record >= rid */
	ushort	size;		/* size of table 		*/
	ushort	defval=OTHERID;		/* default value for local id	*/
	ushort	ret=OTHERID;		/* return value	*/
	struct idtab	*idt=NULL;
	struct idhead	*hp;

	DUPRINT4(DB_RFSYS, "rglid: dev=%d, gp=%x, rid=%u\n", dev, gp, rid);

	rd_lock();

	if (gp && gp->idmap[dev])
		idt = (struct idtab *) gp->idmap[dev];
	else if (Global[dev])
		idt = (struct idtab *) Global[dev];
	else {
		if (rid != defval)
			ret = NO_ACCESS;
		goto rglid_out; 	/* ret already has default value	*/
	}

	/* at this point should have a good table	*/
	DUPRINT2(DB_RFSYS, "rglid: found table=%x\n", idt);

	hp = (struct idhead *) idt;
	size = idt->i_tblsiz;
	defval = idt->i_defval;

	/* check sets ret if it finds a match, if it doesn't, use default */
	ret = (defval)?defval:rid;

	if (size == 0) {
		if (defval && rid != defval)
			ret = NO_ACCESS;
		goto rglid_out;
	}

	hp->i_tries++;

	low = idt + HEADSIZE;
	for (high = low + CACHESIZE; low < high; low++)
		if (low->i_loc == rid) {
			ret = low->i_rem;
			hp->i_hits++;
			goto rglid_out;
		}

	low = high;
	for (high += size; low < high; low++)
		if (low->i_loc == rid ||
		    (low->i_loc == UF && (low+1)->i_loc == rid)) {
			ret = low->i_rem;
			(idt+(hp->i_next))->i_loc = rid;
			(idt+(hp->i_next))->i_rem = ret;
			hp->i_next = (hp->i_next < (hp->i_cend - 1))?
				hp->i_next + 1 : HEADSIZE;
			break;
		}
	/*
	 * No match found.  Need to insure that inaccessible ids are
	 * appropriately flagged.  This is done by doing a forward
	 * mapping.  If the forward mapping changes the input, it
	 * means that this id is inaccessible from this machine, and
	 * NO_ACCESS is returned.
	 */
	if (glid(dev, gp, rid) != rid) {
		ret = NO_ACCESS;
		(idt+(hp->i_next))->i_loc = rid;
		(idt+(hp->i_next))->i_rem = ret;
		hp->i_next = (hp->i_next < (hp->i_cend - 1))?
			hp->i_next + 1 : HEADSIZE;
	}

rglid_out:
	rd_unlock();
	DUPRINT2(DB_RFSYS, "rglid: returns %u\n", ret);
	return (ret);
}
