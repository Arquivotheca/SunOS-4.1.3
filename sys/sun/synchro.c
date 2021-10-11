#ifndef lint
static char sccsid[] = "@(#)synchro.c 1.1 92/07/30 SMI";
#endif

/* synchronization device */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vmmac.h>
#include <sys/mman.h>

#include <sun/fbio.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/vm_hat.h>

#include <vm/as.h>
#include <vm/seg.h>

#include <pixrect/pr_impl_util.h>

addr_t kmem_zalloc();

#define LOCKDEBUG 1

#ifndef LOCKDEBUG
#define LOCKDEBUG 0
#endif LOCKDEBUG

#if LOCKDEBUG
int lock_debug = 0;
#define	DEBUGF(level, args)	_STMT(if (lock_debug >= (level)) printf args;)
#else LOCKDEBUG
#define	DEBUGF(level, args)
#endif LOCKDEBUG

#define LOCK_OFFBASE    (0)

#define	LOCKTIME	3	/* seconds */

/* flag bits */
#define LOCKMAP         0x01
#define UNLOCKMAP       0x02
#define ATTACH          0x04
#define TRASHPAGE       0x08
#define UNGRAB          0x10    /* ungrab called */

struct  segproc {
        int             flag;
        struct  proc    *procp;
        caddr_t         lockaddr;
        struct  seg     *locksegp;
        caddr_t         unlockaddr;
        struct  seg     *unlocksegp;
	dev_t		dev;
	off_t		offset;
};
 
struct  seglock {
        short           sleepf;         /* sleep flag */
	short		allocf;		/* allocated flag */
        off_t           offset;         /* offset into device */
        int    		*page;          /* virtual address of lock page */
        struct  segproc cr;             /* creator  ### */
        struct  segproc cl;             /* client */
        struct  segproc *owner;         /* current owner of lock */
        struct  segproc *other;         /* not owner of lock */
	caddr_t		link;		/* for pool of seglock */
};
 
static	caddr_t trashpage;		/* for trashing unwanted writes */
static	caddr_t lockpage;		/* bunch o' locks */
 
static long     bits[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008,
    0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800,
    0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000,
    0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000
};
 
#define BITSPERLONG     (32)
#define LOCKSET(lp,n)   (lp[(n)/BITSPERLONG] |= bits[(n)&BITSPERLONG-1])
#define LOCKRESET(lp,n) (lp[(n)/BITSPERLONG] &= ~bits[(n)&BITSPERLONG-1])
#define LOCKTEST(lp,n)  (lp[(n)/BITSPERLONG] & bits[(n)&BITSPERLONG-1])
 
static  unsigned long   lock_map[PAGESIZE/sizeof(long)/BITSPERLONG];
 
static  struct  seglock *lock_pool = NULL;
struct  seglock *seglock_findlock();

static	int seglock_dup(/* seg, newsegp */);
static	int seglock_unmap(/* seg, addr, len */);
static	int seglock_free(/* seg */);
static	faultcode_t seglock_fault(/* seg, addr, len, type, rw */);
static	int seglock_checkprot(/* seg, addr, size, prot */);
static	int seglock_badop();
static	int seglock_incore();

struct	seg_ops seglock_ops =  {
	seglock_dup,	/* dup */
	seglock_unmap,	/* unmap */
	seglock_free,	/* free */
	seglock_fault,	/* fault */
	seglock_badop,	/* faulta */
	seglock_badop,	/* unload */
	seglock_badop,	/* setprot */
	seglock_checkprot,/* checkprot */
	seglock_badop,	/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	seglock_badop,	/* sync */
	seglock_incore	/* incore */
};

int
synchroopen(dev, flag)
	dev_t	dev;
	int	flag;
{
	DEBUGF(3, ("synchro_open(%d), flag:%x\n", minor(dev), flag));

	return 0;
}

/*ARGSUSED*/
synchroclose(dev, flag)

	dev_t dev;
	int flag;
{
	DEBUGF(3, ("synchro_close(%d)\n", minor(dev)));

	return 0;
}

/*ARGSUSED*/
int
synchroioctl(dev, cmd, data, flag)
dev_t		dev;
int		cmd;
caddr_t		data;
int		flag;
{
    DEBUGF(3, ("synchro_ioctl(%d, 0x%x)\n", minor(dev), cmd));

    switch (cmd)
    {
	case GRABPAGEALLOC:
	    return(seglock_graballoc(data));
	case GRABATTACH:
	    return(seglock_grabattach(data));
	case GRABPAGEFREE:
	    return(seglock_grabfree(data));
	case GRABLOCKINFO:
	    return(seglock_grabinfo(data));

	default:
	    return ENOTTY;
    }
}

