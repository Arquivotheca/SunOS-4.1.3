/*
 * 
 * @(#)sm.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 * 
 *   SCSI-OS (host adapter driver for Emulex ESP-100/100A (NCR 53C90/53C90A)) 
 *
 * COMPILE-DEFINES:
 * DVMA:
 *	Intended to be used together with SUN's new DVMA Gate-array
 * DMAPOLL:
 *	Special version to use POLL mode for DMA
 * P4DVMA:
 *      Special version for a P4 based DVMA/ESP test board, which has the DVMA
 *	and use level 2 interrupt same as the normal SCSI driver
 * P4ESP:
 *      Special version for a P4 based ESP test board, which use pseudo DMA.
 *	ESP's reg_intr has to be polled due to NO other INT pending bit 
 *	(register) available, even it is not recommented to read "reg_intr" 
 *	when ESP's interrupt pin is not active and could result a lost of 
 * 	interrupt because of a timing window.
 *
 * HISTORY:
 *	5/90	JK 	Added bus analyzer function
 *	3/90	KSAM 	Correct default/negotiate sync period; Fix "sm_off()", 
 *			"sm_idle()", "sm_reset()" to match "si.c". (4.2)
 *	11/89	KSAM 	Add slight delay for dma-drain cmd to settle. (4.1-FCS)
 *	8/89	KSAM 	Merge into SunOS 4.1-pre beta release. (4.1-Beta)
 *	7/89	KSAM 	Correct the initalization value of ESP2's reg_conf_2.
 *			(now both "reg_conf_val" & "reg_conf2_val" are global).
 *	6/89	KSAM 	Correct an "race_cond" back to msg_out and cause a panic
 *			due to unit pointer is NULL. (4.1 alpha9-alpha10)
 *	5/89	KSAM 	Add new dequeing algorithm & optical disk support (4.1)
 *	4/89	KSAM	Correct restore_ptr_msg for Conner 100Mb (4.0.3 FCS)
 *	3/89	KSAM	Correct unexpected disk_dump under synchronous (4.0.3)
 *	2/89	KSAM	Add synchnronous and "auto-reassign" support (4.0.3 B-2)
 *	1/89	KSAM	Skip "sd1 & sd3", add "sel_step" protection (4.0.3 Beta)
 *	12/88	KSAM	Add sm_watch() and minor cleanup (4.0.3 alpha3)
 *	11/88	KSAM	Add latest ESP errata support (4.0.3 alpha2)
 *	10/88	KSAM	Add disconnect support (4.0.3 alpha1)
 *	8/88	KSAM	Add Stingray support
 *	7/88	KSAM	Add debug trace and P4/DVMA test board support
 *	6/88	KSAM	Add Hydra support
 *	5/88	KSAM	Add prelimnary synchronous support
 *	4/88	KSAM	Add P4/ESP test board support
 *	3/88	KSAM	Add disconnect support
 *	2/88	KSAM	initial written (based on 4.0 SunOS)
 *
 * XXX: Note, data structures are allocated to only support one sm host
 * 	adapter driver.
 *
 */
#if (defined sun3x) || (defined sun4)
#include "sm.h"
#if NSM > 0

#ifndef lint
static	char sccsid[] = "@(#)sm.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif	lint

#define SMSYNC		/* turn on synchronous support, if scsi_disre_enable=2 */
			/* can be disabled by writing "scsi_disre_enable = 1" */

#ifdef notdef
#define SMINFO		/* turn on statistic info */
#define SMEPRINT	/* print error printf */
#define SMDEBUG		/* turn on debugging code and printfs */
#define DMAPOLL		/* no DVMA */
#define P4DVMA		/* using P4-DVMA board */
#define SMTRACE 	/* turn on trace code */
#define P4ESP		/* P4 ESP special 3/60 test board */
#endif notdef
/*
 * Generic scsi routines.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#ifdef P4DVMA
#include <sys/mman.h>
#endif P4DVMA

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <mon/idprom.h>

#include <sundev/mbvar.h>
#include <sundev/smreg.h>
#include <sundev/scsi.h>

int	sm_probe(), sm_slave(), sm_attach(), sm_go(), sm_done(), sm_poll();
int	sm_ustart(), sm_start(), sm_deque(), sm_watch();
int	sm_off(), sm_cmd(), sm_reset(), sm_dmacnt();
int	sm_cmd_wait(), sm_getstatus();
char	*sm_str_phase(), *sm_str_state(), *sm_str_error();

extern	int nsm, scsi_host_id, scsi_reset_delay, scsi_debug;
extern	int scsi_disre_enable;
extern	struct scsi_ctlr smctlrs[];		/* per controller structs */
extern	struct mb_ctlr *sminfo[];
extern	struct mb_device *sdinfo[];
extern	u_char sc_cdb_size[];
#ifdef P4DVMA
u_long 	*iomap_base;
extern	struct seg	kseg;
#endif P4DVMA

#if	(defined P4ESP || defined P4DVMA)
struct	mb_driver smdriver = {
	sm_probe, sm_slave, sm_attach, sm_go, sm_done, sm_poll, 
	sizeof (struct scsi_sm_reg), "dx", sdinfo, "sm", sminfo, MDR_BIODMA,
};
/* using different disk module "dx" */
#else
struct	mb_driver smdriver = {
	sm_probe, sm_slave, sm_attach, sm_go, sm_done, sm_poll, 
	sizeof (struct scsi_sm_reg), "sd", sdinfo, "sm", sminfo, MDR_BIODMA,
};
#endif	(defined P4ESP || defined P4DVMA)

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr smsubr = {
	sm_ustart, sm_start, sm_done, sm_cmd, sm_getstatus, sm_cmd_wait,
	sm_off, sm_reset, sm_dmacnt, sm_go, sm_deque,
};

extern struct scsi_unit_subr scsi_unit_subr[];
extern int scsi_ntype;

static struct sm_snap sm_info;
struct sm_snap *smsnap = &sm_info;
static struct scsi_sm_reg *esp_reg_addr;
static struct udc_table *dvma_reg_addr;
static int timeout_un = -1;


/* Log state history of activity */
#define SM_LOG_STATE(arg0, arg1, arg2)  {\
	c->c_last_phase = arg0; \
	c->c_phases[c->c_phase_index].phase = arg0; \
	c->c_phases[c->c_phase_index].target = arg1; \
	c->c_phases[c->c_phase_index].lun = arg2; \
	c->c_phase_index = (++c->c_phase_index) & (NPHASE -1); \
}

#ifdef SMSYNC
u_char sync_period[NTARGETS];	
u_char sync_offset[NTARGETS];
u_char fifo_data[14];		
#endif SMSYNC

u_char reg_conf2_val = 0;		/* configure as an older ESP */

u_char sync_target_err = 0;
u_char prev_prt_cnt = 0;
u_char prev_prt_err = 0xff;

#ifdef SMINFO
u_short target_accstate[NTARGETS];
#endif SMINFO
#ifdef SMDEBUG
struct sm_dma_info {
	u_long un_addr;
	u_long un_count;
	u_long un_flag;
	u_long req_addr;
	u_long req_cnt;
	u_long xfrcnt;
	u_long req_dir;
};

static struct sm_dma_info dmainfo[NTARGETS];
int esp_debug = 0; 
#endif SMDEBUG

#define IDM_SUN4_STING		0x23
#define IDM_SUN3X_HYDRA		0x42
#define IDM_SUN4C_CAMPUS	0x50

/* returned value from sm_cmd */
#define OK		0	/* successful */ 
#define FAIL		1	/* failed, may be recoverable */ 
#define HARD_FAIL	2	/* failed, not recoverable */ 
#define NO_MSG		0	/* don't print reset warning message */ 
#define PRINT_MSG	1	/* print reset warning message */ 

#ifdef SMDEBUG
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (scsi_debug > 2) printf(str)
#define DPRINTF1(str, arg1) \
	if (scsi_debug > 3) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (scsi_debug > 4) printf(str,arg1,arg2)
#define DPRINTF3(str, arg1, arg2, arg3) \
	if (scsi_debug > 4) printf(str,arg1,arg2, arg3)
#define DEBUG_DELAY(cnt) \
	if (scsi_debug > 5) DELAY(cnt)
