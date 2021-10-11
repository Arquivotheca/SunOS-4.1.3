#ifndef lint
static	char sccsid[] = "@(#)kvmgetcmd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "kvm.h"
#include "kvm_impl.h"
#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <vm/as.h>
#include <vm/seg_vn.h>
#include <vm/seg_u.h>
#include <vm/seg_map.h>
#include <vm/anon.h>
#include <vm/page.h>
#include <vm/swap.h>
#ifdef sun4m
#include <machine/devaddr.h>
#endif sun4m

extern char *malloc();

static long page_to_physaddr();
static int vp_to_fdoffset();

/*
 * VERSION FOR MACHINES WITH STACKS GROWING DOWNWARD IN MEMORY
 *
 * On program entry, the top of the stack frame looks like this:
 *
 * hi:	|-----------------------|
 *	|	   0		|
 *	|-----------------------|+
 *	|	   :		| \
 *	|  arg and env strings	|  > no more than NCARGS bytes
 *	|	   :		| /
 *	|-----------------------|+
 *	|	(char *)0	|
 *	|-----------------------|
 *	|  ptrs to env strings	|
 *	|	   :		|
 *	|-----------------------|
 *	|	(char *)0	|
 *	|-----------------------|
 *	|  ptrs to arg strings	|
 *	|   (argc = # of ptrs)	|
 *	|	   :		|
 *	|-----------------------|
 *	|	  argc		| <- sp
 * low:	|-----------------------|
 */

/* define a structure for describing an argument list */
typedef struct {
	int	cnt;		/* number of strings */
	u_long	sp;		/* user virtual addr of first string */
	u_long	se;		/* user virtual addr of end of strings */
} argv_t;

static char **argcopy();
static int stkcopy();
static int getstkpg();
static int getseg();
static int readseg();
static struct page *pagefind();
int _swapread();
int _anonoffset();

/*
 * Convert an array offset to a user virtual address.
 * This is done by adding the array offset to the virtual address
 * of the start of the current page (kd->stkpg counts pages from the
 * top of the stack).
 */
#define	Uvaddr(x)	\
	(USRSTACK - ((kd->stkpg + 1) * PAGESIZE) + ((char*)(x) - kd->sbuf))

/*
 * reconstruct an argv-like argument list from the target process
 */
int
kvm_getcmd(kd, proc, u, arg, env)
	kvm_t *kd;
	struct proc *proc;
	struct user *u;
	char ***arg;
	char ***env;
{
	argv_t argd;
	argv_t envd;
	register char *cp;
	register int eqseen, i;

	if (proc->p_ssize == 0)		/* if no stack, give up now */
		return (-1);

	/*
	 * Read the last stack page into kd->sbuf (allocating, if necessary).
	 * Then, from top of stack, find the end of the environment strings.
	 */
	if (getstkpg(kd, proc, u, 0) == -1)
		return (-1);

	/*
	 * Point to the last byte of the environment strings.
	 */
	cp = &kd->sbuf[PAGESIZE - NBPW - 1];

	/*
	 * Skip backward over any zero bytes used to pad the string data
	 * to a word boundary.
	 */
	for (i = 0; i != NBPW && *--cp == '\0'; i++)
		;

	/*
	 * Initialize descriptors
	 */
	envd.cnt = 0;
	envd.sp = envd.se = Uvaddr(cp + 1); /* this must point at '\0' */
	argd = envd;

	/*
	 * Now, working backwards, count the environment strings and the
	 * argument strings and look for the (int)0 that delimits the
	 * environment pointers.
	 */
	while (*(int *)((u_long)cp & ~(NBPW - 1)) != 0) {
		eqseen = 0;
		while (*cp != '\0') {
			if (*cp-- == '=')
				eqseen = 1;
			if (cp < kd->sbuf) {
				if (kd->stkpg * PAGESIZE > NCARGS ||
				    getstkpg(kd, proc, u, ++kd->stkpg) == -1)
					return (-1);
				else
					cp = &kd->sbuf[PAGESIZE - 1];
			}
		}
		if (eqseen && argd.cnt == 0) {
			envd.cnt++;
			envd.sp = Uvaddr(cp + 1);
		} else {
			argd.cnt++;
			argd.sp = Uvaddr(cp + 1);
		}
		cp--;
	}

	if (envd.cnt != 0) {
		argd.se = envd.sp - 1;
		if (argd.cnt == 0)
			argd.sp = argd.se;
	}

	if (envd.se - argd.sp > NCARGS - 1) {
#ifdef _KVM_DEBUG
		_kvm_error("kvm_getcmd: could not locate arg pointers");
#endif _KVM_DEBUG
		return (-1);
	}

	/* Copy back the (adjusted) vectors and strings */
	if (arg != NULL) {
		if ((*arg = argcopy(kd, proc, u, &argd)) == NULL)
			return (-1);
	}
	if (env != NULL) {
		if ((*env = argcopy(kd, proc, u, &envd)) == NULL) {
			if (arg != NULL)
				(void) free((char *)*arg);
			return (-1);
		}
	}
	return (0);
}

