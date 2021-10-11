#ident	"@(#)id.c 1.1 92/07/30 SMI"

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * IPI disk driver.
 *
 * Handles the following string controllers and disks:
 *	Sun Panther	VME-based IPI-2 String Controller
 *	CDC MPI PA8R2A	IPI-2 Disk
 *	CDC MPI/CM-3	String Controller
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/file.h>
#include <sys/debug.h>
#include <sys/syslog.h>

#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <sun/dkio.h>
#include <sun/autoconf.h>
#include <sundev/mbvar.h>

#include <sundev/ipi_driver.h>
#include <sundev/ipi3.h>
#include <sundev/idvar.h>
#include <sundev/ipi_error.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

#include <values.h>	/* for MAXINT */

#define	DEBUG

#define	alloc(type, count) ((type *)kmem_zalloc((count)*sizeof (type)))

static struct id_ctlr	**id_ctlr;	/* pointer to ctlr pointer array  */
static int		nidc;		/* maximum controller number plus 1 */
#define	NIDC_EXTEND	8		/* amount to extend pointer array by */

static struct id_unit	**id_unit;	/* pointer to unit pointer array */
static int		nid;
#define	NID_EXTEND	8		/* number of units to extend by */

static int id_debug = 0;		/* debugging code */
static int id_print_attr;
static int id_watch_enable = 0;		/* enable watchdog for unit status */

#define	MULT_RBUF	0 /* set when physio supports multiple raw buffers */

#ifdef	MULT_MAJOR
static u_char	bmajor_lookup[NMAJOR];	/* index of block major device */
static u_char	cmajor_lookup[NMAJOR];	/* index of character major device */
static u_char	cmajor_trans[NMAJOR];	/* blk maj num corresponding to char */
#endif /* MULT_MAJOR */

#define	ALIGN(x, i)	(((u_long)x + (u_long)i) & ~(u_long)(i - 1))
#define	NBLKS(n, un)	((u_int)(n) >> (un)->un_log_bshift)

#define	MAX_CMDLEN	64
#define	MAX_RESPLEN	256

/*
 * Time limits in seconds.
 */
#define	ID_TIMEOUT	20	/* time limit on commands (in seconds) */
#define	ID_REC_TIMEOUT	16	/* time limit on recovery commands (seconds) */
#define	ID_DUMP_TIMEOUT	16	/* time limit on ISP-80 dump */
#define	ID_IDLE_TIME	10	/* wait time for ctlr to go idle */
#define	ID_RST_TIMEOUT	45	/* wait time for controller reset */

/*
 * Maximum number of commands that can be outstanding to controllers which
 * do not sort to attempt to optimize seek distance.
 */
#define	MAX_UNSORTED	4	/* maximum outstanding cmds if no seek alg */

/*
 * ID_MAX_PARMS = maximum number of attribute parameters that shall be
 * requested per attribute command.  (The controller may have a lower
 * restriction.)
 */
#define	ID_MAX_PARMS	16
#define	ID_RESP_PER_CTLR	4	/* allocated responses per ctrl */

/*
 * Maximum byte count for an I/O operation.
 */
#define	ID_MAXPHYS	(63 * 1024)

/*
 * Definitions for watchdog routines
 */
#define	ID_WATCH_TIME	17	/* seconds between watchdog runs */
#define	ID_MAX_IDLE	(2*ID_WATCH_TIME)
				/* maximum idle time for online device */
static char	id_watch_started;
static void	id_watch();
static void	id_watch_intr();
static void	id_missing_req();
static int	id_recover();
static int	id_recover_start();
static void	id_recover_intr();
static void	id_recover_ctlr();

#define	b_cylin	b_resid		/* cylinder number for disksort */

/*
 * Autoconfiguration data.
 */
static int	idprobe(), idslave();
static void	idintr();
static void	idasync();
static int	idrestart();
static int	idstart();
static struct mb_ctlr	**idcinfo;

struct mb_driver idcdriver = {
	idprobe, idslave, NULL, NULL, NULL, NULL,
	1, "id", NULL, "idc", NULL,	/* mdr_cinfo = idcinfo set by idprobe */
	MDR_BIODMA
};


/*
 * Attribute tables and functions.
 */
static int id_attr_vendor();
static int id_ctlr_conf();
static int id_ctlr_reconf();
static int id_set_ctlr_reconf();
static int id_ctlr_fat_attr();

static struct ipi_resp_table vendor_id_table[] = {
	/* parm-id	minimum-len			function */
	ATTR_VENDOR,	28,				id_attr_vendor,
	0,		0,				NULL,
};

static struct ipi_resp_table ctlr_conf_table[] = {
	/* parm-id	minimum-len			function */
	ATTR_ADDR_CONF,	sizeof (struct addr_conf_parm),	id_ctlr_conf,
	ATTR_SLVCNF_BIT, sizeof (struct reconf_bs_parm), id_ctlr_reconf,
	ATTR_FAC_ATTACH, 0,				id_ctlr_fat_attr,
	0,		0,				NULL
};

static struct ipi_resp_table ctlr_set_conf_table[] = {
	/* parm-id	minimum-len			function */
	ATTR_SLVCNF_BIT, sizeof (struct reconf_bs_parm), id_set_ctlr_reconf,
	0,		0,				NULL
};

static int id_attr_physdk();
static int id_attr_physdk_cm3();
static int id_phys_bsize();
static int id_log_bsize();
static int id_attr_nblks();

static struct ipi_resp_table attach_resp_table[] = {
	/* parm-id	minimum-len			function */
	ATTR_PHYSDK,	sizeof (struct physdk_parm),	id_attr_physdk,
	ATTR_PHYS_BSIZE, sizeof (struct physbsize_parm), id_phys_bsize,
	ATTR_LOG_BSIZE,	sizeof (struct datbsize_parm),	id_log_bsize,
	ATTR_LOG_GEOM,	sizeof (struct numdatblks_parm), id_attr_nblks,
	0,		0,		NULL
};

static int id_respx();
static int id_deflist_parmlen();

static struct ipi_resp_table id_deflist_table[] = {
	/* parm-id	minimum-len			function */
	RESP_EXTENT,	sizeof (struct respx_parm),	id_respx,
	ATTR_PARMLEN,	sizeof (struct parmlen_parm),	id_deflist_parmlen,
	0,		0,		NULL
};

static int	id_format();
static int	id_rdwr_deflist();
static int	id_ioctl_cmd();
static int	id_error();

static void	id_printerr();
static void	id_retry();
static void	id_q_retry();

static void	id_cmd_intr();
static int	id_send_cmd();

static void	id_build_rdwr();
static int	id_kmem_realloc();
static void	id_update_map();

/*
 * Table for handling response.
 */
struct ipi_err_table {
	char	et_byte;	/* byte in parameter (ID is byte 0) */
	u_char	et_mask;	/* mask to apply or parameter ID */
	u_long	et_flags;	/* error code */
	char	*et_msg;	/* message to print */
};
#define	et_parmid et_mask	/* mask doubles as parameter ID for byte 0 */

/*
 * Constant used to determine equivalent facility parameters from
 * the slave parameter ID.  The facility ID is always 0x10 more.
 */
#define	IPI_FAC_PARM	0x10

/*
 * Macros to initialize table.  Used by IPI_ERR_TABLE_INIT
 */
#define	IPI_ERR_PARM(sid, fid, flags, msg) \
	{ 0,		(sid),		(flags), 	(msg) },
#define	IPI_ERR_BIT(byte, bit, flags, msg) \
	{ (byte),	1<<(bit),	(flags),	(msg) },

static struct ipi_err_table ipi_err_table[] = {
	IPI_ERR_TABLE_INIT()
	{ 0, 		0, 		0,		NULL }
};

static struct ipi_err_table ipi_cond_table[] = {
	IPI_COND_TABLE_INIT()
	{ 0, 		0, 		0,		NULL }
};

static struct opcode_name {
	u_char	opcode;
	char	*name;
} opcode_name[] = {
	{ IP_NOP,		"no-op",	},
	{ IP_ATTRIBUTES,	"Attributes"	},
	{ IP_REPORT_STAT,	"Report Status"	},
	{ IP_READ,		"read"		},
	{ IP_WRITE,		"write"		},
	{ IP_FORMAT,		"format"	},
	{ IP_READ_DEFLIST,	"read defects list"	},
	{ IP_WRITE_DEFLIST,	"write defect list"	},
	{ IP_REALLOC,		"reallocate"	},
	{ IP_ALLOC_RESTOR,	"allocate restore"	},
	{ IP_DIAG_CTL,		"diag ctl"	},
	{ 0xff,			"async rupt"	},
	{ 0,			NULL		}	/* table end */
};

/*
 * Variables for write checking (read-back).
 */
#define	ID_WCHK_BUFL	(64*1024)	/* length of buffer for read-back */
static struct buf	*id_wchk_buf;	/* pointer to buffer for read-back */
static void	id_alloc_wchk();	/* allocate read-back buffer */
static void	id_readback();		/* issue command for read-back */


long	dk_hexunit;		/* mask for dk numbers using hex unit number */
				/* XXX - duplicates line in sys/dk.h */

extern char *	strncpy();

/*
 * Determine existence of a controller.  Called at boot time only.
 */
static
idprobe(reg, ctlr)
	u_short *reg;		/* IPI device (channel/slave/fac) number */
	int	ctlr;		/* slave (string controller) number */
{
	struct id_ctlr	*c;
	int	unit;
	int	fac;
	int	csf = (int)reg;	/* IPI device (channel/slave/fac) number */

#if MULT_MAJOR
	/*
	 * Must initialize major number mapping even if no devices exist,
	 * since open routines could still be called.
	 */
	id_major_map_init();
#endif
	if (IPI_FAC(csf) != IPI_NO_ADDR) {
		printf("idc%d: configured with bad IPI address %x\n",
			ctlr, csf);
		return (0);
	}
	/*
	 * Call ipi_device to be sure channel is configured.
	 * It will be called again in id_init_ctlr(), so some args not needed.
	 */
	if (ipi_device(csf, 0, 0, (void (*)())NULL, (void (*)())NULL,
			(caddr_t)NULL))
		return (0);	/* channel not present */
	/*
	 * Make sure we have enough controller pointers.
	 */
	if (ctlr >= nidc) {
		if (id_kmem_realloc((caddr_t *) &id_ctlr,
				(u_int)nidc * sizeof (struct id_ctlr),
				(u_int)(ctlr+NIDC_EXTEND)
					* sizeof (struct id_ctlr)))
			return (0);
		if (id_kmem_realloc((caddr_t *) &idcinfo,
				(u_int)nidc * sizeof (struct mb_ctlr *),
				(u_int)(ctlr+NIDC_EXTEND)
					* sizeof (struct mb_ctlr *)))
			return (0);
		idcdriver.mdr_cinfo = idcinfo;
		nidc = ctlr + NIDC_EXTEND;
	}

	/*
	 * Allocate controller structure.
	 */
	if ((c = id_ctlr[ctlr]) != NULL)
		return (0);		/* already allocated */
	if ((c = alloc(struct id_ctlr, 1)) == NULL)
		return (0);		/* no memory */
	id_ctlr[ctlr] = c;
	c->c_flags |= IE_INIT_STAT;	/* the status is unknown */
	c->c_fac_flags = (1 << IDC_NUNIT) - 1;	/* assume all facilities */
	c->c_ipi_addr = csf;

	/*
	 * Initialize controller by reading Vendor-ID, attributes.
	 */
	if (id_init_ctlr(csf, ctlr, ID_SYNCH)) {
		/*
		 * It's there.  Configure all online drives on this controller.
		 */
		unit = ID_MAKE_UNIT(IPI_CHAN(csf), IPI_SLAVE(csf), 0);
		for (fac = 0; fac < IDC_NUNIT; fac++, unit++)
			if (c->c_fac_flags & (1 << fac))
				(void) id_findslave(unit, c, ID_SYNCH);
	}
	return (IPI_NSLAVE);
}

/*
 * Initialize controller if it is present.
 * Called through idprobe at boot and by idopen.
 * Returns non-zero if controller is present.
 */
static
id_init_ctlr(reg, ctlr, mode)
	int	reg;		/* IPI device (channel/slave/fac) number */
	int	ctlr;		/* slave (string controller) number */
	int	mode;		/* ID_SYNCH or ID_ASYNCWAIT */
{
	register struct id_ctlr	*c;
	struct mb_ctlr	*mc;
	struct mb_driver *mdr;

	ASSERT(mode != ID_ASYNCH);
	c = id_ctlr[ctlr];
	c->c_ctype = DKC_UNKNOWN;
	c->c_max_q = 1;		/* temporarily limit to one cmd at a time */
	c->c_max_q_len = MAXINT;
	c->c_max_parms = 1;
	c->c_ctlr = ctlr;
	c->c_ipi_addr = reg;

	/*
	 * Search for mb_ctlr structure.  mdr_cinfo isn't setup at probe time.
	 * This could be eliminated later with an autoconf change.  XXX
	 */
	for (mc = mbcinit; mdr = mc->mc_driver; mc++) {
		if (mdr == &idcdriver && mc->mc_ctlr == ctlr)
			break;
	}
	if (mdr) {
		c->c_intpri = mc->mc_intpri;
	}

	if (ipi_device(reg, (int)c->c_intpri, 0, idasync,
			id_recover_ctlr, (caddr_t)c))
		return (0);	/* channel not present */

	c->c_recover.rec_history = 0;
	c->c_recover.rec_state = IDR_RST_CTLR1;
	c->c_recover.rec_substate = 0;
	if (mode == ID_SYNCH)
		c->c_recover.rec_flags = IP_SYNC | IP_NO_RETRY;
	else
		c->c_recover.rec_flags = IP_NO_RETRY;

	(void) id_recover(c, (struct id_unit *)NULL);

	if (mode == ID_ASYNCHWAIT)
		while (c->c_flags & IE_RECOVER)
			(void) sleep((caddr_t)c, PRIBIO);

	if (c->c_recover.rec_state != IDR_NORMAL)
		return (0);

	/*
	 * Allocate enough command and response packets.
	 */
	if (ipi_alloc(reg, c->c_max_q, (int)c->c_rw_cmd_len) == 0 ||
			ipi_alloc(reg, ID_RESP_PER_CTLR, (int)c->c_max_resplen)
				== 0)
		return (0);

	(void) ipi_timeout(reg, ID_TIMEOUT);

	return (IPI_NSLAVE);		/* number of slaves */
}

/*
 * Inspect vendor ID parameter, and set controller type.
 */
/* ARGSUSED */
static int
id_attr_vendor(q, parm_id, parm, len, cp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x50) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	cp;		/* pointer id_ctlr structure */
{
	register struct vendor_parm *vp;
	register struct id_ctlr *c = (struct id_ctlr *)cp;

	vp = (struct vendor_parm *) parm;

	/*
	 * Set command packet length for controllers which don't require
	 * the transfer notification parameter.
	 */
	c->c_rw_cmd_len = sizeof (struct ipi_rdwr_cmd) - sizeof (u_short);
	c->c_max_parms = ID_MAX_PARMS;
	/*
	 * Set controller type based on manufacturer and model.
	 */
	if (strncmp(vp->manuf, "MPI/CM-3", 8) == 0) {
		c->c_ctype = DKC_CDC_9057;
		c->c_flags |= ID_CTLR_VERIFY;
c->c_max_parms = 1;	/* XXX - debug */
	} else if (strcmp(vp->manuf, "IntelliStor") == 0 &&
			strcmp(vp->model, "Model 00") == 0 &&
			strcmp((char *)vp->rev, "R001") == 0)
	{
		c->c_ctype = DKC_FJ_M1060;	/* FJ/Intellistor */
		c->c_flags |= ID_CTLR_VERIFY;
	} else if (strncmp(vp->manuf, "SUNMICRO", 8) == 0 &&
		strncmp(vp->model, "PANTHER", 7) == 0)
	{
		c->c_ctype = DKC_SUN_IPI1;	/* Panther */
		/*
		 * Set flag indicating this controller has enough
		 * sense to schedule requests based on head position.
		 */
		c->c_flags |= ID_SEEK_ALG;
		c->c_rw_cmd_len = sizeof (struct ipi_rdwr_cmdx)
			- sizeof (u_short);
		c->c_max_parms = 1;
		/*
		 * Early versions of Panther don't support the slave
		 * reconfiguration parameter.
		 */
		if (vp->rev[3] == 0)
			c->c_flags |= ID_NO_RECONF;
	} else {
		c->c_ctype = DKC_UNKNOWN;
		printf("idc%d: unknown ctlr vendor id: manuf '%s' model '%s'\n",
			c->c_ctlr, vp->manuf, vp->model);
	}
}


/*
 * Handle addressee configuration parameter for controller.
 */
/* ARGSUSED */
static int
id_ctlr_conf(q, parm_id, parm, len, cp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x65) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	cp;		/* pointer id_ctlr structure */
{
	register struct id_ctlr *c = (struct id_ctlr *)cp;
	register struct addr_conf_parm *acp = (struct addr_conf_parm *) parm;

	c->c_max_cmdlen = acp->ac_max_cmdlen;
	c->c_max_resplen = acp->ac_max_resplen;
	c->c_max_q = (acp->ac_max_queue > 0) ? acp->ac_max_queue : MAXSHORT;
	c->c_max_q_len = (acp->ac_cmdbufsize > 0) ? acp->ac_cmdbufsize : MAXINT;
}

/*
 * Handle slave reconfiguration (bit fields) parameter for controller.
 */
/* ARGSUSED */
static int
id_ctlr_reconf(q, parm_id, parm, len, cp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x6e) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	cp;		/* pointer id_ctlr structure */
{
	register struct id_ctlr *c = (struct id_ctlr *)cp;
	register struct reconf_bs_parm *rp = (struct reconf_bs_parm *) parm;

	if (rp->sr_seek_alg)
		c->c_flags |= ID_SEEK_ALG;
	c->c_reconf_bs_parm = *rp;
}

/*
 * Handle setting of slave reconfiguration (bit fields) parameter for ctlr.
 */