/*ARGSUSED*/
int
synchrosegmap(dev, offset, as, addr, len, prot, maxprot, flags, cred)
    dev_t dev;
    register u_int offset;
    struct as *as;
    addr_t *addr;
    register u_int len;
    u_int prot, maxprot;
    u_int flags;
    struct ucred *cred;
{
    struct segproc dev_a;
    int	seglock_create();
    register    i;
    caddr_t	seglock;

    enum	as_res	as_unmap();
    int	as_map();

    DEBUGF(3, ("synchro_segmap:dev:%x,off:%x,as:%x, addr:%x, len:%x,flags:%x\n",
	dev, offset, as, addr, len, flags));

    if ((seglock = 
	(caddr_t)seglock_findlock((off_t)offset*sizeof(int)/PAGESIZE)) != (caddr_t)NULL)
    {
	if (len != PAGESIZE)
	    return(EINVAL);
    }
    else
	return (ENXIO);

    if ((flags & MAP_FIXED) == 0) 
    {

	/*
	 * Pick an address w/o worrying about
	 * any vac alignment contraints.
 	 */

	map_addr(addr, len, (off_t)0, 0);
	if (*addr == NULL)
	    return (ENOMEM);

    } 
    else
    {
	/*
	 * User specified address -
	 * Blow away any previous mappings.
	 */

	 i = (int)as_unmap((struct as *)as, *addr, len);
    }

    /*
     * record device number; mapping function doesn't do anything smart
     * with it, but it takes it as an argument.  the offset is needed
     * for mapping objects beginning some ways into them.
     */

    dev_a.dev = dev;
    dev_a.offset = offset*sizeof(int)/PAGESIZE;

    DEBUGF(3, ("locksegmap: as:%x,*addr:%x,len:%x,&dev_a:%x\n",
	as, *addr, len, &dev_a));

    return(as_map((struct as *)as, *addr,len, seglock_create, (caddr_t)&dev_a));
}

static
int
seglock_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segproc *dp = (struct segproc *)argsp;
	struct	seglock	*lp;

	DEBUGF(3, ("seglock_create: seg:%x, argsp:%x\n", seg, argsp));

	if ((lp = seglock_findlock(dp->offset)) == NULL)
		return(-1);

	seg->s_ops = &seglock_ops;

	if (lp->cr.procp == u.u_procp)
	    seg->s_data = (char *)&(lp->cr);
	else
	    seg->s_data = (char *)&(lp->cl);

	return (0);
}

static
int
seglock_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segproc *sdp = (struct segproc *)seg->s_data;

	DEBUGF(3, ("seglock_dup: seg:%x, newseg:%x\n", seg, newseg));

	(void) seglock_create(newseg, (caddr_t)sdp);

	return(0);
}

static
int
seglock_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
        register struct segproc *sdp = (struct segproc *)seg->s_data;
        register struct seg *nseg;
        addr_t nbase;
        u_int nsize;
	void	hat_unload();
	void	hat_newseg();
	static	seglock_lockfree();

	DEBUGF(3, ("seglock_unmap: seg:%x, addr:%x, len:%x\n", seg, addr, len));

	seglock_lockfree(seg);

	/*
 	 * Check for bad sizes
 	 */
 	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
 	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
 		panic("segdev_unmap");

        /*
         * Unload any hardware translations in the range to be taken out.
         */

	hat_unload(seg,addr,len);

        /*
         * Check for entire segment
         */
 
 	if (addr == seg->s_base && len == seg->s_size) {
 		seg_free(seg);
 		return (0);
	}

 	/*
 	 * Check for beginning of segment
 	 */
 
 	if (addr == seg->s_base) {
 		seg->s_base += len;
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * Check for end of segment
 	 */
 	if (addr + len == seg->s_base + seg->s_size) {
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * The section to go is in the middle of the segment,
 	 * have to make it into two segments.  nseg is made for
 	 * the high end while seg is cut down at the low end.
 	 */
 	nbase = addr + len;				/* new seg base */
 	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
 	seg->s_size = addr - seg->s_base;		/* shrink old seg */
 	nseg = seg_alloc(seg->s_as, nbase, nsize);
 	if (nseg == NULL)
 		panic("seglock_unmap seg_alloc");
 
 	nseg->s_ops = seg->s_ops;
 
 	/* figure out what to do about the new context */
 
 	nseg->s_data = (caddr_t)new_kmem_alloc(sizeof (struct segproc),KMEM_NOSLEEP);
 
 	/*
 	 * Now we do something so that all the translations which used
 	 * to be associated with seg but are now associated with nseg.
 	 */
 	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);
	return(0);
}