static char **
argcopy(kd, proc, u, arg)
	kvm_t *kd;
	struct proc *proc;
	struct user *u;
	argv_t *arg;
{
	int pcnt;
	int scnt;
	register char **ptr;
	register char **p;
	char *str;

	/* Step 1: allocate a buffer to hold all pointers and strings */
	pcnt = (arg->cnt + 1) * sizeof (char *);	/* #bytes in ptrs */
	scnt = arg->se - arg->sp + 1;			/* #bytes in strings */
	ptr = (char **)malloc((u_int)scnt + pcnt);
	if (ptr == NULL) {
#ifdef _KVM_DEBUG
		_kvm_perror("argcopy");
#endif _KVM_DEBUG
		return (NULL);
	}
	str = (char *)ptr + pcnt;

	/* Step 2: copy the strings from user space to buffer */
	if (stkcopy(kd, proc, u, arg->sp, str, scnt) == -1) {
		(void) free((char *)ptr);
		return (NULL);
	}
	if (str[scnt-1] != '\0') {
#ifdef _KVM_DEBUG
		_kvm_error("argcopy: added NULL at end of strings");
#endif _KVM_DEBUG
		str[scnt-1] = '\0';
	}

	/* Step 3: calculate the pointers */
	for (p = ptr, pcnt = arg->cnt; pcnt-- > 0;) {
		*p++ = str;
		while (*str++ != '\0')
			;
	}
	*p++ = NULL;			/* NULL pointer at end */
	if ((str - (char *)p) != (arg->cnt ? scnt : 0)) {
#ifdef _KVM_DEBUG
		_kvm_error("argcopy: string pointer botch");
#endif _KVM_DEBUG
	}
	return (ptr);
}

/* XXX this and getstkpg should use new kvm_as_read instead of readseg */

/*
 * Copy user stack into specified buffer
 */
static int
stkcopy(kd, proc, u, va, buf, cnt)
	kvm_t *kd;
	struct proc *proc;
	struct user *u;
	u_long va;
	char *buf;
	int cnt;
{
	register int i = 0;
	register u_int off;
	register int pg;
	register int c;

	if ((USRSTACK - va) < cnt) {
#ifdef _KVM_DEBUG
		_kvm_error("stkcopy: bad stack copy length %d", cnt);
#endif _KVM_DEBUG
		return (-1);
	}
	off = va & PAGEOFFSET;
	pg = (int) ((USRSTACK - (va + 1)) / PAGESIZE);

	while (cnt > 0) {
		if ((kd->stkpg != pg) && (getstkpg(kd, proc, u, pg) == -1))
			return (-1);
		c = MIN((PAGESIZE - off), cnt);
		bcopy(&kd->sbuf[off], &buf[i], c);
		i += c;
		cnt -= c;
		off = 0;
		pg--;
	}
	return (0);
}

#ifdef _KVM_DEBUG
#define	getkvm(a, b, m)							\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b))	\
						!= sizeof (*b)) {	\
		_kvm_error("error reading %s", m);			\
		return (-1);						\
	}
#else !_KVM_DEBUG
#define	getkvm(a, b, m)							\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b))	\
						!= sizeof (*b)) {	\
		return (-1);						\
	}
#endif _KVM_DEBUG

/*
 * read a user stack page into a holding area
 */
/*ARGSUSED*/
static int
getstkpg(kd, proc, u, pg)
	kvm_t *kd;
	struct proc *proc;
	struct user *u;
	int pg;
{
	addr_t vaddr;

	/* If no address segment ptr, this is a system process (e.g. biod) */
	if (proc->p_as == NULL)
		return (-1);

	/* First time through, allocate a user stack cache */
	if (kd->sbuf == NULL) {
		kd->sbuf = malloc(PAGESIZE);
		if (kd->sbuf == NULL) {
#ifdef _KVM_DEBUG
			_kvm_perror("can't allocate stack cache");
#endif _KVM_DEBUG
			return (-1);
		}
	}

	/* If no seg struct for this process, get one for the 1st stack page */
	if (kd->useg.s_as != proc->p_as) {
		struct as uas;

		getkvm(proc->p_as, &uas, "user address space descriptor");
		getkvm(uas.a_segs, &kd->useg, "1st user segment descriptor");
		getkvm(kd->useg.s_prev, &kd->useg, "stack segment descriptor");
	}

	kd->stkpg = pg;

	/* Make sure we've got the right seg structure for this address */
	vaddr = (addr_t)(USRSTACK - ((pg + 1) * PAGESIZE));
	if (getseg(kd, &kd->useg, vaddr, &kd->useg) == -1)
		return (-1);
	if (kd->useg.s_as != proc->p_as) {
#ifdef _KVM_DEBUG
		_kvm_error("wrong segment for user stack");
#endif _KVM_DEBUG
		return (-1);
	}
	if (kd->useg.s_ops != kd->segvn) {
#ifdef _KVM_DEBUG
		_kvm_error("user stack segment not segvn type");
#endif _KVM_DEBUG
		return (-1);
	}

	/* Now go find and read the page */
	if (readseg(kd, &kd->useg, vaddr, kd->sbuf, PAGESIZE)
	    != PAGESIZE) {
#ifdef _KVM_DEBUG
		_kvm_error("error reading stack page");
#endif _KVM_DEBUG
		return (-1);
	}
	return (0);
}

