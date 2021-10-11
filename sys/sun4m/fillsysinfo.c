#ifndef lint
static  char sccsid[] = "@(#)fillsysinfo.c 1.1 92/07/30 SMI";
#endif

#include <sys/errno.h>
#include <machine/pte.h>
#include <machine/module.h>
#include <machine/cpu.h>
#include <sys/types.h>
#include <machine/mmu.h>
#include <sun/openprom.h>
#include <machine/devaddr.h>

#ifndef	NULL
#define	NULL 0
#endif	NULL

/*
 * XXX: Disable this and remove all the code before FCS!
 * XXX: c.f. sun4m/autoconf.c and conf.common/master
 */


#ifndef NOPROM
/*
 * The OpenBoot Standalone Interface supplies the kernel with
 * implementation dependent parameters through the devinfo/property mechanism
 */

#define	MAXSYSNAME 40
#define	NMODULES 4

#define	MAXMEMLIST	32	/* %%% hack hack hack %%% */
#define	MAXREGS		32	/* %%% hack hack hack %%% */
#define	MAXNAME		128	/* %%% hack hack hack %%% */

/*
 * as long as we do things like "(1<<slot)", we are gonna
 * have a hard time going above 32 sbus slots; in fact,
 * unless we want to trim the bytes per slot down from
 * 2^28, sixteen is the real limit. Watch for places where
 * the physical address is carried in a long or an int.
 */
#define	SBUS_MAXSLOTS	32

unsigned	sbus_numslots = 0;
unsigned	sbus_basepage[SBUS_MAXSLOTS] = { 0, };
unsigned	sbus_slotsize[SBUS_MAXSLOTS] = { 0, };

extern int 	physmem;
extern	union ptpe	*tmpptes;

struct memlist *availmemory = 0;
struct memlist *physmemory = 0;
struct memlist *virtmemory = 0;

struct machinfo		mach_info 		= 0;
#ifdef	VME
struct vmeinfo 		vme_info 		= 0;
#endif	VME
struct iommuinfo	iommu_info 		= 0;
struct mmcinfo 		mmc_info 		= 0;
struct modinfo 		mod_info[NMODULES] 	= 0;

char   sysname[MAXSYSNAME] 			= 0; /* system name */
char   modname[NMODULES][MAXSYSNAME] 		= 0; /* module names */
char   mcname[MAXSYSNAME] 			= 0; /* mmc name (if any) */

struct dev_reg  obpctx	= 0; /* obp's context table info, reg struc */

struct dev_reg	obp_mailbox = 0; /* obp's mailbox phy addr, each cpu
				    have different mail box address.
				    however, they are all within the
				    same page.  This is used when
				    dumping core occurs.
				 */

typedef enum { XDRBOOL, XDRINT, XDRSTRING } xdrs;

/*
 * structure describing properties that we are interested in querying the
 * OBP for.
 */
struct getprop_info {
	char   *name;
	xdrs	type;
	u_int  *var;
};

/*
 * structure describing nodes that we are interested in querying the OBP for
 * properties.
 */
struct node_info {
	char		*name;
	int		size;
	struct getprop_info *prop;
	struct getprop_info *prop_end;
	unsigned int   *value;
};

/*
 * macro definitions for routines that form the OBP interface
 */
#define	NEXT	(int)prom_nextnode
#define	CHILD	(int)prom_childnode
#define	GETPROP	prom_getprop
#define	GETPROPLEN		prom_getproplen
#define	GETLONGPROP		prom_getprop

extern	dnode_t	prom_nextnode(), prom_childnode();
extern	int	prom_getprop(), prom_getproplen();

#define	CLROUT(a, l)			\
{					\
	register int c = l;		\
	register char *p = (char *)a;	\
	while (c-- > 0)			\
		*p++ = 0;		\
}

#define	CLRBUF(a)	CLROUT(a, sizeof (a))

/*
 * property names
 */
char psname[]    	= "name";
char psdevtype[] 	= "device_type"; /* yes, underscore. */
char psrange[] 		= "ranges";
char psreg[]		= "reg";
char pspagesize[] 	= "page-size";
char psccoher[]   	= "cache-coherence?";

/*
 * node names (value of 'name' property)
 */