/* ARGSUSED */
static int
id_set_ctlr_reconf(q, parm_id, parm, len, cp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x6e) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	cp;		/* pointer id_ctlr structure */
{
	register struct id_ctlr *c = (struct id_ctlr *)cp;
	register struct reconf_bs_parm *rp;

	rp = &c->c_reconf_bs_parm;	/* point to copy in ctlr struct */

	rp->sr_inh_resp_succ = 0;
	rp->sr_inh_ext_substat = 0;
	rp->sr_tnp_req = 1;
	rp->sr_inh_slv_msg = 0;		/* enable slave messages */
	rp->sr_dis_cl1x = 0;		/* enable class 1 transitions */
	*(struct reconf_bs_parm *)parm = *rp;	/* set parm */
	return (0);
}

/*
 * Handle facilities attached parameter for controller.
 */
/* ARGSUSED */
static int
id_ctlr_fat_attr(q, parm_id, parm, len, cp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x6e) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	cp;		/* pointer id_ctlr structure */
{
	struct id_ctlr	*c = (struct id_ctlr *)cp;
	struct fat_parm *fp = (struct fat_parm *)parm;
	int		flags = 0;

	for (; len >= sizeof (*fp); fp++, len -= sizeof (*fp))
		flags |= 1 << fp->fa_addr;
	c->c_fac_flags = flags;
}


/*
 * Slave routine.
 *	Just return zero since the controller probe configures everything.
 */
/* ARGSUSED */
static
idslave(md, reg)
	struct mb_device *md;
	caddr_t	    reg;		/* IPI address */
{
	return (0);
}


/*
 * Initialize unit by taking status and reading label via recovery procedure.
 */
static
id_init_unit(un, mode)
	struct id_unit 	*un;
	int		mode;
{
	if (un->un_lp == NULL &&
		(un->un_lp=(struct dk_label *)kmem_zalloc(DEV_BSIZE)) == NULL)
	{
		printf("id%3x: out of memory for label info\n", un->un_unit);
		return;
	}
	/*
	 * reset most flags.
	 */
	un->un_flags = (un->un_flags & IE_INIT_STAT) | IE_RECOVER;
	un->un_recover.rec_history = 0;
	un->un_recover.rec_state = IDR_STAT_UNIT;
	un->un_recover.rec_substate = 0;
	if (mode == ID_SYNCH)
		un->un_recover.rec_flags = IP_SYNC;
	else
		un->un_recover.rec_flags = 0;
	(void) id_recover(un->un_ctlr, un);
	if (mode == ID_ASYNCHWAIT)
		while (un->un_flags & IE_RECOVER)
			(void) sleep((caddr_t)un, PRIBIO);
}

static
id_findslave(unit, c, mode)
	int		unit;
	struct id_ctlr	*c;
	int		mode;
{
	struct id_unit	*un;
	struct mb_device *md;
	int		ctlr;		/* controller number */
	int		fac;		/* facility number */
	int		csf;		/* IPI channel/slave/facility address */
	extern int	dkn;		/* disk index for iostats */

	ctlr = c->c_ctlr;
	/*
	 * Make sure we have enough unit pointers.
	 */
	if (unit >= nid) {
		if (id_kmem_realloc((caddr_t *)&id_unit,
				(u_int)nid * sizeof (struct id_unit *),
				((u_int)unit + NID_EXTEND)
					* sizeof (struct id_unit *)))
			return (0);
		nid = unit + NID_EXTEND;
	}

	/*
	 * Allocate unit structure.
	 */
	if ((un = id_unit[unit]) == NULL) {
		if ((un = alloc(struct id_unit, 1)) == NULL)
			return (0);		/* no memory */
		id_unit[unit] = un;
		un->un_unit = ID_CSF_PRINT(unit);
	}

	fac = ID_FAC(unit);
	csf = ID_CSF(unit);

	if (c->c_unit[fac] != NULL)
		return (0);			/* unit already configured */

	un->un_ipi_addr = csf;
	un->un_flags |= IE_INIT_STAT;		/* suppress offline messages */
	c->c_unit[fac] = un;
	un->un_ctlr = c;

	/*
	 * Setup number for I/O stats.  Must do before reading label.
	 */
	if (dkn < DK_NDRIVE)
		un->un_dk = dkn++;
	else
		un->un_dk = -1;

	/*
	 * Get attributes and label.
	 */
	id_init_unit(un, mode);

	if (!(un->un_flags & ID_UN_PRESENT)) {
		c->c_unit[fac] = NULL;
		un->un_ipi_addr = -1;
		un->un_ctlr = NULL;
		id_unit[unit] = NULL;
		/*
		 * Give back dk number if no other driver took one.
		 */
		if (dkn == un->un_dk)
			dkn--;
		kmem_free((caddr_t)un, sizeof (struct id_unit));
		return (0);
	}

	if (un->un_dk >= 0)
		dk_hexunit |= 1 << un->un_dk;
	un->un_flags |= ID_ATTACHED;

	/*
	 * We found an online disk.  If it has
	 * never been attached before, we simulate
	 * the autoconf code and set up the necessary
	 * data.
	 */
	for (md = mbdinit; md->md_driver; md++) {
		if (md->md_driver == &idcdriver && md->md_unit == unit) {
			md->md_alive = 1;
			md->md_ctlr = ctlr;
			if ((md->md_mc = idcinfo[ctlr]) != NULL) {
				md->md_hd = md->md_mc->mc_mh;
				md->md_addr = md->md_mc->mc_addr;
			}
			md->md_dk = un->un_dk;
#ifdef	sun4m
			(void) add_ipi_drives(md, md->md_driver);
#endif	sun4m
			break;
		}
	}

	/*
	 * Print the found message and attach the
	 * disk to the system.
	 */
	printf("id%3x at idc%d facility %x\n", un->un_unit, ctlr, ID_FAC(unit));
	return (1);
}

/*
 * Inspect Physical disk configuration parameter - set unit geometry.
 */
/* ARGSUSED */
static int
id_attr_physdk(q, parm_id, parm, len, unp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x5f) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	unp;		/* pointer to id_unit structure */
{
	register struct id_unit *un = (struct id_unit *)unp;
	register struct physdk_parm *pp = (struct physdk_parm *)parm;
	register struct dk_geom *g = &un->un_g;

	if (un->un_ctlr->c_ctype == DKC_CDC_9057)
		return (id_attr_physdk_cm3(q, parm_id, parm, len, unp));
	g->dkg_pcyl = pp->pp_last_cyl + 1; /* number of physical cyls */
	g->dkg_ncyl = pp->pp_last_cyl;		/* allow one alternate */
	g->dkg_acyl = 1;
						/* alternate cylinders */
	g->dkg_bcyl = 0;			/* beginning cylinder */
	g->dkg_nhead = pp->pp_nheads;		/* tracks per cylinder */

	/*
	 * rot/min = 60 sec/min * 1000000 usec/sec  /  usec/rot
	 * 	but try not to divide by zero.
	 */
	if (pp->pp_rotper > 0)
		g->dkg_rpm = 60 * 1000000 / pp->pp_rotper;
	else
		g->dkg_rpm = 3600;		/* reasonable default */
	return (0);
}

/*
 * Inspect Physical disk configuration parameter - set unit geometry.
 *	This version just for CDC CM-3 which has non-standard layout.
 */
/* ARGSUSED */
static int
id_attr_physdk_cm3(q, parm_id, parm, len, unp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x5f) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	unp;		/* pointer to id_unit structure */
{
	register struct id_unit *un = (struct id_unit *)unp;
	register struct physdk_parm_cm3 *pp = (struct physdk_parm_cm3 *)parm;
	register struct dk_geom *g = &un->un_g;

	g->dkg_pcyl = pp->pp_last_cyl + 1; /* number of physical cyls */
	g->dkg_ncyl = pp->pp_last_cyl;		/* allow one alternate */
	g->dkg_acyl = 1;
						/* alternate cylinders */
	g->dkg_bcyl = 0;			/* beginning cylinder */
	g->dkg_nhead = pp->pp_nheads;		/* tracks per cylinder */

	/*
	 * rot/min = 60 sec/min * 1000000 usec/sec  /  usec/rot
	 * 	but try not to divide by zero.
	 */
	if (pp->pp_rotper > 0)
		g->dkg_rpm = 60 * 1000000 / pp->pp_rotper;
	else
		g->dkg_rpm = 3600;		/* reasonable default */
	return (0);
}

/*
 * Inspect Logical Geometry parameter - get logical blocks per track.
 *	The geometry should be already set from the physical disk configuration
 *	parameter, so doublecheck with this parameter.
 */
/* ARGSUSED */
static int
id_attr_nblks(q, parm_id, parm, len, unp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x5f) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	unp;		/* pointer to id_unit structure */
{
	register struct id_unit *un = (struct id_unit *)unp;
	register struct numdatblks_parm *pp = (struct numdatblks_parm *)parm;
	register struct dk_geom *g = &un->un_g;
	int	i;
	int	unit = un->un_unit;

	g->dkg_nsect = pp->bpertrk;	/* number of blocks per track */
	un->un_first_block = pp->strtadr;
	if (pp->strtadr != 0) {
		printf("id%3x: warning: starting block address nonzero: %d\n",
			unit, pp->strtadr);
	}
	if (pp->bpertrk == 0) {
		printf("id%3x: zero blocks per track\n", unit);
		return;
	}
	i = pp->bpercyl / pp->bpertrk;		/* tracks per cylinder */
	if (i != g->dkg_nhead) {
		printf(
"id%3x: inconsistent geometry: blk/cyl %d blk/trk %d heads %d\n",
			unit, pp->bpercyl, pp->bpertrk,
			g->dkg_nhead);
	}
	if (pp->bpercyl == 0) {
		printf("id%3x: zero blocks per cylinder\n", unit);
		return;
	}
	i = pp->bperpart / pp->bpercyl;		/* cylinder count */
	if (i > g->dkg_pcyl) {
		printf(
"id%3x: inconsistent geometry: tot blks %d blks/cyl %d pcyl %d\n",
			unit, pp->bperpart, pp->bpercyl, g->dkg_pcyl);
		g->dkg_pcyl = MIN(i, g->dkg_ncyl);
		g->dkg_ncyl = g->dkg_pcyl - g->dkg_acyl;
		printf("id%3x: setting pcyl %d ncyl %d acyl %d\n",
			unit, g->dkg_pcyl, g->dkg_ncyl, g->dkg_acyl);
	}
}


/*
 * Handle parameter:  Size of disk physical blocks.
 */
/* ARGSUSED */
static int
id_phys_bsize(q, parm_id, parm, len, unp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x5f) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	unp;		/* pointer to id_unit structure */
{
	register struct id_unit *un = (struct id_unit *)unp;
	register struct physbsize_parm *pp = (struct physbsize_parm *)parm;

	if ((un->un_phys_bsize = pp->pblksize) > 0)
		un->un_flags |= ID_FORMATTED;
}

/*
 * Handle parameter:  Size of disk logical blocks.
 */
/* ARGSUSED */
static int
id_log_bsize(q, parm_id, parm, len, unp)
	struct ipiq	*q;
	int	parm_id;	/* parameter ID (0x5f) */
	u_char	*parm;		/* parameter */
	int	len;		/* length of parameter */
	caddr_t	unp;		/* pointer to id_unit structure */
{
	register struct id_unit *un = (struct id_unit *)unp;
	register struct datbsize_parm *pp = (struct datbsize_parm *)parm;
	int	i;

	un->un_log_bsize = pp->dblksize;
	for (i = DEV_BSHIFT; i < NBBY * NBPW; i++) {
		if (un->un_log_bsize == (1 << i)) {
			un->un_log_bshift = i;
			break;
		}
	}
	if (i == NBBY * NBPW) {
		printf("id%3x: invalid logical block size %d\n",
			un->un_unit, un->un_log_bsize);
	}
}

/*
 * Open routines.
 *
 * The block and character devices use diferent open routines so that
 * the multiple major number support can work.
 */
idbopen(dev, flag)
	dev_t	dev;
	int	flag;
{
	return (idopen(ID_BMINOR(dev), flag, 0));
}

idcopen(dev, flag)
	dev_t	dev;
	int	flag;
{
	return (idopen(ID_CMINOR(dev), flag, 1));
}

static
idopen(minor_dev, flag, char_dev)
	u_int	minor_dev;
	int	flag;
	int	char_dev;	/* non-zero if called by character device */
{
	int	unit;
	int	ctlr;
	int	csf;		/* IPI address */
	register struct id_ctlr	*c;
	register struct id_unit	*un;
	int	s;

	un = NULL;
	if ((unit = ID_UNIT(minor_dev)) < nid)
		un = id_unit[unit];
	/*
	 * If this drive wasn't found by auto configuration or a previous
	 * open, see if it has become available since then.
	 */
	if (un == NULL || !(un->un_flags & ID_UN_PRESENT)) {
		/*
		 * Disable IPI interrupts.
		 *
		 * This is minor overkill, since all we really need to do
		 * is lock out disk interrupts while we are doing disk
		 * commands.  However, there is no easy way to tell what
		 * the disk priority is at this point.  Since this situation
		 * only occurs once at disk spin-up, the extra lock-out
		 * shouldn't hurt very much.
		 */
		s = spl_ipi();

		/*
		 * Search for an existing controller.
		 */
		csf = ID_CTLR_CSF(unit);	/* controller's address */
		for (ctlr = 0; ctlr < nidc; ctlr++) {
			if ((c = id_ctlr[ctlr]) && c->c_ipi_addr == csf &&
				((c->c_flags & ID_C_PRESENT) ||
				id_init_ctlr(csf, ctlr, ID_ASYNCHWAIT)))
				break;
		}

		/*
		 * If a controller was found.  Try to initialize unit.
		 */
		if (ctlr < nidc && c && id_findslave(unit, c, ID_ASYNCHWAIT)) {
			un = id_unit[unit];
		}
		(void) splx(s);
	}

	/*
	 * By the time we get here the disk is marked present if it exists
	 * at all.  We simply check to be sure the partition being opened
	 * is nonzero in size.  If a raw partition is opened with the
	 * nodelay flag, we let it succeed even if the size is zero.  This
	 * allows ioctls to later set the geometry and partitions.
	 */
	if (un && (un->un_flags & ID_UN_PRESENT) &&
		(un->un_flags & ID_ATTACHED) &&
		IE_STAT_PRESENT(un->un_flags) &&
		IE_STAT_PRESENT(un->un_ctlr->c_flags))
	{
		/*
		 * If this is the first open, schedule the watchdog routine.
		 */
		if (!id_watch_started) {
			id_watch_started = 1;
			id_watch();
		}
		if (un->un_lpart[ID_LPART(minor_dev)].un_map.dkl_nblk > 0)
			return (0);
		if (char_dev && (flag & (FNDELAY | FNBIO)))
			return (0);
	}
	return (ENXIO);
}

int (*idstrategy_tstpoint)();

idstrategy(bp)
	register struct buf *bp;
{
	register int		unit;	/* unit number */
	register struct id_unit	*un;	/* unit structure */
	register struct id_ctlr *c;	/* controller struct */
	register struct dk_geom *g;	/* geometry pointer */
	register struct diskhd	*dp;	/* queue header*/
	daddr_t			blkno;	/* block number */
	register struct dk_map	*lp;	/* logical partition */
	int	s;

	if (idstrategy_tstpoint != NULL) {
		if ((*idstrategy_tstpoint)(bp)) {
			return;
		}
	}

	if ((unit = ID_BUNIT(bp->b_dev)) >= nid || (un=id_unit[unit]) == NULL) {
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}

	c = un->un_ctlr;
	g = &un->un_g;
	lp = &un->un_lpart[ID_LPART(bp->b_dev)].un_map;

	if (!(un->un_flags & ID_ATTACHED) || !IE_STAT_PRESENT(un->un_flags) ||
			!IE_STAT_PRESENT(c->c_flags))
	{
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		iodone(bp);
	} else if ((blkno = dkblock(bp)) >= lp->dkl_nblk || lp->dkl_nblk == 0) {
		if ((blkno != lp->dkl_nblk) || (bp->b_flags & B_READ) == 0) {
			bp->b_error = EINVAL;
			bp->b_flags |= B_ERROR;
		}
		bp->b_resid = bp->b_bcount;	/* generate EOF return */
		iodone(bp);
	} else if (un->un_log_bsize == 0) {
		iodone(bp);
	} else if (bp->b_bcount < un->un_log_bsize) {
		bp->b_error = EINVAL;		/* not reading a full block */
		bp->b_flags |= B_ERROR;
		iodone(bp);
	} else {
		bp->b_cylin = blkno / (g->dkg_nsect * g->dkg_nhead) +
			lp->dkl_cylno + g->dkg_bcyl;
		/*
		 * If the operation is off the end of the disk, it's an error.
		 * This really shouldn't happen since we checked the block
		 * number against the end of the partition, but the label
		 * might be wrong.
		 */
		if (bp->b_cylin >= g->dkg_ncyl) {
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}

		/*
		 * While playing with queues, lock out interrupts.
		 * If there are already requests queued for this disk,
		 * sort the new request into the list.
		 * Then call idstart to send the command if possible.
		 */
		dp = &un->un_bufs;
		s = spl_ipi();
		if (dp->b_actf) {
			disksort(dp, bp);
		} else {
			bp->av_forw = NULL;
			dp->b_actf = dp->b_actl = bp;
		}
		(void) idstart(un);
		(void) splx(s);
	}
}

int     idstart_busy;                   /* statistics */

/*
 * This routine starts an operation for a specific unit.  It is used only
 * in the normal asynchronous case, and is only called if the controller
 * is not known to be busy.  It is OK to call this routine even if there
 * are no outstanding operations, so it is called whenever the unit
 * becomes free.
 *
 * It is used by idintr() and idstrategy().  It is always called
 * at disk interrupt priority.
 */