/*
 * getseg - given a seg structure, find the appropriate seg for a given address
 *	  (nseg may be identical to oseg)
 */
static int
getseg(kd, oseg, addr, nseg)
	kvm_t *kd;
	struct seg *oseg;
	addr_t addr;
	struct seg *nseg;
{
	struct seg *xseg;

	if (addr < oseg->s_base) {
		xseg = oseg->s_prev;	/* watch out for circularity */
		do {
			getkvm(oseg->s_prev, nseg, "prev segment descriptor");
			if (nseg->s_prev == xseg)
				goto noseg;
			oseg = nseg;
		} while (addr < nseg->s_base);
		if (addr >= (nseg->s_base + nseg->s_size))
			goto noseg;

	} else if (addr >= (oseg->s_base + oseg->s_size)) {
		xseg = oseg->s_next;	/* watch out for circularity */
		do {
			getkvm(oseg->s_next, nseg, "next segment descriptor");
			if (nseg->s_next == xseg)
				goto noseg;
			oseg = nseg;
		} while (addr >= nseg->s_base + nseg->s_size);
		if (addr < nseg->s_base)
			goto noseg;

	} else if (nseg != oseg) {
		*nseg = *oseg;		/* copy if necessary */
	}
	return (0);

noseg:
#ifdef _KVM_DEBUG
	_kvm_error("can't find segment for user address %x", addr);
#endif _KVM_DEBUG
	return (-1);
}

/*
 * readseg - read data described by a virtual address and seg structure.
 *	   The data block must be entirely contained within seg.
 *	   Readseg() returns the number of bytes read, or -1.
 */