char psmmc[]	 = "memory-controller";
char psemmc[]    = "eccmemctl";
char pspmmc[]    = "parmemctl";
#ifdef	VME
char psvme[]	 = "vme";
#endif	VME
char psiommu[]   = "iommu";

/*
 * node types (value of 'device-type' property)
 */
char psproc[]	 = "processor";
char pscpu[]	 = "cpu";
#ifdef	GRAPHICS
char psgraph[]	 = "graphics";
#endif

/*
 * list of well known devices that must be mapped
 */
static struct wkdevice {
	char *wk_namep;
	u_int wk_vaddr;
	u_short wk_maxpgs;
	u_short wk_flags;
#define	V_MUSTHAVE	0x0001
#define	V_OPTIONAL	0x0000
#define	V_MAPPED	0x0002
} wkdevice[] = {
	{ "iommu", V_IOMMU_ADDR, V_IOMMU_PGS, V_MUSTHAVE },
	{ "eccmemctl", V_MEMERR_ADDR, V_MEMERR_PGS, V_OPTIONAL },
	{ "counter", V_COUNTER_ADDR, V_COUNTER_PGS, V_MUSTHAVE },
	{ "interrupt", V_INTERRUPT_ADDR, V_INTERRUPT_PGS, V_MUSTHAVE },
	{ "eeprom", V_EEPROM_ADDR, V_EEPROM_PGS, V_MUSTHAVE },
	{ "sbus", V_SBUSCTL_ADDR, V_SBUSCTL_PGS, V_MUSTHAVE },
#ifdef	VME
	{ "vme", V_VMECTL_ADDR, V_VMECTL_PGS, V_OPTIONAL },
#endif	VME
	{ "auxio", V_AUXIO_ADDR, V_AUXIO_PGS, V_OPTIONAL },
	{ NULL, },
};

map_wellknown_devices()
{
	map_wellknown(NEXT(0), 0, (struct dev_rng *)0);
}

/*
 * map_wellknown - map known devices & registers
 */
map_wellknown(curnode, nrange, rangep)
int curnode;
int nrange;
struct dev_rng *rangep;
{
	u_char tmp_name[MAXSYSNAME];
	int nrb, i;
	extern void apply_range_to_range();
#define	MAXRANGE 8
	struct dev_rng rb[MAXRANGE];

	/* %%% does CHILD return "-1" for end of chain? */
	for (curnode = CHILD((dnode_t) curnode); curnode;
		curnode = NEXT((dnode_t) curnode)) {
		CLRBUF(tmp_name);
		if (GETPROP((dnode_t) curnode, psname, (caddr_t) tmp_name)
		    != -1)
			(void) map_node(curnode, nrange, rangep, tmp_name);

		nrb = GETPROPLEN((dnode_t) curnode, psrange) / sizeof (rb[0]);

		if ((nrb > 0) && (nrb < MAXRANGE)) {
			(void) GETPROP((dnode_t) curnode, psrange,
					(caddr_t) rb);
			apply_range_to_range((char *)tmp_name, nrange,
			    rangep, nrb, rb);

			/* %%% gather sbus info %%% */
			if (!strcmp((char *) tmp_name, "sbus")) {
				sbus_numslots = nrb;
				for (i = 0; i < nrb; ++i) {
					sbus_basepage[i] =
						((rb[i].rng_bustype << 20) |
						mmu_btop(rb->rng_offset));
					sbus_slotsize[i] = rb[i].rng_size;
				}
			}
			map_wellknown(curnode, nrb, rb);
		} else {
			map_wellknown(curnode, nrange, rangep);
		}
	}
}

#ifdef SAS
	/*
	 * this need to be cleanded up.
	 */
#include <mon/openprom.h>

void
build_memlists()
{
	availmemory = *romp->v_availmemory;
	physmemory = *romp->v_physmemory;
	virtmemory = *romp->v_virtmemory;
}
#else
static void
truncate_memlist(mpp, lim)
	struct memlist **mpp;
	unsigned lim;
{
	struct memlist *mp;
	for (mp = *mpp; mp; mp = mp->next) {
		if (mp->size >= lim) {
			mp->size = lim;
			mp->next = 0;
			return;
		}
		lim -= mp->size;
	}
}