static
int
seglock_free(seg)
	struct seg *seg;
{
        register struct segproc *sdp = (struct segproc *)seg->s_data;

	DEBUGF(3, ("seglock_free: seg:%x,pid:%x\n", seg, u.u_procp->p_pid));

	seglock_lockfree(seg);

	return(0);
}
 
/*ARGSUSED*/
static
faultcode_t
seglock_fault(seg, addr, len, type, rw)
	register struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register addr_t adr;
	register struct segproc *current
	 	= (struct segproc *)seg->s_data;
	void	hat_devload();
	void	hat_unload();

	if (type != F_INVAL) {
		return(FC_MAKE_ERR(EFAULT));
	}

	return(seglock_lockfault(seg,addr));

}

/*ARGSUSED*/
static
int
seglock_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	DEBUGF(3, ("seglock_checkprot: seg:%x, addr:%x, len:%x, prot:%x\n", 
	    seg, addr, len, prot));

	return(PROT_READ|PROT_WRITE);
}

/*
 * segdev pages are always "in core".
 */
/*ARGSUSED*/
static
seglock_incore(seg, addr, len, vec)
        struct seg *seg;
        addr_t addr;
        register u_int len;
        register char *vec;
{
        u_int v = 0;

	DEBUGF(3, ("seglock_incore: seg:%x, addr:%x, len:%x, vec:%x\n", 
	    seg, addr, len, vec));

        for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
            v += PAGESIZE)
                *vec++ = 1;
        return (v);
}

static
seglock_badop()
{
	/*
	 * silently fail.
	 */
	DEBUGF(3, ("seglock_badop:\n"));

	return(0);
}

/*
 * search the lock_list list for the specified offset
 *
 */
 
/*ARGSUSED*/
static
struct seglock *
seglock_findlock(off)
off_t   off;
{
	struct	seglock	*lp;

	DEBUGF(2, ("seglock_findlock: offset 0x%x\n", off));

	for (lp = lock_pool; lp != NULL; lp = (struct seglock *)lp->link)
	{
 	    if (off == lp->offset)
		return(lp);
	}

	DEBUGF(2, ("seglock_findlock: could not locate offset 0x%x\n", off));

        return((struct seglock *)NULL);
}

/*ARGSUSED*/
int
seglock_grabattach(cookiep)      /* IOCTL */
caddr_t cookiep;
{
 
    return(EINVAL);
}

/*ARGSUSED*/
static int
seglock_grabinfo(pp)    /* IOCTL */
caddr_t pp;
{
        *(int *)pp = 0;
        return (0);
}

/*ARGSUSED*/
int
seglock_graballoc(pp)    /* IOCTL */
caddr_t pp;
{
        register        int i;
	register	struct seglock	*sp;

	if (trashpage == NULL) {
		lockpage = getpages(2,KMEM_NOSLEEP);
		trashpage = lockpage + PAGESIZE;
/* debugging */
/*
	    for(i=0;i<PAGESIZE/sizeof(long);i++)
		*((int *)lockpage + i) = i; 
 */
/* debugging */
	}
 
	    /* any open lock slots? */
	    for(i = 0; i< PAGESIZE/sizeof(long); i++) 
		if (LOCKTEST(lock_map,i) == 0) {
		    LOCKSET(lock_map,i);
		    break;
	    }
	    if (i >= PAGESIZE/sizeof(long))
		return(ENOMEM); 
		
loop:
	    /* any available seglock ? */
	    for(sp = lock_pool; sp != NULL; sp = (struct seglock *)sp->link) {
		if (sp->allocf == 0) {
		    sp->allocf = 1;
		    sp->page = (int *)lockpage;
		    *(sp->page + i) = 0;
		    sp->offset = i*sizeof(int);
		    sp->cr.flag = 0;
		    sp->cl.flag = 0;
		    sp->cr.procp = u.u_procp;
		    sp->cl.procp = NULL;
		    sp->cr.offset = i*sizeof(int);
		    sp->cl.offset = i*sizeof(int);
		    sp->cr.locksegp = NULL;
		    sp->cl.locksegp = NULL;
		    sp->cr.unlocksegp = NULL;
		    sp->cl.unlocksegp = NULL;
		    sp->owner = NULL;
		    sp->other = NULL;
		    sp->sleepf = 0;
		    *(int *)pp = i*PAGESIZE;
		    return(0);
		}
	    }

	    /* nope, make one */
	    sp = (struct seglock *)new_kmem_alloc(sizeof (struct seglock),
						    KMEM_NOSLEEP);
	    sp->allocf = 0;
	    /* no free locks: add a one to the pool */
	    if (lock_pool == NULL) {
		lock_pool = sp;
		sp->link = NULL;
	    } else {
		sp->link = (caddr_t)lock_pool;
		lock_pool = sp;
	    }
	    goto loop;
}
 