static int
readseg(kd, seg, addr, buf, size)
	kvm_t *kd;
	struct seg *seg;
	addr_t addr;
	char *buf;
	u_int size;
{
	u_int count;

	if ((addr + size) > (seg->s_base + seg->s_size)) {
#ifdef _KVM_DEBUG
		_kvm_error("readseg: segment too small");
#endif _KVM_DEBUG
		return (-1);
	}

	count = 0;

	if (seg->s_ops == kd->segvn) {
		/* Segvn segment */
		struct segvn_data sdata;
		struct anon_map amap;
		struct anon **anp;
		struct anon *ap;
		struct anon anon;
		u_int apsz;
		u_int aoff;
		u_int rsize;

		/* get private data for segment */
		if (seg->s_data == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("NULL segvn_data ptr in segment");
#endif _KVM_DEBUG
			return (-1);
		}
		getkvm(seg->s_data, &sdata, "segvn_data");

		/* Null vnode indicates anonymous page */
		if (sdata.vp != NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("non-NULL vp in segvn_data");
#endif _KVM_DEBUG
			return (-1);
		}
		if (sdata.amp == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("NULL anon_map ptr in segvn_data");
#endif _KVM_DEBUG
			return (-1);
		}

		/* get anon_map structure */
		getkvm(sdata.amp, &amap, "anon_map");
		if (amap.anon == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("anon_map has NULL ptr");
#endif _KVM_DEBUG
			return (-1);
		}
		apsz = (amap.size >> PAGESHIFT) * sizeof (struct anon *);
		if (apsz == 0) {
#ifdef _KVM_DEBUG
			_kvm_error("anon_map has zero size");
#endif _KVM_DEBUG
			return (-1);
		}
		if ((anp = (struct anon **)malloc(apsz)) == NULL) {
#ifdef _KVM_DEBUG
			_kvm_perror("can't allocate anon pointer array");
#endif _KVM_DEBUG
			return (-1);
		}

		/* read anon pointer array */
		if (kvm_read(kd, (u_long)amap.anon, (char *)anp, apsz) != apsz){
#ifdef _KVM_DEBUG
			_kvm_error("error reading anon ptr array");
#endif _KVM_DEBUG
			free((char *)anp);
			return (-1);
		}

		/* since data may cross page boundaries, break up request */
		while (count < size) {
			struct page *paddr;
			struct page page;
			int swapoff;
			struct vnode *vp;
			u_int vpoff;
			long skaddr;
			struct page *pages;
			u_int pages_base;

			aoff = (long)addr & PAGEOFFSET;
			rsize = MIN((PAGESIZE - aoff), (size - count));

			/* index into anon ptrs to find the right one */
			ap = anp[sdata.anon_index +
			    ((addr - seg->s_base)>>PAGESHIFT)];
			if (ap == NULL) {
#ifdef _KVM_DEBUG
				_kvm_error("NULL anon ptr");
#endif _KVM_DEBUG
				break;
			}
			if ((swapoff = _anonoffset(kd, ap, &vp, &vpoff)) == -1) {
				break;
			}
			getkvm(ap, &anon, "anon structure");

			/*
			 * If there is a page structure pointer,
			 * make sure it is valid.  If not, try the
			 * hash table in case the page is free but
			 * reclaimable.
			 */
			paddr = anon.un.an_page;
			if (paddr != NULL) {
				getkvm(paddr, &page, "anon page structure");
				if ((page.p_vnode == vp) &&
				    (vpoff == page.p_offset)) {
					goto gotpage;
				}
#ifdef _KVM_DEBUG
				_kvm_error("anon page struct invalid");
#endif _KVM_DEBUG
			}

			/* try hash table in case page is still around */
			paddr = pagefind(kd, &page, vp, vpoff);
			if (paddr == NULL)
				goto tryswap;

gotpage:
			/* make sure the page structure is useful */
			if (page.p_pagein)
				goto tryswap;
			if (page.p_gone) {
#ifdef _KVM_DEBUG
				_kvm_error("anon page is gone");
#endif _KVM_DEBUG
				break;
			}

			/*
			 * Page is in core (probably).
			 * XXX - the following is going to change when
			 *	 tuples are implemented
			 */

			if (paddr < kd->memseg.epages) {
				pages = kd->memseg.pages;
				pages_base = kd->memseg.pages_base;
			} else {
				struct memseg memseg;
				struct memseg *next;

				/* not in first memseg */
				next = kd->memseg.next;
				memseg.epages = kd->memseg.epages;
				while (paddr > memseg.epages) {
					getkvm(next, &memseg, "next memory segment");
					next = memseg.next;
				}
				pages = memseg.pages;
				pages_base = memseg.pages_base;
			}
			skaddr = aoff +
				(((paddr - pages) + pages_base) << PAGESHIFT);
			if (_uncondense(kd, kd->corefd, &skaddr)) {
#ifdef _KVM_DEBUG
				_kvm_error("%s: anon page uncondense error",
						kd->core);
#endif _KVM_DEBUG
				break;
			}
			if (lseek(kd->corefd, (off_t)skaddr, L_SET) == -1) {
#ifdef _KVM_DEBUG
				_kvm_perror("%s: anon page seek error",
						kd->core);
#endif _KVM_DEBUG
				break;
			}
			if (read(kd->corefd, buf, rsize) != rsize) {
#ifdef _KVM_DEBUG
				_kvm_perror("%s: anon page read error",
						kd->core);
#endif _KVM_DEBUG
				break;
			}
			goto readok;

tryswap:
			/*
			 * If no page structure, page is swapped out
			 */
			if (kd->swapfd == -1)
				break;
			if (_swapread(kd, (long)swapoff, aoff, buf, rsize) !=
			    rsize) {
#ifdef _KVM_DEBUG
				_kvm_perror("%s: anon page read error",
						kd->swap);
#endif _KVM_DEBUG
				break;
			}
readok:
			count += rsize;
			addr += rsize;
			buf += rsize;
		}
		free((char *)anp);	/* no longer need this */

	} else if (seg->s_ops == kd->segmap) {
		/* Segmap segment */
#ifdef _KVM_DEBUG
		_kvm_error("cannot read segmap segments yet");
#endif _KVM_DEBUG
		return (-1);

	} else if (seg->s_ops == kd->segdev) {
		/* Segdev segment */
#ifdef _KVM_DEBUG
		_kvm_error("cannot read segdev segments yet");
#endif _KVM_DEBUG
		return (-1);

	} else {
		/* Segkmem or unknown segment */
#ifdef _KVM_DEBUG
		_kvm_error("unknown segment type");
#endif _KVM_DEBUG
		return (-1);
	}
	if (count == 0)
		return (-1);
	else
		return ((int) count);
}


/* this is like the getkvm() macro, but returns NULL on error instead of -1 */
#ifdef _KVM_DEBUG
#define	getkvmnull(a, b, m)						\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b)) 	\
						!= sizeof (*b)) {	\
		_kvm_error("error reading %s", m);			\
		return (NULL);						\
	}
#else !_KVM_DEBUG
#define	getkvmnull(a, b, m)						\
	if (kvm_read(kd, (u_long)(a), (caddr_t)(b), sizeof (*b)) 	\
						!= sizeof (*b)) {	\
		return (NULL);						\
	}
#endif _KVM_DEBUG

/*
 * pagefind - hashed lookup to see if a page exists for a given vnode
 *	    Returns address of page structure in 'page', or NULL if error.
 */