static int
idstart(un)
	register struct id_unit	*un;
{
	register struct diskhd	*dp = &un->un_bufs;
	register struct buf	*bp;
	register struct ipiq	*q;
	register struct id_ctlr		*c = un->un_ctlr;
	int			dk;
	daddr_t			blkno;
	int			nblks;
	struct un_lpart		*lp;	/* logical partition */
	struct ipiq		*rbq;	/* read-back request */

	while (((q = un->un_wait_q) != NULL || (bp = dp->b_actf) != NULL) &&
			IE_STAT_READY(un->un_flags) &&
			IE_STAT_READY(c->c_flags) &&
			!(un->un_flags & ID_LOCKED))
	{
                if (c->c_cmd_q >= c->c_max_q  ||
                    c->c_cmd_q_len + c->c_rw_cmd_len >= c->c_max_q_len) {
                        c->c_flags |= ID_CTLR_WAIT;
                        idstart_busy++;
                        break;
                }
		/*
		 * First check for a queue already setup but waiting for a
		 * writeback.  If none, setup DMA and get command block.
		 */
		if (q != NULL) {
			un->un_wait_q = NULL;
			bp = q->q_buf;
		} else if ((q = ipi_setup(un->un_ipi_addr, bp,
			c->c_rw_cmd_len, MAX_RESPLEN,
			idrestart, 0)) == NULL)
		{
			if (bp->b_flags & B_ERROR) {
				dp->b_actf = bp->av_forw;
				if (dp->b_actl == bp)
					dp->b_actl = NULL;
				iodone(bp);
				continue;
			}
			return (DVMA_RUNOUT);	/* setup failed */
		}

		/*
		 * If writing and write checking is on for the partition,
		 * set a flag so that read-back will occur.
		 * Also, be sure that read-back buffer is setup.
		 * If it isn't possible to allocate a queue for readback,
		 * put the write ipiq on a list for later.
		 * If the controller is capable of doing readback by itself,
		 * set both IP_WCHK and IP_WCHK_READ to tell id_build_rdwr(),
		 * to add the verify parameter.
		 */
		q_related(q) = NULL;
		rbq = NULL;
		if ((bp->b_flags & B_READ) == 0 &&
				(un->un_wchkmap & (1 << ID_LPART(bp->b_dev)))) {
			if (c->c_flags & ID_CTLR_VERIFY) {
				q->q_flag |= IP_WCHK | IP_WCHK_READ;
			} else {
				if ((rbq = ipi_setup(un->un_ipi_addr,
					id_wchk_buf, c->c_rw_cmd_len,
					MAX_RESPLEN, idrestart, 0)) == NULL)
				{
					if (id_wchk_buf->b_flags & B_ERROR) {
id_printerr(q, "idstart: error setting up readback");
						id_wchk_buf->b_flags &=
							~B_ERROR;
						bp->b_flags |= B_ERROR;
						dp->b_actf = bp->av_forw;
						if (dp->b_actl == bp)
							dp->b_actl = NULL;
						iodone(bp);
						continue;
					}
					/*
					 * Put write request on queue until read
					 * request can be setup.
					 */
					q->q_next = NULL;
					un->un_wait_q = q;
					return (DVMA_RUNOUT); /* setup failed */
				}
				q_related(rbq) = q;
				q_related(q) = rbq;
				q->q_flag |= IP_WCHK;
			}
		}

		/*
		 * Take the buffer off the queue for this disk.
		 */
		dp->b_actf = bp->av_forw;
		if (dp->b_actl == bp)
			dp->b_actl = NULL;

		/*
		 * If the transfer size would take it past the end of the
		 * partition, trim it down.  Also trim it down to a multiple
		 * of the block size.
		 */
		blkno = dkblock(bp);
		lp = &un->un_lpart[ID_LPART(bp->b_dev)];
		nblks = NBLKS(bp->b_bcount, un);
		if (blkno + nblks > lp->un_map.dkl_nblk)
			nblks = lp->un_map.dkl_nblk - blkno;
		bp->b_resid = bp->b_bcount - (nblks << un->un_log_bshift);

		/*
		 * Map block number within partition to real block number.
		 */
		blkno += lp->un_blkno;
		q->q_errblk = (int)blkno;

		/*
		 * Build the IPI command packet.
		 */
		id_build_rdwr(q, un, blkno, nblks);	/* form command */

		/*
		 * Build readback command if needed.
		 */
		if (rbq != NULL)
			id_build_rdwr(rbq, un, blkno, nblks);

		/*
		 * Update counts of outstanding commands.
		 */
		un->un_cmd_q++;
		c->c_cmd_q++;
		c->c_cmd_q_len += c->c_rw_cmd_len;

		/*
		 * Send command.
		 */
		ipi_start(q, (long)0, idintr);

		/*
		 * Update iostat statistics.
		 */
		if ((dk = un->un_dk) >= 0) {
			dk_busy |= 1 << dk;
			dk_seek[dk]++;
			dk_xfer[dk]++;
			if (bp->b_flags & B_READ)
				dk_read[dk]++;
			dk_wds[dk] += bp->b_bcount >> 6;
		}
	}

	/*
	 * If the unit or controller has gone away, give errors for all
	 * queued bufs.
	 */
	if (!IE_STAT_PRESENT(c->c_flags) || !IE_STAT_PRESENT(un->un_flags)) {
		while ((bp = dp->b_actf) != NULL) {
			dp->b_actf = bp->av_forw;
			if (dp->b_actl == bp)
				dp->b_actl = NULL;
			bp->b_error = ENXIO;
			bp->b_flags |= B_ERROR;
			iodone(bp);
		}
	}
	return (0);
}

/*
 * Call idstart for any disks that have queued requests.
 * Used as a callback routine from ipi_setup.  Called when
 * resources become available.
 *
 * Returns non-zero if any commands could not be restarted.
 */

struct id_restart_trace {
        int     un;
        int     res;
        caddr_t arg;
        int     filler;
};
#define ID_RESTART_NTRACE 1024
struct id_restart_trace id_restart_trace[ID_RESTART_NTRACE];
struct id_restart_trace *id_restart_ptr = id_restart_trace;

static int
idrestart(arg)
        caddr_t arg;    /* controller pointer or NULL for all controllers */
{
	struct id_unit *un;
        struct id_ctlr *c = (struct id_ctlr *)arg;
	register int	s;
	register int	stat = 0;
	int		unit;
	static int	next_unit;

	s = spl_ipi();
	unit = next_unit;
	do {
		if (++unit >= nid)
			unit = 0;
                if ((un = id_unit[unit]) && (c == NULL || un->un_ctlr == c) &&
                                (un->un_bufs.b_actf || un->un_wait_q)) {
                        stat = idstart(un);
                        id_restart_ptr->un = un->un_unit;
                        id_restart_ptr->res = stat;
                        id_restart_ptr->arg = arg;
                        id_restart_ptr++;
                        if (id_restart_ptr >=
                                        &id_restart_trace[ID_RESTART_NTRACE])
                                id_restart_ptr = id_restart_trace;

                        if (stat != 0 && c == NULL)
                                break;
                }
	} while (unit != next_unit);
	next_unit = unit;
	(void) splx(s);
	return (stat);
}

/*
 * Build a read/write command for request.  Ipi_setup has already been called.
 */
static void
id_build_rdwr(q, un, blkno, nblks)
	struct ipiq	*q;		/* setup request */
	struct id_unit	*un;		/* unit structure */
	daddr_t		blkno;		/* absolute block number */
	int		nblks;		/* length in blocks */
{
	register struct ipi3header	*ip;
	union packet {
		u_char	*pp;		/* pointer to next packet byte */
		u_long	*lp;		/* pointer parm len, id, and pad) */
		struct cmdx_parm *cxp;	/* pointer to command extent parm */
	} pu;

	q->q_dev = (caddr_t)un;
	q->q_dma_len = nblks << un->un_log_bshift;

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = (struct ipi3header *)q->q_cmd;
	if (q->q_buf->b_flags & B_READ) {
		ip->hdr_opcode = IP_READ;
	} else {
		ip->hdr_opcode = IP_WRITE;
	}
	ip->hdr_mods = IP_OM_BLOCK;

	/*
	 * fill in command extent (block count and block address).
	 */
	pu.pp = (u_char *)ip + sizeof (struct ipi3header);
	*pu.lp++ = (sizeof (struct cmdx_parm)+1) << 8 | CMD_EXTENT;
	pu.cxp->cx_count = nblks;
	pu.cxp->cx_addr = blkno;
	pu.pp += sizeof (struct cmdx_parm);

	/*
	 * Add Transfer parameter if a verify is requested.
	 * Form the three bytes (plus pad byte) in one word.
	 * The combination of IP_WCHK and IP_WCHK_READ flags is used to
	 * indicate that this is desired, and these flags are cleared
	 * after the parameter is added.
	 */
	if ((q->q_flag & (IP_WCHK | IP_WCHK_READ)) == (IP_WCHK | IP_WCHK_READ))
	{
		*pu.lp++ = 2 << 16 | XFER_PARM << 8 | XFER_VERIFY;
		q->q_flag &= ~(IP_WCHK | IP_WCHK_READ);
	}

	/*
	 * Fill in the transfer notification parameter if needed.
	 * Note that this may leave pu.pp non-word-aligned.
	 */
	if (q->q_tnp_len == sizeof (caddr_t)) {
		*pu.lp++ = (sizeof (caddr_t)+1) << 8 | XFER_NOTIFY;
		*pu.lp++ = * (u_long *) q->q_tnp;
	} else if (q->q_tnp_len > 0) {
		*pu.pp++ = q->q_tnp_len + 1;	/* parameter length */
		*pu.pp++ = XFER_NOTIFY;		/* parameter ID */
		bcopy(q->q_tnp, (caddr_t)pu.pp, q->q_tnp_len);
		pu.pp += q->q_tnp_len;
	}

	/*
	 * Form packet length based on where the pointer ended up.
	 */
	ip->hdr_pktlen = (pu.pp - (u_char *)ip) - sizeof (ip->hdr_pktlen);
}

/*
 * Main Interrupt handler - used only for commands issued by idstart().
 */
static void
idintr(q)
	register struct ipiq *q;
{
	register struct id_unit *un;
	register struct id_ctlr *c;
	register struct buf *bp;
	int	started = 0;
	int	done = 0;
	int	flags;
	int	dk;
	int	s;

	if (q) {
		bp = q->q_buf;
		c = (struct id_ctlr *)q->q_ctlr;
		if ((un = (struct id_unit *)q->q_dev) == NULL) {
			printf("idintr: bad rupt. NULL q_dev q %x.\n", q);
			id_printerr(q, "idintr: bad rupt NULL q_dev\n");
			return;
		}

		/*
		 * Check for missing interrupt indication before decrementing
		 * queue counts, since this isn't a completion.
		 */
		if (q->q_result == IP_MISSING) {
			id_missing_req(q);
			return;
		}

		if (--un->un_cmd_q == 0 && (un->un_flags & ID_WANTED))
			wakeup((caddr_t)un);
		c->c_cmd_q--;
		c->c_cmd_q_len -= c->c_rw_cmd_len;
		if (c->c_cmd_q_len < 0) {
			id_printerr(q, 	"idintr: neg c_cmd_q_len");
			c->c_cmd_q_len = 0;
		}

		switch (q->q_result) {
		case IP_SUCCESS:
			/*
			 * If successful, start any waiting commands right away.
			 */
			if ((q->q_flag & IP_WCHK) == 0) {
				s = spl_ipi();
				(void) idstart(un);
				(void) splx(s);
				started = 1;
			}
			done = 1;
			break;

		case IP_ERROR:
		case IP_COMPLETE:
			flags = id_error_parse(q, ipi_err_table,
				id_error, c, un, 0);
			done = 1;
			if ((flags & IE_RETRY)) {
				id_q_retry(q);
				done = 0;
			} else if ((flags & IE_CMD) == 0) {	/* non fatal */
				q->q_result = IP_SUCCESS;
			} else if (bp) {
				bp->b_flags |= B_ERROR;
			}
			break;

		case IP_RETRY:
			id_q_retry(q);
			break;

		default:
			if (bp)
				bp->b_flags |= B_ERROR;
			done = 1;
			break;
		}

		if (done) {
			/*
			 * If a read back must be done on this buffer,
			 * start it now, and delay the completion of the
			 * write request until the readback completes.
			 */
			if ((q->q_flag & IP_WCHK) &&
					q->q_result == IP_SUCCESS) {
				id_readback(q);
			} else if (q->q_flag & IP_WCHK_READ) {
				struct ipiq	*rbq = q;

				/*
				 * This is the completion of a read-back.
				 */
				q = q_related(q);
				bp = q->q_buf;
				if (rbq->q_result == IP_SUCCESS) {
					ipi_free(rbq);
					ipi_free(q);
					if (bp)
						iodone(bp);
				} else if (rbq->q_result == IP_RETRY_FAILED) {
					ipi_free(rbq);
					ipi_free(q);
					if (bp) {
						bp->b_flags |= B_ERROR;
						iodone(bp);
					}
				} else {
					id_printerr(q,
					    "readback failed.  retrying write");
					id_q_retry(q);
				}
			} else {
				/*
				 * Normal (non-readback) completion.
				 *
				 * Since ipi_free may do cache flushes to make
				 * CPU and I/O caches consistent, must do it
				 * before calling iodone().
				 */
				ipi_free(q);
				if (bp)
					iodone(bp);
			}
		}

                if (c->c_flags & ID_CTLR_WAIT) {
                        c->c_flags &= ~ID_CTLR_WAIT;
                        (void)idrestart((caddr_t)c);
                } else if (!started) {
                        s = spl_ipi();
                        (void) idstart(un);
                        (void) splx(s);
                }
 
		un->un_idle_time = 0;

		/*
		 * If nothing queued or running on unit, mark it not busy
		 * for iostats.
		 */
		if (un->un_cmd_q == 0 && (dk = un->un_dk) >= 0 &&
				un->un_bufs.b_actf == NULL)
			dk_busy &= ~(1 << dk);
	}
}


/*
 * Send IPI command.
 *
 * Mode selects whether driver should wait by polling or sleeping or not
 * wait at all.  Returns non-zero on error or poll timeout.
 */
static int
id_send_cmd(q, flags, mode)
	struct ipiq	*q;
	long		flags;
	int		mode;
{
	struct id_unit	*un;
	struct id_ctlr	*c;

	if ((un = (struct id_unit *)q->q_dev) != NULL) {
		if (!IE_STAT_READY(un->un_flags)) {
			return (ENXIO);
		}
		un->un_cmd_q++;
	}
	if ((c = (struct id_ctlr *)q->q_ctlr) != NULL) {
		if (!IE_STAT_READY(c->c_flags)) {
			if (un)
				un->un_cmd_q--;
			return (ENXIO);
		}
		c->c_cmd_q++;
		c->c_cmd_q_len += q->q_cmd->hdr_pktlen + sizeof (u_short);
	}

	switch (mode) {
	case ID_SYNCH:
		ipi_start(q, flags | IP_SYNC, id_cmd_intr);
		if (ipi_poll(q, (long)ID_POLL_TIMEOUT) == 0) {
			id_printerr(q, "poll timeout");
			return (EIO);
		}
		break;

	case ID_ASYNCHWAIT:
		ipi_start(q, flags | IP_WAKEUP, id_cmd_intr);
		while (q->q_result == IP_INPROGRESS || q->q_result == IP_RETRY)
			(void) sleep((caddr_t)q, PRIBIO);
		break;

	case ID_ASYNCH:
		ipi_start(q, flags, id_cmd_intr);
		break;
	}
	return (0);
}

/*
 * Auxilliary Interrupt handler - used for commands not issued via idstart().
 *
 * NOTE:  This interface doesn't return sys/errno.h type error codes in the
 * buffer.  It uses the codes defined in idvar.h instead.  Since this
 * is used only by id_ioctl_cmd and related routines, this is OK.
 */
static void
id_cmd_intr(q)
	register struct ipiq *q;
{
	register struct id_unit *un;
	register struct id_ctlr *c;
	register struct buf *bp;
	int	err;
	int	flags;
	int	len;

	if (q) {
		/*
		 * Check for missing interrupt indication before decrementing
		 * queue counts, since this isn't a completion.
		 */
		if (q->q_result == IP_MISSING) {
			id_missing_req(q);
			return;
		}

		err = IDE_NOERROR;
		bp = q->q_buf;
		c = (struct id_ctlr *)q->q_ctlr;
		if ((un = (struct id_unit *)q->q_dev) != NULL &&
				--un->un_cmd_q == 0 &&
				(un->un_flags & ID_WANTED))
			wakeup((caddr_t)un);
		c->c_cmd_q--;
		if ((len = q->q_cmd->hdr_pktlen) > 0)
			c->c_cmd_q_len -= len + sizeof (u_short);
		if (c->c_cmd_q_len < 0) {
			id_printerr(q, "id_cmd_intr: ctlr cmd_q_len negative");
			c->c_cmd_q_len = 0;
		}

		switch (q->q_result) {
		case IP_SUCCESS:
			if (q->q_retry)
				err = IDE_RETRIED;	/* not an error */
/* XXX - need to know more about the error that caused retry. */
			break;

		case IP_ERROR:
		case IP_COMPLETE:
			flags = id_error_parse(q, ipi_err_table, id_error,
				c, un, 0);
			if ((flags & IE_CMD) == 0 &&
				(q->q_flag & IP_DIAGNOSE) == 0)
			{
				q->q_result = IP_SUCCESS;
			}
			if (flags & IE_CORR) {
				err = IDE_CORR;
			} else if (flags & IE_UNCORR) {
				err = IDE_UNCORR;
			} else {
				err = IDE_FATAL;
			}
			break;

		case IP_RETRY:
			id_q_retry(q);
			return;		/* don't do wakeup */

		default:
			err = IDE_FATAL;
			break;
		}
		if (err && bp) {
			bp->b_flags |= B_ERROR;
			bp->b_error = err;
		}
		if (c->c_flags & ID_CTLR_WAIT) {
                        c->c_flags &= ~ID_CTLR_WAIT;
                        (void)idrestart((caddr_t)c);
                }

		if (q->q_flag & IP_WAKEUP)
			wakeup((caddr_t)q);
		if (un) {
			un->un_idle_time = 0;
		}
	}
}

/*
 * Asynchronous interrupt handler
 */