void
build_memlists()
{
	extern struct memlist *getmemlist();

	availmemory = getmemlist("memory", "available");
	physmemory = getmemlist("memory", "reg");
	virtmemory = getmemlist("virtual-memory", "available");

	if (physmem)
		truncate_memlist(&availmemory, (unsigned)mmu_ptob(physmem));
}
#endif

/*
 * map_node - map this node if it is well known
 * if offsetnode is non-zero then the registers of nodeid are offsets
 * from the range address of offsetnode.
 * base addresses are physical addresses (Sun-4M needs to retrieve
 * bits 32:35 from the address space), sizes are number of bytes.
 * wkdevice table keeps everything in virtual page numbers and number
 * of virtual pages.
 */

int
map_node(nodeid, nrange, rangep, namep)
int nodeid;
int nrange;
struct dev_rng *rangep;
u_char *namep;
{
	struct dev_reg tmp_regs[MAXREGS];
	struct wkdevice *wkp = wkdevice;
	union ptpe *tmpptesp = tmpptes;
	int nreg, index, page, space, tomap;
	struct dev_reg *drp = tmp_regs;
	extern void apply_range_to_reg();

	for (wkp = wkdevice; wkp->wk_namep; ++wkp)
		if (!strcmp(wkp->wk_namep, (char *) namep))
			goto found;
	return (-1);
found:
	/* get the physical address information - 36 bits, assume pfn given */
	nreg = GETPROPLEN((dnode_t) nodeid, psreg) /
		sizeof (struct dev_reg);
	(void) GETLONGPROP((dnode_t) nodeid, psreg, (caddr_t) tmp_regs);
	apply_range_to_reg((char *)namep, nrange, rangep, nreg, tmp_regs);
	index = mmu_btop(wkp->wk_vaddr - KERNELBASE);
	while (nreg > 0) {
		space = drp->reg_bustype;
		page = mmu_btop(drp->reg_addr);
		tomap = mmu_btop(drp->reg_size - 1) + 1;
		while (tomap > 0) {
			tmpptesp[index].ptpe_int =
				PTEOF(space, page, MMU_STD_SRWX, 0);
			index ++;
			page ++;
			tomap --;
		}
		drp ++;
		nreg --;
	}
	wkp->wk_flags |= V_MAPPED;
	return (0);
}

/*
 * Where to put the data from the devinfo tree,
 * before we pass the data on to the machinfo
 * and other system structures.
 * Initialize to default values.
 */
static u_int	iocache 	= 0; /* BOOL, true if we have one */
static u_int 	ioccoher 	= 0; /* BOOL, true if ioc plays coherency */
static u_int 	mid  		= 0; /* int, MID of a module */
static u_int 	nmmus 		= 1; /* int, number of MMUs on a module */
static u_int 	mmusplit 	= 0; /* bool, true if split i/d mmu */
static u_int 	ncaches 	= 1; /* int, number of caches */
static u_int 	csplit 		= 0; /* bool, true if split i/d cache */
static u_int 	pcache 		= 0; /* bool, true if $ is phys. addressed */
static u_int 	wthru 		= 0; /* bool, true if cache is write-through */
static u_int 	clinesize 	= 32; /* int, bytes per cache line */
static u_int 	cnlines 	= 2048;	/* int, number of cache lines */
static u_int 	cassocia 	= 1; /* int, associativity of cache */
static u_int 	ccoher 		= 0; /* bool, true if module $ is coherent */
static u_int 	nctx 		= 8; /* number of contexts in mmu */
static u_int 	sparcversion 	= 7; /* version of sparc chip */
static u_int 	bcbuf 		= 0; /* bool, true if block copy hardware */
static u_int 	bfbuf 		= 0; /* bool, true if block fill hardware */
static u_int 	iclinesize 	= 0; /* int, bytes per icache line */
static u_int 	icnlines 	= 0; /* int, number of icache lines */
static u_int 	icassocia 	= 1; /* int, associativity of icache */
static u_int 	dclinesize 	= 0; /* int, bytes per dcache line */
static u_int 	dcnlines 	= 0; /* int, number of dcache lines */
static u_int 	dcassocia 	= 1; /* int, associativity of dcache */
static u_int 	eclinesize 	= 0; /* int, bytes per ecache line */
static u_int 	ecnlines 	= 0; /* int, number of ecache lines */
static u_int 	ecassocia 	= 1; /* int, associativity of ecache */
static u_int	ec_parity	= 0; /* bool, ecache supports parity */
static u_int	impl		= 0; /* int, implementation number */
static u_int	version		= 0; /* int, version numver */