static struct page *
pagefind(kd, page, vp, off)
	kvm_t *kd;
	struct page *page;
	struct vnode *vp;
	u_int off;
{
#define	PAGE_HASHSZ	(kd->page_hashsz)	/* used by PAGE_HASHFUNC() */
	struct page *pp;

	getkvmnull((kd->page_hash + PAGE_HASHFUNC(vp, off)), &pp,
	    "initial hashed page struct ptr");
	while (pp != NULL) {
		getkvmnull(pp, page, "hashed page structure");
		if ((page->p_vnode == vp) && (page->p_offset == off)) {
			return (pp);
		}
		pp = page->p_hash;
	}
	return (NULL);
}

/*
 * _swapread - read data from the swap device, handling alignment properly,
 *	    Swapread() returns the number of bytes read, or -1.
 */
int
_swapread(kd, addr, offset, buf, size)
	kvm_t *kd;
	long addr;
	u_int offset;
	char *buf;
	u_int size;
{

	if (_uncondense(kd, kd->swapfd, &addr)) {
		/* Does this need to handle the -2 case? */
#ifdef _KVM_DEBUG
		_kvm_error("%s: uncondense error", kd->swap);
#endif _KVM_DEBUG
		return (-1);
	}
	if (lseek(kd->swapfd, addr, L_SET) == -1) {
#ifdef _KVM_DEBUG
		_kvm_perror("%s: seek error", kd->swap);
#endif _KVM_DEBUG
		return (-1);
	}
	if ((offset == 0) && ((size % DEV_BSIZE) == 0)) {
		return (read(kd->swapfd, buf, size));
	} else {
#ifdef _KVM_DEBUG
		_kvm_error("%s: swap offsets not implemented", kd->swap);
#endif _KVM_DEBUG
		return (-1);
	}
}

/*
 * _anonoffset
 *
 * Convert a pointer into an anon array into an offset (in bytes)
 * into the unified swap file.  Since each individual swap file has
 * a separate swapinfo structure, we cache the linked list of swapinfo
 * structures in order to do this calculation faster.  Also, save the
 * real vp and offset for the vnode that contains this page.
 */
int
_anonoffset(kd, ap, vp, vpoffset)
	kvm_t *kd;
	struct anon *ap;
	struct vnode **vp;
	u_int *vpoffset;
{
	register struct swapinfo *sip;

	sip = kd->sip;

	/*
	 * First time through, read in all the swapinfo structures.
	 * Re-use the si_allocs field to store the swap offset (in pages).
	 */
	if (sip == NULL) {
		register struct swapinfo **spp;
		register struct swapinfo *sn;
		register int soff;

		sn = kd->swapinfo;
		spp = &kd->sip;
		soff = 0;		/* keep track of cumulative offsets */
		for (; sn != NULL; spp = &(*spp)->si_next, sn = *spp) {
			*spp = (struct swapinfo *)malloc(sizeof (*sn));
			if (*spp == NULL) {
#ifdef _KVM_DEBUG
				_kvm_perror("no memory for swapinfo");
#endif _KVM_DEBUG
				break;
			}
			if (kvm_read(kd, (u_long)sn, (char*)*spp, sizeof (*sn))
						!= sizeof (*sn)) {
#ifdef _KVM_DEBUG
				_kvm_error("error reading swapinfo");
#endif _KVM_DEBUG
				free((char *)*spp);
				break;
			}
			(*spp)->si_allocs = soff;
			soff += ((*spp)->si_size >> PAGESHIFT);
		}
		*spp = NULL;		/* clear final 'next' pointer */
		sip = kd->sip;		/* reset list pointer */
	}
	/*
	 * At this point, all the swapinfo structures are in memory.
	 * Now find the one that includes the current anon pointer.
	 */
	for (; sip != NULL; sip = sip->si_next) {
		if ((ap >= sip->si_anon) && (ap <= sip->si_eanon)) {
			*vp = sip->si_vp;
			*vpoffset = (ap - sip->si_anon) << PAGESHIFT;
			return ((sip->si_allocs << PAGESHIFT) + *vpoffset);
		}
	}
#ifdef _KVM_DEBUG
	_kvm_error("can't find anon ptr in swapinfo list");
#endif _KVM_DEBUG
	return (-1);
}

/*
 * Find physical address correspoding to an address space/offset
 * Returns -1 if address does not correspond to any physical memory
 */
int
kvm_physaddr(kd, as, vaddr)
	kvm_t *kd;
	struct as *as;
	u_int vaddr;
{
	int fd;
	u_int off;

	_kvm_physaddr(kd, as, vaddr, &fd, &off);
	return (fd == kd->corefd) ? off : (-1);
}