static void
idasync(q)
	register struct ipiq *q;	/* interrupt info */
{
	register int	fac;
	register struct id_ctlr *c;	/* device structure */
	struct id_unit	*un;
	struct id_unit	new_unit;	/* temporary unit structure */

	c = (struct id_ctlr *)q->q_ctlr;
	if ((fac = q->q_resp->hdr_facility) != IPI_NO_ADDR && fac < IDC_NUNIT) {
		un = c->c_unit[fac];
		/*
		 * If the unit structure wasn't allocated, this might be
		 * a new device coming on-line.  Use a temporary unit
		 * structure to make the messages come out right.  The real
		 * unit structure will be allocated when the disk is opened.
		 */
		if (un == NULL) {
			bzero((caddr_t)&new_unit, sizeof (new_unit));
			un = &new_unit;
			un->un_ipi_addr = q->q_csf;
			un->un_unit = IPI_CHAN(q->q_csf) << 8 |
				IPI_SLAVE(q->q_csf) << 4 | fac;
			un->un_ctlr = c;
			un->un_dk = -1;
			c->c_unit[fac] = un;
		}
		q->q_dev = (caddr_t)un;
		(void) id_error_parse(q, ipi_err_table, id_error, c, un, 0);
		if (un == &new_unit) {
			q->q_dev = NULL;
			c->c_unit[fac] = NULL;
		}
	} else
		(void) id_error_parse(q, ipi_err_table, id_error, c,
			(struct id_unit *)NULL, 0);
}

/*
 * Error handler for all kinds of interrupts.  Called through id_error_parse.
 * 	Handles asynchronous interrupts also.
 */
static int
id_error(q, c, un, code, flags, msg, parm)
	struct ipiq	*q;		/* request containing response */
	struct id_ctlr	*c;		/* controller */
	struct id_unit	*un;		/* unit - may be NULL */
	int		code;		/* error result/action code */
	int		flags;		/* error flags */
	char		*msg;		/* message */
	u_char 		*parm;		/* raw parameter (incl ID/len) */
{
	int	new_flags = 0;
	u_int	old_stat;
	u_int	*fp;			/* flags pointer */
	u_int	change_mask;		/* online/offline status change */

	if ((flags & (IE_MSG | IE_PRINT)) && !(q->q_flag & IP_SILENT))
		printf("%s. ", msg);
	if ((flags & IE_FAC) && un == NULL) {
		printf("idc%d: facility status for unknown unit\n", c->c_ctlr);
		new_flags |= IE_PRINT;
	}
	switch (code) {
	case 0:			/* nothing */
		break;
	case IE_RESP_EXTENT:	/* response extent parameter */
		(void) id_respx(q, 0, (struct respx_parm *)(parm + 2), 0,
			q->q_buf);
		break;
	case IE_MESSAGE:	/* for microcode messages */
		{
			struct msg_ucode_parm	*p;
			char	m, *cp;
			int	i;

			p = (struct msg_ucode_parm *)&parm[2];
			if (p->mu_flags & IPMU_FAIL_MSG)
				printf("idc%d: ctlr failure fru %x message: '",
					p->mu_u.mu_m.mu_fru, c->c_ctlr);
			else
				printf("idc%d: ctlr message: '", c->c_ctlr);
			cp = p->mu_u.mu_m.mu_msg;	/* first char of msg */
			i = parm[0] + 1 - (cp - (char *)parm);	/* msg len */
			while (i-- > 0) {
				m = *cp++;
				printf("%c", m == '\n' ? ' ' : m);
			}
			printf("'\n");
		}
		break;
	case IE_STAT_OFF:	/* unit or controller went offline */
	case IE_STAT_ON:	/* unit or controller went online */
		change_mask = (flags & IE_STAT_MASK);
		if (flags & IE_FAC) {
			if (un)
				fp = &un->un_flags;
			else
				break;
		} else {
			fp = &c->c_flags;
		}
		old_stat = *fp;
		if (code == IE_STAT_OFF)
			*fp &= ~change_mask;
		else
			*fp |= change_mask;
		/*
		 * Print a message if the status has changed and a message
		 * hasn't been already printed due to the flags settings.
		 * However, don't print this message if the unit/ctlr flags
		 * or the passed-in flags indicate that this is the
		 * initial status for the unit/ctlr.
		 */
		if (!((flags | old_stat) & IE_INIT_STAT) &&
			old_stat != *fp && !(flags & (IE_MSG | IE_PRINT)))
		{
			id_printerr(q, msg);
		}
		break;
	case IE_IML:		/* reload slave */
id_printerr(q, "id_error: slave IML not supported");	/* XXX */
		new_flags |= IE_RETRY;			/* retry request */
		break;
	case IE_RESET:		/* reset slave */
id_printerr(q, "id_error: slave reset not supported");	/* XXX */
		new_flags |= IE_RETRY;			/* retry request */
		break;
	case IE_QUEUE_FULL:	/* buffer limits exceeded */
id_printerr(q, "id_error: full queue: not handled");	/* XXX */
		new_flags |= IE_RETRY;			/* retry request */
		break;
	case IE_READ_LOG:	/* log full */
id_printerr(q, "id_error: log read not supported");	/* XXX */
		new_flags |= IE_RETRY;			/* retry request */
		break;
	case IE_ABORTED:	/* command aborted */
id_printerr(q, "id_error: aborted command not supported");	/* XXX */
		new_flags |= IE_DUMP | IE_CMD;		/* dump cmd/resp */
		break;
	default:
		break;
	}
	return (new_flags);
}


int
idsize(dev)
	dev_t	dev;
{
	int	unit;
	register struct id_unit *un;

	/*
	 * Call open in case the device wasn't configured yet.
	 */
	if (idbopen(dev, 0))
		return (-1);
	if ((unit = ID_BUNIT(dev)) >= nid || (un = id_unit[unit]) == NULL ||
			!(un->un_flags & ID_ATTACHED))
		return (-1);
	return ((int)un->un_lpart[ID_LPART(dev)].un_map.dkl_nblk);
}

/*
 * Perform logical disk section mapping.
 * Map logical block number into physical block number.
 */
static daddr_t
id_map(minor_dev, blkno, nblk)
	u_int	minor_dev;
	daddr_t	blkno, nblk;
{
	register struct id_unit *un = id_unit[ID_UNIT(minor_dev)];
	register struct un_lpart *lp;


	if (un == NULL || !(un->un_flags & ID_UN_PRESENT))
		return (-1);
	lp = &un->un_lpart[ID_LPART(minor_dev)];
	if (blkno >= lp->un_map.dkl_nblk || (blkno+nblk) > lp->un_map.dkl_nblk)
		return (-1);
	/*
	 * Offset into the correct partition.
	 */
	return (blkno + lp->un_blkno);
}

/*
 * This routine dumps memory to the disk.  It assumes that the memory has
 * already been mapped into mainbus space.  It is called at disk interrupt
 * priority when the system is in trouble.
 */
iddump(dev, addr, blkno, nblk)
	dev_t	dev;
	caddr_t	addr;
	daddr_t	blkno, nblk;
{
	int	unit;
	register struct id_unit *un;
	struct ipiq	*q;
	int		errno = 0;
	daddr_t		rblkno;		/* real block number */
	struct buf	dbuf;

	/*
	 * Check to make sure the operation makes sense.
	 */
	if ((unit = ID_BUNIT(dev)) >= nid || (un = id_unit[unit]) == NULL ||
			!(un->un_flags & ID_UN_PRESENT))
		return (ENXIO);

	bzero((caddr_t)&dbuf, sizeof (dbuf));
	dbuf.b_un.b_addr = (caddr_t)addr;
	dbuf.b_flags = B_BUSY;
	dbuf.b_bcount = nblk * DEV_BSIZE;

	if (un->un_log_bsize != DEV_BSIZE) {
		blkno = (blkno * DEV_BSIZE) >> un->un_log_bshift;
		nblk = (nblk * DEV_BSIZE) >> un->un_log_bshift;
	}
	if ((rblkno = id_map(ID_BMINOR(dev), blkno, nblk)) < 0)
		return (EINVAL);
	rblkno += un->un_first_block;	/* usually zero */

	if ((q = ipi_setup(un->un_ipi_addr, &dbuf, un->un_ctlr->c_rw_cmd_len,
			MAX_RESPLEN, IPI_NULL, IP_NO_DMA_SETUP)) == NULL)
		return (EIO);
	/*
	 * Construct IPI-3 packet.
	 */
	id_build_rdwr(q, un, rblkno, (int)nblk);

	/*
	 * Send command.
	 */
	if (id_send_cmd(q, (long)0, ID_SYNCH) || q->q_result != IP_SUCCESS)
		errno = EIO;
	ipi_free(q);
	return (errno);
}

static void
id_minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > ID_MAXPHYS)
		bp->b_bcount = ID_MAXPHYS;
}

#if !MULT_RBUF
static caddr_t physio_bufs;
#endif

/*
 * Do a raw read using the unit's raw I/O buffer.
 * Called from cdevsw() at normal priority.
 */
int
idread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register int unit = ID_CUNIT(dev);
#if !MULT_RBUF
	struct buf	*bp;
	int	err;
#endif

	if (unit >= nid)
		return (ENXIO);
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    uio->uio_offset % DEV_BSIZE != 0)
		return (EINVAL);
#if MULT_RBUF
	return (physio(idstrategy, NULL, ID_BLKDEV(dev), B_READ,
		id_minphys, uio));
#else
	bp = (struct buf *)
		kmem_fast_zalloc(&physio_bufs, sizeof (struct buf), 1);
	bp->b_flags = 0;
	err = physio(idstrategy, bp, ID_BLKDEV(dev), B_READ, id_minphys, uio);
	kmem_fast_free(&physio_bufs, (caddr_t)bp);
	return (err);
#endif
}

/*
 * Do a raw write using the special per unit buffer.
 * Called from cdevsw() at normal priority.
 */
int
idwrite(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register int unit = ID_CUNIT(dev);
#if !MULT_RBUF
	struct buf	*bp;
	int	err;
#endif

	if (unit >= nid)
		return (ENXIO);
	if ((uio->uio_fmode & FSETBLK) == 0 &&
	    uio->uio_offset % DEV_BSIZE != 0)
		return (EINVAL);
#if MULT_RBUF
	return (physio(idstrategy, NULL, ID_BLKDEV(dev), B_WRITE,
		id_minphys, uio));
#else
	bp = (struct buf *)
		kmem_fast_zalloc(&physio_bufs, sizeof (struct buf), 1);
	bp->b_flags = 0;
	err = physio(idstrategy, bp, ID_BLKDEV(dev), B_WRITE, id_minphys, uio);
	kmem_fast_free(&physio_bufs, (caddr_t)bp);
	return (err);
#endif
}

/*
 * This routine implements the ioctl calls for the disk.
 * It is called from the device switch at normal priority.
 */
/* ARGSUSED */
idioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd, flag;
	caddr_t	data;
{
	register struct id_unit *un = id_unit[ID_CUNIT(dev)];
	struct id_ctlr		*c;
	register union id_ptr {		/* union for usages of third arg */
		struct dk_info	*inf;
		struct dk_conf	*conf;
		struct dk_type	*typ;
		struct dk_cmd	*com;
		struct dk_diag	*diag;
		struct dk_geom	*geom;
		struct dk_map	*map;
		int		*wchk;
		caddr_t		data;
	} p;
	int	i, s, err;

	p.data = data;			/* set union to fourth argument */
	err = 0;			/* no error */

	if (un == NULL)
		return (ENXIO);
	c = un->un_ctlr;

	switch (cmd) {
	/*
	 * Return info concerning the controller.
	 */
	case DKIOCINFO:
		p.inf->dki_ctlr = i = c->c_ipi_addr;
		p.inf->dki_unit = IPI_SLAVE(i);
		p.inf->dki_ctype = c->c_ctype;
#define	ID_DKI_FLAGS (DKI_FMTCYL | DKI_HEXUNIT)
		p.inf->dki_flags = ID_DKI_FLAGS;
		break;
	/*
	 * Return configuration info
	 */
	case DKIOCGCONF:
		(void) strncpy(p.conf->dkc_cname, idcdriver.mdr_cname,
		    DK_DEVLEN);
		p.conf->dkc_ctype = c->c_ctype;
		p.conf->dkc_flags = ID_DKI_FLAGS;
		p.conf->dkc_cnum = c->c_ctlr;
		p.conf->dkc_addr = i = un->un_ipi_addr;
		p.conf->dkc_slave = IPI_FAC(i);
		p.conf->dkc_unit = ID_CUNIT(dev);
		p.conf->dkc_space = idcinfo[c->c_ctlr]->mc_space;
		p.conf->dkc_prio = c->c_intpri;
		p.conf->dkc_vec = 0;
		(void) strncpy(p.conf->dkc_dname, idcdriver.mdr_dname,
			DK_DEVLEN);
		break;
	/*
	 * Return drive info
	 */
	case DKIOCGTYPE:
		p.typ->dkt_drtype = 0;		/* drive type not used */
		p.typ->dkt_hsect = un->un_g.dkg_nsect;
		p.typ->dkt_promrev = 0;
		p.typ->dkt_drstat = 0;
		break;
	/*
	 * Set drive info -- only affects drive type.
	 * 	This doesn't make sense for IPI disks.  The disk
	 *	identifies itself.  Ignore without returning error.
	 */
	case DKIOCSTYPE:
		break;
	/*
	 * Return the geometry of the specified unit.
	 */
	case DKIOCGGEOM:
		*p.geom = un->un_g;
		break;
	/*
	 * Set the geometry of the specified unit.
	 * Currently only the number of cylinders is expected to change from
	 * what was given in the label or attributes.
	 */
	case DKIOCSGEOM:
		s = spl_ipi();
		un->un_g = *p.geom;	/* set geometry */
		id_update_map(un);	/* fix partition table - just in case */
		(void) splx(s);
		break;
	/*
	 * Return the map for the specified logical partition.
	 */
	case DKIOCGPART:
		*p.map = un->un_lpart[ID_LPART(dev)].un_map;
		break;
	/*
	 * Set the map for the specified logical partition.
	 * We raise the priority just to make sure an interrupt
	 * doesn't come in while the map is half updated.
	 */
	case DKIOCSPART:
		s = spl_ipi();
		un->un_lpart[ID_LPART(dev)].un_map = *p.map;
		id_update_map(un);
		(void) splx(s);
		break;
	/*
	 * Return the map for all logical partitions.
	 */
	case DKIOCGAPART:
		for (i = 0; i < ID_NLPART; i++)
			p.map[i] = un->un_lpart[i].un_map;
		break;
	/*
	 * Set the map for all logical partitions.  We raise
	 * the priority just to make sure an interrupt doesn't
	 * come in while the map is half updated.
	 */
	case DKIOCSAPART:
		s = spl_ipi();
		for (i = 0; i < ID_NLPART; i++)
			un->un_lpart[i].un_map = p.map[i];
		id_update_map(un);
		(void) splx(s);
		break;
	/*
	 * Generic IPI command
	 */
	case DKIOCSCMD:
		err = id_ioctl_cmd(un, p.com);
		break;
	/*
	 * Get diagnostics
	 */
	case DKIOCGDIAG:
		*p.diag = un->un_diag;
		break;

	/*
	 * Handle Write Check: turn on or off verification (per partition).
	 */
	case DKIOCWCHK:
		if (!suser())
			return (u.u_error);
		if (un->un_lpart[ID_LPART(dev)].un_map.dkl_nblk == 0)
			return (ENXIO);
		i = *p.wchk;
		s = spl_ipi();
		*p.wchk = ((un->un_wchkmap & (1<<ID_LPART(dev))) != 0);
		if (i) {
			un->un_wchkmap |= (1 << ID_LPART(dev));
			(void) splx(s);
			id_alloc_wchk();
			log(LOG_INFO, "id%3x%c: write check enabled\n",
				un->un_unit, 'a' + ID_LPART(dev));
		} else {
			un->un_wchkmap &= ~(1 << ID_LPART(dev));
			(void) splx(s);
			log(LOG_INFO, "id%3x%c: write check disabled\n",
				un->un_unit, 'a' + ID_LPART(dev));
		}
		break;

	default:
		err = ENOTTY;
		break;
	}
	return (err);
}


/*
 * IPI commands via ioctl interface.
 */
static int
id_ioctl_cmd(un, com)
	struct id_unit	*un;
	struct dk_cmd 	*com;
{
	int		s;
	int		flags = 0;
	long		ipi_flags;
	u_char		opcode;
	int		opmod;
	int		err = 0;
	struct	buf	*bp = NULL;
	faultcode_t	fault_err;

if (id_debug)
printf("id_ioctl_cmd: cmd %x flags %x blkno %x secnt %x bufaddr %x buflen %x\n",
	com->dkc_cmd, com->dkc_flags, com->dkc_blkno, com->dkc_secnt,
	com->dkc_bufaddr, com->dkc_buflen);	/* XXX - debug */

	opcode = com->dkc_cmd;		/* IPI-3 opcode */
	opmod = com->dkc_cmd >> 8;	/* opcode modifiers */
	/*
	 * Check the parameters.
	 */
	switch (opcode) {
	case IP_READ:
		flags |= B_READ;
	case IP_WRITE:
		if (com->dkc_buflen != (com->dkc_secnt << un->un_log_bshift))
			return (EINVAL);
		break;

	case IP_READ_DEFLIST:
		flags |= B_READ;
		/* fallthrough */
	case IP_WRITE_DEFLIST:
		if (com->dkc_buflen == 0)
			return (EINVAL);
		break;

	case IP_FORMAT:
	case IP_REALLOC:
		if (com->dkc_buflen != 0)
			return (EINVAL);
		break;
	default:
		return (EINVAL);
	}

	/*
	 * Don't allow more than max at once.
	 */
	if (com->dkc_buflen > ID_MAXBUFSIZE)
		return (EINVAL);

	s = spl_ipi();
	/*
	 * If this command wants exclusive use of the drive, or if there
	 * is another exclusive user, wait for it.
	 */
	while (un->un_flags & ID_LOCKED) {
		un->un_flags |= ID_WANTED;
		(void) sleep((caddr_t)un, PZERO+1); /* interruptible sleep */
		un->un_flags &= ~ID_WANTED;
	}
	if (com->dkc_flags & DK_ISOLATE) {
		un->un_flags |= ID_LOCKED;	/* prevent further commands. */
		while (un->un_cmd_q != 0) {
			un->un_flags |= ID_WANTED;
			(void) sleep((caddr_t)un, PRIBIO);
			un->un_flags &= ~ID_WANTED;
		}
	}