/*
 * Fill_machinfo is called from early_startup, very early after.  It extracts
 * the system, memory controller and modules information from PROM.
 * If any of these properties are not found, the value of that field is
 * left at the return value from romvec.
 */
#define	ENDADDR(a)	&a[sizeof (a) / sizeof (a[0])]

fill_machinfo()
{
	register int  	curnode;
	register u_int	i;
	register struct machinfo   	*machinfop;
	register struct mmcinfo   	*mmcinfop;

	machinfop	    = &mach_info;
	machinfop->sys_name = (u_char *)(&sysname[0]);
/*
 * XXX - If/when we add types other than CPU_SUN4M_690, we must teach
 * this bit of code about them, as well as the system_setup function.
 */
	machinfop->sys_type = CPU_SUN4M_690;

	mmcinfop	   = &mmc_info;
	mmcinfop->mc_name  = (u_char *)(&mcname[0]);
	mmcinfop->mc_name[0] = '\0';
	mmcinfop->mc_type  = -1;

	for (i = 0; i < NMODULES; i++)
	    mod_info[i].mod_name  = (u_char *)(modname[i]);

	/*
	 * get the root node & it's name
	 */
	curnode = (int)NEXT((dnode_t)0);
	CLRBUF(sysname);
	(void) GETPROP((dnode_t) curnode, psname,
			(caddr_t) machinfop->sys_name);

	/*
	 * initialize all other nodes
	 */
	fill_node(curnode);
}

/*
 * There are two types of node that we are interested in here.  First
 * are those nodes which are viewed as part of the system as opposed to
 * the module architecture. Currently this includes memory controller,
 * iommu & vme. If the given node is not found in the device tree then
 * that device does not exist on the system implementaion we are
 * running on. For instance desktops will probably *not* have a vme
 * node. The default is to assume the non-existence of a device.
 *
 * Second are the 'module' type nodes. Currently this includes
 * processor and graphics although others may be added over time. The
 * kernel is configured to support a maximum of NMODULES.
 *
 * For each node there are a number of property values we are
 * interested in, in order to configure the kernel. In general it is
 * preferable to use these property values as parameters in routines
 * common to all system/module implementations. This allows the values
 * of parameters to be changed in future implementations without
 * requiring changes to be made to the kernel (eg.  increasing cache
 * size).
 *
 * A less prefered method is to specify type information since then
 * code must be added to the kernel for each new type created. In
 * order to allow inovation at the hardware architecture level this is
 * the method used to differentiate some features at the module
 * hardware architecture level. The trick here is to create an
 * implementation independent interface routine that then switched
 * based on type and vectors through to call an implementation
 * dependent function. For instance see mmu_getsyncflt() in mmu_asi.s.
 */

/*
 * Memory controller properties we are interested in:
 * %%% forgot to tell tayfun we need "mc-type"!
 */
static struct getprop_info eccmmc[] = {
	"mc-type",	XDRSTRING, 	(u_int *)&mcname[0],
	"width", 	XDRINT,		&mmc_info.ecc_width,
	"ecc-width", 	XDRINT,		&mmc_info.ecc_width,
	"parity-width", XDRINT,		&mmc_info.parity_width,
};
#define	eccmmc_end 	ENDADDR(eccmmc)

static struct getprop_info parmmc[] = {
	"mc-type",	XDRSTRING, 	(u_int *)&mcname[0],
	"width", 	XDRINT,		&mmc_info.parity_width,
	"ecc-width", 	XDRINT,		&mmc_info.ecc_width,
	"parity-width", XDRINT,		&mmc_info.parity_width,
};
#define	parmmc_end 	ENDADDR(parmmc)

static struct getprop_info mmc[] = {
	"mc-type",	XDRSTRING, 	(u_int *)&mcname[0],
	"ecc-width", 	XDRINT,		&mmc_info.ecc_width,
	"parity-width", XDRINT,		&mmc_info.parity_width,
};
#define	mmc_end 	ENDADDR(mmc)

#ifdef	VME
/*
 * VME properties we are interested in:
 */