#define EPRINTF(str) \
	if (scsi_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (scsi_debug)  printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (scsi_debug) printf(str,arg1,arg2)
#define EPRINTF3(str, arg1, arg2, arg3)	\
	if (scsi_debug) printf(str,arg1,arg2,arg3)
#define EPRINTF4(str, arg1, arg2, arg3, arg4)	\
	if (scsi_debug) printf(str,arg1,arg2,arg3,arg4)
#else SMDEBUG
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 
#define DPRINTF1(str,arg1) 
#define DPRINTF2(str,arg1,arg2)
#define DPRINTF3(str, arg1, arg2, arg3) 
#ifdef SMEPRINT
#define EPRINTF(str)			printf(str)
#define EPRINTF1(str,arg1)		printf(str,arg1)
#define EPRINTF2(str,arg1,arg2)		printf(str,arg1,arg2)
#define EPRINTF3(str,arg1,arg2,arg3)	printf(str,arg1,arg2,arg3)
#define EPRINTF4(str,arg1,arg2,arg3,arg4) printf(str,arg1,arg2,arg3,arg4)
#else
#define EPRINTF(str) 	
#define EPRINTF1(str,arg1) 
#define EPRINTF2(str,arg1,arg2)
#define EPRINTF3(str,arg1,arg2,arg3)
#define EPRINTF4(str,arg1,arg2,arg3,arg4)
#endif SMEPRINT
#endif SMDEBUG

#ifdef SMTRACE
#define TRACE_CHAR(data) 	sm_store_trace(data)
#define TRBUF_LEN	512 	/* trace buffer stuff  */
int trace_index = 0;
char sm_trace[TRBUF_LEN];


static
sm_store_trace(data)
	char data;
{
	register i = trace_index;

	sm_trace[i] = data;
	if (++i >= TRBUF_LEN)
		i = 0;
	sm_trace[i] = '?';	/* mark end */
	trace_index = i;
}
#else
#define TRACE_CHAR(data)
#endif SMTRACE

/*
 * Print out the cdb.
 */
static
sm_print_cdb(un)
	register struct scsi_unit *un;
{
	register u_char size, i;
	register u_char *cp = (u_char *) &un->un_cdb;

	/* If all else fails, use structure size */
	if ((size = sc_cdb_size[CDB_GROUPID(*cp)]) == 0  &&
	    (size = un->un_cmd_len) == 0)
		size = sizeof (struct scsi_cdb);

	for (i = 0; i < size; i++)
		printf("  %x", *cp++);
	printf("\n");
}


/*
 * returns string corresponding to the smsnap cur_err entry.
 */
static char *
sm_str_error(error)
	register u_char error;
{
	static char *invalid_error = "";
	static char *error_strings[] = ERR_STRING_DATA;

	if (error > SE_LAST_ERROR)
		return(invalid_error);

	return(error_strings[error]);
}


/*
/*
 * returns string corresponding to the state log entry.
 */
static char *
sm_str_state(state)
	register u_char state;
{
	static char *invalid_state = "";
	static char *state_strings[] = STATE_STRING_DATA;

	if (state > STATE_LAST)
		return(invalid_state);

	return(state_strings[state]);
}


/*
 * returns string corresponding to the current phase
 */
static char *
sm_str_phase(phase)
	register u_char phase;
{
	register int index = phase & ESP_PHASE_MASK;
	static char *phase_strings[] = PHASE_STRING_DATA;

	return(phase_strings[index]);
}


/*
 * print out the current hardware state
 */
static
sm_print_state(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_sm_reg *smr;
	register struct scsi_unit *un = c->c_un;
	register struct udc_table *dvma;
	register int	dma_count;
	register short	x, y, z;

#ifdef	SMTRACE
	register int index, trace_data;
	register char *traceptr;
#endif	SMTRACE

	if ((smsnap->cur_err == prev_prt_err)  &&  prev_prt_cnt)  {
		/* If already printed once, don't print the same info. */
		EPRINTF1("sm_prt_state: duplicate error, %x\n", smsnap->cur_err);
		return;
        } else {
		/* Log the error. */
		if (smsnap->cur_err != smsnap->cur_err) {
			prev_prt_cnt = 0;
			prev_prt_err = smsnap->cur_err;
		} else {
			prev_prt_cnt++;
		}
        }
	smr = esp_reg_addr;
        dvma = dvma_reg_addr;
	/*dma_count = GET_ESP_COUNT;*/
	dma_count = (ESP_RD.xcnt_hi <<8) | ESP_RD.xcnt_lo;

	printf("sm%d Snap shot: error= 0x%x (%s error)\n", SMNUM(c), 
		smsnap->cur_err, sm_str_error(smsnap->cur_err));
	printf("    state= 0x%x (%s), sub_state= 0x%x\n",
		smsnap->cur_state, sm_str_state(smsnap->cur_state),
		smsnap->sub_state); 
	printf("    esp cmd= 0x%x, xcnt= %d, intr= 0x%x\n",
		ESP_RD.cmd, dma_count, smsnap->esp_intr);

	x = ESP_RD.fifo_flag;			/* needed for fifo dump */
	if (smsnap->esp_stat != ESP_RD.stat) {
		printf("    fifo_flag= 0x%x, status= 0x%x(0x%x), step= 0x%x\n",
			ESP_RD.fifo_flag, smsnap->esp_stat,
			ESP_RD.stat, ESP_RD.step);
	} else {
		printf("    fifo_flag= 0x%x, status= 0x%x, step= 0x%x\n",
			ESP_RD.fifo_flag, smsnap->esp_stat, ESP_RD.step);
	}
	/* Print esp fifo contents */
	if (x &= KEEP_5BITS) {
		printf("    fifo:");
		while (x--) 
			printf(" %x", ESP_RD.fifo_data);
			printf("\n");
	}

#ifdef	SMINFO
	printf("    disconnected targets= %d  last status= 0x%x  last msg= 0x%x\n",
		smsnap->num_dis_target, smsnap->scsi_status, smsnap->scsi_message);
#endif	SMINFO

	if (un) {

#ifdef		SMSYNC
		if (scsi_disre_enable > 1) {
			printf("    sync periods[0-7]:");
			for (x = 0; x < NTARGETS -1; x++)
				printf(" %d", sync_period[x]);
			printf("\n    sync offsets[0-7]:");
			for (x = 0; x < NTARGETS -1; x++)
				printf(" %d", sync_offset[x]);
			printf("\n");
		}
#endif		SMSYNC

		/* If not in data phase, use dma_curcnt rather than esp xcnt */
		if (c->c_last_phase != STATE_DATA_IN  &&
		    c->c_last_phase != STATE_DATA_OUT)
			dma_count = un->un_dma_curcnt;

		if (dvma->scsi_dmaaddr != un->un_dma_addr) {
			printf("    DVMA status= 0x%x  addr= 0x%x (0x%x)\n",
				dvma->scsi_ctlstat,
				dvma->scsi_dmaaddr, un->un_dma_addr);
		} else {
			printf("    DVMA status= 0x%x  addr= 0x%x\n",
				dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
		}
		printf("    target= %d, lun= %d", un->un_target, un->un_lun);
		printf(", count= %d(%d)\n    cdb:", dma_count, un->un_dma_count);
		sm_print_cdb(un);
	}

	/* Print out state history */
	printf("    current phase= %s\n", sm_str_phase(ESP_RD.stat));
	z = c->c_phase_index;
	for (x = 0; x < NPHASE; x++) {
		z = --z & (NPHASE -1);
		y = c->c_phases[z].phase;
		printf("    last phase= %s", sm_str_state((u_char)y));
		if (c->c_phases[z].target != -1)
			printf("   %d", c->c_phases[z].target);

		if (c->c_phases[z].lun != -1)
			printf("   %d\n", c->c_phases[z].lun);
		else
			printf("\n");
	}

#ifdef	SMINFO
        printf("    acc_state:");
        for (i = 0; i < 7; i++) {
                printf(" %x", target_accstate[i]);
        }
        printf("\n");
#endif	SMINFO

#ifdef	SMTRACE
	printf("\n    Following shows the current SCSI-ESP trace:\n");
	index = 0;
	traceptr = (char *)&sm_trace[index];
	while (++index < TRBUF_LEN) {
		trace_data = (int)(*traceptr++) & KEEP_LBYTE;
		printf(" %x", trace_data);
		if ((index == 0x128) || (index == 0x256) || (index == 0x384))
			DELAY(1000000);
	}
	printf("\n    End of sm_driver debug trace\n");
#endif	SMTRACE
}

/*
 * This routine unlocks this driver when I/O
 * has ceased, and a call to sm_start is
 * needed to start I/O up again.  Refer to
 * xd.c and xy.c for similar routines upon
 * which this routine is based
 */
static
sm_watch(c)
	register struct scsi_ctlr *c;
{
        register struct scsi_unit *un;

	/*
         * Find a valid un to call s?start with to for next command
         * to be queued in case DVMA ran out.  Try the pending que
         * then the disconnect que.  Otherwise, driver will hang
         * if DVMA runs out.
         */
        un = (struct scsi_unit *)c->c_tab.b_actf;
        if (un == NULL) 
	        un = (struct scsi_unit *)c->c_disqtab.b_actf;
	if (un != NULL)
        	sm_start(un);
        timeout(sm_watch, (caddr_t)c, 10*hz);
}


/*****************  BOOT-UP of SM.C ****************************************/
/*
 * Determine existence of SCSI host adapter.
 * Returns ZERO for failure, !ZERO= size of sm data structure if ok.
 */
sm_probe(smr, ctlr)
	register struct scsi_sm_reg *smr;
	int ctlr;
{
	register struct scsi_ctlr *c;
	register int	 i, j;

#ifdef P4DVMA
	struct pte pte;
	u_long 	p_addr;	
#endif P4DVMA

	register struct udc_table *dvma;
	long	dvmastat = 0;

	DPRINTF2("sm_probe: ESP v_base= %x, cpu= %x\n", smr, cpu);
#ifdef P4DVMA
	mmu_getkpte((caddr_t)smr, &pte);
	DPRINTF1("sm_probe: pfnum= %x\n", pte.pg_pfnum);
	p_addr = (u_long)mmu_ptob(pte.pg_pfnum); 
	DPRINTF1("sm_probe: ESP p_base= %x\n", p_addr);
	DEBUG_DELAY(1000000);
	TRACE_CHAR(MARK_PROBE);
	TRACE_CHAR(cpu);
	TRACE_CHAR((char)esp_reg_addr);
	TRACE_CHAR((char)((int)esp_reg_addr >> 8));
	TRACE_CHAR((char)((int)esp_reg_addr >> 16));
	TRACE_CHAR((char)((int)esp_reg_addr >> 24));
#endif P4DVMA
	
	/* for hydra and stingray, the offset of DVMA is 0x1000 from scsi */
	/* controller, which will be in the same virtual page */
	/* hence we could just add offset to the given "smr" */
	switch (cpu) {	/* external variable having machine type */
#ifdef P4DVMA
	    case IDM_SUN3_M25:
		DPRINTF("sm_probe: using sun_3/60 offset\n");
#endif P4DVMA
	    case IDM_SUN4_STING:
		DPRINTF("sm_probe: using Stingray offset\n");
		dvma = (struct udc_table *)((int)smr + STINGRAY_DMA_OFFSET);
		break;
	    case IDM_SUN3X_HYDRA:
		DPRINTF("sm_probe: using Hydra offset\n");
		dvma = (struct udc_table *)((int)smr + HYDRA_DMA_OFFSET);
		break;
	    case IDM_SUN4C_CAMPUS:
		DPRINTF("sm_probe: using sun_4C offset\n");
		/* for now, just add offset, should ask regmap for space */
		dvma = (struct udc_table *)((int)smr + CAMPUS_DMA_OFFSET);
		break;
	    default:
		EPRINTF1("sm_probe: unknown CPU type= %x\n", cpu);
		return(ZERO);
	}
	DPRINTF2("sm_probe: checking DVMA_base= %x, dma_addr= %x\n",
		dvma, &dvma->scsi_dmaaddr);
	DEBUG_DELAY(1000000);
	/* peek to see DVMA exist */
	if ((peekl((long *)&(dvma->scsi_ctlstat), (long *)&(dvmastat))) == -1) {
	   EPRINTF2("sm_probe: peek on DVMA addr= %x, data= %x FAILED\n", 
		&(dvma->scsi_ctlstat), dvmastat);
		return (ZERO);
	}
	dvmastat = dvma->scsi_ctlstat;
	DPRINTF2("sm_probe: DVMA exists, vm_addr= %x, stat= %x\n", 
		dvma, dvmastat);
	TRACE_CHAR((char)dvmastat);
	TRACE_CHAR((char)(dvmastat >> 8));
	TRACE_CHAR((char)(dvmastat >> 16));
	TRACE_CHAR((char)(dvmastat >> 24));
	if (dvmastat & DVMA_REQPEND) {
		EPRINTF1("sm_probe: DVMA req_pend stuck, stat= %x\n", 
			dvmastat);
		return (ZERO);
	}
	if (dvmastat & DVMA_RESET) {
		DPRINTF1("sm_probe: DVMA reset asserted, stat= %x\n", 
			dvmastat);
		/* attempt to clear it, if not successful, error return */
		dvma->scsi_ctlstat = 0;
		DELAY(1000);
		if (dvma->scsi_ctlstat & DVMA_RESET) {
		   EPRINTF1("sm_probe: DVMA reset stuck, stat= %x\n", 
				dvma->scsi_ctlstat);
		   return(ZERO);
		}
		DPRINTF1("sm_probe: deasserted DVMA reset, stat= %x\n", 
			dvma->scsi_ctlstat);
	}
	/* write/read/compare DVMA 's address reg */
	i = DVMA_TPATTERN;
	dvma->scsi_dmaaddr = i;
	DPRINTF2("sm_probe: DVMA test, wr= %x, rd= %x\n",
		i, dvma->scsi_dmaaddr); 
	if (dvma->scsi_dmaaddr != i) {
		EPRINTF2("sm_probe: DVMA failed, wr= %x, rd= %x\n", 
			i, dvma->scsi_dmaaddr);
		return (ZERO);
	}
	dvma->scsi_dmaaddr = NULL;
	DPRINTF2("sm_probe: DVMA PASSED, stat= %x, addr= %x\n",
		dvma->scsi_ctlstat, dvma->scsi_dmaaddr); 
	DEBUG_DELAY(1000000);
	
	/* check existence and operational status of DVMA before */
	/* checking on ESP.  If scsi_reset in DVMA is asserted, the */
	/* ESP register will not be accessible and DVMA itself is */
	/* NOT in a operational state */ 
	c = &smctlrs[ctlr];
	/* Check ESP existence -- peek on "ESP reg_xcnt_lo" */
	if ((peekc((char *)&(ESP_RD.xcnt_lo))) == -1) {
		EPRINTF1("sm_probe: peek on ESP's addr= %x FAILED\n",
			&ESP_RD.xcnt_lo);
		DEBUG_DELAY(1000000);
		return (ZERO);
	}
	DPRINTF("sm_probe: ESP exist\n");
	
	/* perform a simple write, read and compare validation test to
	   "ESP reg_conf".  If error, return(ZERO) */
	i = 0x55;
	ESP_WR.conf = i;
	j = ESP_RD.conf;
	DPRINTF2("sm_probe: ESP wr= %x, rd= %x\n", i, j);
	if ((u_char)i != (u_char)j) {
		DEBUG_DELAY(1000000);
		return (ZERO);
	}
	i = 0xaa;
	ESP_WR.conf = i;
	j = ESP_RD.conf;
	DPRINTF2("sm_probe: ESP wr: = %x, rd= %x\n", i, j);
	if ((u_char)i != (u_char)j) {
		DEBUG_DELAY(1000000);
		return (ZERO);
	}
	i = 0;
	ESP_WR.conf = i;
	j = ESP_RD.conf;
	if ((u_char)i != (u_char)j)  {
		DPRINTF2("sm_probe: ESP FAILED, wr= %x, rd= %x\n", i, j);
		DEBUG_DELAY(1000000);
		return (ZERO);
	}
	DPRINTF("sm_probe: ESP testing of reg_conf PASSED\n");

	/* As of now, ESP mostly on board, not VME */
	c->c_flags = SCSI_ONBOARD; 

	/* Perform a simple write, read & compare validation test to ESP_II */
	i = 0x1f;	
	ESP_WR.conf2 = (u_char)i;	/* write */
	j = ESP_RD.conf2;		/* read */ 
#ifdef SMINFO
	EPRINTF2("sm_probe: checking ESP2, i= %x, j= %x\n", i, j);
#endif SMINFO
	if (i == (j & 0x1f)) {	/* compare-only 5 bits are readable */
		ESP_WR.conf2 = reg_conf2_val;	
		c->c_flags |= SCSI_ESP2;
#ifdef SMINFO
		EPRINTF1("sm_probe: ESP2 found, conf_2 set to %\x\n",
			ESP_RD.conf2);
#endif SMINFO
	} else {
		ESP_WR.conf2 = 0;	/* clear it, since most bit are reserved */
#ifdef SMINFO
		EPRINTF1("sm_probe: ESP2 not found, conf_2 set to %x\n", 
			ESP_RD.conf2);
#endif SMINFO
		c->c_flags |= SCSI_ESP;
	}	
	/* Initialize controller (c) structure */
	if (scsi_disre_enable) {
		DPRINTF("sm_probe:  disconnect ENABLED\n");
		c->c_flags |= SCSI_EN_DISCON;
	} else {
		c->c_flags &= ~SCSI_EN_DISCON;
		DPRINTF("sm_probe:  disconnect DISABLED\n");
	}

#ifdef P4DVMA	
	get_iomapbase();
#endif P4DVMA
	DEBUG_DELAY(1000000);

	/* Initialize last state log */
	for (i = 0; i < NPHASE; i++) {
		c->c_phases[i].phase = STATE_FREE;
		c->c_phases[i].target = -1;
		c->c_phases[i].lun = -1;
	}
	c->c_phase_index = 0;

	c->c_flags |= SCSI_PRESENT;
	c->c_reg = (int)smr;	/* store ESP register pointer */
	c->c_ss = &smsubr;	/* set high level subroutine pointer */
	c->c_name = "sm";	/* store this driver's name */
	c->c_tab.b_actf = NULL;
	c->c_tab.b_active = C_INACTIVE;

	esp_reg_addr = smr;
        dvma_reg_addr = dvma;
	smsnap->cur_retry = RESET_ALL_SCSI;
        sm_reset(c, NO_MSG);
	smsnap->cur_state = STATE_FREE;
	c->c_un = NULL;
        timeout(sm_watch, (caddr_t)c, 10*hz);
	DPRINTF1("sm_probe:  returning size= %x\n", 
		sizeof(struct scsi_sm_reg));
	TRACE_CHAR(MARK_PROBE);
	DEBUG_DELAY(1000000);
	return (sizeof (struct scsi_sm_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
sm_slave(md, smr)
	register struct mb_device *md;
	struct scsi_sm_reg *smr;
{
	register struct scsi_unit *un;
	register int type;

#ifdef 	lint
	smr = smr;	/* for compatiblity with all existing sX.c */
#endif	lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype)
		panic("sm_slave:  unknown type in md_flags\n");

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == NULL)
		panic("sm_slave:  md_flags scsi type not configured\n");

	un->un_c = &smctlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}


/*
 * Attach device (boot time).
 */
sm_attach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct mb_ctlr *mc = md->md_mc;
        register struct scsi_ctlr *c = &smctlrs[md->md_ctlr];

	DPRINTF1("sm_attach: type= %x\n", type); 
	if (type >= scsi_ntype)
		panic("sm_attach:  unknown type in md_flags\n");

	c->c_intpri = mc->mc_intpri;

	/* Call top-level driver s?attach routine */
	(*scsi_unit_subr[type].ss_attach)(md);
	
	/* As this is on-board, there is nothing further do do. */
}


/*
 * corresponding to un is an md.  if this md has SCSI commands
 * queued up (md->md_utab.b_actf != NULL) and md is currently
 * not active (md->md_utab.b_active == ZERO), this routine queues the
 * "un" on its queue of devices (c->c_tab) for which to run commands
 * Note that md is active if its un is already queued up on c->c_tab,
 * or md is on the scsi_ctlr's disconnect queue c->c_disqtab.
 */
sm_ustart(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct mb_device *md = un->un_md;
	int s;

	/* upper level had called diskort() to insert bp into unit md */
	
	DPRINTF("sm_ustart:\n");
	s = splr(pritospl(c->c_intpri));
	if ((md->md_utab.b_actf != NULL) && 
	    (md->md_utab.b_active == MD_INACTIVE)) {
		/* unit has at least one job before it got put on active list */
		if (c->c_tab.b_actf == NULL) 
			c->c_tab.b_actf = (struct buf *)un;
		else 
			((struct scsi_unit *)(c->c_tab.b_actl))->un_forw = un;
		
		un->un_forw = NULL;
		c->c_tab.b_actl = (struct buf *)un;
		md->md_utab.b_active |= MD_QUEUED;
	}
	(void)splx(s);
}

/*
 * this un is removed from the active
 * position of the scsi_ctlr's queue of devices (c->c_tab.b_actf) and
 * queued on scsi_ctlr's disconnect queue (c->c_disqtab).
 */
sm_discon_queue(un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	/*DPRINTF1("sm_discon_queue:\n");*/

	/* a disconnect has occurred, so mb_ctlr is no longer active */
	c->c_tab.b_active &= C_QUEUED;
	c->c_un = NULL;
	
	/* remove disconnected unit-md from controller active queue */
	if ((c->c_tab.b_actf = (struct buf *)un->un_forw) == NULL)
		c->c_tab.b_actl = NULL;
	
	/* put unit-md on controller's disconnect queue */
	if (c->c_disqtab.b_actf == NULL)
		c->c_disqtab.b_actf = (struct buf *)un;
	else
		((struct scsi_unit *)(c->c_disqtab.b_actl))->un_forw = un;

	c->c_disqtab.b_actl = (struct buf *)un;
	un->un_forw = NULL;
}

/*
 * searches the scsi_ctlr's disconnect queue for an md whose
 * corresponding un is the scsi unit defined by (target,lun).
 * then requeues this md on mc_ctlr's queue of devices.
 */
sm_recon_queue(c, target, lun, flag)
	register struct scsi_ctlr *c;
	register short target, lun, flag;
{
	register struct scsi_unit *un, *pun;
	/*DPRINTF("sm_recon_queue:\n");*/

	/* search the disconnect queue */
	for (un=(struct scsi_unit *)(c->c_disqtab.b_actf), pun=NULL; un;
	     pun=un, un=un->un_forw) {
		if ((un->un_target == target) && (un->un_lun == lun))
			break;
	}
	/*
	 * Could not find the device in the disconnect queue, die!
	 * Note, if flag set, just return fail as we're trying
	 * to figure out if we've started the request or not.
	 */
	if (un == NULL) {
		if (flag) 
			return (FAIL);

		/* dump out disconnnect queue */
		printf("sm%d:  sm_recon_queue: can't find target= %d, lun= %d\n",
			SMNUM(c), target, lun);
		printf("  disconnect queue:\n");
		for (un = (struct scsi_unit *)(c->c_disqtab.b_actf), pun = NULL;
		     un; pun = un, un = un->un_forw) {
			printf("\tun_target= %d, lun= %d\n",
				un->un_target, un->un_lun);
		}
		return (FAIL);
	}
	/*
         * If inhibit reconnect flag set, exit.  This is used by deque
         * to determine whether to reset everything or ignore timeout.
         */
        if (flag)
                return(OK);

	/* remove un from the disconnect queue */
	if (un == (struct scsi_unit *)(c->c_disqtab.b_actf))
		c->c_disqtab.b_actf = (struct buf *)(un->un_forw);
	else
		pun->un_forw = un->un_forw;
	
	if (un == (struct scsi_unit *)c->c_disqtab.b_actl)
		c->c_disqtab.b_actl = (struct buf *)pun;
   
	/* requeue un at the active position of scsi_ctlr's queue of devices */
	if (c->c_tab.b_actf == NULL)  {
		un->un_forw = NULL;
		c->c_tab.b_actf = c->c_tab.b_actl = (struct buf *)un;
	} else {
		un->un_forw = (struct scsi_unit *)(c->c_tab.b_actf);
		c->c_tab.b_actf = (struct buf *)un;
	}
	
	/* scsi_ctlr now has an actively running SCSI command */
	c->c_tab.b_active |= C_ACTIVE;
	c->c_un = un;

	/* If finished flushing disconnect que, clear interlock */
	if (c->c_disqtab.b_actf == NULL  &&  (c->c_tab.b_active & C_FLUSHING)) {
		c->c_tab.b_active &= ~C_FLUSHING;
		printf("sm%d:  warning, scsi bus saturated\n", SMNUM(c));
	}
	return(OK);
}


/* starts the next SCSI command */
sm_start(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c;
	register struct mb_device *md;
	register struct buf *bp;
	int s;
	
	TRACE_CHAR(MARK_START);
	/*DPRINTF("sm_start:\n");*/

	 /*return immediately if passed NULL un */
        if (un == NULL) {
		EPRINTF("sm_start: un NULL\n");
                return;
	}
	/*
	 * return immediately, if the ctlr is actively running a SCSI cmd.
	 * Only start a new job if nothing is active or reconnecting.
	 */
	c = un->un_c;
	s = splr(pritospl(c->c_intpri));
	if (c->c_tab.b_active != C_INACTIVE) {
		DPRINTF1("sm_start: locked (0x%x)\n", c->c_tab.b_active);
		(void)splx(s);
		return;
	}
	/*
	 * if there are currently no SCSI devices queued to run
	 * a command, then simply return.  otherwise, obtain the
	 * next md for which a command should be run.
	 */
	if ((un=(struct scsi_unit *)(c->c_tab.b_actf)) == NULL) {
		 DPRINTF("sm_start: no device to run\n");
		 (void)splx(s);
		 return;
	}
	md = un->un_md;
	c->c_tab.b_active |= C_QUEUED;
	/*
         * if an attempt was already made to run this command, but the
         * attempt was pre-empted by a SCSI bus reselection then DVMA
         * has already been set up, and we can call sigo directly.
         */
	if (md->md_utab.b_active & MD_PREEMPT) {
		/*DPRINTF("sm_start: starting preempted\n");*/
	      	c->c_un = un;
	        sm_go(md);
		goto START_RTN;
	}
	if (md->md_utab.b_active & MD_IN_PROGRESS) {
		/*DPRINTF1("sm_start: md in progress\n");*/
		goto START_RTN;
	}
	md->md_utab.b_active |= MD_IN_PROGRESS;
	bp = md->md_utab.b_actf;	/* put bp into the active position */
	md->md_utab.b_forw = bp;

	/* Only happens when called by sm_intr */
	if (bp == NULL) {
		DPRINTF("sm_start: bp is NULL\n");
		goto START_RTN;
	}
	/*
	 * special commands which are initiated by the high-level driver, 
	 * are run using its special buffer, un->un_sbuf. In most cases, 
	 * main bus set-up has already been done, so sm_go() can be called
	 * for on-line formatting, we need to call mbsetup.
	 */
	if ((*un->un_ss->ss_start)(bp, un)) {
		c->c_un = un;
		if ((bp == &un->un_sbuf) &&
		    ((un->un_flags & SC_UNF_DVMA) == 0) &&
		    ((un->un_flags & SC_UNF_SPECIAL_DVMA) == 0)) {
			sm_go(md);
		} else { 
			if (mbugo(md) == 0)
				md->md_utab.b_active |= MD_NODVMA;
      		}
	} else {
		/* error from upper level, release md */
		sm_done(md);
	}

START_RTN:
	c->c_tab.b_active &= C_ACTIVE;
	(void)splx(s);
}



/*
 * Start up a scsi operation.
 */
sm_go(md)
	register struct mb_device *md;
{
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	register int i;		/* for disk info */
	int s, type;
	/*DPRINTF("sm_go:\n"); */

	if (md == NULL)
		panic("sm_go: queueing error1\n");
	
	c = &smctlrs[md->md_mc->mc_ctlr];
	s = splr(pritospl(c->c_intpri));
	
	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);
	
	bp = md->md_utab.b_forw;
	if (bp == NULL) {
		EPRINTF("sm_go: bp is NULL\n");
		(void)splx(s);
		return;
	}
	un->un_baddr = MBI_ADDR(md->md_mbinfo);

	if (md->md_utab.b_active & MD_NODVMA) {
		md->md_utab.b_active &= ~MD_NODVMA;
		md->md_utab.b_active |= MD_PREEMPT;
		(*un->un_ss->ss_mkcdb)(un);
		(void)splx(s);
		return;
	}
	c->c_tab.b_active |= C_QUEUED;	
	
	/* Diddle stats if necessary */
	if ((i = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<i;
		dk_xfer[i]++;
		if (bp->b_flags & B_READ)
			dk_read[i]++;
		dk_wds[i] += bp->b_bcount >> 6;
	}
	/*
	 * Make the command block and fire it up in interrupt mode.
	 * If it fails right off the bat, call the interrupt routine 
	 * to handle the failure.
	 */
	if (md->md_utab.b_active & MD_PREEMPT)
		md->md_utab.b_active &= ~MD_PREEMPT;
	else
		(*un->un_ss->ss_mkcdb)(un);
	
	if ((i=sm_cmd(c, un, 1)) != OK) {
		TRACE_CHAR((char)i);
		switch (i) {
		case FAIL:
			DPRINTF("sm_go: sm_cmd FAILED\n"); 
			(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
			break;
		case HARD_FAIL:
		default:
			DPRINTF("sm_go: sm_cmd HARD FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
			break;
		}
	}
	c->c_tab.b_active &= C_ACTIVE;
	(void)splx(s);
}


/* the SCSI command is done, so start up the next command
 */
sm_done(md)
	register struct mb_device *md;
{
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	int type;
	int s;


	TRACE_CHAR(MARK_DONE);
	DPRINTF("sm_done:\n");
	c = &smctlrs[md->md_mc->mc_ctlr];
	s = splr(pritospl(c->c_intpri));
	bp = md->md_utab.b_forw;
	
	type = TYPE(md->md_flags);

	/* more reliable than un= c->c_un */
	/* since c->c_un could be NULL from disconnect */
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);
	
	/* advance mb_device queue */
	md->md_utab.b_actf = bp->av_forw;
	
	/* we are done, so clear buf in active position of 
	 * md's queue. then call iodone to complete i/o
	 */
	md->md_utab.b_forw = NULL;
	
	/* just got done with i/o so mark ctlr inactive 
	 * then advance to next md on the ctlr
	 */
	c->c_tab.b_actf = (struct buf *)(un->un_forw);
	
	/* advancing the ctlr's queue has removed md from 
	 * the queue.  if there are any more i/o for this 
	 * md, sm_ustart will queue up md again (at tail). 
	 * first, need to mark md as inactive (not on queue)
	 */
	DPRINTF1("sm_done: md_utab= %x\n", md->md_utab.b_active);
	md->md_utab.b_active = MD_INACTIVE;
	sm_ustart(un);	/* requeue unit again if job pending */
	iodone(bp);

	/* start up the next command on the scsi_ctlr */
	sm_start(un);
	(void)splx(s);
	TRACE_CHAR(MARK_DONE);
}

/*
 * Bring a unit offline.  Note, if unit already offline, don't print anything.
 * If flag = SE_FATAL, take device offline.
 */
sm_off(c, un, flag)
register struct scsi_ctlr *c;
register struct scsi_unit *un;
int flag;
{
	struct mb_device *md = un->un_md;
	char *msg = "online";

	if (un->un_present) { 
		if (flag == SE_FATAL) { 
			msg = "offline"; 
			un->un_present = 0; 
		} 
		printf("sm%d:  %s%d, unit %s\n", SMNUM(c), 
			scsi_unit_subr[md->md_flags].ss_devname, 
			un->un_unit, msg); 
	}
}

/*
 * Remove timed-out I/O request and report error to
 * it's interrupt handler.
 * Return OK if sucessful, FAIL if not.
 */
sm_deque(c, un)
register struct scsi_ctlr *c;
register struct scsi_unit *un;
{
	struct udc_table *dvma = dvma_reg_addr;
	int 	s;
	/*DPRINTF("sm_deque:\n");*/

	/* Lock out the rest of sm till we've finished the dirty work. */
	s = splr(pritospl(c->c_intpri));

	/*
	 * If current SCSI I/O request is the one that timed out,
	 * Reset the SCSI bus as this looks serious.
	 */
	if (un == c->c_un) {
		if (dvma->scsi_ctlstat & DVMA_INT_MASK) {	
		 	(void) splx(s);

			/* Hardware lost int., attempt to continue... */
			printf("sm%d:  sm_deque: lost interrupt\n", SMNUM(c));
                        sm_print_state(c);
                        sm_intr(c);
			return (OK);

                } else {
			/*
			 * Really did timeout.
			 * If things died in the middle of data phase,
			 * disable sync mode for this target.
			 */
			if ((c->c_last_phase == STATE_DATA_IN    ||
			     c->c_last_phase == STATE_DATA_OUT)  &&
			    !(sync_target_err & (1 <<un->un_target))) {
				sync_target_err |= (1 << un->un_target);
				printf("sm%d:  target %d reverting to async operation\n",
					SMNUM(c), un->un_target);
			}
			/* If no error to date, report timeout error. */
			if (smsnap->cur_err == SE_NO_ERROR)
				smsnap->cur_err = SE_TOUTERR;

                        (void) splx(s);
			return (FAIL);
                }
 	}
 	if (c->c_tab.b_actf == NULL) {
                /*
		 * No active cmd running, search for timed out request
		 * on disconnect queue.  If not found, we're still
		 * waiting to run it.
		 */
                if (sm_recon_queue(c, un->un_target, un->un_lun, 0)) {
                        (void) splx(s);

			/* still waiting to run,  restart timeout */
			if (scsi_debug) {
				printf("sm%d:  I/O request still waiting to start\n",
					SMNUM(c));
	                	sm_print_state(c);
			}
			return (OK);
                }
                (void) splx(s);

		/* If no error to date, report timeout error. */
		if (smsnap->cur_err == SE_NO_ERROR)
			smsnap->cur_err = SE_TOUTERR;

		/* really did timeout in disconnect que,  reset */
                (*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
                c->c_tab.b_active &= C_QUEUED;          /* clear interlock */
		return (FAIL);

        } else {
                /*
		 * Another request is active. If timed out request is
		 * still waiting to run, restart timeout.  If
		 * it's already disconnected, things get interesting.
		 * In this case we may have saturated the bus so we stop
		 * sending further commands and wait for the disconnect
		 * que to flush.  If it doesn't clear, the device really
		 * timed out.
		 */
                if (sm_recon_queue(c, un->un_target, un->un_lun, 1)) {
                        (void) splx(s);

			/* died in active que,  restart timeout */
			if (scsi_debug) {
				printf("sm%d:  I/O request still waiting to start\n",
					SMNUM(c));
	                	sm_print_state(c);
			}
			return (OK);
                }
		if (! (c->c_tab.b_active & C_FLUSHING)) {
			/* flush disconnect que and see what happens... */
			timeout_un = (int)un;
			c->c_tab.b_active |= C_FLUSHING;
			(void) splx(s);
			printf("sm%d:  draining disconnect que\n", SMNUM(c));
			return (OK);
		}
		/*
		 * If another device timed out while we're draining
		 * the disconnect que, ignore it.  If the one we're
		 * timing out on times out again, die!
		 */
		if (timeout_un != (int)un  &&  (c->c_tab.b_active & C_FLUSHING)) {
			(void) splx(s);
			printf("sm%d:  drain in progress, restarting timeout\n",
				 SMNUM(c));
			return (OK);
		}
		printf("sm%d:  disconnect que drain failed\n", SMNUM(c));
		timeout_un = -1;
		c->c_tab.b_active &= ~C_FLUSHING;

		/* If no error to date, report timeout error. */
		if (smsnap->cur_err == SE_NO_ERROR)
			smsnap->cur_err = SE_TOUTERR;

		/* died in disconnect que,  reset */
		(void) splx(s);
		return (FAIL);
        }
}
 

/************ HARDWARE DEPENDENT PORTIONS ************************************/
/*
 * Pass a command to the SCSI bus.
 * OK if fully successful, 
 * Return FAIL (1) on failure (may be retryable),
 * HARD_FAIL (2) if we failed due to major hardware problem
 */
sm_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr; /* if 0, run cmd to completion in polled mode
			    * if 1, allow disconnects if enabled and use ints */
{
	struct mb_device *md = un->un_md;
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register int i;
	register u_char *cp;
	int s, l;

	smr = esp_reg_addr;
        dvma = dvma_reg_addr;

	TRACE_CHAR(MARK_CMD);
	DPRINTF2("sm_cmd: intr= %x, state= %x\n", intr, smsnap->cur_state);
	DPRINTF1("sm_cmd: active= %x\n", c->c_tab.b_active);
	DPRINTF2("sm_cmd: dmaaddr= %x, cnt= %x\n", 
		un->un_dma_addr, un->un_dma_count);
	
	s = splr(pritospl(c->c_intpri));
	if ((intr == 1) && (c->c_tab.b_active >= C_ACTIVE)) {
		/* if controller just reconnected, preempt this job */
		md->md_utab.b_active |= MD_PREEMPT;
		DPRINTF1("sm_cmd:  current locked out, active= %x\n",
			md->md_utab.b_active);
		(void)splx(s);
		DEBUG_DELAY(1000000);
		return (OK);
	}
#ifdef notdef
	if ((smsnap->cur_state != STATE_FREE) &&
	    (smsnap->cur_state != STATE_ENRSEL_REQ)) {
		EPRINTF2("sm_cmd: state= %x locked out, stat= %x\n",
			smsnap->cur_state, smsnap->esp_stat);
		md->md_utab.b_active |= MD_PREEMPT;
		(void)splx(s);
		DEBUG_DELAY(1000000);
		return (OK);
	}
#endif notdef
	c->c_tab.b_active |= C_ACTIVE;	
	(void)splx(s);
	if (! intr) { 		/* POLL mode */
		/* disable ESP's INT, but DVMA could be enabled later */
		c->c_flags &= ~SCSI_EN_DISCON;
		dvma->scsi_ctlstat &= ~DVMA_INTEN;
	} else {
		/*
		 * If disconnect/reconnect globally disabled or only
		 * disabled for this command set internal flag.
		 * Otherwise, we enable disconnects and reconnects.
		 */
		if (((scsi_disre_enable) == 0) || 
		     (un->un_flags & SC_UNF_NO_DISCON)) 
		     	c->c_flags &= ~SCSI_EN_DISCON;
		else  
			c->c_flags |= SCSI_EN_DISCON;
		dvma->scsi_ctlstat |= DVMA_INTEN;
		un->un_wantint = 1;
	}
	
	if (c->c_flags & SCSI_EN_DISCON) {
		/* fill in the identify message before cmd bytes */
		ESP_WR.fifo_data = DISCON_MSG | (un->un_lun & KEEP_3BITS); 
	}
	/* Determine size of the cdb.  Since we're smart, we look at group code 
	 * and guess.  If we don't recognize the group id, we use the specified
	 * cdb length.  If both are zero, use max. size of data structure.
	 */
	if (((s = sc_cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) == 0) && 
	   ((s = un->un_cmd_len) == 0)) {
		s = (int)sizeof(struct scsi_cdb);
		DPRINTF2("sm%d: invalid cdb size, using size= %d\n", 
			SMNUM(c), sizeof (struct scsi_cdb));
	}
	cp = (u_char *)&un->un_cdb.cmd;
	/* Write "ESP reg_fifo" with all command bytes */
	DPRINTF("sm_cmd: cmd byte: ");
	for (i = 0; (u_char)i < (u_char)s; i++)  {
		DPRINTF1(" %x ", *cp);
		ESP_WR.fifo_data = *cp++;
	}
#ifdef notdef
	if (smsnap->cur_state == STATE_DSRSEL_DONE) {
  		ESP_WR.cmd = CMD_EN_RESEL;
		DPRINTF("sm_cmd: re-enable resel cmd\n");
	}
#endif notdef
	DPRINTF1("sm_cmd: un_flag= %x\n", un->un_flags);
	DEBUG_DELAY(1000000);
	
	smsnap->cur_err = smsnap->scsi_status = SE_NO_ERROR; 
	smsnap->cur_state = STATE_SEL_REQ;
	smsnap->cur_target = un->un_target;
#ifdef	SMINFO
        target_accstate[MAXID(un->un_target)] = TAR_START;
#endif	SMINFO

#ifdef	SMDEBUG
	dmainfo[MAXID(un->un_target)].un_addr = un->un_dma_addr;
	dmainfo[MAXID(un->un_target)].un_count = un->un_dma_count;
	dmainfo[MAXID(un->un_target)].un_flag = un->un_flags;
#endif	SMDEBUG
	DEBUG_DELAY(1000000);
	ESP_WR.busid = un->un_target;	/* dest-ID= write only 3 bit encoded */
	ESP_WR.timeout = DEF_TIMEOUT;
#ifdef	SMSYNC
	if (sync_offset[un->un_target]) {

		/* If disk changed, force synch negotiation on new one. */ 
		if (un->un_present == 0) {
                	EPRINTF1("sm_intr: target %d, clearing sync known\n",
				un->un_target);
			smsnap->sync_known &= ~(1 << un->un_target);
			sync_period[un->un_target] = 0;
			sync_offset[un->un_target] = 0;
		}
        	ESP_WR.sync_period = sync_period[un->un_target];
                ESP_WR.sync_offset = sync_offset[un->un_target];
                DPRINTF1("sm_intr: set sync on target= %d\n", un->un_target);
	   	DPRINTF2("         sync_period= %d, offset= %d\n",
	                sync_period[un->un_target], sync_offset[un->un_target]);
        } else {
             	DPRINTF("sm_intr: no sync\n");
                ESP_WR.sync_period = 0;
                ESP_WR.sync_offset = 0;
	}
#endif	SMSYNC

	/* update ESP sync regs with previous values */
	if (c->c_flags & SCSI_EN_DISCON) {
		DPRINTF1("\nsm_cmd: sending SEL_ATN cmd to id: %x\n", 
			un->un_target);
		DEBUG_DELAY(1000000);

#ifdef		SMSYNC
		/*
		 * If target is there and we haven't checked if it can
		 * do synchronous (assuming it's enabled), do so now.
		 * If this is first time we've talked to it, wait until
		 * the next time to eliminate confusion as whether it's
		 * there or not.
		 */
		if ((smsnap->sync_known & (1 << un->un_target)) == 0 &&
		    un->un_present  &&  scsi_disre_enable > 1) {
			DPRINTF1("sm_cmd: start sync_chking on target= %x\n",
				un->un_target);
			smsnap->cur_state = STATE_SYNCHK_REQ;
                    	ESP_WR.cmd = CMD_SEL_STOP;
		} else {	
              		ESP_WR.cmd = CMD_SEL_ATN;
		}
#else		SMSYNC
		ESP_WR.cmd = CMD_SEL_ATN;
#endif		SMSYNC
	} else { 
		DPRINTF1("\nsm_cmd: sending SEL_NO_ATN cmd to id: %x\n", 
			un->un_target);
		DEBUG_DELAY(1000000);
		ESP_WR.cmd = CMD_SEL_NOATN;
	}
	SM_LOG_STATE(STATE_ARBITRATE, un->un_target, un->un_lun);

#ifdef	SMDEBUG
	if (esp_debug) {
		EPRINTF2("sm_cmd: sending cmd= %x to target= %x\n",
			un->un_cdb.cmd, un->un_target);
	}
#endif	SMDEBUG
	/* ESP will interrupt if selection timed out */
	/* Xfer set up during data phase */
	if (intr)
		return(OK);

	DPRINTF("sm_cmd: POLL mode\n");
	TRACE_CHAR(MARK_CMD + 1);
	l = LOOP_CMDTOUT;		/* relative short timeout */
	while ((--l) && (smsnap->cur_err == SE_NO_ERROR) &&
	       (smsnap->cur_state != STATE_FREE)) {
		if (sm_poll())	/* re-allow timeout for any interrupt */
			l = LOOP_CMDTOUT; 
	}
	DPRINTF2("sm_cmd: POLL done, state= %x, err= %x\n", 
		smsnap->cur_state, smsnap->cur_err); 
	DEBUG_DELAY(1000000);
	/* returned check condition status during POLL mode (open time) */
	cp = (u_char *)&un->un_scb;
	cp[0] = smsnap->scsi_status;	/* store the bus status */
	DPRINTF2("sm_cmd: scsi_status= %x, dma_stat= %x\n", 
		smsnap->scsi_status, dvma->scsi_ctlstat); 
	TRACE_CHAR(smsnap->scsi_status);
	TRACE_CHAR(dvma->scsi_ctlstat);
	TRACE_CHAR(MARK_CMD);
	DEBUG_DELAY(1000000);
	if (l == 0) {
                i = HARD_FAIL;
        } else {
            switch (smsnap->cur_err) {
                case SE_NO_ERROR:
                    i = OK;		/* 1 */
                    break;
                case SE_RETRYABLE:
                    i = FAIL;		/* 2 */
                    break;
                case SE_FATAL:		/* 3 */
                default:
                    DPRINTF2("sm_cmd: REQ failed, err= %x, target= %x\n",
                        smsnap->cur_err, un->un_target);
                    DPRINTF2("sm_cmd: CMD= %x, state= %x\n",
                        un->un_cdb.cmd, smsnap->cur_state);
                    DPRINTF2("sm_cmd: ESP_stat= %x, scsi_stat= %x\n",
                        smsnap->esp_stat, smsnap->scsi_status);
		    DEBUG_DELAY(1000000);
                    i = HARD_FAIL;
             }
        }
	c->c_tab.b_active &= C_QUEUED; /* leave active bit cleared by sm_start() */
        DPRINTF1("sm_cmd: return= %x\n", i);
        return(i); 
}


/*
 * Set up the SCSI-ESP and DVMA logic for a dma transfer
 * only called within sm_intr's data_phase
 */
static
sm_dma_setup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
#ifdef SMTRACE
	u_char *ptr;
	int  i;
#endif SMTRACE

 	un = c->c_un;
	TRACE_CHAR(MARK_SETUP);
	TRACE_CHAR(un->un_dma_count);
	if (un->un_dma_count == 0) {
		un->un_dma_curcnt = 0;
		un->un_dma_curdir = SM_NO_DATA;
		DPRINTF("sm_dma_setup: NO data_xfr needed\n");
		return;
	}

	/* ensure the data buffer is within the DVMA space */
	if (un->un_dma_addr < (u_int)DVMA) {
		un->un_dma_addr += (u_int)DVMA;
		DPRINTF2("sm_dma_setup: new_addr: %x, DVMA= %x\n", 
			un->un_dma_addr, (u_int)DVMA);
	}

	un->un_dma_curaddr = un->un_dma_addr;
	un->un_dma_curcnt = un->un_dma_count;
#ifdef SMDEBUG
	if (((un->un_dma_curcnt & 0xf) == 0xf) ||
   	    ((un->un_dma_curaddr & 0xf) == 0x1)) {
		EPRINTF2("sm_dma_setup: DATA_XFR, addr: %x, cnt= %x\n", 
			un->un_dma_addr, un->un_dma_count);
		esp_debug =1 ;
		sm_debug_break(5);
	}
#endif SMDEBUG
#ifdef	P4DVMA
	/* set up I-O mapper for the P4 DVMA-ESP */
	set_iomap(un->un_dma_addr, un->un_dma_count);
#endif	P4DVMA

#ifdef	SMTRACE
	DPRINTF1("sm_dma_setup: MEM_addr= %x, data= ", un->un_dma_curaddr);
	ptr = (u_char *)un->un_dma_addr;
	for (i = 0; i < 20; i++, ptr++) {
		DPRINTF1(" %x ", *ptr);
	}
	DPRINTF("\n");
	DEBUG_DELAY(1000000);
#endif	SMTRACE

#ifdef	DMAPOLL	
	if (un->un_flags & SC_UNF_RECV_DATA)  {
		DPRINTF("sm_cmd: setting a POLL-read from SCSI\n");
		un->un_dma_curdir = SM_RECV_DATA; /* read */
	} else {
		/* set write direction */
		DPRINTF("sm_cmd: setting a POLL-write to SCSI\n");
		un->un_dma_curdir = SM_SEND_DATA;
	}
#else	DMAPOLL	
	smr = esp_reg_addr;
        dvma = dvma_reg_addr;
	dvma->scsi_dmaaddr = un->un_dma_curaddr;
	if (un->un_flags & SC_UNF_RECV_DATA) {
		DPRINTF("sm_dma_setup: read from SCSI\n");
		un->un_dma_curdir = SM_RECV_DATA; /* read */
		dvma->scsi_ctlstat |= (DVMA_ENDVMA | DVMA_WRITE); /* to memory */
		SM_LOG_STATE(STATE_DATA_IN, un->un_dma_curcnt, -1);
	} else {
		DPRINTF("sm_dma_setup: write to SCSI\n");
		un->un_dma_curdir = SM_SEND_DATA;
		dvma->scsi_ctlstat |= DVMA_ENDVMA;	/* enable DMA */	
		SM_LOG_STATE(STATE_DATA_OUT, un->un_dma_curcnt, -1);
	}
	DPRINTF2("sm_dma_setup: DVMA stat= %x addr= %x\n", 
		dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
	/* DVMA will be enabled for all data transfer */
#endif	DMAPOLL
#ifdef	not
	SET_ESP_COUNT(un->un_dma_curcnt);
#endif	not
	ESP_WR.xcnt_lo = (u_char)un->un_dma_curcnt;
	ESP_WR.xcnt_hi = (u_char)(un->un_dma_curcnt >> 8);
	TRACE_CHAR(MARK_SETUP);
	DEBUG_DELAY(1000000);
}

/*
 * Cleanup up the SCSI-ESP and DVMA logic after a dma transfer
 * even with previous errors, however, count will not be updated 
 */
static
sm_dma_cleanup(c, espstat)
	struct scsi_ctlr *c;
	u_char	espstat;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	u_long	dvmastat, l;
	u_short req_cnt, dma_cnt, esp_cnt, fifo_cnt;
	register u_short xfrcnt;

#ifdef SMTRACE
	int i;
	u_char *ptr;
#endif SMTRACE
	
	if ((un->un_dma_count == 0) ||
	    (un->un_dma_curdir == SM_NO_DATA)) {
		DPRINTF2("sm_dma_cleanup: No cnt, stat= %x, intr= %x\n",
			espstat, smsnap->esp_intr);
	    return(OK);
	}
#ifdef SMTRACE
	TRACE_CHAR(MARK_CLEANUP);
	DPRINTF1("sm_dma_cleanup: ORG_addr= %x, NEW_data= ", 
		un->un_dma_addr);
	ptr = (u_char *)un->un_dma_addr;
	for (i = 0; i < 20; i++, ptr++) {
		DPRINTF1(" %x ", *ptr);
	}
	DPRINTF1("\nsm_dma_cleanup: ORG_cnt= %x\n",
		 un->un_dma_count);
	DEBUG_DELAY(1000000);
#endif SMTRACE
	
	req_cnt = un->un_dma_curcnt;
#ifdef DMAPOLL
	if (smsnap->cur_err == SE_NO_ERROR) {
		/* update xfrcnt ONLY when there is NO error */
		un->un_dma_count = req_cnt;
		un->un_dma_addr = un->un_dma_curaddr;
		DPRINTF2("sm_dma_cleanup: POLL_dma_addr= %x cnt= %x\n", 
			un->un_dma_addr, un->un_dma_count);
	}
#else
	smr = esp_reg_addr;
        dvma = dvma_reg_addr;

	dvmastat = dvma->scsi_ctlstat;
	DPRINTF2("sm_dma_cleanup: dvma_stat= %x, addr= %x\n", 
		dvmastat, dvma->scsi_dmaaddr);
	TRACE_CHAR(dvmastat);
	TRACE_CHAR(dvmastat >> 8);
	TRACE_CHAR(dvmastat >> 16);
	TRACE_CHAR(dvmastat >> 24);

	if ((dvmastat & DVMA_ENDVMA) == 0) {
		DPRINTF2("sm_dma_cleanup: dvma= %x not enabled, espstat= %x\n",
			dvmastat, espstat);
	    return(OK);
	}
	if (dvmastat & DVMA_REQPEND) {
		dvmastat = LOOP_4SEC;	/* must be over 2sec */
		while ((dvma->scsi_ctlstat & DVMA_REQPEND) && (--dvmastat)) ;
		if (dvmastat == 0) {
		   EPRINTF1("sm_dma_cleanup: DVMA req_pend err, stat= %x\n",
				dvma->scsi_ctlstat); 
		   smsnap->cur_err = SE_REQPEND;
		   return(FAIL);
		}
	}
	/* need drain when DVMA is write (scsi read) */
	if (un->un_dma_curdir == SM_RECV_DATA) {	/* DVMA_WRITE */
	    DPRINTF("sm_dma_cleanup: send DVMA_drain\n");
	    DEBUG_DELAY(1000000);
	    /* wait for pack count to be cleared */
	    dvmastat = 2000;
	    while ((dvma->scsi_ctlstat & DVMA_PACKCNT) && (--dvmastat)) {
		dvma->scsi_ctlstat |= DVMA_DRAIN;
		DELAY(200);
	    }
	    if ((dvmastat == 0) && (dvma->scsi_ctlstat & DVMA_PACKCNT)) {
		EPRINTF2("sm_dma_cleanup: DVMA drain_err, stat= %x, addr= %x\n",
			dvma->scsi_ctlstat, dvma->scsi_dmaaddr); 
		smsnap->cur_err = SE_DVMAERR;
		dvma->scsi_ctlstat |= DVMA_FLUSH; /* self-clear */
		dvma->scsi_ctlstat &= ~(DVMA_ENDVMA | DVMA_WRITE);	
		return(FAIL);
	    }   
	    if (dvma->scsi_ctlstat & DVMA_ERRPEND) {  
		/* memory exception error */
		dvmastat = (u_long)un->un_dma_curaddr & KEEP_24BITS;
		dvmastat += (u_long)req_cnt;
		l = dvma->scsi_dmaaddr & KEEP_24BITS;  
		DPRINTF2("sm_dma_cleanup: dvma_reg_addr = %x, addr= %x\n",
			l, dvmastat);
		if (l < dvmastat) {	/* within transfer range */
		    EPRINTF2("sm_dma_cleanup: DVMA mem_err, stat= %x, addr= %x\n", 
				dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
		    smsnap->cur_err = SE_MEMERR; 
		    dvma->scsi_ctlstat |= DVMA_FLUSH; /* self-clear */
		    dvma->scsi_ctlstat &= ~(DVMA_ENDVMA | DVMA_WRITE);	
		    return(FAIL);
		}
	    }	
	}
	/* need to clear DVMA's read-ahead state machine */
	DPRINTF("sm_dma_cleanup: send DVMA_flush\n");
	DEBUG_DELAY(1000000);
	dvma->scsi_ctlstat |= DVMA_FLUSH; /* self-clear */
	/* disable DVMA_xfr */	
	dvma->scsi_ctlstat &= ~(DVMA_ENDVMA | DVMA_WRITE);	

	/* see how much DVMA xfr'd */
	dvmastat = dvma->scsi_dmaaddr;
	dma_cnt = (u_short)dvmastat - un->un_dma_curaddr;
	/* see how much ESP xfr'd */
	fifo_cnt = ESP_RD.fifo_flag & KEEP_5BITS;
	DPRINTF2("sm_dma_cleanup: dma_cnt= %x, fifo_cnt= %x\n",
		dma_cnt, fifo_cnt);	
	/* Figure out how much the esp chip may have xferred
         * If the XZERO bit is set, we can assume that the
         * ESP xferred all we asked for.  */
	if (espstat & ESP_STAT_XZERO) {
	    esp_cnt = req_cnt - fifo_cnt;

	} else {
#ifdef	not
		xfrcnt = GET_ESP_COUNT;
#endif	not
	    	xfrcnt = ESP_RD.xcnt_hi << 8;
	    	xfrcnt |= ((u_short)ESP_RD.xcnt_lo & KEEP_LBYTE);
		esp_cnt = req_cnt - xfrcnt;
		/* correct ESP xfr_cnt reported error */
		DPRINTF2("sm_dma_cleanup: espcnt= %x, fifo_flag= %x\n",
			esp_cnt, fifo_cnt);
#ifdef SMSYNC
		if ((esp_cnt == 0) && (dma_cnt > 16)) {
			EPRINTF2("sm_dma_cleanup: cnt= %x, req_cnt= %x,",
				un->un_dma_count, req_cnt);
			EPRINTF2(" xfrcnt= %x, dma_cnt= %x\n",
				xfrcnt, dma_cnt);
			esp_cnt = dma_cnt & (0xf);	
		}
#endif SMSYNC
		esp_cnt -= fifo_cnt;
	}
 	/* If we were sending out the scsi bus, then we believe the 
	 * (possibly faked) transfer count register, since the esp 
	 * chip may have prefetched to fill it's fifo.  */
	if (un->un_dma_curdir == SM_SEND_DATA) {
		xfrcnt = esp_cnt;
	} else {
		/* Else, if we were coming from the scsi bus, we only count
                 * that which got pumped through the dma engine.  */
		xfrcnt = dma_cnt;
	}
	DPRINTF2("sm_dma_cleanup: req_cnt= %x, xfrcnt= %x\n",
		req_cnt, xfrcnt);	
#endif DMAPOLL
	if (fifo_cnt) 
		ESP_WR.cmd = CMD_FLUSH;
	un->un_dma_count = req_cnt - xfrcnt;
	un->un_dma_addr = un->un_dma_curaddr + xfrcnt;
#ifdef SMDEBUG
	dmainfo[MAXID(un->un_target)].req_addr = un->un_dma_curaddr;
	dmainfo[MAXID(un->un_target)].req_cnt = req_cnt;
	dmainfo[MAXID(un->un_target)].xfrcnt = xfrcnt;
	dmainfo[MAXID(un->un_target)].req_dir = un->un_dma_curdir;
	if (esp_debug) {
		EPRINTF2("sm_dma_cleanup: final_addr= %x, cnt= %x,",
			un->un_dma_curaddr, un->un_dma_curcnt);
		EPRINTF2(" DVMA_ctlstat= %x, DVMA_addr= %x\n",
			dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
	}
#endif SMDEBUG
	/* clear current counts */
	un->un_dma_curaddr = un->un_dma_curcnt = 0;	
	un->un_dma_curdir = SM_NO_DATA;
#ifdef SMSYNC
	ESP_WR.sync_period = 0;
	ESP_WR.sync_offset = 0;
#endif SMSYNC
	TRACE_CHAR(MARK_CLEANUP);
	DEBUG_DELAY(1000000);
	return(OK);
}

/*
 * Poll and Service a SCSI bus interrupt.
 */
sm_poll()
{
	register struct scsi_ctlr *c;
	register struct udc_table *dvma = dvma_reg_addr;
	int serviced = 0;
	u_long	dvmastat;

	for (c = smctlrs; c < &smctlrs[nsm]; c++) {
	   if ((c->c_flags & SCSI_PRESENT) == 0)
		continue;
	   dvmastat = dvma->scsi_ctlstat;
	   if ((dvmastat & DVMA_INT_MASK) == 0) 
		continue;
	   DPRINTF1("sm_poll: INT found, dma_stat=%x\n", dvmastat);
	   serviced = 1;
	   DEBUG_DELAY(1000000);
	   sm_intr(c);	
	}
	return (serviced);
}


/*
 * Service a scsi interrupt.
 */
sm_intr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register struct scsi_unit *un;
	register int i, s;
	u_char 	*cp, espstat;
	u_long 	dvmastat, l;

	DPRINTF("sm_intr:\n");
	smr = esp_reg_addr;
        dvma = dvma_reg_addr;

	dvma->scsi_ctlstat |= DVMA_INTEN;

INTR_HANDLING: 				
	smsnap->sub_state = TAR_START;	
	un = c->c_un;			/* get the current unit pointer */
	TRACE_CHAR(MARK_INTR);
	dvmastat = dvma->scsi_ctlstat;
	DPRINTF2("sm_intr: dma_stat: %x, un= %x\n", dvmastat, un);
	TRACE_CHAR((char)dvmastat);
	TRACE_CHAR((char)(dvmastat >> 8));
	TRACE_CHAR((char)(dvmastat >> 16));
	TRACE_CHAR((char)(dvmastat >> 24));

        if ((dvmastat & DVMA_INT_MASK) == 0) {	/* INT bit not set */
		EPRINTF1("sm_intr: DVMA *NO* int_pend, stat= %x\n", dvmastat);
		smsnap->cur_err = SE_SPURINT; 
		goto INTR_CHK_ERR;
	}
	
	/* reading ESP's interrupt register will clear INT source */
	smsnap->esp_step = ESP_RD.step & KEEP_3BITS; /* only keep 3 bits */
	smsnap->esp_stat = ESP_RD.stat & STAT_RES_MASK;
	espstat = smsnap->esp_stat;
#ifdef SMSYNC
	if (un && (sync_offset[un->un_target])) {
	    	DPRINTF2("sm_intr_I: stat= %x, state= %x\n",
			smsnap->esp_stat, smsnap->cur_state);
        }
#endif SMSYNC
	smsnap->esp_intr = ESP_RD.intr;		/* clear INT */
	DPRINTF2("sm_intr: step= %x, stat= %x\n", 
		smsnap->esp_step, smsnap->esp_stat);
	/* check dvma's req_pend, err_pend, and int_pend */
	/* should only have int_pend set, if NO error */
	if (smsnap->cur_state == STATE_DATA_REQ) {
	   dvmastat = dvma->scsi_ctlstat;
	   if (dvmastat & DVMA_CHK_MASK) {
	    	EPRINTF2("sm_intr: DVMA stat_err= %x, intr= %x\n", 
			dvmastat, smsnap->esp_intr);
	    	smsnap->cur_retry++;
	    	if (dvmastat & DVMA_REQPEND) {	
			EPRINTF1("sm_intr: DVMA req_pend, stat= %x\n", dvmastat);
			/* DMA request pending, should not ocurred */ 
			l = LOOP_4SEC;	/* must be over 2sec */
			/* wait for a short while to see it will clear itself */
			while ((--l) && (dvma->scsi_ctlstat & DVMA_REQPEND));
			if (l == 0) {
			    	EPRINTF1("sm_intr: DVMA req_tout, stat= %x\n", 
					dvma->scsi_ctlstat);
				smsnap->cur_err = SE_REQPEND; 
				goto INTR_CHK_ERR;
			}
	    	}
		if (dvmastat & DVMA_ERRPEND) {	/* memory exception error */
			EPRINTF1("sm_intr: DVMA mem_err, stat= %x\n", dvmastat);
			dvmastat = (u_long)un->un_dma_curaddr & KEEP_24BITS;
			dvmastat += (u_long)un->un_dma_curcnt;
			l = dvma->scsi_dmaaddr & KEEP_24BITS;  
			DPRINTF2("sm_intr: dvma_reg_addr = %x, last_addr= %x\n",
				l, dvmastat);
			if (l < dvmastat) {	/* within transfer range */
	    		    EPRINTF2("sm_intr: DVMA mem_err, stat= %x, addr= %x\n", 
				dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
		    	smsnap->cur_err = SE_MEMERR; 
			goto INTR_CHK_ERR;
		    }
		}	
 	    }
	    if (espstat & STAT_ERR_MASK) {
	   	EPRINTF2("sm_intr: esp stat_err= %x, phase= %x\n",
			espstat, smsnap->cur_state);
		   /* ESP's status reg ERROR */
		if (espstat & ESP_STAT_PERR) {
			/* parity error only valid at data phase */
			EPRINTF("sm_intr: SCSI bus parity error\n");
			smsnap->cur_err = SE_PARITY;
			goto INTR_CHK_ERR;
	   	}
		if (un != NULL) {
		    /* if mismatch, the 'DMA" direction is wrong */
		   if ((((dvma->scsi_ctlstat & DVMA_WRITE) == 0) && 
		      (un->un_dma_curdir == SM_RECV_DATA)) ||
		      ((dvmastat & DVMA_WRITE) && 
		      (un->un_dma_curdir == SM_SEND_DATA))) {
			EPRINTF2("sm_intr: wrong dma dir= %x, phase= %x\n",
				un->un_dma_curdir, smsnap->cur_state);
			smsnap->cur_err = SE_DIRERR;
			goto INTR_CHK_ERR;
		   }
#ifdef SMSYNC
	  	   /* phase changes during synchronous data transfer */
     		   if (sync_offset[un->un_target] != 0) {
			EPRINTF("sm_intr: sync_phase err\n");
			smsnap->cur_err = SE_PHASERR;
			goto INTR_CHK_ERR;
		   }
#endif SMSYNC
		   if ((ESP_RD.fifo_flag & KEEP_5BITS) == MAX_FIFO_FLAG) { 
			/* fifo overwritten */
			EPRINTF1("sm_intr: fifo_flag ERROR= %x\n", 
				ESP_RD.fifo_flag);
			/* the 'TOP of FIFO' had been overwritten */
			smsnap->cur_err = SE_FIFOVER;
			goto INTR_CHK_ERR;
		   }
		}
		/* If none of the above, GROSS error is from cmd overflown */
		/* top_of_cmd had been overwritten */
		/* ignore this for P4-ESP for timing reason */
		if (smsnap->cur_err == SE_NO_ERROR) {
			EPRINTF1("sm_intr: curcmd= %x\n", ESP_RD.cmd);
			smsnap->cur_err = SE_CMDOVER;
			goto INTR_CHK_ERR;
		}
	   }
	}
INTR_CHK_STAT:
	/* CORRECTON for ESP ERRATA: to ensure phase change detection reset */
	smsnap->esp_stat &= ESP_PHASE_MASK;
	s = ESP_RD.stat & STAT_RES_MASK;
	i = s & ESP_PHASE_MASK;		/* check new stat */
	if (i != smsnap->esp_stat) {
		DPRINTF2("sm_intr: phase changed, old= %x, new= %x\n", 
			smsnap->esp_stat, i);
		/* update both stats */
		smsnap->esp_stat = i;
		espstat = s;
		if (i = ESP_RD.intr) {
		    DPRINTF2("sm_intr: INTR changed, old= %x, new= %x\n", 
			smsnap->esp_intr, i);
		    smsnap->esp_intr = i;
		}
	}
#ifdef SMDEBUG
	if ((smsnap->cur_retry > 10) && (esp_debug)) {
	    	EPRINTF2("sm_intr: NEW stat= %x, state= %x,", 
			espstat, smsnap->cur_state);
	    	EPRINTF2(" intr= %x, retry= %x\n", 
			smsnap->esp_intr, smsnap->cur_retry);
		sm_debug_break(4);
	}
#endif SMDEBUG
	if (++smsnap->cur_retry > MAX_RETRY) {
        	EPRINTF2("sm_intr: err in chk_stat= %x, state= %x,",
			espstat, smsnap->cur_state);
        	EPRINTF2(" intr= %x, retry= %x\n",
			smsnap->esp_intr, smsnap->cur_retry);
                smsnap->cur_err = SE_PHASERR;
                goto INTR_CHK_ERR;
        }
	TRACE_CHAR(MARK_INTR + 1);
	TRACE_CHAR(smsnap->cur_retry);
	TRACE_CHAR(smsnap->cur_state);
	TRACE_CHAR(smsnap->esp_step);
	TRACE_CHAR(espstat);
	TRACE_CHAR(smsnap->esp_stat);
	TRACE_CHAR(smsnap->esp_intr);
	DEBUG_DELAY(1000000);
	switch (smsnap->esp_intr) {
	   case INT_ILL_DISCON:  	/* discon and illegal cmd INTs */
	   case INT_ILL_DISCON_OK:  	/* discon and fcmp, illegal cmd INTs */
	   case INT_ILL_DISCON_OK1:  	/* discon and bus, illegal cmd INTS */
		DPRINTF2("sm_intr: illegal discon, state= %x, stat= %x\n",
			smsnap->cur_state, espstat);
	   case ESP_INT_DISCON:  	/* disconnect INT */
		smsnap->sub_state |= TAR_DISCON;
		switch (smsnap->cur_state) {
		    case STATE_MSG_DISCON: /* disconnect msg been accepted */
INTR_END_DISCON:
#ifdef SMSYNC
                	ESP_WR.sync_period = 0;
                	ESP_WR.sync_offset = 0;
			DPRINTF2("sm_intr: discon, state= %x, stat= %x\n", 
				smsnap->cur_state, smsnap->esp_stat);
#endif SMSYNC
	    		if ((int)++smsnap->num_dis_target > 0) {
		   		ESP_WR.cmd = CMD_EN_RESEL;
				DPRINTF1("sm_intr: Enable resel, target= %x\n", 
					smsnap->num_dis_target);
				smsnap->cur_state = STATE_ENRSEL_REQ;
			}
			sm_discon_queue(un);
#ifdef SMINFO
                	target_accstate[MAXID(un->un_target)] |= TAR_DISCON;
#endif SMINFO
			un = c->c_un;
			goto INTR_CHK_NEXTINT;
		    case STATE_MSG_CMPT: /* command completed msg accepted */
INTR_END_CMPT:
#ifdef SMSYNC
                	ESP_WR.sync_period = ESP_WR.sync_offset = 0;
#endif SMSYNC
			/* All ZERO= good, All other, bit 1= check cond, */
			/* bit 3= busy, bit 4=rsv conflict are errors */
			if (smsnap->scsi_status & SCSI_BUSY_MASK) {
				DPRINTF1("sm_intr: bus_stat= %x\n", 
					smsnap->scsi_status);
#ifdef SMINFO
                                if (smsnap->scsi_status & 0x2)
                                        target_accstate[MAXID(un->un_target)] |=
                                                TAR_CHK_COND;
                                if (smsnap->scsi_status & 0x8)
                                        target_accstate[MAXID(un->un_target)] |=
                                                TAR_BUSY;
#endif SMINFO
				smsnap->cur_err = SE_RETRYABLE;
			}
			DPRINTF1("sm_intr: discon_target= %x\n",
				smsnap->num_dis_target);
	    		if ((int)smsnap->num_dis_target > (int)0) {
		   		ESP_WR.cmd = CMD_EN_RESEL;
				DPRINTF1("sm_intr: Enable resel, target= %x\n", 
					smsnap->num_dis_target);
			}
			smsnap->cur_state = STATE_FREE;
		    case STATE_FREE:	
			smsnap->sub_state |= TAR_JOBDONE;
			/* Entire job is completed, clear job now */
			goto INTR_JOB_DONE;

		    case STATE_SEL_REQ:		/* selection command timeout */
		    case STATE_SYNCHK_REQ:	/* sync_selection cmd timeout */
			DPRINTF2("sm_intr: cmd_tout, phase= %x, step= %x\n",
				smsnap->cur_state, smsnap->esp_step);
			smsnap->cur_err = SE_SELTOUT;
			goto INTR_CHK_ERR;

		    case STATE_ENRSEL_REQ:      /* reselection timed out */	
			DPRINTF2("sm_intr: cmd_tout, phase= %x, step= %x\n",
				smsnap->cur_state, smsnap->esp_step);
			smsnap->cur_err = SE_RSELTOUT;
			goto INTR_CHK_ERR;
#ifdef SMSYNC
                    case STATE_MSGOUT_REQ:      /* reselection timed out */
                    case STATE_SYNC_DONE:
                        DPRINTF2("sm_intr: msg_out, state= %x, stat= %x\n",
                                smsnap->cur_state, ESP_RD.stat);
                        DPRINTF2("sm_intr: un= %x, espstat= %x\n",
                                un, smsnap->esp_stat);
                        if (un != NULL)
                            DPRINTF2("sm_intr: curcnt= %x, count= %x\n",
                                un->un_dma_curcnt, un->un_dma_count);
                        ESP_WR.cmd = CMD_NOP;
                        goto INTR_CHK_STAT;
#endif SMSYNC
		    case STATE_MSGIN_REQ:
		    case STATE_MSG_REJECT:
			EPRINTF2("sm_intr: reject job, c_active= %x, cnt= %x,",
				c->c_tab.b_active, un->un_dma_count);
			EPRINTF2(" state= %x, m_active= %x\n",
				smsnap->cur_state, un->un_md->md_utab.b_active);
			/* preempt the job now, and retry later */
			un->un_md->md_utab.b_active |= MD_PREEMPT;
                        un->un_wantint = 0;
                        c->c_tab.b_active = C_INACTIVE;
                        smsnap->cur_state = STATE_FREE;
	    		if ((int)smsnap->num_dis_target > 0) {
		   		ESP_WR.cmd = CMD_EN_RESEL;
				smsnap->cur_state = STATE_ENRSEL_REQ;
			}
                        goto INTR_CHK_NEXTINT;
		    default:
			/* could be from target dropping BSY during data xfr */
			EPRINTF("sm_intr: lost busy\n");
			if (smsnap->cur_state == STATE_DATA_REQ) {
				smsnap->cur_err = SE_LOST_BUSY;
				(void) sm_dma_cleanup(c, espstat);
				goto INTR_CHK_ERR;
			}
			if (smsnap->cur_state == STATE_MSGOUT_DONE) {
				EPRINTF1("msg_out done, msg= %x\n", 
					smsnap->scsi_message);
#ifdef				SMDEBUG
				sm_debug_break(40);
#endif				SMDEBUG
				if (smsnap->scsi_message == SC_DISCONNECT)
					goto INTR_END_DISCON;
				else		
					goto INTR_END_CMPT;
			}
			ESP_WR.cmd = CMD_NOP;
			goto INTR_CHK_STAT;
	            }
	   case ESP_INT_ILLCMD:		/* illegal command */
	   case INT_ILL_BUS:		/* illegal cmd/bus INTS  */
	   case INT_ILL_FCMP:		/* illegal cmd/fcmp INTS */ 
		DPRINTF2("sm_intr: illegal esp_cmd= %x, int= %x\n", 
			ESP_RD.cmd, smsnap->esp_intr);
		DPRINTF2("sm_intr: pre_phase= %x, stat= %x\n", 
			smsnap->cur_state, espstat);
		DPRINTF2("sm_intr: dmastat= %x, curcmd= %x\n", 
			dvma->scsi_ctlstat, ESP_RD.cmd);
		if (smsnap->cur_state == STATE_FREE) {
#ifdef SMDEBUG
			if (esp_debug) {
			  EPRINTF2("sm_intr: err data phase, state= %x, flag= %x\n",
				smsnap->cur_state, ESP_RD.fifo_flag);
			  EPRINTF2("sm_intr: stat= %x, cmd= %x\n",
				ESP_RD.stat, ESP_RD.cmd);
			}
#endif SMDEBUG
	    		if ((int)smsnap->num_dis_target > 0) {
		   		ESP_WR.cmd = CMD_EN_RESEL;
				DPRINTF1("sm_intr: Enable resel, target= %x\n", 
					smsnap->num_dis_target);
				smsnap->cur_state = STATE_ENRSEL_REQ;
			}
                        goto INTR_CHK_NEXTINT;
		}
		/* fall down to continue */
	   case ESP_INT_BUS:    /* Bus-service Int, command completed */
				/* unless msg received and ACK still asserted */
	   case ESP_INT_FCMP:   /* Function-completed Int */
				/* with msg received and ACK being asserted */
	   case INT_OK_MASK: 	/* both FCMP and BUS Ints are set */	 
	     DPRINTF2("sm_intr: ESP f_cmp/b_service INT, stat= %x, state= %x\n",
		ESP_RD.stat, smsnap->cur_state);
		switch (smsnap->cur_state) {
		case STATE_DSRSEL_REQ:
			DPRINTF("sm_intr: resel disabled\n");
       		  	smsnap->cur_state = STATE_DSRSEL_DONE;
			goto INTR_CHK_NEXTINT;
		case STATE_SYNCHK_REQ:
			ESP_WR.cmd = CMD_NOP;
			ESP_WR.cmd = CMD_FLUSH;
                        DPRINTF2("sm_intr: sync_step= %x, stat= %x\n", 
				smsnap->esp_step, smsnap->esp_stat);
			/* fall to msg_out phase */
                        break;
		case STATE_SEL_REQ:
			smsnap->cur_retry = 0;
			/*
			 * Do not flush out fifo here due to sync data might
			 * be already in it.
			 */
			DPRINTF2("sm_intr: sel_done, step= %x, stat= %x\n",
				smsnap->esp_step, espstat);
		        DPRINTF2("sm_intr: target= %x, lun= %x\n", 
				un->un_target, un->un_lun);
			/* both sel w/wo ATN need 4 steps to complete OK */
			/* only sel w/ATN/stop is 3 steps to complete OK */
			smsnap->sub_state |= TAR_SELECT;
#ifdef SMSYNC
			/* update ESP sync regs with previous values */
			if (sync_offset[un->un_target]) {
          	            ESP_WR.sync_period = sync_period[un->un_target];
                            ESP_WR.sync_offset = sync_offset[un->un_target];
			    DPRINTF2("sm_int_S: sync_period= %d, off= %d\n",
	                     sync_period[un->un_target], sync_offset[un->un_target]);
                            DPRINTF1("sm_intr: set sync after SEL on target= %x\n",
				un->un_target);
                        } else {
			    /*
			     * With sync_enabled, need to program
			     * sync_offset when mixing sync/async devices.
			     */
			    DPRINTF1("sm_intr: set async on target %d\n", 
				un->un_target);
                            ESP_WR.sync_period = 0;
                            ESP_WR.sync_offset = 0;
			}
#endif SMSYNC
			switch (smsnap->esp_step) {
			case FOUR:
				/* arb, sel, msg_out, cmd phases=ok */
				SM_LOG_STATE(STATE_COMMAND, un->un_cdb.cmd, -1);
				break;
			case ONE:
				/*
				 * SEL_W/WO_ATN: no such step sequence for async.
				 * May be from Synch check though.
				 */
#ifdef SMSYNC
				if ((smsnap->sync_known & (1 << un->un_target))
				    == 0) {
					DPRINTF2("sm_intr: do sync_query, stat= %x, target= %x\n",
						smsnap->esp_stat, un->un_target);
	            	              	smsnap->cur_state = STATE_SYNCHK_REQ;
					/* fall to msg_out phase */
                       	   	  	goto INTR_CHK_STAT;
				}
                   		DPRINTF1("sm_intr: sync_sel, target= %x\n",
					un->un_target);
			        /*
				 * For sync testing, arb & sel=ok, 1 msg byte out.
				 * Let it fall through to handle the sync
				 * negotiation.
				 */
				break;
#endif SMSYNC
				/* Let it fall to check status */
			case TWO:	
				/*
				 * arb, sel=ok, 1 msg byte out, no cmd bytes out.
				 * If async, same case as step=three.
				 */
			case THREE:	
				/*
				 * Arb, sel, msg_out=ok, cmd bytes not all out.
				 * If status phase, let it fall through to get
				 * target's check condition, etc.
				 */
				SM_LOG_STATE(STATE_COMMAND, un->un_cdb.cmd, -1);
			        if ((smsnap->esp_stat == ESP_PHASE_STATUS)  ||
			            (smsnap->esp_stat == ESP_PHASE_DATA_IN) ||
			            (smsnap->esp_stat == ESP_PHASE_DATA_OUT))

					break;
			default:
				/*
				 * All others: no such step sequence.
			   	 * case ZERO: arb=ok, sel=fail
				 * should not happen, should be disconnect INTR.
				 */
				EPRINTF2("sm_intr: bad sel_step= %x, stat= %x\n",
					smsnap->esp_step, smsnap->esp_stat);
				smsnap->cur_err = SE_ILLCMD;
				goto INTR_CHK_ERR;
			}
#ifdef SMINFO
       			target_accstate[MAXID(un->un_target)] |= TAR_SELECT;
#endif SMINFO
			break;
		  case STATE_ATN_REQ:
			/* ATN remain asserted util last msg byte */
			/* fifo should be at top after flush */
                        DPRINTF1("sm_intr: Send abort_msg, state= %x\n",
				smsnap->cur_state);
                        /* send abort_msg */
			if (smsnap->cur_state == STATE_DATA_REQ) {
				DPRINTF("sm_intr: try to clean up xfr\n");
				if (sm_dma_cleanup(c, espstat))
					goto INTR_CHK_ERR;
			}
                        ESP_WR.cmd = CMD_FLUSH;
			ESP_WR.cmd = CMD_NOP;
                        ESP_WR.fifo_data = SC_ABORT;
                        ESP_WR.cmd = CMD_SET_ATN;
                        ESP_WR.cmd = CMD_TRAN_INFO;
                        smsnap->cur_state = STATE_MSGOUT_REQ;
                        goto INTR_CHK_NEXTINT;
		  case STATE_MSG_DISCON:
#ifdef SMDEBUG
			EPRINTF2("sm_intr: race_cond, target= %x, intr= %x,", 
				un->un_target, smsnap->esp_intr);
			EPRINTF2(" cmd= %x, stat= %x\n", 
				ESP_RD.cmd, smsnap->esp_stat);
#endif SMDEBUG
			break;
		  case STATE_MSG_CMPT:
#ifdef SMDEBUG
			EPRINTF2("sm_intr: race_cond1, target= %x, cmd= %x,", 
				un->un_target, ESP_RD.cmd); 
			EPRINTF2(" stat= %x, intr= %x\n", 
				smsnap->esp_stat, smsnap->esp_intr);
#endif SMDEBUG
			break;
		  default:
			break; 		/* ignore and fall down */
		}
		TRACE_CHAR(MARK_INTR + 3);
#ifdef SMDEBUG
		if (ESP_RD.stat & 0x80) {
			EPRINTF2("sm_intr: f_cmp INT, stat= %x, state= %x\n",
				ESP_RD.stat, smsnap->cur_state);
			sm_debug_break(30);
			esp_debug = 1;
		}
#endif SMDEBUG
		switch (smsnap->esp_stat) {   	/* current SCSI bus phase */
		  case ESP_PHASE_DATA_IN:	/* either data in or out */
		  case ESP_PHASE_DATA_OUT:	/* either data in or out */
			smsnap->sub_state |= TAR_DATA;
#ifdef SMSYNC
			if (smsnap->cur_state == STATE_DATA_REQ) {
#ifdef SMDEBUG
			   if (esp_debug) {
			   	EPRINTF2("sm_intr: data, state= %x, offset= %x\n", 
				   smsnap->cur_state, sync_offset[un->un_target]);
			   	EPRINTF2("sm_intr: dma_addr= %x, DVMA_addr= %x\n", 
				   un->un_dma_curaddr, dvma->scsi_dmaaddr);
				sm_debug_break(6);
			    }
#endif SMDEBUG
			    if (sync_offset[un->un_target]) {
#ifdef SMDEBUG
			       if (esp_debug) {
				   EPRINTF2("sm_intr: DATA_stat= %x, flag= %x\n",
					 espstat, i);
				   EPRINTF2("sm_intr: ORIG cnt= %x, addr= %x\n", 
					un->un_dma_curcnt, un->un_dma_curaddr); 
				   EPRINTF2("sm_intr: DVMA stat= %x, addr= %x\n", 
					dvma->scsi_ctlstat, dvma->scsi_dmaaddr); 
				   EPRINTF2("sm_intr: ESP cnt_hi= %x, lo= %x\n", 
					ESP_RD.xcnt_hi, ESP_RD.xcnt_lo);
			    	}
#endif SMDEBUG
				s = 0;
				i = ESP_RD.fifo_flag; 
				espstat = ESP_RD.stat & STAT_RES_MASK;
				smsnap->esp_stat = espstat & ESP_PHASE_MASK;
				if (smsnap->esp_stat == ESP_PHASE_DATA_IN) {	
				    if (i & 0x1f) { 	/* stat= 1 */
#ifdef SMDEBUG
				      if (esp_debug) {
			  	EPRINTF2("sm_intr: sync_in stat= %x, byte= %x\n",
					   espstat, i);
				EPRINTF2("sm_intr: ORIG cnt= %x, count= %x\n", 
					   un->un_dma_curcnt, un->un_dma_count); 
				      }
#endif SMDEBUG
					/* DVMA already enabled */
					ESP_WR.xcnt_lo = (u_char)(i & KEEP_5BITS);
					ESP_WR.cmd = CMD_TRAN_INFO | CMD_DMA;
					EPRINTF2("sm_intr: NEW flag= %x, stat= %x\n", 
						ESP_RD.fifo_flag, ESP_RD.stat);
					goto INTR_CHK_NEXTINT;
			 	    }
#ifdef SMDEBUG
				    if (esp_debug) {
		 	   DPRINTF2("sm_intr: no new REQ, flag= %x, stat= %x\n",
					i, espstat);
				    }
#endif SMDEBUG
				    s++;
				} else {
				   if (smsnap->esp_stat == ESP_PHASE_DATA_OUT) { 
				   	if (i & 0x20) {	
#ifdef SMDEBUG
				      if (esp_debug) {
			  	EPRINTF2("sm_intr: sync_out stat= %x, byte= %x\n",
					    espstat, i);
				  	 EPRINTF2("sm_intr: ORIG cnt= %x, addr= %x\n",
					    un->un_dma_curcnt, un->un_dma_curaddr); 
				  	EPRINTF2("sm_intr: DVMA stat= %x, addr= %x\n",
					    dvma->scsi_ctlstat, dvma->scsi_dmaaddr); 
				  	EPRINTF2("sm_intr: ESP cnt_hi= %x, lo= %x\n", 
						ESP_RD.xcnt_hi, ESP_RD.xcnt_lo);
				      }
#endif SMDEBUG
					   /* all dma setting is still in effect */
					   ESP_WR.cmd = CMD_TRAN_INFO | CMD_DMA;
				   EPRINTF2("sm_intr: NEW flag= %x, stat= %x\n", 
						ESP_RD.fifo_flag, ESP_RD.stat);
	  				    goto INTR_CHK_NEXTINT;
					}
#ifdef SMDEBUG
				   if (esp_debug) {
			     EPRINTF2("sm_intr: sync_off flag= %x, stat= %x\n",
						i, espstat);
				   }
#endif SMDEBUG
					s++;
				   } else {
#ifdef SMDEBUG
				     EPRINTF2("sm_intr: wrong phase= %x, flag= %x\n",
					smsnap->esp_stat, ESP_RD.fifo_flag);
#endif SMDEBUG
					ESP_WR.cmd = CMD_NOP;
					goto INTR_CHK_STAT;
				   }
				}
				if (s) {	/* clear for the long REQ */
#ifdef SMDEBUG
				    if (esp_debug)
					EPRINTF("sm_intr: wait for next valid REQ\n");
#endif SMDEBUG
				    ESP_WR.cmd = CMD_MSG_ACPT;
				    /* It turns out that this can also come about 
				     * if the target resets (and goes back to async 
				     * SCSI mode) and we don't know about it.
				     * The degenerate case for this is turning off 
				     * a lunchbox- this clears it's state. The 
				     * trouble is is that we'll get a check 
				     * condition (likely) on the next command 
				     * after a power-cycle for this target, but 
				     * we'll have to go into a DATA IN phase to 
				     * pick up the sense information for the 
				     * Request Sense that will likely follow that
				     * Check Condition.  As a temporary hack, I'll 
				     * clear the 'sync_known' flag for this
				     * target so that the next selection for this 
				     * target will renegotiate the sync protocol 
				     * to be followed.  */
				    smsnap->sync_known &= ~(1 << un->un_target);
				    goto INTR_CHK_NEXTINT;
				}
			   }
#ifdef SMDEBUG
			   if (esp_debug)
				EPRINTF("sm_intr: re-check phase again\n");
#endif SMDEBUG
			   ESP_WR.cmd = CMD_NOP;
			   goto INTR_CHK_STAT;
			}		
#endif SMSYNC
			if ((un->un_dma_count == 0) ||
			    (smsnap->cur_state & STATE_MSG_CMPT) ||
			    (smsnap->cur_state == STATE_COMP_REQ)) {
			    	if (smsnap->cur_state == STATE_COMP_REQ) {
#ifdef SMDEBUG
				   EPRINTF2("sm_intr: D_err, cnt= %x, flag= %x\n",
					ESP_RD.xcnt_lo, ESP_RD.fifo_flag);
				   EPRINTF2("sm_intr: stat= %x, cnt= %x\n",
					ESP_RD.stat, un->un_dma_count);
#endif SMDEBUG
					goto INTR_MSG_COMP;
				} else {
				   DPRINTF2("sm_intr: intr= %x, un_target= %x\n", 
					smsnap->esp_intr, un->un_target);
				   if (smsnap->esp_intr == ESP_INT_FCMP) {
#ifdef SMDEBUG
				       EPRINTF1("sm_intr: race_cond2, state= %x\n",
						smsnap->cur_state);
#endif SMDEBUG
                                       ESP_WR.cmd = CMD_MSG_ACPT;
				   }
				   ESP_WR.cmd = CMD_NOP;
				   goto INTR_CHK_STAT;
				}
			}
#ifdef SMSYNC
			if (sync_offset[un->un_target]) {
        			ESP_WR.sync_period = sync_period[un->un_target];
		                ESP_WR.sync_offset = sync_offset[un->un_target];
               			DPRINTF1("sm_intr: set sync on target= %d\n", 
					un->un_target);
        		} else {
             			DPRINTF("sm_intr: set no sync on ESP\n");
		                ESP_WR.sync_period = 0;
               			ESP_WR.sync_offset = 0;
			}
#endif SMSYNC
			/* valid dma xfr, enable dvma and R/W*/
			/* un could be NULL from "c->c_un", */
			/* which was disconnected earlier */
			sm_dma_setup(c);
#ifdef DMAPOLL
			ESP_WR.xcnt_lo = ESP_WR.xcnt_hi = 0; 
#ifdef	not
			SET_ESP_COUNT(0);
#endif	not
			ESP_WR.cmd = CMD_FLUSH; 
			ESP_WR.cmd = CMD_NOP; 
			DPRINTF1("sm_intr_X: cur fifo_flag= %x\n",
				ESP_RD.fifo_flag); 
			/* ESP's xfr cntr has been set up with dmacnt */
			cp = (u_char *)un->un_dma_addr;
			s = un->un_dma_curcnt;
			DPRINTF2("sm_intr_X: XFR_addr= %x, cnt= %x\n", 
				cp, s);
			l = LOOP_4SEC;
			/* ESP xcnters had been set up ... NO DMA */
			if (un->un_dma_curdir == SM_RECV_DATA) {
			    DPRINTF("sm_intr: DMA_POLL receive data=");
			    ESP_WR.cmd = CMD_TRAN_INFO;
		   	    while (--l) {
				if (ESP_RD.fifo_flag) {
				    i = ESP_RD.fifo_data;
				    *cp++ = (char)i;
				    DPRINTF1(" %x", i);
				    i = ESP_RD.stat & STAT_RES_MASK;
				    prev_prt_err = ESP_RD.intr;
				    s--;
				    if (s == 0)
					break;
			    	    ESP_WR.cmd = CMD_TRAN_INFO;
				}
			    }
			    DPRINTF2("\nsm_intr: R_remain cnt= %x, flag= %x", 
				s, ESP_RD.fifo_flag);
			    DPRINTF2("\nsm_intr: intr= %x, stat= %x", 
				prev_prt_err, i);
			    if (un->un_cdb.cmd == SC_REQUEST_SENSE) 
				/* even not enough bytes, force DONE */
				s = 0;
    	    		    un->un_dma_curaddr += 
				(un->un_dma_curcnt - s);
            		    un->un_dma_curcnt = s;
			    DPRINTF2("\nsm_intr: XFR_done, adr: %x, cnt= %x\n",
				un->un_dma_curaddr, un->un_dma_curcnt);
			} else {	/* write */
			    DPRINTF("sm_intr: DMA-POLL sending data\n");
			    i = *cp++;
			    ESP_WR.fifo_data = (char)i;
			    ESP_WR.cmd = CMD_TRAN_INFO;
			    DPRINTF1(" %x", i);
		   	    while (--l) {
		    		if (ESP_RD.fifo_flag == 0) {
				    i = ESP_RD.stat & STAT_RES_MASK;
				    prev_prt_err = ESP_RD.intr;
				    s--;
				    if (s == 0)
					break;
			    	    i = *cp++;
			    	    ESP_WR.fifo_data = (char)i;
				    DPRINTF1(" %x", i);
			            ESP_WR.cmd = CMD_TRAN_INFO;
			    	}
			    }
			    DPRINTF2("\nsm_intr: W_remain cnt= %x, flag= %x", 
				s, ESP_RD.fifo_flag);
			    DPRINTF2("\nsm_intr: intr= %x, stat= %x", 
				prev_prt_err, ESP_RD.stat);
    	    		    un->un_dma_curaddr 
				+= (un->un_dma_curcnt - s);
            		    un->un_dma_curcnt = s;
			    DPRINTF2("\nsm_intr: XFR_done, adr: %x, cnt= %x\n",
				un->un_dma_addr, un->un_dma_curcnt);
			}
			if (l == 0  &&  s) {
			    DPRINTF1("sm_intr: XFR_tout, cnt=%x\n", s);
			    i = ESP_RD.stat & ESP_PHASE_MASK;
			    DPRINTF2("sm_intr: XFR_tout, flag: %x, stat= %x\n",
				ESP_RD.fifo_flag, i); 
				if (i == smsnap->esp_stat) {
			     	   EPRINTF2("sm_intr: XFR_tout, flag: %x, stat= %x\n",
					ESP_RD.fifo_flag, i); 
					smsnap->cur_err = SE_TOUTERR;
					goto INTR_CHK_ERR;
				}
				espstat = ESP_RD.stat & STAT_RES_MASK;
				smsnap->esp_stat = i;
				smsnap->esp_intr = ESP_RD.intr;
			}
			DPRINTF("sm_intr: POLL_xfr done, chk next_stat\n");
			ESP_WR.cmd = CMD_NOP;
			goto INTR_CHK_STAT;
#else DMAPOLL
#ifdef SMINFO
                	target_accstate[MAXID(un->un_target)] |= TAR_DATA;
#endif SMINFO
			/* add in per ESP errata note */
			/* ESP transfer counters had been set up */
                       	ESP_WR.cmd = CMD_NOP | CMD_DMA;
			smsnap->cur_state = STATE_DATA_REQ;
			TRACE_CHAR(MARK_INTR);
			DPRINTF2("sm_intr: start dma, adr= %x, adr= %x\n",
				dvma->scsi_ctlstat, dvma->scsi_dmaaddr);
			ESP_WR.cmd = CMD_TRAN_INFO | CMD_DMA;
			goto INTR_CHK_NEXTINT;
#endif DMAPOLL
		  case ESP_PHASE_STATUS:
			SM_LOG_STATE(STATE_STATUS, smsnap->scsi_status, -1);
			smsnap->sub_state |= TAR_STATUS;
			DPRINTF2("sm_intr: status phase, state= %x, stat= %x\n", 
				smsnap->cur_state, espstat);
			if (smsnap->cur_state == STATE_DATA_REQ) {
				if (sm_dma_cleanup(c, espstat))
					goto INTR_CHK_ERR;
			} else {
				ESP_WR.cmd = CMD_FLUSH;
				ESP_WR.cmd = CMD_NOP;
			}
#ifdef SMSYNC
			/* target rejected sync query */
			if (smsnap->cur_state == STATE_SYNCHK_REQ) {
				ESP_WR.cmd = CMD_TRAN_INFO;
				smsnap->scsi_status = ESP_RD.fifo_data;
				TRACE_CHAR(MARK_INTR + 5);
				TRACE_CHAR(smsnap->scsi_status);
				TRACE_CHAR(smsnap->scsi_message);
				DPRINTF1("sm_intr_S: sync_BUS_ERR stat= %x\n", 
					smsnap->scsi_status);
				s = ESP_RD.stat & STAT_RES_MASK;
				espstat = s;
				smsnap->esp_stat = s & ESP_PHASE_MASK;
				smsnap->esp_intr = ESP_RD.intr;
				DPRINTF2("sm_intr_S: stat= %x, intr= %x\n", 
					smsnap->esp_stat, smsnap->esp_intr);
				if (smsnap->esp_intr == ESP_INT_FCMP) {
					ESP_WR.cmd = CMD_MSG_ACPT;
				}
				smsnap->cur_state = STATE_SYNC_DONE;
				ESP_WR.cmd = CMD_NOP;
				goto INTR_CHK_STAT;
			} else {
				DPRINTF("sm_intr: send complete_seq\n");
				ESP_WR.cmd = CMD_COMP_SEQ;
			}
#else	SMSYNC
			DPRINTF("sm_intr: send complete_seq\n");
                        ESP_WR.cmd = CMD_COMP_SEQ;
#endif SMSYNC

#ifdef SMINFO
                	target_accstate[MAXID(un->un_target)] |= TAR_STATUS;
#endif SMINFO
	  	 	smsnap->cur_state = STATE_COMP_REQ;
		  	/* target has status and message */
			DEBUG_DELAY(1000000);
			break;
		  case ESP_PHASE_MSG_IN:
INTR_MSG_COMP:
#ifdef SMDEBUG
		        DPRINTF2("sm_intr: msg_in phase, state= %x, flag= %x\n",
				smsnap->cur_state, ESP_RD.fifo_flag);
#endif SMDEBUG
			smsnap->sub_state |= TAR_MSGIN;
			DPRINTF2("sm_intr: espstat= %x, stat= %x\n",
				espstat, ESP_RD.stat);
			switch (smsnap->cur_state) {
			   case STATE_COMP_REQ:	/* two byte available */
				smsnap->scsi_status = ESP_RD.fifo_data; 
			   case STATE_MSGIN_REQ:	/* only one byte */
				smsnap->scsi_message = ESP_RD.fifo_data;
				break;		/* two bytes available */
			   case STATE_SYNCHK_REQ:
				DPRINTF1("sm_intr: sync_chk msg_in flag= %x\n",
					ESP_RD.fifo_flag);
				ESP_WR.cmd = CMD_NOP;
				ESP_WR.cmd = CMD_FLUSH;
				goto INTR_MSG_IN;	
			   case STATE_MSG_REJECT:
				DPRINTF1("sm_intr: sync msg_in flag= %x\n",
					ESP_RD.fifo_flag);
				DELAY(100);
				ESP_WR.cmd = CMD_NOP;
				goto INTR_CHK_STAT;	
			   case STATE_MSGOUT_DONE:
				DPRINTF1("sm_intr: sync_msg bytes= %x\n",
					ESP_RD.fifo_flag);
				smsnap->scsi_message = ESP_RD.fifo_data;
				break;
			   case STATE_MSG_DISCON:
				DPRINTF1("sm_intr: msg_discon, int= %x\n",
					smsnap->esp_intr);
				if (smsnap->esp_intr == ESP_INT_BUS)
					ESP_WR.cmd = CMD_MSG_ACPT;
				ESP_WR.cmd = CMD_NOP;
				goto INTR_END_DISCON;
			   case STATE_MSG_CMPT:
				DPRINTF1("sm_intr: msg_cmpt, int= %x\n",
					smsnap->esp_intr);
				if (smsnap->esp_intr == ESP_INT_BUS)
					ESP_WR.cmd = CMD_MSG_ACPT;
				ESP_WR.cmd = CMD_NOP;
				goto INTR_END_CMPT;
			   case STATE_DATA_REQ:
		                DPRINTF2("sm_intr: msg_in_data, stat= %x, cnt= %x\n",
					dvma->scsi_ctlstat, un->un_dma_curcnt);
				if (sm_dma_cleanup(c, espstat))
					goto INTR_CHK_ERR;
				/* fall through */
			   default:
				/* get message byte */
INTR_MSG_IN:
				s = ESP_RD.fifo_flag & KEEP_5BITS;
				DPRINTF2("sm_intr: msg_in state= %x, flag= %x\n",
					smsnap->cur_state, s);
				if (s > 1) {
#ifdef SMDEBUG
		  		    EPRINTF2("sm_intr: msg_in flag= %x, state= %x\n",
					s, smsnap->cur_state);
		  		    EPRINTF2("sm_intr: stat= %x, intr= %x\n",
					smsnap->esp_stat, smsnap->esp_intr);
#endif SMDEBUG
				    ESP_WR.cmd = CMD_FLUSH;
				    ESP_WR.cmd = CMD_NOP;
				    /* let it falls */
				}
				if (s == 1) {
				    DPRINTF2("sm_intr: sync_msg, state= %x, byte= %x\n", 
					smsnap->cur_state, smsnap->scsi_message);
				    smsnap->scsi_message = ESP_RD.fifo_data;
				} else {
				    DPRINTF1("sm_intr: SYNC_state= %x\n", 
					smsnap->cur_state);
				    smsnap->cur_state = STATE_MSGIN_REQ;
				    ESP_WR.cmd = CMD_TRAN_INFO;
				    goto INTR_CHK_NEXTINT;
				}
			}
                        DPRINTF2("sm_intr: status= %x, message= %x\n",
                                smsnap->scsi_status, smsnap->scsi_message);
			DEBUG_DELAY(1000000);
#ifdef SMINFO
                	target_accstate[MAXID(un->un_target)] |= TAR_MSGIN;
#endif SMINFO
			/* send a message accpt cmd at the END */
			switch (smsnap->scsi_message) {
			   case SC_COMMAND_COMPLETE:
				SM_LOG_STATE(STATE_MSG_CMPT,
					 un->un_dma_count, -1);
#ifdef SMINFO
                                target_accstate[MAXID(un->un_target)] |= TAR_COMP_MSG;
#endif SMINFO
				DPRINTF("sm_intr: Cmd_comp_msg\n");
				smsnap->cur_state = STATE_MSG_CMPT;
				break;
			   case SC_NO_OP:
				SM_LOG_STATE(STATE_MSG_NOP, -1, -1);
#ifdef SMINFO
                                target_accstate[MAXID(un->un_target)] |= TAR_COMP_MSG;
#endif SMINFO
				smsnap->cur_state = STATE_MSG_CMPT;
				/* save the scsi_status for later on */
				/* a disconnect INT should come after */
				/* sending out "msg_accpt" cmd */
				/* at that time, we are TOTALLY done */
				break;
			   case SC_DISCONNECT: /* target wanted to disconnect */
				SM_LOG_STATE(STATE_MSG_DISCON, -1, -1);
#ifdef SMINFO
                                target_accstate[MAXID(un->un_target)] |= TAR_DIS_MSG;
#endif SMINFO
				DPRINTF("sm_intr: Discon_msg\n");
				/* no need to call sm_disconnect() since */
				/* SAVE_PTR msg will update un_dma_info */
				smsnap->cur_state = STATE_MSG_DISCON;
				break;
			   case SC_SAVE_DATA_PTR:
				SM_LOG_STATE(STATE_MSG_SAVE_PTR, un->un_dma_count, -1);
#ifdef SMINFO
                                target_accstate[MAXID(un->un_target)] |= TAR_SAVE_MSG;
#endif SMINFO
			        DPRINTF2("sm_intr: Saveptr_msg, stat= %x, adr= %x\n",
					ESP_RD.stat, dvma->scsi_dmaaddr);
				smsnap->cur_state = STATE_SPEC_SAVEPTR;
				break;
			   case SC_RESTORE_PTRS:
				SM_LOG_STATE(STATE_MSG_RESTORE_PTR, -1, -1);
				DPRINTF("sm_intr: Restore_ptr_msg\n");
				/* NORMALLY no need to restore pointer, since 
				   data phase will set thing up */
				/* if target see parity errors, it sends back */
				/* restore_msg, from cmd_phase, data_phase */
				smsnap->cur_state = STATE_SPEC_RESTORE;
				break;
			   /* TBD: for NOW, just lock all other error message */
			   case SC_PARITY:
				SM_LOG_STATE(STATE_MSG_PARITY, -1, -1);
				EPRINTF("sm_intr: Parity_err_msg\n");
				smsnap->cur_err  = SE_PARITY;
				smsnap->cur_state = STATE_MSG_PARITY;
				break;
			   case SC_EXTENDED_MESSAGE:	
#ifdef SMINFO
                                target_accstate[MAXID(un->un_target)] |= TAR_EXT_MSG;
#endif SMINFO
				DPRINTF1("sm_intr: extended_msg, state= %x\n", 
					smsnap->cur_state);
#ifdef SMSYNC
                                DPRINTF("sm_intr: send msg_acpt\n");
                                ESP_WR.cmd = CMD_MSG_ACPT;
                                ESP_WR.cmd = CMD_NOP;   /* needed */
                                smsnap->cur_state = STATE_MSG_SYNC;
                                dvmastat = LOOP_1SEC;
                                s = 4;
                                /* fifo_data[0] is sync_msg */
                                fifo_data[0] = smsnap->scsi_message;
                                cp = (u_char *)&fifo_data[1];
                                while (s && (--dvmastat)) {
                                        DELAY(10);	/* needed for Quantum */
                                        if (i = ESP_RD.intr) {
                                           DPRINTF2("sm_intr: Xint= %x, flag= %x\n",
                                                i, ESP_RD.fifo_flag);
                                           if (i == ESP_INT_FCMP)
                                                ESP_WR.cmd = CMD_MSG_ACPT;
                                           if (i == ESP_INT_BUS)
						ESP_WR.cmd = CMD_TRAN_INFO;
                                           DELAY(10);
                                           if (ESP_RD.fifo_flag & KEEP_5BITS) {
                                                i = ESP_RD.fifo_data;
                                                *cp++ = (u_char)i;
                                        DPRINTF2("sm_intr: Mdata= %x, flag= %x\n",
                                                        i, ESP_RD.fifo_flag);
                                                s--;
                                           }
                                        }
                                }
                                i = ESP_RD.stat & STAT_RES_MASK;
                                s = ESP_RD.intr;
                                DPRINTF2("sm_intr: S_end, stat= %x, intr= %x\n",
                                        i, s);
                                if (dvmastat == 0)  {
                                    DPRINTF("sm_intr: SYNC not supported\n");
                                    sync_offset[un->un_target] =  0;
                                } else {
                                   /* fifo_data[0] has sync_msg */
                                   DPRINTF2("sm_intr: msg_len= %x, code= %x\n",
					 fifo_data[1], fifo_data[2]);
                                   DPRINTF2("sm_intr: period= %x, offset= %x\n",
                                        fifo_data[3], fifo_data[4]);
				   if (fifo_data[1] != SC_EXT_LENGTH) {
					EPRINTF1("sm_intr: bad sync_len= %x\n",
						fifo_data[1]);
				   }
                                   switch (fifo_data[2]) {
                                     case  ZERO: /* modify data ptr */
                                        i = ((int)fifo_data[3] & KEEP_LBYTE) << 24;
                                        i |= ((int)fifo_data[4] & KEEP_LBYTE) << 16;
                                        i |= ((int)fifo_data[5] & KEEP_LBYTE) << 8;
                                        i |= ((int)fifo_data[6] & KEEP_LBYTE);
                                        DPRINTF1("sm_intr: M_ptr, value= %x\n", i);
                                        /* add SIGNED argument to current cnt */
                                        un->un_dma_addr += i;
                                        un->un_dma_curcnt += i;
					SM_LOG_STATE(STATE_MSG_EXTEND, fifo_data[2],-1);
                                        break;
                                     case SC_SYNCHRONOUS: /* 1= sync xfr code */
					if (sm_set_sync(un))
					     goto INTR_MSG_REJECT;
					ESP_WR.sync_period = 
                                		sync_period[un->un_target];
					ESP_WR.sync_offset = 
                                		sync_period[un->un_target];
					SM_LOG_STATE(STATE_MSG_SYNC,
						 un->un_target, -1);
					break;
                                     default:
					SM_LOG_STATE(STATE_MSG_EXTEND, fifo_data[2],-1);
                                        EPRINTF1("sm_intr: Bad sync_msg, d2= %x\n",
						fifo_data[2]);
INTR_MSG_REJECT:
					/* send reject msg */	
                                	DPRINTF("sm_intr_R: reject sync_msg\n");
                                	ESP_WR.cmd = CMD_SET_ATN; 
                                	ESP_WR.cmd = CMD_MSG_ACPT;
	                                smsnap->cur_state = STATE_MSGOUT_REQ;
                                   }
                                }
                                DPRINTF1("sm_intr: sync_known on target= %x done\n",
                                        un->un_target);
                                smsnap->sync_known |= (1 << un->un_target);
                                smsnap->cur_state = STATE_SYNC_DONE;
				break;
#else SMSYNC
                                EPRINTF("sm_intr: sync_msg not supported\n");
INTR_MSG_REJECT:
			        /* force it goes to msg_out */
                                DPRINTF("sm_intr: reject sync_msg\n");
                                ESP_WR.cmd = CMD_SET_ATN; 
                                ESP_WR.cmd = CMD_MSG_ACPT;
                                smsnap->cur_state = STATE_MSGOUT_REQ;
                                goto INTR_CHK_STAT;
#endif SMSYNC
			   case SC_DEVICE_RESET:
				SM_LOG_STATE(STATE_MSG_RESET, -1, -1);
				EPRINTF("sm_intr: RESET_err msg\n");
				smsnap->cur_state = STATE_RESET;
				smsnap->cur_err  = SE_RESET;
				break;	
			   case SC_ABORT:
				SM_LOG_STATE(STATE_MSG_ABORT, -1, -1);
				EPRINTF("sm_intr: ABORT_err msg\n");
				smsnap->cur_state = STATE_MSG_ABORT;
                                goto INTR_MSG_REJECT;
			   case SC_IDENTIFY:
			   case SC_DR_IDENTIFY:
				SM_LOG_STATE(STATE_MSG_IDENT, -1, -1);
				EPRINTF2("sm_intr: ident_msg %x, phase= %x\n", 
					smsnap->scsi_message, smsnap->cur_state);
				EPRINTF1("sm_intr: fifo_flag= %x\n", 
					ESP_RD.fifo_flag); 
				break;		/* ignored */
			   case SC_MSG_REJECT:
				SM_LOG_STATE(STATE_MSG_REJECT, -1, -1);
#ifdef SMSYNC
                		if ((smsnap->sync_known & (1 << un->un_target)) 
				   == 0) {
					/*
					 * Target does not support SYNC transfers.
					 * Set to ASYNC and set target
					 * sync_checking done.
					 */
					EPRINTF1("sm_intr: asynch only, target= %d\n", 
						un->un_target);
					ESP_WR.sync_period = 0;
					ESP_WR.sync_offset = 0;
					sync_period[un->un_target] = 0;
                                        sync_offset[un->un_target] = 0;
                                        smsnap->sync_known |= (1 << un->un_target);
                                } else {
					EPRINTF2("sm_intr: msg reject, flag= 0x%x, target= %x\n",
						ESP_RD.fifo_flag, un->un_target);
				}
#endif SMSYNC
				smsnap->cur_state = STATE_MSG_REJECT;
				break;
			   /*case SC_LINK_CMD_CPLT:*/
			   /*case SC_FLAG_LINK_CMD_CPLT:*/
			   default:
				SM_LOG_STATE(STATE_MSG_UNKNOWN,
					 smsnap->scsi_message, -1);
				EPRINTF2("sm_intr: unsupported_msg: %x, phase= %x\n", 
					smsnap->scsi_message, smsnap->cur_state);
				/* accept the message, but send an abort msg */
			        EPRINTF1("sm_intr: fifo_flag= %x\n", ESP_RD.fifo_flag);
                                /* send reject msg */
                                ESP_WR.fifo_data = SC_MSG_REJECT;
                                ESP_WR.cmd = CMD_SET_ATN;
                                ESP_WR.cmd = CMD_TRAN_INFO;
                                smsnap->cur_state = STATE_MSGOUT_REQ;
                                goto INTR_CHK_NEXTINT;
			}
			DPRINTF("sm_intr: Send msg_acpt\n"); 
			/* send a message accpt cmd */
			ESP_WR.cmd = CMD_NOP;	/* needed */
			ESP_WR.cmd = CMD_MSG_ACPT;
			goto INTR_CHK_NEXTINT;	/* will result bus_int */
	          case ESP_PHASE_MSG_OUT:
#ifdef SMDEBUG
			if (esp_debug) {
				EPRINTF1("msg_out, un= %x\n", un);
				sm_debug_break(38);
			}
#endif SMDEBUG
			if ((un == NULL) || (smsnap->cur_state == STATE_ENRSEL_REQ))
				goto INTR_CLEAR_MSGOUT;
#ifdef SMINFO
                        target_accstate[MAXID(un->un_target)] |= TAR_MSGOUT;
#endif SMINFO
			DPRINTF2("sm_intr: sync_known= %x, target= %x\n",
                                smsnap->sync_known, un->un_target);
#ifdef SMSYNC
                        /* check the target been query on sync */
                        if ((smsnap->sync_known & (1 << un->un_target)) == 0) {
                                ESP_WR.fifo_data = SC_EXTENDED_MESSAGE;
                                ESP_WR.fifo_data = SC_EXT_LENGTH;
                                ESP_WR.fifo_data = SC_SYNCHRONOUS;
                                ESP_WR.fifo_data = DEF_SYNC_PERIOD;
				ESP_WR.fifo_data = DEF_SYNC_OFFSET;
				SM_LOG_STATE(STATE_MSG_SYNC, -1, -1);
				DPRINTF2("sm_intr: sync_query, flag= %x, state= %x\n",
					ESP_RD.fifo_flag, smsnap->cur_state);
				/* keep cur_state as synchk-req */
			} else {
INTR_CLEAR_MSGOUT:
			   smsnap->cur_state = STATE_MSGOUT_DONE;
			   /* with sync support, send a NOP to clear the msg_out */
                           /* without this, we need a way to enable sync_offset */
			   DPRINTF2("sm_intr: msg_out, state= %x, flag= %x\n",
					smsnap->cur_state, ESP_RD.fifo_flag);
			   ESP_WR.fifo_data = SC_NO_OP;
                           ESP_WR.cmd = CMD_SET_ATN;
			}
                        ESP_WR.cmd = CMD_TRAN_INFO;
			goto INTR_CHK_NEXTINT;
#else SMSYNC
INTR_CLEAR_MSGOUT:
                                /* could be from parity error */
		      EPRINTF2("sm_intr: unexpected msg_out, state= %x, stat= %x\n",
				smsnap->cur_state, smsnap->esp_stat);
                                ESP_WR.cmd = CMD_FLUSH;
				ESP_WR.cmd = CMD_NOP;
                        	ESP_WR.fifo_data = SC_ABORT;
                                ESP_WR.cmd = CMD_SET_ATN;
       	                        ESP_WR.cmd = CMD_TRAN_INFO;
                               	smsnap->cur_state = STATE_MSGOUT_REQ;
                               	goto INTR_CHK_NEXTINT;
                        }
#endif SMSYNC
		  case ESP_PHASE_COMMAND:
			SM_LOG_STATE(STATE_COMMAND, un->un_cdb.cmd, -1);
			 DPRINTF2("sm_intr: command phase, state= %x, flag= %x\n",
                                smsnap->cur_state, ESP_RD.fifo_flag);
#ifdef SMSYNC
			 if (smsnap->cur_state != STATE_CMD_DONE) {
                             ESP_WR.cmd = CMD_FLUSH;
                             ESP_WR.cmd = CMD_NOP;
                             /* attempt to send command bytes out */
                             cp = (u_char *)&un->un_cdb.cmd;
                             if (((s = sc_cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) 
			        == 0) && ((s = un->un_cmd_len) == 0))
                      		  s = (int)sizeof(struct scsi_cdb);
                             DPRINTF1("sm_intr: sync_cmd, cnt= %x\n", s);
			     for (i = 0; (u_char)i < (u_char)s; i++)  {
		                DPRINTF1(" %x ", *cp);
                		ESP_WR.fifo_data = *cp++;
        		     }
                             ESP_WR.cmd = CMD_TRAN_INFO;
                             smsnap->cur_state = STATE_CMD_DONE;
#ifdef SMINFO
                             target_accstate[MAXID(un->un_target)] |= TAR_COMMAND;
#endif SMINFO
			   }
			   goto INTR_CHK_NEXTINT;
#else
#ifdef SMINFO
                           target_accstate[MAXID(un->un_target)] |= TAR_COMMAND;
#endif SMINFO
                           ESP_WR.cmd = CMD_NOP;
                           goto INTR_CHK_STAT;
#endif SMSYNC
		  default: 	/* all others are bad, just set PHASE error */
			EPRINTF1("sm_intr: bad phase: %x\n", smsnap->cur_state);
			smsnap->cur_err = SE_PHASERR;
		}
		TRACE_CHAR(MARK_INTR + 4);
		break;
	   case INT_ILL_RESEL:	/* illegal cmd/resel */
	   case INT_ILL_RESEL_OK:	/* illegal cmd/resel/fcmp */
	   case INT_ILL_RESEL_OK1:	/* illegal cmd/resel/bus */
		DPRINTF2("sm_intr: illegal resel= %x, int= %x\n", 
			ESP_RD.cmd, smsnap->esp_intr);
		DPRINTF2("sm_intr: pre_phase= %x, stat= %x\n", 
			smsnap->cur_state, smsnap->esp_stat);
		/* fall down to continue */
	    case ESP_INT_RESEL:		/* reselection INT */
	    case INT_RESEL_OK:
		switch (smsnap->cur_state) {
		  case STATE_SEL_REQ:
		  case STATE_SYNCHK_REQ:
			DPRINTF2("sm_intr: sel_collison, stat= %x, flag= %x\n",
				ESP_RD.stat, ESP_RD.fifo_flag);
			/* do not read off fifo_data, since we might have */
			/* all nine bytes - select and reselect data */
			/* but we must clear the "illegal interrupt */
			DPRINTF1("\nsm_intr: collison_intr= %x\n", s);
			/* leave the c_tab.b_active to continue on reselection */
                        un->un_md->md_utab.b_active |= MD_PREEMPT;
                        un->un_wantint = 0;
			/* fall down */
       		  case STATE_DSRSEL_REQ:
			DPRINTF2("sm_intr: resel_cont, state= %x, stat= %x\n",
				smsnap->cur_state, smsnap->esp_stat);
			s = ESP_RD.intr;
			DPRINTF2("sm_intr: intr= %x, flag= %x\n", 
				s, ESP_RD.fifo_flag);
			/* fall down */
		  case STATE_FREE:
		  case STATE_ENRSEL_REQ:
			smsnap->sub_state = TAR_RESELECT;
			/* Get target's identify message from "ESP reg_fifo" */
                        /* two bytes in fifo, (bus id) & (identify message+lun) */
                        /* target id is NOT ENCODED */
                        DPRINTF1("sm_intr: resel fifo_flag= %x\n",
                                ESP_RD.fifo_flag);
			DPRINTF1("sm_intr: recon_int, state= %x\n", 
				smsnap->cur_state);

	                s = smsnap->scsi_message = ESP_RD.fifo_data;
			for (i = 0; i < 7; i++) {
				if (s & 1)
					break;
				s >>= 1;
			}

			/* If bus id = host id, we're in trouble. */
	                if ((smsnap->scsi_message & ~scsi_host_id) == 0) {
				EPRINTF1("sm_intr: recon_host_err, id= %x\n",
					 i);
				SM_LOG_STATE(STATE_BAD_RESELECT, -1, -1);
				smsnap->cur_err = SE_RECONNECT;	
				goto INTR_CHK_ERR;
			}
			s = ESP_RD.fifo_data;		/* ident + lun */
			/* no need to service the interrupt */
                        if ((s & SC_IDENTIFY) == 0) {
                                EPRINTF1("sm_intr: bad resel ident_msg= %x\n", s);
				SM_LOG_STATE(STATE_BAD_RESELECT, i, -1);
                                smsnap->cur_err = SE_RECONNECT;
                                goto INTR_CHK_ERR;
                        }
 			s &= KEEP_3BITS;	/* lun */
			SM_LOG_STATE(STATE_RESELECT, i, s);
			/* attempt to reconnect previous disconnected job */
			DPRINTF2("sm_intr: reselected id = %x, lun= %x\n", 
				c->c_recon_target, i);
			DEBUG_DELAY(1000000);
			ESP_WR.cmd = CMD_FLUSH;
			ESP_WR.cmd = CMD_NOP;
#ifdef SMSYNC
                        if (sync_offset[i]) {
                           DPRINTF2("sm_intr: sync_RESEL on target= %x, stat= %x\n",
				i, smsnap->esp_stat);
			        ESP_WR.sync_period = sync_period[i];
                                ESP_WR.sync_offset = sync_offset[i];
				TRACE_CHAR(MARK_INTR);
			} else {
                                DPRINTF("sm_intr: set no sync on ESP after resel\n");
			        ESP_WR.sync_period = 0;
                                ESP_WR.sync_offset = 0;
			}
#endif SMSYNC
                        ESP_WR.cmd = CMD_MSG_ACPT;
			c->c_recon_target = i;		/* target */ 
			if (sm_recon_queue(c, (short)i, (short)s, 0)) {
			   smsnap->cur_err = SE_RECONNECT;	
			   goto INTR_CHK_ERR;
			}
			un = c->c_un;
		  	smsnap->cur_state = STATE_RESEL_DONE;
			smsnap->num_dis_target--;

			DPRINTF1("sm_intr: reselect_OK, flag= %x\n",
				ESP_RD.fifo_flag);
			DEBUG_DELAY(1000000);
#ifdef SMINFO
                	target_accstate[MAXID(un->un_target)] = TAR_RESELECT;
#endif SMINFO
			smsnap->cur_retry = 0;
			smsnap->cur_target = un->un_target;
			goto INTR_CHK_NEXTINT;
		default:
			EPRINTF1("sm_intr: Bad resel_state= %x\n", 
				smsnap->cur_state);
			smsnap->cur_err = SE_PHASERR;	
			goto INTR_CHK_ERR;
		}	

	   case ESP_INT_RESET:		/* external scsi bus reset */
		EPRINTF("sm_intr: external scsi bus reset\n");
		smsnap->cur_err = SE_RESET;
		goto INTR_CHK_ERR;

           case ESP_INT_SELATN:		/* selected with ATN -- target cmd */ 
   	   case ESP_INT_SEL:		/* selected without ATN -- target cmd */
	   default:
		EPRINTF1("sm_intr: bad interrupt= %x\n", smsnap->esp_intr);
		smsnap->cur_err = SE_PHASERR;
	}

INTR_CHK_ERR:
	TRACE_CHAR(MARK_INTR + 6);
	TRACE_CHAR(smsnap->cur_err);
	/* Just simply pass the status to upper level callers */
	if (smsnap->cur_err) {
		/* for ERROR case, no DMA update */
#ifdef		SMSYNC
		DPRINTF2("sm_intr: ERR= %x, target= %x\n",
			smsnap->cur_err, un->un_target);
		DPRINTF2("sm_intr: state= %x, stat= %x\n",
			smsnap->cur_state, smsnap->esp_stat);
#endif		SMSYNC
		DEBUG_DELAY(1000000);
		/*
		 * For PARITY or any error, HOST can assert ATN with
		 * msg=ABORT to let target know the error condition,
		 * other than just RESET.
		 */
		switch (smsnap->cur_err) {
		case SE_SELTOUT:  /* selection timed out */
			DPRINTF2("sm_intr: selection timeout, step= %x, cmd= %x,",
				smsnap->esp_step, ESP_RD.cmd);
			DPRINTF2(" stat= %x, intr= %x\n",
				smsnap->esp_stat, smsnap->esp_intr);
			ESP_WR.cmd = CMD_FLUSH;
			ESP_WR.cmd = CMD_NOP;
#ifdef			SMSYNC
			sync_period[un->un_target] = ESP_WR.sync_period = 0;
			sync_offset[un->un_target] = ESP_WR.sync_offset = 0;
			/*
			 * If synch check fails and we've been using it
			 * earlier, it must not like synch messages.
			 * Otherwise, it's not there....
			 */
			if (smsnap->cur_state == STATE_SYNCHK_REQ  &&
			    un->un_present) { 
				/* Device doesn't like sych message! */
				if (scsi_debug) {
					printf("sm%d:  target %d reverting to async operation\n",
						SMNUM(c), un->un_target);
				}
				smsnap->sync_known |= 1 << un->un_target;
				smsnap->cur_err = SE_RETRYABLE;
			} else {
				/* Device is not there, take it offline. */
				smsnap->cur_err = SE_FATAL;
				sm_off(c, un, SE_FATAL);
				smsnap->sync_known &= ~(1 << un->un_target);
			}
#else			SMSYNC
			/* Device is not there, take it offline. */
			smsnap->cur_err = SE_FATAL;
			sm_off(c, un, SE_FATAL);	/* take unit offline */
#endif			SMSYNC
			break;

		case SE_RECONNECT: /* failed to reconnect job */	
		case SE_RSELTOUT:  /* reselection timed out */
		case SE_RESET:	   /* external reset detected */
		case SE_PARITY:    /* internal/external parity ERR */
		case SE_FIFOVER:   /* ESP status error, fifo overflow */
		case SE_CMDOVER:   /* ESP status error, command overflow */ 
			smsnap->cur_retry = RESET_INT_ONLY;
			sm_reset(c, PRINT_MSG);
			smsnap->cur_err = SE_RETRYABLE; /* retry later */
			break;

		case SE_MEMERR:	   /* memory exception error during DMA */
		case SE_DVMAERR:   /* DVMA Drain stuck */
		case SE_MSGERR:	   /* ESP has un-expected message */
		case SE_PHASERR:   /* ESP has un-expected phase */
		case SE_LOST_BUSY: /* lost busy */
		case SE_TOUTERR:   /* timeout error */
			smsnap->cur_retry = RESET_ALL_SCSI;
			sm_reset(c, PRINT_MSG);
			smsnap->cur_err = SE_RETRYABLE;
			break;

		case SE_ILLCMD:		/* illegal command detected */
		case SE_DIRERR:		/* DMA direction, could trash MEM */
		case SE_REQPEND:	/* DMA's request pending stuck */
			smsnap->cur_retry = RESET_ALL_SCSI;
			sm_reset(c, PRINT_MSG);
			smsnap->cur_err = SE_FATAL;
			break;

		case SE_SPURINT:	
		default:
			printf("sm%d:  spurious interrupt\n", SMNUM(c));
			sm_print_state(c);
			smsnap->cur_err = SE_NO_ERROR;
			goto INTR_CHK_NEXTINT;
		}
	    smsnap->cur_state = STATE_FREE;
	}
	DPRINTF2("sm_intr: end_err= %x, state= %x\n", 
		smsnap->cur_err, smsnap->cur_state);
        DEBUG_DELAY(1000000);

INTR_JOB_DONE:
	if ((smsnap->cur_state == STATE_FREE) && (un != NULL)) {
		/* return scsi bus status (chk condition, etc) */
		cp = (u_char *)&un->un_scb;
		cp[0] = smsnap->scsi_status;	
		c->c_tab.b_active &= C_QUEUED; /* release queue lock */ 
		DPRINTF1("sm_intr: scsi_status= %x\n", smsnap->scsi_status); 
		if (un->un_wantint) {	/* error or not */
			un->un_wantint = 0;
			/* final errors= NO_ERROR, TIMEOUT, RETRYABLE, FATAL */
			/* iodone() will be indirectly called by upper-level*/
#ifdef			SMDEBUG
			if (esp_debug) {
			    if (un->un_dma_count) {
			    	EPRINTF2("sm_intr: ret_resid= %x, flag= %x\n", 
				   un->un_dma_count, un->un_flags);
			    }
		  	    EPRINTF2("sm_intr: job done, estat= %x, stat= %x\n", 
				smsnap->esp_stat, ESP_RD.stat);
			}
#endif			SMDEBUG
#ifdef			SMINFO
			target_accstate[MAXID(un->un_target)] |= TAR_JOBDONE;
#endif			SMINFO

		     	(*un->un_ss->ss_intr)(c, un->un_dma_count, 
				smsnap->cur_err);
			DEBUG_DELAY(1000000);
    			if ((smsnap->cur_err) &&
	    		    ((int)smsnap->num_dis_target > (int)0)) {
   				ESP_WR.cmd = CMD_EN_RESEL;
			/*
			 * Need to re-send EN_RESEL cmd, since ESP will
			 * disable reselection after completing reconnection.
			 */ 
			DPRINTF1("sm_intr: num_dis_target= %x\n", 
				smsnap->num_dis_target);
			/*
			 * Set ESP's internal mode to respond target
			 * reselection.
			 */
				smsnap->cur_state = STATE_ENRSEL_REQ;
     	 		/* NO interrupt will be generated from this command */
    			}
			smsnap->cur_err = SE_NO_ERROR;
		}
		/*
		 * Re-enable RESEL after a completion of current command
		 * and there was a disconnect still pending.
		 */
		un = c->c_un;
		smsnap->cur_retry = 0; 
	}	/* All other states skip clearing active */

INTR_CHK_NEXTINT:
	smsnap->sub_state &= ~TAR_START;	
	/* ONLY after job completed, check had previous job waiting on RESEL */
	/* if so, turn on "en_resel" to pick up target's reconnection */
	/* un could be NULL from flushing thorugh sm_idle */
	TRACE_CHAR(MARK_INTR + 7);
	TRACE_CHAR(smsnap->num_dis_target);
	/* STAYING in SERVICE ROUTINE to shorten the interrupt respond time */ 
	if (dvma->scsi_ctlstat & DVMA_INTPEND) {
		DPRINTF1("sm_intr: new INT pending, dma_stat= %x\n", 
			dvma->scsi_ctlstat);
		goto INTR_HANDLING;
	}
	TRACE_CHAR(MARK_INTR + 8);
	TRACE_CHAR(smsnap->cur_state);
	DPRINTF1("sm_intr: done, phase= %x\n", smsnap->cur_state);

	/* un could be NULL from "c->c_un", which was disconnected earlier */
	/* un checking has been added in sm_start() as of 9/1/88 */
	if ((smsnap->cur_state == STATE_FREE) ||
	    (smsnap->cur_state == STATE_ENRSEL_REQ)) {
		un = (struct scsi_unit *)c->c_tab.b_actf;
		if (un != NULL) 
			sm_start(un);	/* ALL done, fire up the next job */
	}
	TRACE_CHAR(MARK_INTR);
}


/*
 * No current activity for the scsi bus. May need to flush some
 * disconnected tasks if a scsi bus reset occurred before the
 * target reconnected, since a scsi bus reset causes targets to 
 * "forget" about any disconnected activity.
 * Also, enable reconnect attempts.
 */
sm_idle(c, flag)
register struct scsi_ctlr *c;
int	flag;
{
	register struct scsi_unit *un;
	int	i, s;

	DPRINTF("sm_idle:\n");
	if (c->c_flags & SCSI_FLUSHING) {
		DPRINTF1("sm_idle: flushing, flags 0x%x\n", c->c_flags);
		return;
	}
	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {
		DPRINTF("sm_idle: flushing disconnect que\n"); 
		s = splr(pritospl(c->c_intpri));        /* time critical */
		c->c_tab.b_active = C_ACTIVE;           /* engage interlock */ 
		/* 
		 * Force current I/O request to be preempted and put it 
		 * on disconnect que so we can flush it.  
		 */
		un = (struct scsi_unit *)c->c_tab.b_actf; 
		if ((un != NULL) && 
		    (un->un_md->md_utab.b_active & MD_IN_PROGRESS)) {
                        EPRINTF("sm_idle: dequeued active req\n");
                        sm_discon_queue(un);
                }
                /* now in process of flushing tasks */
                c->c_flags &= ~SCSI_FLUSH_DISQ;
                c->c_flags |= SCSI_FLUSHING;
                c->c_flush = c->c_disqtab.b_actl;

		for ((un = (struct scsi_unit *)c->c_disqtab.b_actf); 
                     (un && (c->c_flush)); 
		     (un = (struct scsi_unit *)c->c_disqtab.b_actf)) {
			
			/* keep track of last task to flush */
			if (c->c_flush == (struct buf *)un) 
				c->c_flush = NULL;
			
			/* requeue on controller active queue */
			if (sm_recon_queue(c, un->un_target, un->un_lun, 0))
				continue;

			i = un->un_dma_curcnt;	
			DPRINTF2("sm_idle: target= %d, lun= %d,", 
				un->un_target, un->un_lun);
			DPRINTF2(" state= 0x%x, resid= %d\n", 
				smsnap->cur_state, i);
			(*un->un_ss->ss_intr)(c, i, flag);
			sm_off(c, un, flag);
		}
		c->c_flags &= ~SCSI_FLUSHING;
                c->c_tab.b_active &= C_QUEUED;  /* Clear interlock */
                (void) splx(s);
	}
}



/*
 * Return residual count for a dma.
 */
sm_dmacnt(c)
	struct scsi_ctlr *c;
{
	return (c->c_un->un_dma_count);
}

/* only two arguments, per upper target driver */
sm_reset(c, msg_enable)
register struct scsi_ctlr *c;
int msg_enable;		/* for printing error infos */
{
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register int i, j;

	TRACE_CHAR(MARK_RESET);
	TRACE_CHAR(smsnap->cur_state);
	smr = esp_reg_addr;
        dvma = dvma_reg_addr;

	if (dvma->scsi_ctlstat & DVMA_REQPEND) {
		sm_print_state(c);
		smsnap->cur_state = STATE_DVMA_STUCK;
		smsnap->cur_retry = RESET_EXT_ONLY;
	}

	/* write ESP reg_conf to disable RESET_INT */	
	ESP_WR.conf = ESP_CONF_DISRINT;

	switch (smsnap->cur_retry) {
	case RESET_EXT_ONLY:
		if (msg_enable  ||  scsi_debug) {
			printf("sm%d:  resetting scsi bus\n", SMNUM(c));
			sm_print_state(c);
		}
		ESP_WR.cmd = CMD_RESET_ESP;	/* hard-reset ESP chip */
		ESP_WR.cmd = CMD_NOP; 		/* needed for ESP bug */
		ESP_WR.cmd = CMD_RESET_SCSI;
		ESP_WR.cmd = CMD_NOP; 		/* NOP needed for ESP bug */
		SM_LOG_STATE(STATE_RESET, smsnap->cur_retry, 0);
		DELAY(scsi_reset_delay);	/* Allow reset recovery time */
		break;

	case RESET_INT_ONLY:
		/* if being called by upper level, just do internal reset */
		if (scsi_debug || (msg_enable  &&  smsnap->cur_err != SE_RESET)) {
			printf("sm%d:  resetting interface\n", SMNUM(c));
			sm_print_state(c);
		}
		/* DVMA_EN = ZERO, INTEN=ZERO, FLUSH=1 */
		dvma->scsi_ctlstat = DVMA_FLUSH; /* soft dvma clear */
		DELAY(10);
		dvma->scsi_ctlstat = 0;		/* disable int. */
		/* NO ints */
		ESP_WR.cmd = CMD_RESET_ESP;	/* hard-reset ESP chip */
		ESP_WR.cmd = CMD_NOP; 		/* needed for ESP bug */
		SM_LOG_STATE(STATE_RESET, 0, 1);
		if (smsnap->cur_err != SE_RESET)
			dvma->scsi_ctlstat = DVMA_INTEN; /* enable int. */
		break;

	case RESET_ALL_SCSI:
	default:
		if (msg_enable) {
			printf("sm%d:  resetting scsi bus\n", SMNUM(c));
			sm_print_state(c);
		}
		/* implied a reset to both ESP and SCSI bus */
		dvma->scsi_ctlstat = DVMA_RESET;	/* hard DVMA reset */	
		DELAY(10);
		dvma->scsi_ctlstat = 0;	/* clear reset */
		ESP_WR.cmd = CMD_RESET_SCSI;
		ESP_WR.cmd = CMD_NOP; 		/* NOP needed for ESP bug */
		SM_LOG_STATE(STATE_RESET, -1, -1);
		DELAY(scsi_reset_delay);	/* Allow reset recovery time */
		break;
	}
	i = ESP_RD.intr;			/* clear reset interrupt */
/*	ESP_WR.cmd = CMD_NOP;			/* needed after reset */
	DPRINTF1("sm_reset: esp_intr after reset= %x\n", i);
	ESP_WR.clock_conv = DEF_CLK_CONV; 	/* 5 (@24Mhz); 4 @20Mhz */
	ESP_WR.timeout = DEF_TIMEOUT;	 	/* 93 (hex) (250msec@24Mhz) */
	ESP_WR.sync_period = 0;		 	/* async */
	ESP_WR.sync_offset = 0;		 	/* async */

	/* Enable parity checking, enable reset_int, and set host bus_id. */
	j = 1;
	for (i = 0; i < NTARGETS -1; i++) {
		if (scsi_host_id == j)
			break;
		j <<= 1;
	}
	ESP_WR.conf = i;
	DPRINTF1("sm_reset: scsi_host_id= %d\n", i);

	/*
	 * Set up some default sm_snap. Variables cur_err, cur_retry,
	 * and scsi_status will be cleared in sm_cmd.
	 */
	smsnap->num_dis_target = smsnap->cur_retry = 0;
	smsnap->scsi_message = SC_COMMAND_COMPLETE;
	smsnap->cur_state = smsnap->sub_state = STATE_FREE;

	/*
	 * Force sync negotiate on all targest except ones which timed out
	 * in data phase on us.
	 */
	smsnap->sync_known = sync_target_err;

#ifdef	SMSYNC
	for (i = 0; i < NTARGETS -1; i++) {
#ifdef		SMINFO
                target_accstate[i] = 0;
#endif		SMINFO 
		sync_period[i] = 0;	/* clear sync period reg */ 
		sync_offset[i] = 0;	/* initaially set ASYNC xfr */
	}
#endif	SMSYNC

	/* set ESP's internal mode bit to respond target reselection */
	TRACE_CHAR(dvma->scsi_ctlstat);
#ifdef	SMDEBUG
	DPRINTF2("sm_reset: actf= %x, discon_actf= %x\n", 
		c->c_tab.b_actf, c->c_disqtab.b_actf);
#endif	SMDEBUG	

	if ((c->c_disqtab.b_actf != NULL) || (c->c_tab.b_actf != NULL)) {
		/* both active and disconnect queue needs to be flushed */
		c->c_flags |= SCSI_FLUSH_DISQ;
		DPRINTF1("sm_reset: flushing discon_queue, sync_known= %x\n",
			smsnap->sync_known);
		sm_idle(c, SE_TIMEOUT);
	}
	TRACE_CHAR(MARK_RESET);
	DEBUG_DELAY(1000000);
}
					
					
sm_set_sync(un)
struct scsi_unit *un;
{
	u_int  period, offset, tickval = 0, regval = 0;

	period = fifo_data[3] & 0xff;
	offset = fifo_data[4] & 0xff;
	DPRINTF2("sm_set_sync: target's negotiate_vaule= %xh, off= %x\n", 
		period, offset);
        if (offset == 0) { /* async */
		sync_offset[un->un_target] = 0;
		EPRINTF1("sm_set_sync: target %d set to async\n", 
			un->un_target);
		return(OK);
        }
	if (offset > DEF_SYNC_OFFSET) {	/* 15 */
		EPRINTF1("sm_set_sync: sync offset (%d) out of range\n", offset);
		return(FAIL);
	} else {
	  	/* if not, check sync period value */
		if ((period > MAX_SYNC_PERIOD) || (period < MIN_SYNC_PERIOD)) {
			EPRINTF1("sm_set_sync: sync period (%d) out of range\n",
				 period);
			return(FAIL);

		} else {
                        /*
			 * Conversion method for received PERIOD value to the
		  	 * number of input clock ticks to the ESP.
			 * 
			 * We adjust the input period value such that we always
			 * will transmit data *not* faster than the period 
			 * value received.  
			 */ 
			regval = CLOCK_PERIOD;
			DPRINTF1("sm_set_sync: ESP clock_period= %d\n", 
				regval);
	DPRINTF2("sm_set_sync: host_period (min-max)= %d nsec\n", 
				MIN_SYNC_CYL * regval, MAX_SYNC_CYL *regval);
			 while(((period<<2)+tickval) % regval) 
				tickval++; 
	DPRINTF2("sm_set_sync: target req_period= %d(nsec), diff= %d\n", 
				period << 2, tickval);
			 tickval = ((period<<2)+tickval) / regval;
 
			 /* 
			  * Now, we have a good integer number of input ESP 
			  * clock ticks that will guarantee safe transfer 
			  * to the target. We have to now figure out what 
			  * sync period register value to use. The ESP period 
			  * register is a pretty strange beast. We also have 
			  * to change the value if we are in 'Dirty Cable' 
			  * mode (set in the configuration register).  
			  */ 
			 if (tickval < MIN_SYNC_CYL) { 
				regval = MIN_SYNC_CYL; 
			 } else {
				if (tickval & ~0x1f) { 
					regval = tickval & 0x1f; 
				} else { 
					regval = tickval; 
				}
                                sync_period[un->un_target] = regval;
                                sync_offset[un->un_target] = offset;
DPRINTF3("sm_set_sync: SET for target(%d)..sync_period= %d, offset= %d\n", 
	un->un_target, sync_period[un->un_target], sync_offset[un->un_target]);
			}
		}
	}
	return(OK);
}

/* add here for compatiblity with existing SCSI drivers */
sm_getstatus()
{
}

sm_cmd_wait()
{
}

#ifdef P4DVMA
/* special routine for the P4-I/O mapper and debugg trigger */
#define P4IOMAP_ADDR	0xff200000
#define P4IOMAP_MASK	0xffffe000
#define P4IOMAP_DTMASK	0xfffffff8
#define P4IOMAP_DT	1 	/* DT descriptor type = valid page */
#define P4IOMAP_SIZE	0x2000	/* 8K byte page */
#define P4DEBUG_ADDR	0xff800000

get_iomapbase()
{
	int	extent, offset;
	long	a = 0;
	u_int  pageval;
	register u_long *ptr;	
	int s;

	/* find and reserve a page in kernel map for the P4-I/O Mapper */
	offset = P4IOMAP_ADDR;
	offset &= MMU_PAGEOFFSET;
	pageval = btop(P4IOMAP_ADDR);
	
	extent = mmu_btopr(offset + P4IOMAP_SIZE);
	DPRINTF2("get_iomapbase: pageval= %x, extent= %x\n", 
		pageval, extent);
	
	s = splhigh();
	a = rmalloc(kernelmap, (long)extent);
	(void)splx(s);
	if (a == 0)
	   EPRINTF("get_iomapbase: out of kernel map\n");
	iomap_base = (u_long *)((int)kmxtob(a) | offset);
	segkmem_mapin(&kseg, (addr_t)iomap_base, (u_int)mmu_ptob(extent),
		PROT_READ | PROT_WRITE, pageval, 0);
	DPRINTF1("get_iomapbase: iomap_base= %x\n", iomap_base);
	ptr = iomap_base;
	for (offset = 0; offset < 0x800; offset++)	
		*ptr++ = NULL;
	DPRINTF("get_iopmap: CLEAR iomap, but set first entry\n");
}

static
set_iomap(v_addr, xfrcnt)
	u_long v_addr;
	int	xfrcnt;
{
	struct pte pte;
	register u_long p_addr;
	register u_long *iomap_ptr;
	
	register int	numpg;
	int	startpg, endpg;
	
	iomap_ptr = iomap_base; 
	DPRINTF2("set_iomap: dma_addr= %x, cnt= %x\n", v_addr, xfrcnt);

	startpg = v_addr >> 13;
	endpg = (v_addr+xfrcnt-1) >> 13;
	numpg = endpg - startpg + 1;

	p_addr = v_addr & 0x00ffffff;	/* take 24 bit */
	p_addr >>= 13;			/* index entry */
	p_addr <<= 2;			/* long word write to I/O map */
	DPRINTF2("set_iomap: virtual_iomap= %x, index= %x\n", 
		iomap_ptr, p_addr);
	(u_long)iomap_ptr += p_addr;	/* points to the entry */
	DPRINTF2("set_iomap: index= %x, pgcnt= %x\n", iomap_ptr, numpg);
	while (numpg) {
		mmu_getkpte((caddr_t)v_addr, &pte);
		DPRINTF1("set_iomap: pfnum= %x\n", pte.pg_pfnum);
		p_addr = (u_long)mmu_ptob(pte.pg_pfnum); 
		DPRINTF1("set_iomap: physical dma_addr= %x\n", p_addr);
		/* mask last 3 bits, write protect, and DT type */
		(u_long)p_addr &= P4IOMAP_DTMASK;	
 		(u_long)p_addr |= P4IOMAP_DT; /* turn on access vaild bit */
		DPRINTF1("set_iomap: new_paddr= %x\n", p_addr);
	
		*iomap_ptr = p_addr;	/* store the p_addr in I/O map */
		DPRINTF2("set_iomap: I/O map_addr= %x, read_in= %x\n", 
			iomap_ptr, *iomap_ptr);
		if (--numpg == 0)
			break;
		v_addr += P4IOMAP_SIZE;	/* 8k per entry */
		DPRINTF1("set_iomap: next v_addr= %x\n", v_addr);
		iomap_ptr++;
		if (iomap_ptr > iomap_base + P4IOMAP_SIZE) 
			EPRINTF1("set_iomap: IOMAP_ERR, new ptr= %x\n", 
				iomap_ptr);
	}
	DPRINTF("set_iomap: done\n");
}

p4trigger(index)
	u_long	index;
{
	u_long 	*ptr;
	u_long	l;
	
	ptr = iomap_base;
	(u_long)ptr += index;
	l = *ptr;		/* trigger the index into debug space */
}
#endif P4DVMA

#ifdef SMDEBUG
sm_debug_break(position)
	int position;
{
	EPRINTF1("sm_debug_break: from= %x\n", position);
}
#endif SMDEBUG


#endif NSM > 0
#endif (defined sun3x) || (defined sun4)

/* end of ESP SCSI OS driver (sm.c) ... KSAM */