/* internal interface that also finds swap offset if any */
/* fd is either kd->corefd if in core, kd->swapfd if in swap or -1 if nowhere */
_kvm_physaddr(kd, as, vaddr, fdp, offp)
	kvm_t *kd;
	struct as *as;
	u_int vaddr;
	int *fdp;
	u_int *offp;
{
	struct seg s, *seg, *fseg;

	*fdp = -1;
	/* get first seg structure */
	seg = &s;
	if (as->a_segs == kd->Kas.a_segs) {
	  if(kd->Bufheapseg.s_size && (vaddr >= (u_int)kd->Bufheapseg.s_base))
		fseg = &kd->Bufheapseg;
	  else
		fseg = (vaddr < (u_int)kd->Kseg.s_base) ?
			&kd->Ktextseg : &kd->Kseg;
	} else {
		fseg = &s;
		getkvm(as->a_segs, fseg, "1st user segment descriptor");
	}

	/* Make sure we've got the right seg structure for this address */
	if (getseg(kd, fseg, (addr_t)vaddr, seg) == -1)
		return (-1);

	if (seg->s_ops == kd->segvn) {
		/* Segvn segment */
		struct segvn_data sdata;
		u_int off;
		struct vnode *vp;
		u_int vpoff;

		off = (long)vaddr & PAGEOFFSET;

		/* get private data for segment */
		if (seg->s_data == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("NULL segvn_data ptr in segment");
#endif _KVM_DEBUG
			return (-1);
		}
		getkvm(seg->s_data, &sdata, "segvn_data");

		/* Try anonymous pages first */
		if (sdata.amp != NULL) {
			struct anon_map amap;
			struct anon **anp;
			struct anon *ap;
			u_int apsz;

			/* get anon_map structure */
			getkvm(sdata.amp, &amap, "anon_map");
			if (amap.anon == NULL)
				goto notanon;

			/* get space for anon pointer array */
			apsz = ((amap.size + PAGEOFFSET) >> PAGESHIFT) *
				sizeof (struct anon *);
			if (apsz == 0)
				goto notanon;
			if ((anp = (struct anon **)malloc(apsz)) == NULL) {
#ifdef _KVM_DEBUG
				_kvm_perror("anon pointer array");
#endif _KVM_DEBUG
				return (-1);
			}

			/* read anon pointer array */
			if (kvm_read(kd, (u_long)amap.anon,
			    (char *)anp, apsz) != apsz) {
#ifdef _KVM_DEBUG
				_kvm_error("error reading anon ptr array");
#endif _KVM_DEBUG
				free((char *)anp);
				return (-1);
			}

			/* index into anon ptrs to find the right one */
			ap = anp[sdata.anon_index +
			    btop((addr_t)vaddr - seg->s_base)];
			free((char *)anp); /* no longer need this */
			if (ap == NULL)
				goto notanon;
			anon_to_fdoffset(kd, ap, fdp, offp, off);
			return (0);
		}
notanon:

		/* If not in anonymous; try the vnode */
		vp = sdata.vp;
		vpoff = sdata.offset + ((addr_t)vaddr - seg->s_base);

		/*
		 * If vp is null then the page doesn't exist.
		 */
		if (vp != NULL) {
			vp_to_fdoffset(kd, vp, fdp, offp, vpoff);
		} else {
#ifdef _KVM_DEBUG
			_kvm_error("Can't translate virtual address");
#endif _KVM_DEBUG
			return (-1);
		}
	} else if (seg->s_ops == kd->segkmem) {
		/* Segkmem segment */
		register int poff;
		register u_long paddr;
		register u_int p;

		if (as->a_segs != kd->Kas.a_segs)
			return (-1);

		poff = vaddr & PAGEOFFSET;	/* save offset into page */
		vaddr &= PAGEMASK;		/* round down to start of page */

		if ((vaddr >= (u_int)kd->Kseg.s_base) && (vaddr < kd->Syslimit)) {
		  	/* kseg */
			p = klvtop(vaddr) >> PAGESHIFT;
			if (!pte_valid(&kd->Sysmap[p])) {
#ifdef _KVM_DEBUG
				_kvm_error(
					"kvm_read: invalid kernel page %d", p);
#endif _KVM_DEBUG
				return (-1);
			}
			paddr = poff + ptob(MAKE_PFNUM(&kd->Sysmap[p]));
		} else if (vaddr >= (KERNELBASE + NBPG) &&
				vaddr < kd->econtig) {
		  	/* ktextseg */
			paddr = kvtop(kd, vaddr) + poff;
		} else if ( kd->Bufheapseg.s_size && kd->Heapptes &&
			   (vaddr >= (u_int)kd->Bufheapseg.s_base) &&
			   (vaddr < (u_int)kd->Bufheapseg.s_base +
			    (u_int)kd->Bufheapseg.s_size)) {
		  	/* bufheapseg */
			p = (vaddr - (u_int)kd->Bufheapseg.s_base) >>PAGESHIFT;
			if (!pte_valid(&kd->Heapptes[p])) {
#ifdef _KVM_DEBUG
				_kvm_error(
					"kvm_read: invalid kernel page %d", p);
#endif _KVM_DEBUG
				return (-1);
			}
			paddr = poff + ptob(MAKE_PFNUM(&kd->Heapptes[p]));
#ifdef sun4m
		} else if (kd->is_mp) {
			register int pageix, pix, pte;
			register u_int region;

			region = (vaddr >> MMU_STD_RGNSHIFT);
			if ((region == 
			    ((u_int)VA_PERCPU >> MMU_STD_RGNSHIFT)) ||
			    (region ==
			    ((u_int) VA_PERCPUME >> MMU_STD_RGNSHIFT))) {
				if (region ==
				    ((u_int)VA_PERCPU >> MMU_STD_RGNSHIFT))
					kd->cpuid = (vaddr >> 20) & 0xF;
				pageix = (vaddr >> PAGESHIFT) & 0x3F;
				pix = kd->cpuid * NL3PTEPERPT + pageix;
				pte = (int) kd->percpu_l3tbl[pix].ptpe_int;
				if ((pte & 3) != MMU_ET_PTE) {
#ifdef _KVM_DEBUG
					_kvm_error(
					  "kvm_read: vaddr %x is not mapped",
					  vaddr);
#endif _KVM_DEBUG
				return (-1);
				}
				if (pte & 0xF0000000) {
#ifdef _KVM_DEBUG
					_kvm_error(
					  "kvm_read: vaddr %x is not memory",
					  vaddr);
#endif _KVM_DEBUG
				return (-1);
				}
				paddr = ((pte & ~0xFF) << 4) + poff;
			} else {
#ifdef _KVM_DEBUG
			  _kvm_error("kvm_read: can't translate virtual addr %x",
				vaddr);
#endif _KVM_DEBUG
				return (-1);
			}
#endif sun4m
		} else {
#ifdef _KVM_DEBUG
			_kvm_error("kvm_read: can't translate virtual addr %x",
					vaddr);
#endif _KVM_DEBUG
			return (-1);
		}
		*fdp = kd->corefd;
		*offp = paddr;
	} else if (seg->s_ops == kd->segu) {
		/* Segu segment */
		int slot, pageno;
		struct segu_segdata segudata;
		struct segu_data segu_slot_data;
		struct anon *ap;
		u_int aoff;

		slot = (vaddr - (u_int)seg->s_base) / ptob(SEGU_PAGES);
		aoff = (long)vaddr & PAGEOFFSET;
		pageno = btop(vaddr - (slot * ptob(SEGU_PAGES) + (u_int)seg->s_base));
		getkvm(seg->s_data, &segudata, "segu segment data");
		getkvm(segudata.usd_slots + slot,
			&segu_slot_data, "segu per-slot data");
		ap = segu_slot_data.su_swaddr[pageno];
		anon_to_fdoffset(kd, ap, fdp, offp, aoff);
	} else if (seg->s_ops == kd->segmap) {
		/* Segmap segment */
		struct segmap_data sdata;
		struct smap *smp;
		struct smap *smap;
		u_int smpsz;
		u_int off;
		struct vnode *vp = NULL;
		u_int vpoff;

		off = (long)vaddr & MAXBOFFSET;
		vaddr &= MAXBMASK;

		/* get private data for segment */
		if (seg->s_data == NULL) {

#ifdef _KVM_DEBUG
			_kvm_error("NULL segmap_data ptr in segment");
#endif _KVM_DEBUG
			return (-1);
		}
		getkvm(seg->s_data, &sdata, "segmap_data");

		/* get space for smap array */
		smpsz = (seg->s_size >> MAXBSHIFT) * sizeof (struct smap);
		if (smpsz == 0) {
#ifdef _KVM_DEBUG
			_kvm_error("zero-length smap array");
#endif _KVM_DEBUG
			return (-1);
		}
		if ((smap = (struct smap *)malloc(smpsz)) == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("no memory for smap array");
#endif _KVM_DEBUG
			return (-1);
		}

		/* read smap array */
		if (kvm_read(kd, (u_long)sdata.smd_sm,
		    (char *)smap, smpsz) != smpsz) {
#ifdef _KVM_DEBUG
			_kvm_error("error reading smap array");
#endif _KVM_DEBUG
			(void) free((char *)smap);
			return (-1);
		}

		/* index into smap array to find the right one */
		smp = &smap[((addr_t)vaddr - seg->s_base)>>MAXBSHIFT];
		vp = smp->sm_vp;

		if (vp == NULL) {
#ifdef _KVM_DEBUG
			_kvm_error("NULL vnode ptr in smap");
#endif _KVM_DEBUG
			free((char *)smap); /* no longer need this */
			return (-1);
		}
		vpoff = smp->sm_off + off;
		free((char *)smap); /* no longer need this */
		vp_to_fdoffset(kd, vp, fdp, offp, vpoff);
	} else if (seg->s_ops == kd->segdev) {
#ifdef _KVM_DEBUG
		_kvm_error("cannot read segdev segments yet");
#endif _KVM_DEBUG
		return (-1);
	} else {
#ifdef _KVM_DEBUG
		_kvm_error("unknown segment type");
#endif _KVM_DEBUG
		return (-1);
	}
	return (0);
}