	/*
	 * If this command moves data, we need to map
	 * the user buffer into DVMA or IPI space.
	 */
	if (com->dkc_buflen > 0) {
		/*
		 * Get a raw I/O buf.
		 */
		bp = alloc(struct buf, 1);
		if (bp == NULL) {
			err = ENOMEM;
			goto errout;
		}
		bp->b_flags = B_BUSY | B_PHYS | flags;
		bp->b_un.b_addr = com->dkc_bufaddr;
		bp->b_bcount = com->dkc_buflen;
		bp->b_resid = 0;
		bp->b_blkno = com->dkc_blkno;
		bp->b_proc = u.u_procp;
		u.u_procp->p_flag |= SPHYSIO;
		/*
		 * Fault lock the address range of the buffer.
		 */
		fault_err = as_fault(u.u_procp->p_as,
			bp->b_un.b_addr, (u_int)bp->b_bcount,
			F_SOFTLOCK, (flags & B_READ) ? S_READ : S_WRITE);
		if (fault_err) {
			if (FC_CODE(fault_err) == FC_OBJERR)
				err = FC_ERRNO(fault_err);
			else
				err = EFAULT;
		} else if (buscheck(bp) < 0) {
			err = EFAULT;
		}
	}

	/*
	 * Set IPI layer flags.
	 */
	ipi_flags = 0;
	if (com->dkc_flags & DK_DIAGNOSE)
		ipi_flags |= IP_DIAGNOSE;
	if (com->dkc_flags & DK_SILENT)
		ipi_flags |= IP_SILENT;

	/*
	 * Execute the command.
	 */
	if (err == 0) {
		switch (opcode) {
		case IP_FORMAT:
			err = id_format(un, com->dkc_blkno, com->dkc_secnt,
				opmod);
			break;

		case IP_READ:
		case IP_WRITE:
			err = id_rdwr(un, bp, com->dkc_blkno, ipi_flags);
			break;

		case IP_READ_DEFLIST:
		case IP_WRITE_DEFLIST:
			err = id_rdwr_deflist(un, bp, opmod);
			break;

		case IP_REALLOC:
			err = id_realloc(un, com->dkc_blkno, com->dkc_secnt,
				opmod);
			break;

		case IP_ATTRIBUTES:
		default:
			err = EINVAL;
			break;
		}
	}

	/*
	 * Release memory and DVMA resources.
	 */
	if (com->dkc_buflen > 0 && fault_err == 0) {
		(void) as_fault(u.u_procp->p_as, bp->b_un.b_addr,
		(u_int)bp->b_bcount, F_SOFTUNLOCK,
		(flags & B_READ) ? S_READ : S_WRITE);
	}
errout:
	if (err && (com->dkc_flags & DK_DIAGNOSE)) {
		un->un_diag.dkd_errcmd = com->dkc_cmd;
		if (bp && (bp->b_flags & B_ERROR)) {
			un->un_diag.dkd_errsect = bp->b_blkno;
			un->un_diag.dkd_severe = IDE_SEV(bp->b_error);
			un->un_diag.dkd_errno = bp->b_error;
		} else {
			un->un_diag.dkd_errsect = com->dkc_blkno;
			un->un_diag.dkd_severe = DK_FATAL;
			un->un_diag.dkd_errno = IDE_FATAL;
		}

	}
if (id_debug && err) { /* XXX */
	printf("id_ioctl_cmd: id%3x err %d (0x%x)\n", un->un_unit, err, err);
}
	if (bp)
		kmem_free((caddr_t)bp, sizeof (struct buf));
	/*
	 * If we locked it, unlock it.
	 */
	if (com->dkc_flags & DK_ISOLATE) {
		un->un_flags &= ~ID_LOCKED;	/* prevent further commands. */
		if (un->un_flags & ID_WANTED)
			wakeup((caddr_t)un);
	}
	(void) splx(s);
	return (err);
}

/*
 * This routine verifies that the block read is indeed a disk label.
 */
static
islabel(unit, l)
	u_int	unit;
	register struct dk_label *l;
{
	if (l->dkl_magic != DKL_MAGIC)
		return (0);
	if (!ck_cksum(l)) {
		printf("id%3x: corrupt label\n", unit);
		return (0);
	}
	return (1);
}

/*
 * This routine checks the checksum of the disk label.
 * It is used by islabel().
 */
static
ck_cksum(l)
	register struct dk_label *l;
{
	register short *sp, sum = 0;
	register short count = sizeof (struct dk_label)/sizeof (short);

	sp = (short *)l;
	while (count--)
		sum ^= *sp++;
	return (sum ? 0 : 1);
}

/*
 * This routine puts the label information into the various parts of
 * the unit structure.  It is always called at disk interrupt priority.
 */
static
uselabel(un, l)
	register struct id_unit *un;
	register struct dk_label *l;
{
	register struct dk_geom	*g = &un->un_g;
	int	i, intrlv;

	/*
	 * Print out the disk description.
	 */
	if (l->dkl_magic == DKL_MAGIC)
		printf("id%3x: <%s>\n", un->un_unit, l->dkl_asciilabel);
	/*
	 * Check the label geometry against the geometry values read
	 * from the physical disk configuration attribute parameter.
	 * If they don't match, take the ones from the label, since they
	 * affect conversion of the partition starting cylinder to block number.
	 */
	g->dkg_acyl = g->dkg_pcyl - g->dkg_ncyl;	/* for checking */
	if (l->dkl_ncyl > g->dkg_ncyl || l->dkl_pcyl > g->dkg_pcyl ||
			l->dkl_acyl < g->dkg_acyl ||
			g->dkg_nhead != l->dkl_nhead ||
			g->dkg_nsect != l->dkl_nsect ||
			g->dkg_rpm != l->dkl_rpm) {
		printf(
"id%3x: warning: label doesn't match IPI attribute info\n",
			un->un_unit);
		printf(
"id%3x: label ncyl %d pcyl %d acyl %d head %d sect %d rpm %d\n",
			un->un_unit, l->dkl_ncyl, l->dkl_pcyl, l->dkl_acyl,
			l->dkl_nhead, l->dkl_nsect, l->dkl_rpm);
		printf(
"id%3x: geom  ncyl %d pcyl %d acyl %d head %d sect %d rpm %d\n",
			un->un_unit, g->dkg_ncyl, g->dkg_pcyl, g->dkg_acyl,
			g->dkg_nhead, g->dkg_nsect, g->dkg_rpm);
	}
	g->dkg_intrlv = l->dkl_intrlv;
	g->dkg_nhead = l->dkl_nhead;
	g->dkg_nsect = l->dkl_nsect;
	g->dkg_rpm = l->dkl_rpm;
	g->dkg_ncyl = l->dkl_ncyl;
	g->dkg_pcyl = l->dkl_pcyl;
	g->dkg_acyl = l->dkl_acyl;

	/*
	 * Fill in the logical partition map.
	 */
	for (i = 0; i < NDKMAP; i++)
		un->un_lpart[i].un_map = l->dkl_map[i];
	id_update_map(un);	/* update map with geometry info */

	/*
	 * Set up data for iostat.
	 */
	if (un->un_dk >= 0) {
		intrlv = g->dkg_intrlv;
		if (intrlv <= 0 || intrlv >= g->dkg_nsect)
			intrlv = 1;
		dk_bps[un->un_dk] = (60 << un->un_log_bshift)
			* g->dkg_nsect / intrlv;
	}
	un->un_flags |= ID_LABEL_VALID;
}

/*
 * Read or write disk data.
 *
 * The block number is absolute:  no patrition mapping is needed.
 *
 * Returns non-zero error number on failure.
 * Calls iodone on buffer when fininshed.
 */
/* ARGUSED */
static
id_rdwr(un, bp, block, ipi_flags)
	register struct id_unit	*un;	/* unit structure pointer */
	register struct buf	*bp;	/* buffer info */
	daddr_t	block;			/* starting physical block number */
	long	ipi_flags;
{
	struct ipiq		*q;
	int			errno = 0;	/* error return code */
	int			nblks;

	if ((q = ipi_setup(un->un_ipi_addr, bp, un->un_ctlr->c_rw_cmd_len,
			MAX_RESPLEN, IPI_SLEEP, (int)ipi_flags)) == NULL)
		return (EIO);
	nblks = NBLKS(bp->b_bcount, un);
	q->q_errblk = (long)block;

	/*
	 * Construct IPI-3 packet.
	 */
	id_build_rdwr(q, un, block, nblks);

	/*
	 * Send command.
	 */
	if ((errno = id_send_cmd(q, (long)IP_ABS_BLOCK, ID_ASYNCHWAIT)) == 0 &&
			q->q_result != IP_SUCCESS)
	{
		bp->b_blkno = (daddr_t)q->q_errblk;
		errno = EIO;
	}
	ipi_free(q);
	return (errno);
}

/*
 * Format disk.
 * If first_block is less than zero, format entire disk.
 */
static
id_format(un, first_block, nblocks, opmod)
	register struct id_unit	*un;	/* unit structure pointer */
	daddr_t		first_block;	/* first block number */
	int		nblocks;	/* block count, all if < 0 */
	int		opmod;		/* opcode modifier */
{
	struct ipiq		*q;
	int			errno = 0;	/* error return code */
	int			blk_per_cyl;
	struct ipi3header	*ip;
	int			i;
	char			*cp;

	/*
	 * If a block number range is specified, make sure it is
	 * on a cylinder boundary.
	 */
	if (nblocks > 0 || first_block != 0) {
		blk_per_cyl = un->un_g.dkg_nsect;
		if ((first_block % blk_per_cyl) || (nblocks % blk_per_cyl))
			return (EINVAL);
	}

	if ((q = ipi_setup(un->un_ipi_addr, (struct buf *)NULL, sizeof (*ip),
			MAX_RESPLEN, IPI_SLEEP, 0)) == NULL)
		return (EIO);

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = q->q_cmd;
	ip->hdr_opcode = IP_FORMAT;
	ip->hdr_mods = opmod | IP_OM_BLOCK;

	cp = (char *)(ip + 1);				/* point past header */

	/*
	 * Insert block size parameter, if used.
	 */
	if ((un->un_ctlr->c_flags & ID_NO_BLK_SIZE) == 0) {
		for (i = (int)(cp + 2); (i % sizeof (long)) != 0; i++)
			*cp++ = 0;			/* pad */
		*cp++ = sizeof (struct bsize_parm) + 1;	/* parm + id */
		*cp++ = BSIZE_PARM;
		((struct bsize_parm *) cp)->blk_size = DEV_BSIZE;
		cp += sizeof (struct bsize_parm);
	}

	/*
	 * Insert extent parameter, if needed.
	 */
	if (nblocks > 0 || first_block != 0) {
		for (i = (int)(cp + 2); (i % sizeof (long)) != 0; i++)
			*cp++ = 0;			/* pad */
		*cp++ = sizeof (struct cmdx_parm) + 1;	/* parm + id */
		*cp++ = CMD_EXTENT;
		((struct cmdx_parm *) cp)->cx_count = nblocks;
		((struct cmdx_parm *) cp)->cx_addr = first_block;
		cp += sizeof (struct cmdx_parm);
	}

	ip->hdr_pktlen = (cp - (char *)ip) - sizeof (ip->hdr_pktlen);

	q->q_dev = (caddr_t)un;
	q->q_time = 0;				/* no timeout */

	/*
	 * Send command.
	 */
	if ((errno = id_send_cmd(q, (long)IP_ABS_BLOCK, ID_ASYNCHWAIT)) == 0) {
		if (q->q_result != IP_SUCCESS) {
			printf("id%3x: id_format failed. result %x\n",
				un->un_unit, q->q_result);
			errno = EIO;
		}
	}

	ipi_free(q);
	return (errno);
}

/*
 * Read or write defect list.
 */
static
id_rdwr_deflist(un, bp, opmod)
	register struct id_unit	*un;	/* unit structure pointer */
	register struct buf	*bp;	/* buffer */
	int			opmod;	/* opcode modifier */
{
	struct ipiq		*q;
	int			errno = 0;	/* error return code */
	struct ipi3header	*ip;
	int			i;
	char			*cp;

	if ((q = ipi_setup(un->un_ipi_addr, bp, sizeof (*ip),
			MAX_RESPLEN, IPI_SLEEP, 0)) == NULL)
		return (EIO);

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = q->q_cmd;
	ip->hdr_opcode =
		(bp->b_flags & B_READ) ? IP_READ_DEFLIST : IP_WRITE_DEFLIST;
	ip->hdr_mods = opmod;
	cp = (char *)(ip + 1);				/* point past header */

	/*
	 * Add Transfer notification if required.
	 */
	if (q->q_tnp_len == sizeof (caddr_t)) {
		for (i = (int)(cp + 2); (i % sizeof (caddr_t)) != 0; i++)
			*cp++ = 0;			/* pad */
		*cp++ = sizeof (caddr_t) + 1;
		*cp++ = XFER_NOTIFY;
		* ((caddr_t *) cp)++ = * (caddr_t *) q->q_tnp;
	} else if (q->q_tnp_len > 0) {
		*cp++ = q->q_tnp_len + 1;
		*cp++ = XFER_NOTIFY;
		bcopy(q->q_tnp, cp, q->q_tnp_len);
		cp += q->q_tnp_len;
	}

	/*
	 * Insert Request Parameters as Data Parameter.
	 */
	*cp++ = 3;				/* parameter length */
	*cp++ = ATTR_REQPARM;			/* request parm parameter */
	if (un->un_ctlr->c_ctype == DKC_SUN_IPI1)
		*cp++ = RESP_AS_NDATA;
	else
		*cp++ = RESP_AS_DATA;		/* parms as data */
	*cp++ = DEFLIST_TRACK;			/* track defects list parm */

	/*
	 * Add command extent parameter for controllers that need it.
	 */
	if (un->un_ctlr->c_ctype == DKC_SUN_IPI1) {
		for (i = (int)(cp + 2); (i % sizeof (long)) != 0; i++)
			*cp++ = 0;			/* pad */
		*cp++ = sizeof (struct cmdx_parm) + 1;	/* len including ID */
		*cp++ = CMD_EXTENT;
		((struct cmdx_parm *) cp)->cx_count = bp->b_bcount;
		((struct cmdx_parm *) cp)->cx_addr = 0;
		cp += sizeof (struct cmdx_parm);
	}

	ip->hdr_pktlen = (cp - (char *)ip) - sizeof (ip->hdr_pktlen);

	q->q_dev = (caddr_t)un;
	bp->b_resid = bp->b_bcount;

	/*
	 * Send command.
	 */
	if ((errno = id_send_cmd(q,
		(long)IP_RESP_SUCCESS | IP_ABS_BLOCK | IP_BYTE_EXT,
		ID_ASYNCHWAIT)) == 0)
	{
		if (q->q_result != IP_SUCCESS) {
			bp->b_blkno = (daddr_t)q->q_errblk;
			printf("id%3x: id_rdwr_deflist failed. result %x\n",
				un->un_unit, q->q_result);
			errno = EIO;
		} else if (q->q_resp) {
			/*
			 * Handle response extent or parameter length parameter.
			 * This will set the residual count in the buffer.
			 */
			ipi_parse_resp(q, id_deflist_table, (caddr_t)bp);
		}
	}
	ipi_free(q);
	return (errno);
}

/*
 * Handle response extent parameter from read/write defect list.
 */
/* ARGSUSED */
static int
id_respx(q, id, rp, len, bp)
	struct ipiq		*q;	/* request */
	int			id;	/* parameter ID - not used */
	struct respx_parm	*rp;	/* parameter */
	int			len;	/* parameter length - not used */
	struct buf		*bp;	/* buffer */
{
	struct id_unit	*un;

	if (bp != NULL) {
		q->q_errblk = rp->rx_addr;
		if (q->q_flag & IP_BYTE_EXT) {
			bp->b_resid = rp->rx_resid;
		} else if ((un = (struct id_unit *)q->q_dev) != NULL) {
			bp->b_resid = rp->rx_resid << un->un_log_bshift;
		}
	}
	return (0);
}


/*
 * Handle parameter length parameter from read/write defect list.
 */
/* ARGSUSED */
static int
id_deflist_parmlen(q, id, rp, len, bp)
	struct ipiq		*q;	/* request */
	int			id;	/* parameter ID - not used */
	struct parmlen_parm	*rp;	/* parameter */
	int			len;	/* parameter length - not used */
	struct buf		*bp;	/* buffer */
{
	bp->b_resid = bp->b_bcount - rp->parmlen;
	return (0);
}

/*
 * Reallocate a defective block.
 */
static
id_realloc(un, first_block, nblocks, opmod)
	register struct id_unit	*un;	/* unit structure pointer */
	daddr_t		first_block;	/* first block number */
	int		nblocks;	/* block count, all if < 0 */
	int		opmod;		/* opcode modifier */
{
	struct ipiq		*q;
	int			errno = 0;	/* error return code */
	struct ipi3header	*ip;
	int			i;
	char			*cp;

	if (nblocks <= 0)
		return (EINVAL);

	if ((q = ipi_setup(un->un_ipi_addr, (struct buf *)NULL,
			sizeof (*ip) + sizeof (struct cmdx_parm) +
			sizeof (long), MAX_RESPLEN, IPI_SLEEP, 0)) == NULL)
		return (EIO);

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = q->q_cmd;
	ip->hdr_opcode = IP_REALLOC;
	ip->hdr_mods = opmod | IP_OM_BLOCK;

	cp = (char *)(ip + 1);				/* point past header */

	/*
	 * Insert extent parameter, if needed.
	 */
	if (nblocks > 0 || first_block != 0) {
		for (i = (int)(cp + 2); (i % sizeof (long)) != 0; i++)
			*cp++ = 0;			/* pad */
		*cp++ = sizeof (struct cmdx_parm) + 1;	/* parm + id */
		*cp++ = CMD_EXTENT;
		((struct cmdx_parm *) cp)->cx_count = nblocks;
		((struct cmdx_parm *) cp)->cx_addr = first_block;
		cp += sizeof (struct cmdx_parm);
	}

	ip->hdr_pktlen = (cp - (char *)ip) - sizeof (ip->hdr_pktlen);

	q->q_dev = (caddr_t)un;

	/*
	 * Send command.
	 */
	if ((errno = id_send_cmd(q, (long)IP_ABS_BLOCK, ID_ASYNCHWAIT)) == 0) {
		if (q->q_result != IP_SUCCESS) {
			printf("id%3x: id_realloc failed. result %x\n",
				un->un_unit, q->q_result);
			errno = EIO;
		}
	}

	ipi_free(q);
	return (errno);
}