/*ARGSUSED*/
int
seglock_grabfree(pp)     /* IOCTL */
caddr_t pp;
{
    register    struct seglock  *lp;
    int		seglock_timeout();
    off_t	offset = ((*(int *)pp)/PAGESIZE)*sizeof(int);
 
    if ((lp = seglock_findlock((off_t)offset)) == NULL)
        return(EINVAL);

    LOCKRESET(lock_map, offset);

    lp->cr.flag |= UNGRAB;

    if (lp->sleepf) {   /* client will acquire the lock in this case */
	untimeout(seglock_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
    return(0);
}

#define _LOCKMAP(sp,addr,page)            \
        hat_devload(sp,                 \
                    addr,               \
                    fbgetpage((caddr_t)page),    \
                    PROT_READ|PROT_WRITE|PROT_USER,0)
 
#define _LOCKUNMAP(segp)          \
        hat_unload(segp,                \
                   (segp)->s_base,      \
                   (segp)->s_size);
 
/*ARGSUSED*/
static
seglock_lockfree(seg)
struct seg *seg;
{
    register struct segproc *sdp = (struct segproc *)seg->s_data;
    struct	seglock	*lp;
 
    if ((lp = seglock_findlock((off_t)sdp->offset))==NULL)
        return;

    if (lp->cr.locksegp == seg)
        lp->cr.locksegp = NULL;
    if (lp->cr.unlocksegp == seg)
        lp->cr.unlocksegp = NULL;
    if (lp->cl.locksegp == seg)
        lp->cl.locksegp = NULL;
    if (lp->cl.unlocksegp == seg)
        lp->cl.unlocksegp = NULL;
 
    if (lp->cr.locksegp == NULL && lp->cr.unlocksegp == NULL &&
        lp->cl.locksegp == NULL && lp->cl.unlocksegp == NULL) {
        LOCKRESET(lock_map, sdp->offset);
        lp->allocf = 0;
    }

    lp->cr.flag |= UNGRAB;
    if (lp->sleepf) {
	untimeout(seglock_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
}

static
int
seglock_timeout(lp)
struct	seglock *lp;
{
    struct  segproc	*np;
    void	hat_devload();

    np = &lp->cr;
    if (np->flag & LOCKMAP) {
	_LOCKUNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->flag & UNLOCKMAP)
	_LOCKUNMAP(np->unlocksegp);
    _LOCKMAP(np->unlocksegp,np->unlockaddr,lp->page);
    np->flag |= UNLOCKMAP;
    np->flag &= ~TRASHPAGE;
 
    np = &lp->cl;
    if (np->locksegp && np->flag & LOCKMAP) {
	_LOCKUNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->unlocksegp) {
	if (np->flag & UNLOCKMAP)
	    _LOCKUNMAP(np->unlocksegp);
	_LOCKMAP(np->unlocksegp,np->unlockaddr,trashpage);
	np->flag |= (UNLOCKMAP | TRASHPAGE);
    }
    wakeup((caddr_t)lp->page);
    return(0);
}

/*
 * Handle lock segment faults here...
 *
 */

static
int
seglock_lockfault(seg,addr)
register struct seg *seg;
addr_t  addr;
{
        register struct seglock *lp;
        register struct segproc *current
                = (struct segproc *)seg->s_data;
        struct  segproc *sp;
	void	hat_devload();
	void	hat_unload();
	extern	int	hz;
	int	s;
 
        /* look up the segment in the lock_list */
 
        lp = seglock_findlock((off_t)current->offset);
        if (lp == NULL)
            return (FC_MAKE_ERR(EFAULT));

	s = splsoftclock();

        if (lp->cr.flag & UNGRAB) {
            _LOCKMAP(seg,addr,trashpage);
	    (void)splx(s);
            return(0);
        }    
 
        /* initialization necessary? */
 
        if (lp->cr.procp == u.u_procp && lp->cr.locksegp == NULL) {
            lp->cr.locksegp = seg;
            lp->cr.lockaddr = addr;
        } else
        if (lp->cr.procp != u.u_procp && lp->cl.locksegp == NULL) {
            lp->cl.procp = u.u_procp;
            lp->cl.locksegp = seg;
            lp->cl.lockaddr = addr;
        } else
        if (lp->cr.procp == u.u_procp && lp->cr.locksegp != seg) {
            lp->cr.unlocksegp = seg;
            lp->cr.unlockaddr = addr;
        } else
        if (lp->cl.procp == u.u_procp && lp->cl.locksegp != seg) {
            lp->cl.unlocksegp = seg;
            lp->cl.unlockaddr = addr;
        }
 
	if( *(int *)lp->page == 0) {
            if (!lp->sleepf) {
                if (lp->owner == NULL) {
                    if (lp->cr.procp == u.u_procp) {
                        lp->owner = &lp->cr;
                        lp->other = &lp->cl;
                    } else {
                        lp->owner = &lp->cl;
                        lp->other = &lp->cr;
                    }   
                }   
                /* switch ownership? */
                if (lp->other->locksegp == seg) {
                    if (lp->owner->flag & LOCKMAP) {
                        _LOCKUNMAP(lp->owner->locksegp);
                        lp->owner->flag &= ~LOCKMAP;
                    }   
                    if (lp->owner->unlocksegp != NULL)
                        if (lp->owner->flag & UNLOCKMAP) {
                            _LOCKUNMAP(lp->owner->unlocksegp);
                            lp->owner->flag &= ~TRASHPAGE;
                            lp->owner->flag &= ~UNLOCKMAP;
                        }   
                    sp = lp->owner;
                    lp->owner = lp->other;
                    lp->other = sp;
                }   
                /* map lock segment to page */
                _LOCKMAP(lp->owner->locksegp,lp->owner->lockaddr,lp->page);
                lp->owner->flag |= LOCKMAP;
                if (lp->owner->unlocksegp != NULL) {
                    if (lp->owner->flag & UNLOCKMAP) /***/
                        _LOCKUNMAP(lp->owner->unlocksegp);
                    _LOCKMAP(lp->owner->unlocksegp,
                           lp->owner->unlockaddr,lp->page);
                    lp->owner->flag &= ~TRASHPAGE;
                    lp->owner->flag |= UNLOCKMAP;
                }
		(void)splx(s);
                return(0);
            }    
	    (void)splx(s);
            return(0);
        }

	if (*(int *)lp->page == 1) {
 
            if (lp->sleepf) {
                if (lp->owner->flag & UNLOCKMAP)        /***/
                    _LOCKUNMAP(lp->owner->unlocksegp);
                _LOCKMAP(lp->owner->unlocksegp,lp->owner->unlockaddr,trashpage);
                lp->owner->flag |= TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
                if (lp->owner->flag & LOCKMAP) {
                    _LOCKUNMAP(lp->owner->locksegp);
                    lp->owner->flag &= ~LOCKMAP;
                }
		untimeout(seglock_timeout,(caddr_t)lp);
                wakeup((caddr_t)lp->page);       /* wake up sleeper */
		(void)splx(s);
                return(0);
            }
 
            if (lp->owner->procp==u.u_procp && lp->owner->unlocksegp==seg) {
                if (lp->owner->flag & UNLOCKMAP)        /***/
                    _LOCKUNMAP(lp->owner->unlocksegp);
                _LOCKMAP(lp->owner->unlocksegp,lp->owner->unlockaddr,lp->page);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
		(void)splx(s);
                return(0);
            }
 
            if (lp->owner->flag & UNLOCKMAP) {
                _LOCKUNMAP(lp->owner->unlocksegp);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag &= ~UNLOCKMAP;
            }

            lp->sleepf = 1;

	    if (lp->cr.procp == u.u_procp)	/* creator about to sleep */
		timeout(seglock_timeout,(caddr_t)lp,LOCKTIME*hz);

            (void)sleep((caddr_t)lp->page,PRIBIO);  	/* goodnight, gracie */
                                        /* we wake up */
            lp->sleepf = 0;             /* clear sleepf */

            if (lp->cr.flag & UNGRAB) {
		_LOCKMAP(seg,addr,trashpage);
		(void)splx(s);
                return(0);
            }
 
            sp = lp->owner;             /* switch owner and other */
            lp->owner = lp->other;
            lp->other = sp;
 
            /* map new owner to page */
            _LOCKMAP(lp->owner->locksegp,lp->owner->lockaddr,lp->page);
            lp->owner->flag |= LOCKMAP;
 
            if (lp->owner->flag & TRASHPAGE) {
                _LOCKUNMAP(lp->owner->unlocksegp);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
                _LOCKMAP(lp->owner->unlocksegp,lp->owner->unlockaddr,lp->page);
            }
        }
	(void)splx(s);
	return(0);
}