/* convert anon pointer/offset to fd (swap, core or nothing) and offset */
anon_to_fdoffset(kd, ap, fdp, offp, aoff)
	kvm_t *kd;
	struct anon *ap;
	int *fdp;
	u_int *offp;
	u_int aoff;
{
	struct anon anon;
	struct page *paddr;
	struct page page;
	int swapoff;
	struct vnode *vp;
	u_int vpoff;
	long skaddr;

	if (ap == NULL) {
#ifdef _KVM_DEBUG
		_kvm_error("anon_to_fdoffset: null anon ptr");
#endif _KVM_DEBUG
		return (-1);
	}

	if ((swapoff = _anonoffset(kd, ap, &vp, &vpoff)) == -1) {
		return (-1);
	}

	getkvm(ap, &anon, "anon structure");

	/*
	 * If there is a page structure pointer,
	 * make sure it is valid.  If not, try the
	 * hash table in case the page is free but
	 * reclaimable.
	 */
	paddr = anon.un.an_page;
	if (paddr != NULL) {
		getkvm(paddr, &page, "anon page structure");
		if ((page.p_vnode == vp) &&
		    (vpoff == page.p_offset)) {
			goto gotpage;
		}
#ifdef _KVM_DEBUG
		_kvm_error("anon page struct invalid");
#endif _KVM_DEBUG
		return (-1);
	}

	/* try hash table in case page is still around */
	paddr = pagefind(kd, &page, vp, vpoff);
	if (paddr == NULL) {
		*fdp = kd->swapfd;
		*offp = swapoff + aoff;
		return (0);
	}

gotpage:
	/* make sure the page structure is useful */
	if (page.p_pagein) {
		*fdp = kd->swapfd;
		*offp = swapoff + aoff;
		return (0);
	}
	if (page.p_gone) {
#ifdef _KVM_DEBUG
		_kvm_error("anon page is gone");
#endif _KVM_DEBUG
		return (-1);
	}

	/*
	 * Page is in core.
	 */
	skaddr = page_to_physaddr(kd, paddr);
	if (skaddr == -1) {
#ifdef _KVM_DEBUG
		_kvm_error("anon_to_fdoffset: can't find page 0x%x", paddr);
#endif _KVM_DEBUG
		return (-1);
	}
	*fdp = kd->corefd;
	*offp = skaddr + aoff;
	return (0);
}