/*
 * Build attribute command.
 *	Used only during recovery.  Leaves the number of parameters read
 *	in c->c_rec_data or un->un_rec_data.  (XXX - ugly side-effect).
 */
static struct ipiq *
id_build_attr_cmd(c, un, rt, callback)
	struct id_ctlr	*c;		/* controller */
	struct id_unit	*un;		/* unit (may be NULL) */
	register struct ipi_resp_table *rt;	/* response table */
	int	(*callback)();		/* callback routine for ipi_setup */
{
	register struct ipi_resp_table *rtp;	/* response table pointer */
	struct ipiq		*q;
	int			nparms;		/* number of parameter IDs */
	u_char			parms[ID_MAX_PARMS];
	u_char			*cp;
	int			addr;		/* IPI device address */
	struct ipi3header	*ip;
	u_char			*pp;		/* parameter pointer */
	int			len;		/* command length */

	cp = parms;
	nparms = 0;
	for (rtp = rt; rtp->rt_parm_id != 0 && nparms < c->c_max_parms; rtp++) {
		if (rtp->rt_parm_id == ATTR_SLVCNF_BIT &&
				(c->c_flags & ID_NO_RECONF))
			continue;
		*cp++ = rtp->rt_parm_id;
		nparms++;
	}
	if (un) {
		un->un_recover.rec_data = rtp - rt;
		addr = un->un_ipi_addr;
	} else {
		c->c_recover.rec_data = rtp - rt;
		addr = c->c_ipi_addr;
	}
	if (nparms <= 0)
		return (NULL);
	/*
	 * Figure length of command packet from table size.
	 * Doing this linearly is very cheap, since the tables are always short.
	 * Length is header + parm len, id, flags, + one for each attribute id.
	 */
	len = sizeof (struct ipi3header) + 3 + nparms;

	if ((q = ipi_setup(addr, (struct buf *)NULL, len, MAX_RESPLEN,
			callback, 0)) == NULL)
		return (q);

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = (struct ipi3header *)q->q_cmd;
	bzero ((caddr_t)ip, (u_int)len);

	ip->hdr_opcode = IP_ATTRIBUTES;
	ip->hdr_mods = IP_OM_REPORT;

	/*
	 * Build request parameters parameter.
	 */
	pp = cp = (u_char *)(ip + 1);	/* point to start of parameters */
	cp++;				/* skip parameter length */
	*cp++ = ATTR_REQPARM;		/* request parm parameter */
	*cp++ = RESP_AS_PKT;		/* parameters in response */

	/*
	 * Insert parameter IDs into request parm parameter.
	 */
	bcopy((caddr_t)parms, (caddr_t)cp, (u_int)nparms);
	cp += nparms;

	/*
	 * Set parameter length and packet length.
	 */
	*pp = (cp - pp) - sizeof (*pp);
	ip->hdr_pktlen = (cp - (u_char *)ip) - sizeof (ip->hdr_pktlen);
	if (ip->hdr_pktlen + 2 != len)
		printf("id_build_attr_cmd: miscalculated len %d pktlen %d",
			len, ip->hdr_pktlen);
	q->q_dev = (caddr_t)un; 		/* may be NULL */
	return (q);
}

/*
 * Build set attribute command.
 *	Used only during recovery.  Leaves the number of parameters read
 *	in c->c_rec_data or un->un_rec_data.  (XXX - ugly side-effect).
 */
static struct ipiq *
id_build_set_attr_cmd(c, un, rtp, callback)
	struct id_ctlr	*c;		/* controller */
	struct id_unit	*un;		/* unit (may be NULL) */
	register struct ipi_resp_table *rtp;	/* response table */
	int	(*callback)();		/* callback routine for ipi_setup */
{
	struct ipiq		*q;
	u_char			*cp;
	int			addr;		/* IPI device address */
	struct ipi3header	*ip;
	int			len;		/* command length */
	int			nparms;		/* number of parameters */
	caddr_t			arg;		/* arg to parameter function */

	if (rtp->rt_parm_id)
		nparms = 1;
	else
		nparms = 0;
	if (un) {
		un->un_recover.rec_data = nparms;
		addr = un->un_ipi_addr;
		arg = (caddr_t)un;
	} else {
		c->c_recover.rec_data = nparms;
		addr = c->c_ipi_addr;
		arg = (caddr_t)c;
	}
	if (nparms <= 0)
		return (NULL);
	/*
	 * Figure length of command packet from parameter size.
	 * Length is header + parm len, id, flags, + one for each attribute id.
	 */
	len = sizeof (struct ipi3header) + 3 + rtp->rt_min_len;

	if ((q = ipi_setup(addr, (struct buf *)NULL, len, MAX_RESPLEN,
			callback, 0)) == NULL)
		return (q);

	/*
	 * Form IPI-3 Command packet.
	 */
	ip = (struct ipi3header *)q->q_cmd;
	bzero ((caddr_t)ip, (u_int)len);

	ip->hdr_opcode = IP_ATTRIBUTES;
	ip->hdr_mods = IP_OM_LOAD;

	/*
	 * Insert parameter ID into request parm parameter.
	 */
	cp = (u_char *)(ip + 1);
	while (((int)cp + 2) % sizeof (long) != 0)
		*cp++ = 0;			/* pad for alignment */
	*cp++ = rtp->rt_min_len + 1;		/* length (including ID byte) */
	*cp++ = rtp->rt_parm_id;		/* parameter ID */
	(*rtp->rt_func)(q, rtp->rt_parm_id, cp, rtp->rt_min_len, arg);
	cp += rtp->rt_min_len;

	/*
	 * Set parameter length and packet length.
	 */
	ip->hdr_pktlen = (cp - (u_char *)ip) - sizeof (ip->hdr_pktlen);
	q->q_dev = (caddr_t)un; 		/* may be NULL */
	return (q);
}


/*
 * This routine prints out an error message with the appropriate device
 * or controller number and the block number if known.  The message may
 * be NULL, in which case no newline is printed.
 */
static void
id_printerr(q, msg)
	struct ipiq	*q;
	char		*msg;
{
	register struct buf *bp;
	register struct id_unit *un;
	register struct opcode_name *np;
	struct un_lpart	*lp;

	if (q->q_flag & IP_SILENT)
		return;
	if ((un = (struct id_unit *)q->q_dev) != NULL) {
		if ((bp = q->q_buf) != NULL) {
			if ((un->un_flags & ID_LABEL_VALID) &&
				!(q->q_flag & IP_ABS_BLOCK))
			{
				lp = &un->un_lpart[ID_LPART(bp->b_dev)];
				printf("id%3x%c: block %d", un->un_unit,
					ID_LPART(bp->b_dev) + 'a',
					q->q_errblk - lp->un_blkno);
				printf(" (%d abs)", q->q_errblk);
			} else {
				printf("id%3x: block %d", un->un_unit,
					q->q_errblk);
			}
		} else {
			printf("id%3x", un->un_unit);
		}
	} else if (q->q_ctlr) {
		printf("idc%d", ((struct id_ctlr *)(q->q_ctlr))->c_ctlr);
	} else {
		printf("id at ipi %x", q->q_csf);
	}
	for (np = opcode_name; np->name; np++) {
		if (np->opcode == q->q_cmd->hdr_opcode) {
			printf(": %s: ", np->name);
			break;
		}
	}
	if (np->name == NULL)
		printf(": op %x: ", q->q_cmd->hdr_opcode);
	if (msg)
		printf("%s\n", msg);
}

#ifdef MULT_MAJOR
/*
 * Establish maps for translating multiple major number into unit number.
 */
id_major_map_init()
{
	extern int nblkdev, nchrdev;
	register int	i;
	register int	cmajor;
	register int	bmajor;
	register struct cdevsw	*cdp;
	register struct bdevsw	*bdp;
	static char	init_done;

	if (init_done)
		return;
	init_done = 1;

	for (i = 0; i < NMAJOR; i++) {
		bmajor_lookup[i] = cmajor_lookup[i] =
			cmajor_trans[i] = NO_MAJOR;
	}

	/*
	 * setup cmajor_lookup[major(dev)] to be set number.
	 */
	cmajor = 0;
	for (cdp = cdevsw, i = 0; i < nchrdev; cdp++, i++) {
		if (cdp->d_open == idcopen) {
			cmajor_lookup[i] = cmajor++;
		}
	}

	/*
	 * setup bmajor_lookup[major(dev)] to be set number.
	 */
	bmajor = 0;
	for (bdp = bdevsw, i = 0; i < nblkdev; bdp++, i++) {
		if (bdp->d_open == idbopen) {
			bmajor_lookup[i] = bmajor++;
		}
	}

	if (cmajor != bmajor || cmajor <= 0) {
		printf("id_major_map_init:  error: cmajor %d bmajor %d\n",
			cmajor, bmajor);
		panic("id_major_map_init: blk/chr major count mismatch");
	}

	/*
	 * setup cmajor_trans[cmajor(i)] to be block major number.
	 */
	for (cmajor = 0; cmajor < nchrdev; cmajor++) {
		if (cmajor_lookup[cmajor] == NO_MAJOR)
			continue;
		for (bmajor = 0; bmajor < nblkdev; bmajor++) {
			if (bmajor_lookup[bmajor] == cmajor_lookup[cmajor]) {
				cmajor_trans[cmajor] = bmajor;
				break;
			}
		}
		if (bmajor == nblkdev) {
			printf("id_major_map_init: major %d\n", cmajor);
			panic("id_major_map_init: no blk dev for chr dev");
		}
	}

	/*
	 * check bmajor lookup for completeness.
	 */
	for (bmajor = 0; bmajor < nblkdev; bmajor++) {
		if (bmajor_lookup[bmajor] == NO_MAJOR)
			continue;
		for (cmajor = 0; cmajor < nchrdev; cmajor++) {
			if (bmajor_lookup[bmajor] == cmajor_lookup[cmajor]) {
				if (cmajor_trans[cmajor] != bmajor) {
	printf("id_major_map_init: cmaj %d bmaj %d trans %d\n",
						cmajor, bmajor,
						cmajor_trans[cmajor]);
					panic("id_major_map_init: bad trans");
				}
				break;
			}
		}
		if (cmajor == nchrdev) {
			printf("id_major_map_init: blk major %d ", bmajor);
			panic("id_major_map_init: no chr dev for blk dev");
		}
	}
}
#endif /* MULT_MAJOR */

/*
 * Watchdog for idle controllers or devices that have gone away.
 */
static void
id_watch()
{
	struct id_unit	**unp;
	struct id_unit	*un;
	struct id_ctlr	*c;
	struct ipiq	*q;

	if (id_watch_enable == 0)		/* patchable enable/disable */
		goto resched;

	for (unp = id_unit; unp != NULL && unp < &id_unit[nid]; unp++) {
		if ((un = *unp) == NULL)
			continue;
		/*
		 * If the unit isn't idle, assume the command timeout
		 * will trigger if the device goes offline.
		 */
		if (un->un_cmd_q > 0) {
			un->un_idle_time = 0;
			continue;
		}
		/*
		 * If the unit is idle, add the interval since we last
		 * checked to the idle time.  This exaggerates a bit.
		 */
		un->un_idle_time += ID_WATCH_TIME;

		/*
		 * If the unit has been idle for too long, issue a REPORT
		 * STATUS command.
		 */
		if (un->un_idle_time > ID_MAX_IDLE) {
			c = un->un_ctlr;
			if (!IE_STAT_READY(un->un_flags) ||
					!IE_STAT_READY(c->c_flags))
				continue;

			q = ipi_setup(un->un_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), MAX_RESPLEN,
				IPI_NULL, 0);
			if (q == NULL)
				continue;

			q->q_cmd->hdr_opcode = IP_REPORT_STAT;
			q->q_cmd->hdr_mods = IP_OM_CONDITION;
			q->q_cmd->hdr_pktlen = IPI_HDRLEN;
			q->q_dev = (caddr_t)un;
			q->q_time = ID_REC_TIMEOUT;

			un->un_cmd_q++;
			c->c_cmd_q++;
			c->c_cmd_q_len += sizeof (struct ipi3header);

			ipi_start(q, (long)IP_RESP_SUCCESS, id_watch_intr);

			un->un_idle_time = 0;
		}
	}
resched:
	timeout((int (*)())id_watch, (caddr_t)NULL, ID_WATCH_TIME*hz);
}

/*
 * Interrupt handler for watchdog.
 */
static void
id_watch_intr(q)
	struct ipiq	*q;
{
	if (q) {
		id_cmd_intr(q);
		switch (q->q_result) {
		case IP_SUCCESS:
			(void) id_error_parse(q, ipi_cond_table, id_error,
				(struct id_ctlr *)q->q_ctlr,
				(struct id_unit *)q->q_dev, IE_FAC);
			ipi_free(q);
			break;
		case IP_MISSING:
			id_missing_req(q);
			break;
		default:
			ipi_free(q);
			break;
		}
	}
}

/*
 * Walk the table for each parameter in the response given.
 * If any action should be taken, take it by calling the function given.
 *
 * Remember, speed is not important here.  Robustness and code size are.
 */
static int
id_error_parse(q, err_table, fcn, c, un, flags)
	register struct ipiq	*q;	/* request with response */
	struct ipi_err_table *err_table;
	int		(*fcn)();	/* function to handle errors */
	struct id_ctlr	*c;
	struct id_unit	*un;
	register int	flags;		/* initial error flags */
{
	register struct ipi_err_table	*etp;	/* error table pointer */
	register struct ipi3resp *rp;
	register u_char	*cp;
	struct {			/* parameter unpacking buffer */
		long	align;		/* to force alignment */
		u_char	pad[2];		/* to maintain alignment for buf */
		u_char	len;		/* parameter length */
		u_char	id;		/* parameter ID */
		u_char	buf[254];	/* rest of parameter (after len/ID) */
	} parm;
	u_char		*pp;		/* parameter pointer */
	int		id;		/* parameter id */
	int		plen;		/* parameter length */
	int		rlen;		/* response length */
	int		pflags;		/* flags on parameter */
	int		bflags;		/* flags on bit */
	int		result;		/* result for current check */
	int		dryrun;		/* set during dry run */

	/*
	 * The table is read twice, once to see if any print flags are set and
	 * again to do the real work.
	 */
	dryrun = 1;
loop:
	if ((rp = q->q_resp) == NULL) {
		id_printerr(q, "id_error_parse: no response where expected");
		return (IE_CMD);
	}
	rlen = rp->hdr_pktlen + sizeof (rp->hdr_pktlen)
		- sizeof (struct ipi3resp);
	cp = (u_char *)rp + sizeof (struct ipi3resp);

	for (; rlen > 0; cp += plen) {
		if ((plen = cp[0] + 1) > rlen)
			break;
		rlen -= plen;
		if (plen <= 1)
			continue;	/* padding byte */

		/*
		 * Find parameter ID in the table.
		 */
		id = cp[1];
		pflags = 0;
		for (etp = err_table; etp->et_mask; etp++) {
			if (etp->et_byte != 0)
				continue;
			if (etp->et_parmid == id)
				break;
			if (etp->et_parmid + IPI_FAC_PARM == id) {
				pflags |= IE_FAC;
				break;
			}
		}

		/*
		 * If parameter not found in table, skip it.
		 */
		if (etp->et_mask == 0)
			continue;
		/*
		 * Check (and fix) alignment if the parameter contains more
		 * than just the length and parameter ID.
		 */
		pp = cp;
		if (plen > 2 && (((int)cp + 2) % sizeof (long)) != 0) {
			pp = &parm.len;
			bcopy((caddr_t)cp, (caddr_t)pp, (u_int)plen);
		}

		/*
		 * Set flags according to flags found in ID entry.
		 */
		pflags |= etp->et_flags & ~IE_MASK | flags;
		flags |= pflags & IE_RESP_MASK;
		result = etp->et_flags & IE_MASK;
		if (!dryrun || (pflags & IE_FIRST_PASS)) {
			if (pflags & (IE_MSG | IE_PRINT))
				id_printerr(q, (char *)NULL);
			/*
			 * run handler for parameter.
			 */
			flags |= (*fcn)(q, c, un, result, pflags,
				etp->et_msg, pp);
		}

		/*
		 * Test each condition mentioned in the table until
		 * the next parameter ID entry.
		 */
		bflags = 0;			/* in case no bit matches */
		while ((++etp)->et_byte) {
			if (etp->et_byte + 1 < plen &&
				(etp->et_mask & cp[etp->et_byte + 1]))
			{
				bflags = etp->et_flags & ~IE_MASK | pflags;
				flags |= bflags & IE_RESP_MASK;
				result = etp->et_flags & IE_MASK;
				if (!dryrun) {
					if ((bflags ^ pflags) & IE_MSG)
						id_printerr(q, (char *)NULL);
					/*
					 * run handler for this match.
					 */
					flags |= (*fcn)(q, c, un, result,
						bflags, etp->et_msg, pp);
				}
			}
		}
		if (!dryrun && ((pflags | bflags) & (IE_MSG | IE_PRINT)) &&
				!(q->q_flag & IP_SILENT))
			printf("\n");
	}
	if (dryrun) {
		dryrun = 0;
		goto loop;
	}
	if ((flags & IE_DUMP) && (!(q->q_flag & IP_SILENT) || id_debug)) {
		ipi_print_cmd(IPI_CHAN(q->q_csf), q->q_cmd, "command in error");
		ipi_print_resp(IPI_CHAN(q->q_csf), rp, "response of error");
	}
	return (flags);
}