static struct getprop_info vme[] = {
	"iocache?", 	XDRBOOL,	&vme_info.iocache,
};
#define	vme_end 	ENDADDR(vme)
#endif	VME

/*
 * IOMMU properties we are interested in:
 */
static struct getprop_info iommu[] = {
	psccoher, 	XDRBOOL,	&iommu_info.ccoher,
	pspagesize,	XDRINT,		&iommu_info.pagesize,
};
#define	iommu_end 	ENDADDR(iommu)

/*
 * Processor module properties we are interested in:
 */
static struct getprop_info module[] = {

	/* general module parameters */
	"sparcversion", 	XDRINT,		&sparcversion,
	"mid",  		XDRINT,		&mid,
	"bcopy?", 		XDRBOOL,	&bcbuf,
	"bfill?", 		XDRBOOL,	&bfbuf,

	/* mmu */
	"nmmus", 		XDRINT,		&nmmus,
	"mmu-splitid?",		XDRBOOL,	&mmusplit,
	"mmu-nctx", 		XDRINT,		&nctx,

	/* general cache */
	"ncaches", 		XDRINT,		&ncaches,
	"cache-splitid?", 	XDRBOOL,	&csplit,
	"cache-physical?", 	XDRBOOL,	&pcache,
	"cache-wthru?", 	XDRBOOL, 	&wthru,
	"cache-coherence?",	XDRBOOL,	&ccoher,
	"cache-associativity", 	XDRINT,		&cassocia,

	/* combined cache */
	"cache-nlines",		XDRINT,		&cnlines,
	"cache-line-size", 	XDRINT,		&clinesize,

	/* icache */
	"icache-line-size",	XDRINT,		&iclinesize,
	"icache-nlines",	XDRINT,		&icnlines,
	"icache-associativity", XDRINT,		&icassocia,

	/* dcache */
	"dcache-line-size",	XDRINT,		&dclinesize,
	"dcache-nlines",	XDRINT,		&dcnlines,
	"dcache-associativity", XDRINT,		&dcassocia,

	/* ecache */
	"ecache-line-size",	XDRINT,		&eclinesize,
	"ecache-nlines",	XDRINT,		&ecnlines,
	"ecache-associativity", XDRINT,		&ecassocia,
	"ecache-parity?",	XDRBOOL,	&ec_parity,

	/* module type */
	"implementation",	XDRINT,		&impl,
	"version",		XDRINT,		&version,

	/* prom context table */
	"context-table",        XDRSTRING,      (u_int *)&obpctx,

	/* prom mailbox physaddr */
	"mailbox",        	XDRSTRING,      (u_int *)&obp_mailbox,

};
#define	module_end ENDADDR(module)

/*
 * Graphics module properties we are interested in:
 */
#ifdef GRAPHICS
static struct getprop_info graphics[] = {
};
#define	graphics_end ENDADDR(graphics)
#endif

/*
 * The 'system' nodes we are interested in:
 */
static struct node_info wantnodes[] = {
	psmmc,	 sizeof (psmmc),   mmc,    mmc_end,	0,
	psemmc,	 sizeof (psemmc),  eccmmc, eccmmc_end,	0,
	pspmmc,	 sizeof (pspmmc),  parmmc, parmmc_end,	0,
#ifdef	VME
	psvme,	 sizeof (psvme),   vme,	   vme_end,	&mach_info.vme,
#endif	VME
	psiommu, sizeof (psiommu), iommu,  iommu_end,	&mach_info.iommu,
};
#define	wantnodes_end ENDADDR(wantnodes)

/*
 * The 'module' device-types we are interested in:
 */
static struct node_info wantmodules[] = {
	pscpu,	 sizeof (pscpu),   module,	module_end,	0,
	psproc,	 sizeof (psproc),  module,	module_end,	0,
#ifdef	GRAPHICS
	psgraph, sizeof (psgraph), graphics,	graphics_end,	0,
#endif
};
#define	wantmodules_end ENDADDR(wantmodules)


/*
 * Walk the tree looking for interesting nodes
 */