/* convert vnode pointer/offset to fd (core or nothing) and offset */
static int
vp_to_fdoffset(kd, vp, fdp, offp, vpoff)
	kvm_t *kd;
	struct vnode *vp;
	int *fdp;
	u_int *offp;
	u_int vpoff;
{
	struct page *paddr;
	struct page page;
	u_int off;
	long skaddr;

	off = vpoff & PAGEOFFSET;
	vpoff &= PAGEMASK;

	if (vp == NULL) {
#ifdef _KVM_DEBUG
		_kvm_error("vp_to_fdoffset: null vp ptr");
#endif _KVM_DEBUG
		return (-1);
	}

	paddr = pagefind(kd, &page, vp, vpoff);
	if (paddr == NULL) {
#ifdef _KVM_DEBUG
		_kvm_error("vp_to_fdoffset: page not mapped in");
#endif _KVM_DEBUG
		return (-1);
	}

	/* make sure the page structure is useful */
	if (page.p_pagein) {
#ifdef _KVM_DEBUG
		_kvm_error("vp_to_fdoffset: page in transit");
#endif _KVM_DEBUG
		return (-1);
	}
	if (page.p_gone) {
#ifdef _KVM_DEBUG
		_kvm_error("vp_to_fdoffset: page is gone");
#endif _KVM_DEBUG
		return (-1);
	}

	/*
	 * Page is in core.
	 */
	skaddr = page_to_physaddr(kd, paddr);
	if (skaddr == -1) {
#ifdef _KVM_DEBUG
		_kvm_error("vp_to_fdoffset: can't find page 0x%x", paddr);
#endif _KVM_DEBUG
		return (-1);
	}
	*fdp = kd->corefd;
	*offp = skaddr + off;
	return (0);
}

/* convert page pointer to physical address */
static long
page_to_physaddr(kd, paddr)
	kvm_t *kd;
	struct page *paddr;
{
	struct page *pages;
	u_int pages_base;

	/*
	 * Find out which memory segment page is in.
	 */
	if (paddr < kd->memseg.epages) {
		pages = kd->memseg.pages;
		pages_base = kd->memseg.pages_base;
	} else {
		struct memseg memseg;
		struct memseg *next;

		/* not in first memseg */
		next = kd->memseg.next;
		memseg.epages = kd->memseg.epages;
		while (paddr > memseg.epages) {
			getkvm(next, &memseg, "next memory segment");
			next = memseg.next;
			/*
			 * if next is null, and we try to use it, the getkvm
			 * macro will have us return -1.
			 */
		}
		pages = memseg.pages;
		pages_base = memseg.pages_base;
	}
	return (((paddr - pages) + pages_base) << PAGESHIFT);
}