/*
 * Missing interrupt recovery.
 *
 * When an IPI command fails to give a completion response in the allotted
 * time, this routine is called.
 *
 * The recovery method:
 *
 * The point of probable failure is determined by attempting to communicate
 * with the nearest component first and working out on the channel until
 * the problem area is determined.
 *
 * First, a NOP command is sent to the channel adaptor board.  If that times
 * out, the channel board is reset, the controllers reset, and all outstanding
 * I/O is re-issued.
 *
 * XXX - a method to test whether one controller or an entire channel is bad
 * must be developed.
 *
 * If the channel board is OK, a NO-Op command is issued to the particular
 * slave device.  If that times out, the controller is reset and all
 * outstanding I/O is re-issued.
 *
 * If the slave is OK, the command is aborted and re-issued.  After a number
 * of retries, the command is considered to have failed and an error is issued.
 *
 * Alternate Reverse order method:
 *
 * This is not currently used, but could be a better way of doing things.
 *
 * If not an Abort command, issue an Abort command in case the hang up is
 * in the disk or string controller.  Mark the controller so that no other
 * commands will be issued to the controller until the abort comes back.
 *
 * If an Abort command fails or times out, reset the slave.  If that times out,
 * reset the channel and all the slaves on it.  If that times out,
 * reset the entire board (all channels).
 *
 * Some controllers may not be able to accept aborts for individual commands.
 * Handle these by escalating to doing a reset of the controller.
 *
 * After abort/reset processing, re-issue any commands that were outstanding
 * but hadn't completed.
 *
 * Always called with interrupts masked.
 *
 * If q has IP_SYNC flag set, will not return until recovery completed or
 * failure has occurred.  Otherwise, just starts recovery.  The interrupts
 * generated will continue the recovery.
 */
static void
id_missing_req(q)
	register struct ipiq	*q;
{
	register struct id_ctlr		*c;
	int	s;

	if ((c = (struct id_ctlr *)q->q_ctlr) == NULL) {
		id_printerr(q, "missing rupt. q_ctlr NULL");
		return;
	}

	/*
	 * If recovery is already in progress, return.
	 */
	if (c->c_flags & IE_RECOVER) {
		id_printerr(q, "missing interrupt - recovery in progress");
		return;
	} else {
		id_printerr(q, "missing interrupt - attempting recovery");
	}

	/*
	 * Setup controller in recovery mode.
	 */
	c->c_flags |= IE_RECOVER;
	c->c_flags &= ~IE_TRACE_RECOV;
	c->c_recover.rec_history = 0;
	c->c_recover.rec_state = IDR_TEST_BOARD;
	c->c_recover.rec_substate = 0;
	c->c_recover.rec_flags = (q->q_flag & IP_SYNC) | IP_NO_RETRY;
	s = spl_ipi();
	(void) id_recover(c, (struct id_unit *)NULL);
	(void) splx(s);
}


/*
 * State transition table for recovery.
 */
struct id_next_state {
	char	ns_success;	/* next state if command successful */
	char	ns_failed;	/* next state if command failed */
	char	ns_failed_again; /* next state when command fails second time */
	char	*ns_msg;	/* message */
};

/*
 * Recovery table for controller.
 */
static struct id_next_state id_next_state[] = {
/*    present		next state	next state 	next state */
/*    state		on success	on failure 	on 2nd fail */

/*  0 IDR_NORMAL */	IDR_NORMAL,	IDR_NORMAL,	IDR_NORMAL,
	"recovery complete",
/*
 * Recovery from missing interrupt.
 */
/*  1 IDR_TEST_BOARD */	IDR_TEST_CHAN,	IDR_RST_BOARD,	IDR_REC_FAIL,
	"test board",
/*  2 IDR_RST_BOARD */	IDR_RST_CHAN,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"reset board",
/*  3 IDR_TEST_CHAN */	IDR_TEST_CTLR,	IDR_RST_CHAN,	IDR_RST_BOARD,
	"test chan",
/*  4 IDR_RST_CHAN */	IDR_RST_CTLR,	IDR_RST_BOARD,	IDR_REC_FAIL,
	"reset chan",
/*  5 IDR_TEST_CTLR */	IDR_IDLE_CTLR,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"test ctlr",
/*  6 IDR_RST_CTLR */	IDR_GET_XFR,	IDR_RST_CHAN,	IDR_REC_FAIL,
	"reset ctlr",
/*  7 IDR_GET_XFR */	IDR_SET_XFR,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"get xfr",
/*  8 IDR_SET_XFR */	IDR_STAT_CTLR,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"set xfr",
/*  9 IDR_STAT_CTLR */	IDR_ID_CTLR,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"stat ctlr",
/* 10 IDR_ID_CTLR */	IDR_ATTR_CTLR,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"id ctlr",
/* 11 IDR_ATTR_CTRL */	IDR_SATTR_CTLR1, IDR_RST_CTLR,	IDR_RST_CHAN,
	"attributes",
/* 12 IDR_SET_XFR1 */	IDR_RETRY,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"set xfr",
/* XXX - no path to IDR_ABORT for now (ISP-80 doesn't abort) */
/* 13 IDR_ABORT */	IDR_RETRY,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"abort",
/* 14 IDR_RETRY */	IDR_NORMAL,	IDR_RST_CTLR,	IDR_RST_CHAN,
	"retry",
/* 15 IDR_REC_FAIL */	IDR_REC_FAIL,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"recovery failed",

/*
 * Controller Re-initialization or initial configuration.
 */
/* XXX - eventually do more in the failure cases of these. */
/* 16 IDR_RST_CTLR1 */	IDR_GET_XFR1,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"reset ctlr",
/* 17 IDR_GET_XFR1 */	IDR_SET_XFR2,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"get xfr",
/* 18 IDR_SET_XFR2 */	IDR_STAT_CTLR1,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"set xfr",
/* 19 IDR_STAT_CTLR1 */ IDR_ID_CTLR1,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"stat ctlr",
/* 20 IDR_ID_CTLR1 */	IDR_ATTR_CTLR1,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"id ctlr",
/* 21 IDR_ATTR_CTRL1 */	IDR_SATTR_CTLR2, IDR_REC_FAIL,	IDR_REC_FAIL,
	"attr ctlr",
/* 22 IDR_SET_XFR3 */	IDR_NORMAL,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"set xfr",

/*
 * Unit Re-initialization or initial configuration..
 */
/* 23 IDR_STAT_UNIT */	IDR_ATTR_UNIT,	IDR_UNIT_FAILED, IDR_UNIT_FAILED,
	"stat unit",
/* 24 IDR_ATTR_UNIT */	IDR_READ_LABEL,	IDR_UNIT_FAILED, IDR_UNIT_FAILED,
	"attr unit",
/* 25 IDR_READ_LABEL */	IDR_UNIT_NORMAL, IDR_UNIT_FAILED, IDR_UNIT_FAILED,
	"read label",
/* 26 IDR_UNIT_NORMAL */ IDR_UNIT_NORMAL, IDR_UNIT_NORMAL, IDR_UNIT_NORMAL,
	"unit recovery complete",
/* 27 IDR_UNIT_FAILED */ IDR_UNIT_FAILED, IDR_UNIT_FAILED, IDR_UNIT_FAILED,
	"unit recovery failed",

/*
 * Additional states inserted.
 */
/* 28 IDR_SATTR_CTLR1 */ IDR_SET_XFR1,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"set ctlr attr",
/* 29 IDR_SATTR_CTLR2 */ IDR_SET_XFR3,	IDR_REC_FAIL,	IDR_REC_FAIL,
	"set ctlr attr",
/* 30 IDR_IDLE_CTLR */	 IDR_NORMAL,	IDR_DUMP_CTLR,	IDR_RST_CTLR,
	"idle ctlr",
/* 31 IDR_DUMP_CTLR */	 IDR_RST_CTLR,	IDR_RST_CTLR,	IDR_RST_CTLR,
	"dump ctlr",
};

#define	IDR_NSTATES (sizeof (id_next_state) / sizeof (struct id_next_state))

/*
 * Wrapper for calling id_recover through timeout().  Only one arg.
 */
static int
id_recover_idle(c)
	struct id_ctlr	*c;
{
	int	s;

	s = spl_ipi();
	(void) id_recover(c, (struct id_unit *)NULL);
	(void) splx(s);
	return (0);
}

/*
 * This routine does the real work of recovering a controller.
 * It gets reinvoked to issue the commands for every stage of recovery
 * or loops if in synchronous mode.
 */
static int
id_recover(c, un)
	register struct id_ctlr	*c;
	register struct id_unit	*un;
{
	register struct ipiq	*q;
	struct id_rec_state	*rp;
	u_int			*flagsp;
	long			ipi_flags;
	struct buf		lbuf;
	int			mode;
	int			state;
	int			next_state;
	int			(*callback)();
	int			unit;
	u_char			*cp;

	if (un) {
		rp = &un->un_recover;
		flagsp = &un->un_flags;
	} else {
		rp = &c->c_recover;
		flagsp = &c->c_flags;
	}
	*flagsp |= IE_RECOVER | IE_RECOVER_ACTV;
	ipi_flags = rp->rec_flags;		/* flags for ipi requests */
	callback = (ipi_flags & IP_SYNC) ? IPI_NULL : id_recover_start;
	if (id_debug) {
		*flagsp |= IE_TRACE_RECOV;
	}

	while (*flagsp & IE_RECOVER) {
		state = rp->rec_state;
		if (*flagsp & IE_TRACE_RECOV) {
			if (un)
				printf("id%3x: id_recover      %d %s\n",
					un->un_unit, state,
					id_next_state[state].ns_msg);
			else
				printf("idc%d: id_recover      %d %s\n",
					c->c_ctlr, state,
					id_next_state[state].ns_msg);
		}
		next_state = id_next_state[state].ns_success;

		/*
		 * Build command.
		 */
		q = NULL;

		switch (state) {
		case IDR_NORMAL:		/* recovery complete */
			/*
			 * If a re-initialization became necessary during
			 * recovery, switch over to the right state to do it.
			 */
			if (c->c_flags & IE_RE_INIT) {
				c->c_flags &= ~IE_RE_INIT;
				rp->rec_state = IDR_RST_CTLR1;
				rp->rec_history = 0;
				break;
			}
			rp->rec_history |= 1 << state;
			if (c->c_flags & ID_C_PRESENT)
				printf("idc%d: Recovery complete.\n",
					c->c_ctlr);
			/*
			 * If there is no seek algorithm, allow a smaller number
			 * of commands to be sent to the controller so that the
			 * rest can be sorted in the host.
			 */
			if (!(c->c_flags & ID_SEEK_ALG))
				c->c_max_q = MIN(c->c_max_q, MAX_UNSORTED);
			c->c_flags &= ~IE_RECOVER;
			c->c_flags |= ID_C_PRESENT;
			/*
			 * Call idstart in case I/O was waiting for recovery to
			 * complete.
			 */
			for (unit = 0; unit < IDC_NUNIT; unit++)
				if ((un = c->c_unit[unit]) != NULL)
					(void) idstart(un);
			if (!(ipi_flags & IP_SYNC))
				wakeup((caddr_t)c);
			break;

		case IDR_REC_FAIL:		/* recovery failed miserably */
			/*
			 * Mark controller unusable.
			 */
			if (c->c_flags & ID_C_PRESENT)
		printf("idc%d: Recovery failed.  Controller marked unusable.\n",
					c->c_ctlr);
			c->c_flags &= ~(IE_RECOVER | IE_PAV);
			rp->rec_history |= 1 << state;
			/*
			 * Issue errors for pending commands.
			 */
			while ((q = c->c_retry_q) != NULL) {
				c->c_retry_q = q->q_next;
				if ((un = (struct id_unit *)q->q_dev) != NULL)
					un->un_cmd_q++;
				c->c_cmd_q++;
				c->c_cmd_q_len +=
					q->q_cmd->hdr_pktlen + sizeof (u_short);
				ipi_complete(q, IP_RETRY_FAILED);
			}
			if (!(ipi_flags & IP_SYNC))
				wakeup((caddr_t)c);
			break;

		case IDR_UNIT_NORMAL:		/* unit recovery complete */
			un->un_flags |= ID_UN_PRESENT;
			un->un_flags &= ~IE_RECOVER;
			rp->rec_history |= 1 << state;
			if ((!q || q->q_result != IP_MISSING) && un->un_lp) {
				/*
				 * see comment in id_recover_intr() about
				 * reading the label.
				 */
				if (islabel(un->un_unit, un->un_lp))
					uselabel(un, un->un_lp);
				kmem_free((caddr_t)un->un_lp, DEV_BSIZE);
				un->un_lp = NULL;
			}
			if (!(ipi_flags & IP_SYNC))
				wakeup((caddr_t)un);
			break;

		case IDR_UNIT_FAILED:
			/*
			 * Unit may or may not be usable.  The LABEL_VALID
			 * flag and partition sizes will determine whether
			 * it can be opened.  Leave it's status alone so it
			 * can be opened for formatting, etc.
			 */
			un->un_flags &= ~IE_RECOVER;
			rp->rec_history |= 1 << state;
			if ((!q || q->q_result != IP_MISSING) && un->un_lp) {
				/*
				 * see comment in id_recover_intr() about
				 * reading the label.
				 */
				if (islabel(un->un_unit, un->un_lp))
					uselabel(un, un->un_lp);
				kmem_free((caddr_t)un->un_lp, DEV_BSIZE);
				un->un_lp = NULL;
			}
			if (!(ipi_flags & IP_SYNC))
				wakeup((caddr_t)un);
			break;

		case IDR_TEST_BOARD:		/* see if board is present */
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_test_board(q, ipi_flags, id_recover_intr);
			break;

		/*
		 * Unimplemented cases:  just go to next step.
		 */
/* XXX - test channel not implemented yet. */
		case IDR_TEST_CHAN:
			rp->rec_history |= 1 << state;
			rp->rec_state = next_state;
			rp->rec_substate = 0;
			break;

		case IDR_GET_XFR:
		case IDR_GET_XFR1:
			c->c_xfer_mode[0] = 0;
			bzero((caddr_t)&lbuf, sizeof (lbuf));
			lbuf.b_flags = B_BUSY | B_READ;
			lbuf.b_un.b_addr = (caddr_t)c->c_xfer_mode;
			lbuf.b_bcount = sizeof (c->c_xfer_mode);
			if ((q = ipi_setup(c->c_ipi_addr, &lbuf,
					sizeof (struct ipi3header),
					MAX_RESPLEN, callback, 0)) == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_get_xfer_mode(q, ipi_flags, id_recover_intr);
			break;

		/*
		 * Set transfer settings - use interlocked if possible,
		 * since attributes haven't been read.  If not, rate must
		 * not be 20 MB/sec.
		 */
		case IDR_SET_XFR:
		case IDR_SET_XFR2:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			if ((c->c_xfer_mode[0] & IP_INLK_CAP) == 0)
				mode = IP_STREAM;
			else
				mode = 0;
			ipi_set_xfer_mode(q, ipi_flags, id_recover_intr, mode);
			break;

		/*
		 * Set transfer settings - attributes have been read, so
		 * use streaming if possible and proper rate.
		 */
		case IDR_SET_XFR1:
		case IDR_SET_XFR3:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			if ((c->c_xfer_mode[0] & IP_STREAM_CAP))
				mode = IP_STREAM;
/* XXX - only handles 10 MB/sec now - get attributes later */
			else
				mode = 0;
			ipi_set_xfer_mode(q, ipi_flags, id_recover_intr, mode);
			break;

		case IDR_TEST_CTLR:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_cmd->hdr_opcode = IP_NOP;
			q->q_cmd->hdr_mods = 0;
			q->q_cmd->hdr_pktlen = IPI_HDRLEN;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags, id_recover_intr);
			break;

		case IDR_RST_BOARD:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			ipi_reset_board(q, ipi_flags, id_recover_intr);
			break;

		case IDR_RST_CHAN:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_reset_chan(q, ipi_flags, id_recover_intr);
			break;

		case IDR_RST_CTLR:
		case IDR_RST_CTLR1:
			q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
				sizeof (struct ipi3header), 0, callback, 0);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_RST_TIMEOUT;
			ipi_reset_slave(q, ipi_flags | IP_LOG_RESET,
				id_recover_intr);
			break;

		case IDR_STAT_CTLR:
		case IDR_STAT_CTLR1:
			if ((q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
					sizeof (struct ipi3header),
					MAX_RESPLEN, callback, 0)) == NULL)
				goto setup_failed;
			q->q_cmd->hdr_opcode = IP_REPORT_STAT;
			q->q_cmd->hdr_mods = IP_OM_CONDITION;
			q->q_cmd->hdr_pktlen = IPI_HDRLEN;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_STAT_UNIT:
			if ((q = ipi_setup(un->un_ipi_addr, (struct buf *)NULL,
					sizeof (struct ipi3header),
					MAX_RESPLEN, callback, 0)) == NULL)
				goto setup_failed;
			q->q_cmd->hdr_opcode = IP_REPORT_STAT;
			q->q_cmd->hdr_mods = IP_OM_CONDITION;
			q->q_cmd->hdr_pktlen = IPI_HDRLEN;
			q->q_dev = (caddr_t)un;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_ID_CTLR:
		case IDR_ID_CTLR1:
			c->c_ctype = DKC_UNKNOWN;
			q = id_build_attr_cmd(c, (struct id_unit *)NULL,
				vendor_id_table, callback);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_ATTR_CTLR:
		case IDR_ATTR_CTLR1:
			q = id_build_attr_cmd(c, (struct id_unit *)NULL,
				&ctlr_conf_table[rp->rec_substate], callback);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_SATTR_CTLR1:
		case IDR_SATTR_CTLR2:
			if (c->c_flags & ID_NO_RECONF) {
				rp->rec_history |= 1 << state;
				rp->rec_state = next_state;
				rp->rec_substate = 0;
				break;
			}
			q = id_build_set_attr_cmd(c, (struct id_unit *)NULL,
				&ctlr_set_conf_table[rp->rec_substate],
				callback);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_ATTR_UNIT:
			q = id_build_attr_cmd(c, un,
				&attach_resp_table[rp->rec_substate], callback);
			if (q == NULL)
				goto setup_failed;
			q->q_time = ID_REC_TIMEOUT;
			ipi_start(q, ipi_flags | IP_RESP_SUCCESS,
				id_recover_intr);
			break;

		case IDR_READ_LABEL:
			un->un_flags &= ~ID_LABEL_VALID;
			if (!(un->un_flags & ID_FORMATTED)) {
				rp->rec_history |= 1 << state;
				rp->rec_state = next_state;
				break;
			}
			if (un->un_lp == NULL) {
id_printerr(q, "id_recover: no memory for label"); /* XXX */
rp->rec_state = IDR_UNIT_FAILED;
break;
			}
			un->un_lp->dkl_magic = 0;
			bzero((caddr_t)&lbuf, sizeof (lbuf));
			lbuf.b_flags = B_BUSY | B_READ;
			lbuf.b_un.b_addr = (caddr_t)un->un_lp;
			lbuf.b_bcount = sizeof (struct dk_label);
			if ((q = ipi_setup(un->un_ipi_addr, &lbuf,
					c->c_rw_cmd_len,
					MAX_RESPLEN, callback, 0)) == NULL)
				goto setup_failed;
			id_build_rdwr(q, un, (daddr_t)0, 1); /* 1 block label */
			q->q_buf = NULL;	/* buffer is local */
			ipi_start(q, ipi_flags, id_recover_intr);
			break;

		case IDR_RETRY:			/* re-issue commands */
			while ((q = c->c_retry_q) != NULL) {
				c->c_retry_q = q->q_next;
				id_retry(q);
			}
			rp->rec_history |= 1 << state;
			rp->rec_state = next_state;
			break;

		case IDR_IDLE_CTLR:		/* wait for cmds to finish */
			/*
			 * Wait in this state in case the missing interrupt was
			 * because we the controller was just busy with other
			 * commands.  No new commands are sent in the hope
			 * that things can get cleaned up.
			 *
			 * rp->rec_substate is time (seconds) in this state.
			 */
			if (c->c_cmd_q == 0) {
				rp->rec_history |= 1 << state;
				rp->rec_state = next_state;
				rp->rec_substate = 0;
			} else if ((ipi_flags & IP_SYNC) == 0 &&
					rp->rec_substate++ < ID_IDLE_TIME) {
				timeout(id_recover_idle, (caddr_t)c, hz);
				goto out;	/* Think about this. */
			} else {	/* time is up */
				rp->rec_history |= 1 << state;
				rp->rec_state = id_next_state[state].ns_failed;
				rp->rec_substate = 0;
			}
			break;

		case IDR_DUMP_CTLR:		/* take firmware dump */
			if (c->c_ctype != DKC_SUN_IPI1) {
				rp->rec_history |= 1 << state;
				rp->rec_state = next_state;
				break;
			}
			printf("idc%d: driver requesting controller dump\n",
				c->c_ctlr);
			if ((q = ipi_setup(c->c_ipi_addr, (struct buf *)NULL,
					sizeof (struct ipi3header)
					+ sizeof (struct ipi_diag_num) + 2,
					MAX_RESPLEN, callback, 0)) == NULL)
				goto setup_failed;
			q->q_cmd->hdr_opcode = IP_DIAG_CTL;
			q->q_cmd->hdr_mods = 0;
			cp = (u_char *)(q->q_cmd + 1);	/* past header */
			*cp++ = sizeof (struct ipi_diag_num)+1;	/* parm + id */
			*cp++ = IPI_DIAG_NUM;		/* parameter ID */
			((struct ipi_diag_num *)cp)->dn_diag_num = IPDN_DUMP;
			q->q_cmd->hdr_pktlen = cp + sizeof (struct ipi_diag_num)
				- (u_char *)q->q_cmd - sizeof (u_short);
			q->q_dev = (caddr_t)un;
			q->q_time = ID_DUMP_TIMEOUT;
			ipi_start(q, ipi_flags | IP_SILENT, id_recover_intr);
			break;

		default:
			printf("idc%d: id_recover: bad state %d\n",
				c->c_ctlr, rp->rec_state);
			rp->rec_history |= 1 << state;
			rp->rec_state = IDR_REC_FAIL;
			break;
		}

		/*
		 * Wait for command to complete.
		 * Id_recover_intr() will handle the ending status.
		 */
		if (q) {
			if (ipi_flags & IP_SYNC) {
				if (ipi_poll(q, (long)ID_POLL_TIMEOUT) == 0) {
					q->q_result = IP_MISSING;
					id_recover_intr(q);
					/*
					 * Clear sync flag so subsequent
					 * controller reset will allow it
					 * to be freed (after IP_RETRY_FAIL).
					 */
					q->q_flag &= ~IP_SYNC;
				} else {
					ipi_free(q);
				}
			} else if (q->q_result == IP_INPROGRESS) {
				break;	/* interrupt routine will restart */
			}
		}
	}