fill_node(nodeid)
int nodeid;
{
	int cnodeid;
	char tmpname[MAXSYSNAME]; /* XXX is there a max or use getproplen()? */
	struct node_info * np;

	for (cnodeid = CHILD((dnode_t) nodeid); cnodeid;
		cnodeid = NEXT((dnode_t) cnodeid))
		fill_node(cnodeid);

	CLRBUF(tmpname);
	if (GETPROP((dnode_t) nodeid, psname, tmpname) == -1)
		return;

	/*
	 * See if this node is one of the 'system' nodes
	 * If so setup various parameters based on it's property
	 * values
	 */
	for (np = wantnodes; np < wantnodes_end; ++np)
		if (!strncmp(np->name, tmpname, np->size)) {
			fill_nodeinfo(nodeid, np);
			return;
		}



	/*
	 * Not a 'system' node we are interested in but maybe a module
	 * Get the 'device-type' and then search to see if we are
	 * interested in it.
	 */
	CLRBUF(tmpname);
	if (GETPROP((dnode_t) nodeid, psdevtype, tmpname) == -1)
		return;

	for (np = wantmodules; np < wantmodules_end; ++np)
		if (!strncmp(np->name, tmpname, np->size)) {
			fill_modinfo(nodeid, np);
			return;
		}
}

/*
 * Retrieve the values of interesting properties for this node
 */
fill_nodeinfo(nodeid, np)
int nodeid;
struct node_info *np;
{
	struct getprop_info * gpp;

	if (np->value)
		np->value[0] += 1;

	for (gpp = (struct getprop_info *)np->prop;
		gpp < (struct getprop_info *)np->prop_end; gpp++) {

		/*
		 * properties can be one of three types (int, bool or string)
		 */
		switch (gpp->type) {
		case XDRSTRING:
			(void) GETPROP((dnode_t) nodeid, gpp->name,
					(caddr_t) gpp->var);
			break;

		case XDRINT:
			(void) GETPROP((dnode_t) nodeid, gpp->name,
					(caddr_t) gpp->var);
			break;

		case XDRBOOL:
			if (GETPROPLEN((dnode_t) nodeid, gpp->name) == -1) {
				gpp->var[0] = 0;
			} else {
				gpp->var[0] = 1;
			}
			break;

		default:
			break;
		}
	}
}

fill_modinfo(nodeid, np)
int nodeid;
struct node_info *np;
{
	struct modinfo *modinfop;
	char tmpname[MAXSYSNAME];
	extern char *strcpy();

	modinfop = &mod_info[mach_info.nmods++];

	CLRBUF(tmpname);
	if (GETPROP((dnode_t) nodeid, psname, tmpname) == -1)
		return;
	(void) strcpy((char *) modinfop->mod_name, tmpname);

	fill_nodeinfo(nodeid, np);

	modinfop->mid 		= mid;
	modinfop->nmmus 	= nmmus;
	modinfop->splitid_mmu 	= mmusplit;
	modinfop->ncaches 	= ncaches;
	modinfop->splitid_cache = csplit;
	modinfop->phys_cache 	= pcache;
	modinfop->write_thru 	= wthru;
	modinfop->clinesize 	= clinesize;
	modinfop->cnlines 	= cnlines;
	modinfop->cassociate 	= cassocia;
	modinfop->ccoher 	= ccoher;
	modinfop->mmu_nctx	= nctx;
	modinfop->sparc_ver 	= sparcversion;
	modinfop->bcopy 	= bcbuf;
	modinfop->bfill 	= bfbuf;
	modinfop->iclinesize 	= iclinesize;
	modinfop->icnlines 	= icnlines;
	modinfop->icassociate 	= icassocia;
	modinfop->dclinesize 	= dclinesize;
	modinfop->dcnlines 	= dcnlines;
	modinfop->dcassociate 	= dcassocia;
	modinfop->eclinesize 	= eclinesize;
	modinfop->ecnlines 	= ecnlines;
	modinfop->ecassociate 	= ecassocia;
	modinfop->ec_parity	= ec_parity;

	/* get processor type */
	switch (impl) {
		case 0x0:	/* Viking module */
			if (ecnlines)
				modinfop->mod_type = CPU_VIKING_MXCC;
			else
				modinfop->mod_type = CPU_VIKING;
			break;
		case 0x1:	/* ROSS module */
			if (version == 0xf)
				modinfop->mod_type = CPU_ROSS605;
			else
				modinfop->mod_type = CPU_ROSS604;
			break;
		default:
			break;
	}
}
#endif NOPROM