out:
	*flagsp &= ~IE_RECOVER_ACTV;
	return (0);

setup_failed:
	if (callback == IPI_NULL)
		printf("idc%d: synchronous command setup failed\n", c->c_ctlr);
	*flagsp &= ~IE_RECOVER_ACTV;
	return (DVMA_RUNOUT);
}


/*
 * Called when a controller tried to get recovery started but couldn't get
 * a request setup.  This routine is called through the callback
 * when a setup becomes possible and should be reattempted.
 */
/*ARGSUSED*/
static int
id_recover_start(arg)
	caddr_t arg;
{
	register struct id_ctlr	*c;
	struct id_unit		*un;
	int			i;
	int			ctlr;
	int			stat;
	int			s;

	stat = 0;
	s = spl_ipi();
	for (ctlr = 0; ctlr < nidc && stat == 0; ctlr++) {
		if ((c = id_ctlr[ctlr]) == NULL)
			continue;
		if (c->c_flags & IE_RECOVER_ACTV)
			continue;
		if (c->c_flags & IE_RECOVER) {
			stat = id_recover(c, (struct id_unit *)NULL);
		} else {
			for (i = 0; i < IDC_NUNIT && stat == 0; i++) {
				if ((un = c->c_unit[i]) != NULL &&
						un->un_flags & IE_RECOVER)
					stat = id_recover(c, un);
			}
		}
	}
	(void) splx(s);
	return (stat);
}

/*
 * Interrupt handler for recovery actions.
 */
static void
id_recover_intr(q)
	register struct ipiq	*q;
{
	register struct id_ctlr *c;
	register struct id_unit	*un;
	struct id_rec_state	*rp;
	int			flags;
	struct id_next_state	*nsp;
	int			state, next_state;
	int			stay_in_state = 0;

	if (q == NULL)
		return;

	c = (struct id_ctlr *)q->q_ctlr;
	un = (struct id_unit *)q->q_dev;
	rp = un ? &un->un_recover : &c->c_recover;

	if ((state = rp->rec_state) >= IDR_NSTATES) {
		printf("id_recover_intr: bad state %d\n", state);
		id_printerr(q, "bad recovery state");
		return;
	}

	switch (q->q_result) {
	case IP_ERROR:
	case IP_COMPLETE:
		if (q->q_resp) {
			flags = id_error_parse(q, ipi_err_table, id_error,
				c, un, 0);
			if ((flags & IE_CMD) == 0)	/* non fatal */
				q->q_result = IP_SUCCESS;
		}
		break;

	case IP_RETRY_FAILED:
		/*
		 * When retry has failed, this result means a request
		 * sent by id_recover() was missing and now that the
		 * controller has been reset it can be freed.
		 */
		ipi_free(q);
		return;
	}

	switch (state) {
	case IDR_STAT_CTLR:
	case IDR_STAT_CTLR1:
		/*
		 * Evaluate the response from Report Addressee Status.
		 */
		if (q->q_result == IP_SUCCESS)
			(void) id_error_parse(q, ipi_cond_table, id_error,
				c, (struct id_unit *)NULL, IE_INIT_STAT);
		else if (q->q_result == IP_MISSING)
			break;
		/*
		 * Check flags.  If the controller isn't ready,
		 * consider the status operation a failure for the
		 * purposes of selecting the next state.  Ignore the
		 * IE_RECOVER and IE_RE_INIT flags for this test.
		 */
		if (!IE_STAT_READY(c->c_flags & ~(IE_RECOVER | IE_RE_INIT))) {
			q->q_result = IP_ERROR;
		}
		c->c_flags &= ~IE_INIT_STAT;
		break;

	case IDR_STAT_UNIT:
		/*
		 * Evaluate the response from Report Addressee Status.
		 */
		if (q->q_result == IP_SUCCESS)
			(void) id_error_parse(q, ipi_cond_table,
				id_error, c, un, IE_FAC);
		else if (q->q_result == IP_MISSING)
			break;
		/*
		 * Check flags.  If the unit isn't ready,
		 * consider the status operation a failure for the
		 * purposes of selecting the next state.  Ignore the
		 * IE_RECOVER and IE_RE_INIT flags for this test.
		 */
		if (!IE_STAT_READY(un->un_flags & ~(IE_RECOVER | IE_RE_INIT))) {
			q->q_result = IP_ERROR;
			un->un_flags &= ~ID_UN_PRESENT;
		} else {
			un->un_flags |= ID_UN_PRESENT;
		}
		un->un_flags &= ~IE_INIT_STAT;
		break;

	case IDR_ID_CTLR:
	case IDR_ID_CTLR1:
		if (id_print_attr)
			ipi_print_resp(IPI_CHAN(q->q_csf), q->q_resp,
				"ctlr id response");
		if (q->q_result == IP_SUCCESS) {
			ipi_parse_resp(q, vendor_id_table, (caddr_t)c);
			if (c->c_ctype == DKC_UNKNOWN)
				printf("idc%d: unknown string ctlr type\n",
					c->c_ctlr);
		}
		break;

	case IDR_ATTR_CTLR:
	case IDR_ATTR_CTLR1:
		if (q->q_result == IP_SUCCESS) {
			if (id_print_attr)
				ipi_print_resp(IPI_CHAN(q->q_csf), q->q_resp,
					"ctlr attr response");
			ipi_parse_resp(q, &ctlr_conf_table[rp->rec_substate],
				(caddr_t)c);
			rp->rec_substate += rp->rec_data;
			/*
			 * Stay in this state if not done with table.
			 */
			stay_in_state =
				ctlr_conf_table[rp->rec_substate]
				.rt_parm_id;
			if (stay_in_state == 0)
				c->c_flags &= ~IE_RE_INIT;
		}
		break;

	case IDR_ATTR_UNIT:
		if (q->q_result == IP_SUCCESS) {
if (id_print_attr)
	ipi_print_resp(IPI_CHAN(q->q_csf), q->q_resp, "unit attr response");
			ipi_parse_resp(q, &attach_resp_table[rp->rec_substate],
				(caddr_t)un);
			rp->rec_substate += rp->rec_data;
			/*
			 * Stay in this state if not done with table.
			 */
			stay_in_state =
				attach_resp_table[rp->rec_substate].rt_parm_id;
		}
		break;

	case IDR_RETRY:
		if (c->c_retry_q)
			stay_in_state = 1;
		break;

	case IDR_READ_LABEL:
		/*
		 * We used to check the label here, but that has the
		 * unfortunate consequence of examing data returned by
		 * dvma before the dvma mappings are released, but more
		 * importantly before the i/o cache has been flushed.
		 * In certain cases islabel() would determine the label
		 * bad while it was simply that not all the bytes of the
		 * label had made it to memory yet.  Instead of checking here,
		 * we check in the IDR_UNIT_NORMAL and IDR_UNIT_FAILED
		 * cases in id_recover() just before kmem_free() is called
		 * to release the buffer used for the label.  By that point
		 * the i/o cache has been flushed (indirectly by ipi_free()
		 * after the IDR_READ_LABEL case in id_recover()).
		 */
		/*
		if (q->q_result==IP_SUCCESS &&
		    islabel(un->un_unit, un->un_lp))
			uselabel(un, un->un_lp);
		*/
		break;
	}

	nsp = &id_next_state[state];
	if (!stay_in_state) {
		if (q->q_result == IP_SUCCESS)
			next_state = nsp->ns_success;
		else if (rp->rec_history & (1 << state))
			next_state = nsp->ns_failed_again;
		else
			next_state = nsp->ns_failed;
		rp->rec_state = next_state;
		rp->rec_history |= (1 << state);
		rp->rec_substate = 0;
		rp->rec_data = 0;
	} else {
		next_state = state;
	}

	if (un && (un->un_flags & IE_TRACE_RECOV)) {
		printf("id%3x: id_recover_intr %d %s result %x\n",
			un->un_unit, state, nsp->ns_msg, q->q_result);
	} else if (c->c_flags & IE_TRACE_RECOV) {
		printf("idc%d: id_recover_intr %d %s result %x\n",
			c->c_ctlr, state, nsp->ns_msg, q->q_result);
	}
	if (!(q->q_flag & IP_SYNC)) {
		if (q->q_result != IP_MISSING)
			ipi_free(q);		/* return ipiq to free list */
		if (((un ? un->un_flags : c->c_flags) & IE_RECOVER_ACTV) == 0)
			/* run next step of recovery */
			(void) id_recover(c, un);
	}
}

/*
 * XXX - recovery should be hierarchical.  Find a way to let the
 * channel driver do its own recovery depending on it's own capabilities.
 */

/*
 * Recover controller.
 *	Called by IPI layer when the channel has been reset and a controller
 *	reset is required.
 *
 *	The channel may have been reset because a controller on the channel
 *	was having problems, or because another channel on the same board
 *	needed recovery and the entire board was reset.
 *
 *	It may be possible to do a physical-only reset, allowing commands
 *	already in progress to be retained.  However, if the entire board
 *	was reset, a physical and logical reset must be performed, otherwise
 * 	the channel board state will not be consistent with the requests
 *	outstanding.
 */
static void
id_recover_ctlr(c, code)
	struct id_ctlr	*c;
	int		code;	/* XXX - may indicate action eventually */
{
printf("idc%d: id_recover_ctlr\n", c->c_ctlr);
	switch (code) {
	case IPR_REINIT:
	case IPR_RESET:
		c->c_flags |= IE_RE_INIT;
		c->c_flags &= ~ID_C_PRESENT;
		break;
	default:
		printf("idc%d: id_recover_ctlr: bad case %x\n",
			c->c_ctlr, code);
		break;
	}
}

static void
id_retry(q)
	struct ipiq	*q;
{
	struct id_unit	*un;
	struct id_ctlr	*c;
	struct buf	*bp;

	if ((un = (struct id_unit *)q->q_dev) != NULL) {
		un->un_cmd_q++;
	}
	if ((c = (struct id_ctlr *)q->q_ctlr) != NULL) {
		c->c_cmd_q++;
		c->c_cmd_q_len += q->q_cmd->hdr_pktlen + sizeof (u_short);
	}

	/*
	 * Reset residue and error block number, if appropriate.
	 */
	if ((bp = q->q_buf) != NULL) {
		q->q_errblk = (int) (dkblock(bp) +
			un->un_lpart[ID_LPART(bp->b_dev)].un_blkno);
		bp->b_resid = bp->b_bcount - q->q_dma_len;
	}
	ipi_retry(q);
}

/*
 * Queue a q for eventual or immediate retry.
 */
static void
id_q_retry(q)
	struct ipiq	*q;
{
	struct id_ctlr	*c;

	if ((c = (struct id_ctlr *)q->q_ctlr) &&
			(c->c_flags & (IE_RECOVER | IE_RE_INIT)))
	{
		q->q_next = c->c_retry_q;
		c->c_retry_q = q;
	} else {
		id_retry(q);
	}
}


/*
 * Reallocate kmem_alloc-ed area.
 *	Only useful if newsize larger than oldsize.
 *	Works even if oldsize is zero.
 *	Returns non-zero on error, leaving old area allocated.
 */
static int
id_kmem_realloc(pp, oldsize, newsize)
	caddr_t	*pp;		/* pointer to area pointer */
	u_int	oldsize;	/* length of old area */
	u_int	newsize;	/* length of new area */
{
	caddr_t	new;
	caddr_t old;

	if (newsize < oldsize || (new = kmem_alloc(newsize)) == NULL)
		return (-1);
	old = *pp;
	bcopy(old, new, oldsize);
	bzero(new + oldsize, newsize - oldsize);
	*pp = new;
	kmem_free(old, oldsize);
	return (0);
}


/*
 * Update starting block numbers for each of the partitions.
 * This saves converting a cylinder offset to a block offset on each command.
 * Must be called when changing partition map or geometry.
 */
static void
id_update_map(un)
	struct id_unit	*un;
{
	struct un_lpart	*lp;

	for (lp = un->un_lpart; lp < &un->un_lpart[NDKMAP]; lp++) {
		lp->un_blkno = (lp->un_map.dkl_cylno + un->un_g.dkg_bcyl)
			* un->un_g.dkg_nhead * un->un_g.dkg_nsect
			+ un->un_first_block;
	}
}

/*
 * Allocate and initialize read-back buffer for write checking.
 */
static void
id_alloc_wchk()
{
	register struct buf	*bp;

	if (id_wchk_buf == NULL) {
		bp = alloc(struct buf, 1);
		id_wchk_buf = bp;
		bp->b_un.b_addr = (caddr_t)kmem_alloc(ID_WCHK_BUFL);
		bp->b_flags = B_BUSY | B_PHYS | B_READ;
		bp->b_bcount = ID_WCHK_BUFL;
	}
}

/*
 * Do read-back after write.
 *	Called with write request after it has completed.
 *	The read-back request is all set up and ready to go.
 */
static void
id_readback(q)
	struct ipiq	*q;
{
	register struct id_unit	*un = (struct id_unit *)q->q_dev;
	register struct id_ctlr	*c = un->un_ctlr;
	int			dk;
	struct ipiq		*rbq;	/* read-back request */

	rbq = q_related(q);
	/*
	 * Update counts of outstanding commands.
	 */
	un->un_cmd_q++;
	c->c_cmd_q++;
	c->c_cmd_q_len += c->c_rw_cmd_len;

	rbq->q_buf = NULL;	/* so rupt handler doesn't mess with buf */

	/*
	 * Send command.
	 * If the WCHK_READ flag is already on, do a retry, since the command
	 * has been previously issued.  Otherwise, do a start.
	 */
	if (rbq->q_flag & IP_WCHK_READ) {
		id_q_retry(rbq);
	} else {
		rbq->q_flag |= IP_WCHK_READ;
		ipi_start(rbq, (long)0, idintr);
	}

	/*
	 * Update iostat statistics.
	 */
	if ((dk = un->un_dk) >= 0) {
		dk_busy |= 1 << dk;
		dk_seek[dk]++;
		dk_xfer[dk]++;
		dk_read[dk]++;
		dk_wds[dk] += q->q_buf->b_bcount >> 6;
	}
}
