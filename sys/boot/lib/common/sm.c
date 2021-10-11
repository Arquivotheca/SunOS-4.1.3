/*
 * @(#)sm.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 *      Emulex ESP SCSI (low-level) driver
 *
 *
 *      3/89    KSAM    Send back exact amount of transferred bytes for 
 *			Front-load 1/2 tape per 4.0.3 FCS release.
 *      11/88   KSAM    Add in support for 4.0.3 beta release
 *      9/88    KSAM    Change to match the new 4.0 REV-2 SCSI structure
 *      8/88    KSAM    Add in Stingray support
 *      5/88    KSAM    initial written
 */
#if (defined sun4) || (defined sun3x)

#define REL4            	   /* enable release 4.00 mods */

#ifdef notdef
#define SMEPRINT	          /* Allow error printfs */
#define SMDEBUG		          /* Allow compiling of debug code */
#endif notdef

#ifdef  REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <stand/smreg.h>
#include <stand/scsi.h>
#include <stand/saio.h>
#else REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "smreg.h"
#include "scsi.h"
#include "saio.h"
#endif REL4


#ifdef  REL4
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#else REL4
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#endif REL4

/* for bring-up stage, print all error messages */
#ifdef SMDEBUG
int sm_debug = 1;               /* normally enable  error-printfs */
/* Handy debugging 0, 1, and 2 argument printfs, NEED "sm_debug" */
#define DPRINTF(str) \
        if (sm_debug > 2) printf(str)
#define DPRINTF1(str, arg1) \
        if (sm_debug > 3) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
        if (sm_debug > 4) printf(str,arg1,arg2)
#define DEBUG_DELAY(cnt) \
        if (sm_debug > 5) DELAY(cnt)
#define EPRINTF(str) \
        if (sm_debug) printf(str)
#define EPRINTF1(str, arg1) \
        if (sm_debug)  printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
        if (sm_debug) printf(str,arg1,arg2)
#else SMDEBUG
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 	
#define DPRINTF1(str, arg1) 
#define DPRINTF2(str, arg1, arg2) 
#ifdef SMEPRINT
#define EPRINTF(str) 			printf(str)
#define EPRINTF1(str, arg1) 		printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) 	printf(str,arg1,arg2)
#else
#define EPRINTF(str)
#define EPRINTF1(str, arg1) 
#define EPRINTF2(str, arg1, arg2) 
#endif SMEPRINT
#endif SMDEBUG

/*
 * Low-level routines common to all devices on the SCSI bus.
 */

/*
 * Interface to the routines in this module is via a second "h_sip"
 * structure contained in the caller's local variables.
 *
 * This "h_sip" must be initialized with h_sip->smboottab = sm_driver
 * and must then be devopen()ed before any I/O can be done.
 *
 * The device must be closed with devclose().
 */

/*
 * The interfaces we export
 */
extern int  scsi_debug;
extern char *devalloc();
extern char *resalloc();
extern int  nullsys();
extern u_char sc_cdb_size[];
int	smopen(), smdoit(), sm_reset();

/* special snapshot data structure for sm.c */ 
struct sm_snap smreg_info;
int	scsi_reset = ZERO;

#define FAIL		-1	
#define OK		0	


/*
 * Open the SCSI host adapter.
 * if found, return (OK=0), FAIL=-1 otherwise.
 */
int
smopen(h_sip)
	register struct host_saioreq	*h_sip;
{
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;

	register int	i, j;
	struct idprom id;
	int base;
	int scsi_nstd;
	enum MAPTYPES space;
	u_long	dvmastat;

	if (idprom(IDFORM_1, &id) == IDFORM_1) {
		space = MAP_OBIO;
		scsi_nstd = 1;
	        DPRINTF1("smopen: id.machine= %x\n", id.id_machine);
		switch (id.id_machine) {
#ifdef sun3x
		   case IDM_SUN3X_HYDRA:
			base = HYDRA_SCSI_BASE;
			i = HYDRA;
			break;
#endif sun3x
#ifdef sun4
		   case IDM_SUN4_STINGRAY:
			base = STINGRAY_SCSI_BASE;
			i = STINGRAY;
			break;
#endif sun4
	           default:
			EPRINTF1("smopen: unknown machine type= %x\n", 
				id.id_machine);
			/* other configuration not supported */
			return(FAIL);	
		}
	} else {
		/* unknown id type not supported */
		EPRINTF1("smopen: UNKNOWN id type= %x\n", 
			idprom(IDFORM_1, &id));
		return(FAIL);	
	}
	DPRINTF1("smopen: ctlr= %x\n", h_sip->ctlr);
	/* get base address of registers */
	if (h_sip->ctlr < scsi_nstd) {
		h_sip->ctlr = base + (h_sip->ctlr * SM_SIZE);
	} else {
		EPRINTF2("smopen: invalid cltr_num= %x, busid= %x\n", 
			h_sip->ctlr, h_sip->unit);
		return(FAIL);	
	}
	DPRINTF1("smopen: REG_base= %x\n", h_sip->ctlr);
	DEBUG_DELAY(1000000);
	/* now map it in */
	h_sip->devaddr = devalloc(space, h_sip->ctlr, 
		    sizeof(struct scsi_sm_reg));
	if (h_sip->devaddr == ZERO) {
		EPRINTF("smopen: Can not allocate ESP space\n");
		return(FAIL);	
	}
	DPRINTF1("smopen: sm_addr = %x\n", h_sip->devaddr);

	/* allocate dma resources for on-board host */
	h_sip->dmaaddr = h_sip->devaddr + STINGRAY_DMA_OFFSET;

	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;

#ifdef sun4
	DPRINTF1("smopen: peeking DVMA's ctl_stat @ %x\n", 
		&dvma->ctl_stat);
/* I only modify the lib/sun4/probe.s */
        if ((peekl((long *)&(dvma->ctl_stat))) == -1) {
                EPRINTF1("smopen: peek on DVMA-scsi dma_addr= %x FAILED\n",
               		&(dvma->ctl_stat));
        	DEBUG_DELAY(1000000);
		return(FAIL);	
        }
	DPRINTF1("smopen: DVMA exists, reg_stat= %x\n", dvma->ctl_stat);
#endif sun4
       	DEBUG_DELAY(1000000);
        dvmastat = dvma->ctl_stat;
        if (dvmastat & DVMA_REQPEND) {    
                EPRINTF1("smopen: DVMA-scsi req_pend stuck, stat= %x\n", 
			dvmastat);
		return(FAIL);	
        }
        if (dvmastat & DVMA_RESET) {
                DPRINTF1("smopen: DVMA reset asserted, stat= %x\n", i);
                /* attempt to clear it, if not successful, error return */
                dvma->ctl_stat = ZERO;
                for (i = 0; i < LOOP_2MSEC; i++);
                dvmastat = dvma->ctl_stat;
                if (dvmastat & DVMA_RESET) {
                   EPRINTF1("smopen: DVMA-scsi reset stuck, stat= %x\n", 
			dvmastat);
		   return(FAIL);	
                }
                DPRINTF1("smopen: deasserted DVMA reset, stat= %x\n", i);
        }
        /* write/read/compare DVMA 's address reg */
        dvmastat = DVMA_TPATTERN;
	dvma->dma_addr = dvmastat;
        DPRINTF2("smopen: DVMA test, wr= %x, rd= %x\n", 
		dvmastat, dvma->dma_addr);
        if (dvma->dma_addr != dvmastat) {
            EPRINTF2("smopen: DVMA-scsi mismatched dma_addr, wr= %x, rd= %x\n",
                        i, dvma->dma_addr);
		return(FAIL);	
        }
        dvma->dma_addr = ZERO;
        DPRINTF2("smopen: DVMA-scsi PASSED, stat= %x, addr= %x\n",
                dvma->ctl_stat, dvma->dma_addr);
        DEBUG_DELAY(1000000);
        
        /* check existence and operational status of DVMA before */
        /* checking on ESP.  If scsi_reset in DVMA is asserted, the */
        /* ESP register will not be accessible and DVMA itself is */
        /* NOT in a operational state */
	/* Check ESP existence -- peek on "ESP reg_xcnt_lo" */
#ifdef sun4
	DPRINTF1("smopen: peeking ESP's xcnt_lo @ %x\n", 
		&ESP_RD.xcnt_lo);
/* I only modify the lib/sun4/probe.s */
	if ((peekc((u_char *)&(ESP_RD.xcnt_lo))) == -1) {	
		EPRINTF1("smopen: peek on ESP's addr= %x FAILED\n",
                        &ESP_RD.xcnt_lo);
                DEBUG_DELAY(1000000);
		return(FAIL);	
	}
	DPRINTF1("smopen: ESP exists, reg_conf= %x\n", ESP_RD.conf);
#endif sun4
        /* perform a simple write, read and compare validation test to
           "ESP reg_conf".  If error, return */
        i = 0x55;
        ESP_WR.conf = i;
        j = ESP_RD.conf;
        DPRINTF2("smopen: ESP wr= %x, rd= %x\n", i, j);
        if ((u_char)i != (u_char)j) {
                DEBUG_DELAY(1000000);
		return(FAIL);	
        }         
        i = 0xaa;
        ESP_WR.conf = i;
        j = ESP_RD.conf;
        DPRINTF2("smopen: ESP wr: = %x, rd= %x\n", i, j);
        DEBUG_DELAY(1000000);
        if ((u_char)i != (u_char)j) {
                DEBUG_DELAY(1000000);
		return(FAIL);	
        }         
        i = ZERO;
        ESP_WR.conf = i;
        j = ESP_RD.conf;
        if ((u_char)i != (u_char)j)  {
                DPRINTF2("smopen: ESP FAILED, wr= %x, rd= %x\n", i, j);
                DEBUG_DELAY(1000000);
		return(FAIL);	
        }         
	/* Perform a simple write, read & compare validation test to ESP_II */
        i = ESP_TPATTERN;
        ESP_WR.conf2 = i;               /* write */
        j = ESP_RD.conf2;               /* read */
        if ((u_char)i == (u_char)j) {   /* compare */
                ESP_WR.conf2 = ZERO;    /* make ESP 2 function as ESP */
                DPRINTF("smopen: ESP_2 found, config to ESP\n");
        }         
        else
                DPRINTF("smopen: ESP_2 not found\n");

        DEBUG_DELAY(1000000);

	/* set all the routine pointers */
        DPRINTF("smopen: setting doit() pointer\n");
	h_sip->doit = smdoit;
	h_sip->reset = sm_reset;
	if (scsi_reset == ZERO) {
	    /* need to reset SCSI bus when call from le() boot, since */
	    /* firmware will not call scsi driver to reset the bus */
		sm_reset(h_sip, RESET_ALL_SCSI);
		scsi_reset++;
	}
	else {
		sm_reset(h_sip, RESET_INT_ONLY);
	}
        DPRINTF1("smopen: done, smr= %x\n", smr);
	return(OK);		/* successfully opened */
}


/*
 * Write a command to the SCSI bus.
 *
 * The supplied h_sip is the one opened by smopen().
 * DMA is done based on h_sip->ma and h_sip->cc.
 *
 * Returns -1 for error, otherwise returns the residual count
 *
 * FIXME, this must be accessed via a boottab vector,
 * to allow host adap to switch.
 * Must pass cdb, scb in h_sip somewhere...
*/
int
smdoit(cdb, scb, h_sip)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	struct host_saioreq *h_sip;
{
	register struct scsi_sm_reg *smr;
	register struct sm_snap *smsnap = &smreg_info;
	register struct udc_table *dvma;
	register u_char *cp;
	register int i, size;
	u_long	toutcnt;

	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;
        
	DPRINTF("smdoit: POLL/NO_disconnect mode\n");
	cp = (u_char *)cdb;
	size = sc_cdb_size[CDB_GROUPID(*cp)];
	/* Write "ESP reg_fifo" with all command bytes */
	DPRINTF("smdoit: cmd byte=");
	for (i = 0; i < size; i++) {
		DPRINTF1(" %x", *cp);
		ESP_WR.fifo_data = *cp++;
	}
	DPRINTF1("\nsmdoit: fifo_flag @= %x\n", ESP_RD.fifo_flag);
    	DEBUG_DELAY(1000000);
	DPRINTF1("smdoit: ready to send cmd= %x\n", cdb->scc_cmd);
	switch (cdb->scc_cmd) {
	  case SC_READ:
	  case SC_REQUEST_SENSE:
	  case SC_INQUIRY:
	  case SC_MODE_SENSE:
		smsnap->sync_chk = SM_RECV_DATA;
		break;
	  case SC_WRITE:
	  case SC_MODE_SELECT:
	  case SC_WRITE_FILE_MARK:
	  case SC_WRITE_VERIFY:
		smsnap->sync_chk = SM_SEND_DATA;
		break;
	    default:
		smsnap->sync_chk = SM_NO_DATA;
	}
	/* clear some status */
	smsnap->cur_err = (smsnap->scsi_status = ZERO); 
	smsnap->cur_retry = ZERO;
        smsnap->cur_state = STATE_SEL_REQ;
       	
	ESP_WR.timeout = DEF_TIMEOUT;
    	ESP_WR.busid = h_sip->unit;	/* target SCSI ID */

	DPRINTF1("smdoit: selecting target= %x\n", h_sip->unit);
    	DEBUG_DELAY(1000000);
	toutcnt = LOOP_CMDTOUT;		

	ESP_WR.cmd = CMD_SEL_NOATN; 	/* NO identify message */
    	while ((--toutcnt) && (smsnap->cur_err == ZERO) &&
	       (smsnap->cur_state != STATE_FREE)) {
		/* poll on ESP_INT thru DVMA status reg */
		if (dvma->ctl_stat & DVMA_INTPEND)  {
			sm_intr(h_sip);
			toutcnt = LOOP_CMDTOUT;	
		}	
    	}
	DPRINTF2("smdoit: err= %x, state= %x\n", 
		smsnap->cur_err, smsnap->cur_state);
	DPRINTF2("smdoit: resid= %x, status= %x\n", 
		h_sip->cc, smsnap->esp_stat);
	cp = (u_char *)scb;
	*cp = smsnap->scsi_status;	/* return scsi bus status */
	DPRINTF1("smdoit: done, bus_status= %x\n", smsnap->scsi_status);
        DEBUG_DELAY(1000000);
	if ((toutcnt == ZERO) || (smsnap->cur_err))  {
	   EPRINTF2("sm_cmd: CMD_TIMED-OUT, err= %x, toutcnt= %x\n", 
		smsnap->cur_err, toutcnt);
	   EPRINTF2("sm_cmd: cmd= %x, esp_stat= %x\n", 
		cdb->scc_cmd, ESP_RD.stat);
	   EPRINTF2("sm_cmd: state= %x, cc= %x\n", 
		smsnap->cur_state, h_sip->cc);
	   EPRINTF2("sm_cmd: dma_stat= %x, dma_addr= %x\n", 
		dvma->ctl_stat, dvma->dma_addr);
           DEBUG_DELAY(1000000);
           return(FAIL);
	}
	return (h_sip->cc);		/* successful */
}

static int
sm_dma_setup(h_sip)
	register struct host_saioreq *h_sip;
{       
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register struct sm_snap *smsnap = &smreg_info;
#ifdef SMDEBUG
	u_char *ptr;
	int	i;
#endif SMDEBUG

	if ((h_sip->cc == ZERO) || (smsnap->sync_chk == SM_NO_DATA)) {
		DPRINTF1("sm_dma_setup: return due to xfr_cnt= %x\n",
			h_sip->cc);
		return;
	}
	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;
	
#ifdef SMDEBUG
	DPRINTF1("sm_dma_setup: ORG_addr= %x, data= ", h_sip->ma);
        ptr = (u_char *)h_sip->ma;
        for (i = 0; i < 20; i++, ptr++) {
                DPRINTF1(" %x ", *ptr);
        }
	DPRINTF1("\nsm_dma_setup: ORG_cnt= %x\n", h_sip->cc);
#endif SMDEBUG
	
	dvma->dma_addr = (u_long)h_sip->ma;	/* set dma address */
	ESP_WR.xcnt_lo = (u_char)h_sip->cc;
	ESP_WR.xcnt_hi = (u_char)(h_sip->cc >> 8);
	ESP_WR.cmd = CMD_NOP | CMD_DMA;		/* NOP needed */
	DPRINTF2("sm_dma_setup: xcnt_lo= %x, xcnt_hi= %x\n", 
		ESP_RD.xcnt_lo, ESP_RD.xcnt_hi);

	/* enable DMA */
	if (smsnap->sync_chk == SM_SEND_DATA) {
	        DPRINTF("sm_cmd: setting a write to SCSI\n");
               	dvma->ctl_stat |= DVMA_ENDVMA;	
		dvma->ctl_stat &= ~DVMA_WRITE;
	}	
	else {
	        DPRINTF("sm_cmd: setting a read to SCSI\n");
		dvma->ctl_stat |= DVMA_ENDVMA | DVMA_WRITE;
	}
	DEBUG_DELAY(1000000);
}

/*
 * Reset some register information after a dma operation.
 */
static int
sm_dma_cleanup(h_sip, espstat)
	register struct host_saioreq *h_sip;
	u_char espstat;
{
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register struct sm_snap *smsnap = &smreg_info;
	register int i;
	u_long	toutcnt;
	u_short dma_cnt, esp_cnt, xfrcnt, fifo_cnt, req_cnt;
#ifdef SMDEBUG
	u_char *ptr;
 
	DPRINTF1("sm_dma_cleanup: ORG_addr= %x, NEW_data= ", h_sip->ma);
        ptr = (u_char *)h_sip->ma;
        for (i = 0; i < 20; i++, ptr++) {
                DPRINTF1(" %x ", *ptr);
        }
	DPRINTF1("\nsm_dma_cleanup: ORG_cnt= %x\n", h_sip->cc);
#endif SMDEBUG

	req_cnt = h_sip->cc;	
	if ((req_cnt == ZERO) || (smsnap->sync_chk == SM_NO_DATA)) {
		DPRINTF2("sm_dma_setup: return, xfr_cnt= %x, stat= %x\n", 
			req_cnt, espstat);
		return(ZERO);
	}
	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	
	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;
	
	dvma->ctl_stat &= ~DVMA_ENDVMA;	
	toutcnt = dvma->ctl_stat;
	DPRINTF2("sm_dma_cleanup: dvma_stat= %x, addr= %x\n",
                toutcnt, dvma->dma_addr);
        DEBUG_DELAY(1000000);
  	if (toutcnt & DVMA_REQPEND) {
                toutcnt = LOOP_4SEC;    /* must be over 2sec */
                while ((dvma->ctl_stat & DVMA_REQPEND) && (--toutcnt)) ;
                if (toutcnt == ZERO) {
		 EPRINTF1("sm_dma_cleanup: DVMA-scsi REQ_PEND struck, stat= %x\n",
                                dvma->ctl_stat);
                        smsnap->cur_err = SE_REQPEND;
                        return(1);
                }
        }

	if (smsnap->sync_chk == SM_RECV_DATA) {
	    DPRINTF("sm_dma_cleanup: send DVMA_drain\n");
            DEBUG_DELAY(1000000);
            dvma->ctl_stat |= DVMA_DRAIN; /* send DRAIN anyway */
            i = LOOP_2MSEC;
            while ((dvma->ctl_stat & DVMA_DRAIN) && (--i));
            if (i == ZERO) {
                EPRINTF1("sm_dma_cleanup: DVMA-scsi drain stuck, stat= %x\n",
                        dvma->ctl_stat);
                smsnap->cur_err = SE_DVMAERR;
		dvma->ctl_stat |= DVMA_FLUSH;
		dvma->ctl_stat &= ~(DVMA_ENDVMA | DVMA_WRITE);
                return(1);
             }
	}
 	DEBUG_DELAY(1000000);
        dvma->ctl_stat |= DVMA_FLUSH; /* self-clear */
        /* disable DVMA_xfr */
        dvma->ctl_stat &= ~(DVMA_ENDVMA | DVMA_WRITE);

        /* see how much DVMA xfr'd */
	DPRINTF2("sm_dma_cleanup: ORIG addr= %x, cnt= %x\n", h_sip->ma, req_cnt);
        toutcnt = dvma->dma_addr;
	DPRINTF2("sm_dma_cleanup: espstat= %x, DVMA_addr= %x\n", espstat, toutcnt);
        dma_cnt = (u_short)toutcnt - (u_short)h_sip->ma;
        /* see how much ESP xfr'd */
	fifo_cnt = ESP_RD.fifo_flag & KEEP_5BITS;
	DPRINTF2("sm_dma_cleanup: fifo_cnt= %x, dma_cnt= %x\n",
                fifo_cnt, dma_cnt);

        /* Figure out how much the esp chip may have xferred
         * If the XZERO bit is set, we can assume that the
         * ESP xferred all we asked for.  */
        if (espstat & ESP_STAT_XZERO)
                esp_cnt = req_cnt - fifo_cnt;
        else  {
                xfrcnt = ESP_RD.xcnt_hi << 8;
                xfrcnt |= ((u_short)ESP_RD.xcnt_lo & KEEP_LBYTE);
                esp_cnt = req_cnt - xfrcnt;
	DPRINTF1("sm_dma_cleanup: ESP_cntr= %x\n", xfrcnt);
                /* correct ESP xfr_cnt reported error */
                if ((esp_cnt == 0) && (dma_cnt > 16)) {
                        esp_cnt = dma_cnt & (0xf);
                }
                esp_cnt -= fifo_cnt;
        }
        /* If we were sending out the scsi bus, then we believe the
         * (possibly faked) transfer count register, since the esp
         * chip may have prefetched to fill it's fifo.  */
	DPRINTF2("sm_dma_cleanup: dir= %x, espcnt= %x\n",
                smsnap->sync_chk, esp_cnt);
	if (smsnap->sync_chk == SM_SEND_DATA)
                xfrcnt = esp_cnt;
        else {
                /* Else, if we were coming from the scsi bus, we only count
                 * that which got pumped through the dma engine.  */
                xfrcnt = dma_cnt;
        }
        DPRINTF2("sm_dma_cleanup: dir= %x, xfrcnt= %x\n",
                smsnap->sync_chk, xfrcnt);
	/* updated dma_count. NOTE: different from OS-> update the count */
	/* to remaining bytes; while HERE = return to upper level with */
	/* exact amount of bytes transferred */
	h_sip->cc = xfrcnt;	
#ifdef notdef
	/* skip updating the addr, upper level driver might reuse the address */
	/* without restoring to the original value (expecting NO change) */
	h_sip->ma += xfrcnt;	
#endif notdef
	DPRINTF2("sm_dma_cleanup: FINAL addr= %x, cnt= %x\n",
                h_sip->ma, h_sip->cc);
        smsnap->sync_chk = SM_NO_DATA;
        if (fifo_cnt) 
		ESP_WR.cmd = CMD_FLUSH;
	return(ZERO);
}

/*
 * Reset SCSI control logic.
 */
static int
sm_reset(h_sip, cond)
	register struct host_saioreq *h_sip;
	int	cond;
{
	register struct scsi_sm_reg *smr;
	register struct udc_table *dvma;
	register struct sm_snap *smsnap = &smreg_info;
	register int i;
	u_long	toutcnt;

	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;
	
	DPRINTF2("sm_reset: state= %x, dma_stat= %x\n", 
		smsnap->cur_state, dvma->ctl_stat);
	DEBUG_DELAY(1000000);
        if (dvma->ctl_stat & DVMA_REQPEND)
                smsnap->cur_state = STATE_DVMA_STUCK;

	ESP_WR.conf = ESP_CONF_DISRINT;		/* disable reset INT */

	/* for hard reset, cur_state must be STATE_RESET */
        switch (cond) {
           case RESET_EXT_ONLY:
                DPRINTF("sm_reset: reset ESP/SCSI bus only\n");
                ESP_WR.cmd = CMD_RESET_ESP;     /* hard-reset ESP chip */
                ESP_WR.cmd = CMD_NOP;           /* needed for ESP bug */
                ESP_WR.cmd = CMD_RESET_SCSI;
                ESP_WR.cmd = CMD_NOP;           /* NOP needed for ESP bug */
		for (toutcnt = ZERO; toutcnt < LOOP_4SEC; toutcnt++);
	        i = ESP_RD.intr;        /* read to clear reset interrupt */
#ifdef lint
i = i;
#endif lint
                break;
           case RESET_INT_ONLY:
                DPRINTF("sm_reset: reset ESP only\n");
                /* DVMA_EN = ZERO, INTEN=ZERO, FLUSH=1 */
                dvma->ctl_stat = DVMA_FLUSH;    /* soft dvma clear */
                DELAY(100);
                dvma->ctl_stat = ZERO;          /* clear it */
                /* NO ints */
                ESP_WR.cmd = CMD_RESET_ESP;     /* hard-reset ESP chip */
                ESP_WR.cmd = CMD_NOP;           /* needed for ESP bug */
		break;
           case RESET_ALL_SCSI:
           default:
                DPRINTF("sm_reset: reset DVMA/ESP/SCSI bus\n");
                /* implied a reset to both ESP and SCSI bus */
                dvma->ctl_stat = DVMA_RESET;    /* hard DVMA reset */
                DPRINTF1("sm_reset: dvma_stat= %x\n", dvma->ctl_stat);
                DELAY(100);
                dvma->ctl_stat = ZERO;          /* clear reset */
                ESP_WR.cmd = CMD_RESET_SCSI;
                ESP_WR.cmd = CMD_NOP;           /* NOP needed for ESP bug */
		/* allow more than 2 second for scsi reset to settle */
		/* Give reset scsi devices time to recover (> 2 Sec) */
		for (toutcnt = ZERO; toutcnt < LOOP_4SEC; toutcnt++);
       	 	i = ESP_RD.intr;        /* read to clear reset interrupt */
#ifdef lint
i = i;
#endif lint
	}
	/* enable parity, enable reset_int, and set host bus_id */
	ESP_WR.clock_conv = DEF_CLK_CONV; 
	ESP_WR.timeout = DEF_TIMEOUT;    
        ESP_WR.sync_offset = ZERO;         /* async */
#ifdef notdef
	/* ignore parity enable for backward campatibilty with old shoebox */
	ESP_WR.conf = ESP_CONF_PAREN | DEF_ESP_HOSTID;
#endif 
	ESP_WR.conf = DEF_ESP_HOSTID;

	DPRINTF1("sm_reset: esp_conf= %x\n", ESP_RD.conf);
        DEBUG_DELAY(1000000);
	/* set up some default sm_snap */
        /* "cur_err, cur_retry, and scsi_status" */
	/* will be cleared in "sm_cmd() */  
	smsnap->cur_state = STATE_FREE;
        smsnap->scsi_message = SC_COMMAND_COMPLETE;
	/* dvam's ctl_stat is ZERO, which means no INTEN; */
        DPRINTF1("sm_reset: done, DVMA_stat= %x\n", dvma->ctl_stat);
}

/*
 * Handle a scsi interrupt.
 */
sm_intr(h_sip)
	struct host_saioreq *h_sip;
{
	register struct scsi_sm_reg *smr;
	register struct sm_snap *smsnap = &smreg_info;
	register struct udc_table *dvma;
	register int i, s;
	u_long toutcnt;
	u_char	espstat;

	/* get to scsi control logic registers */
	smr = (struct scsi_sm_reg *)h_sip->devaddr;
	dvma = (struct udc_table *)h_sip->dmaaddr;
	
	s = dvma->ctl_stat;
	DPRINTF1("sm_intr: dma_stat: %x\n", s);
	/* check dvma's req_pend, err_pend, and int_pend */
        /* should only have int_pend set, if NO error */
        if ((s & DVMA_INT_MASK) == ZERO) {
	   	DPRINTF1("sm_intr: DVMA *NO* int_pend, stat: %x\n", s);
	   	goto INTR_CHK_ERR;
	}
	/* Do not read ESP reg_interrupt before reading status and step reg */
	/* after reading ESP reg_interrupt, ESP's INT will get cleared */
	smsnap->esp_step = ESP_RD.step;
	smsnap->esp_stat = ESP_RD.stat;
	espstat = smsnap->esp_stat;
	smsnap->esp_intr = ESP_RD.intr;		/* clear INT */
	DPRINTF2("sm_intr: state= %x, int= %x\n", 
		smsnap->cur_state, smsnap->esp_intr);
	DPRINTF2("sm_intr: step= %x, stat= %x\n", 
		smsnap->esp_step, espstat);
 
	if (smsnap->cur_state == STATE_DATA_REQ) {
	   s = dvma->ctl_stat;
	   DPRINTF1("sm_intr: dma_stat: %x\n", s);
	   smsnap->cur_retry++;
	   if (s & DVMA_CHK_MASK) {
              if (s & DVMA_REQPEND) {	/* DMA request pending */ 
	 	EPRINTF1("sm_intr: DVMA-scsi req_pend, stat= %x\n", s);
                /* DMA request pending, should not ocurred */
                toutcnt = LOOP_4SEC;    /* must be over 2sec */
                /* wait for a short while to see it will clear itself */
                while ((--toutcnt) && (dvma->ctl_stat & DVMA_REQPEND));
                if (toutcnt == ZERO) {
                        smsnap->cur_err = SE_REQPEND;
                        goto INTR_CHK_ERR;
                }
	      }
	      if (s & DVMA_ERRPEND) {	/* memory exception error */
		smsnap->cur_err = SE_MEMERR; 
	 	DPRINTF1("sm_intr: DVMA mem_err, stat= %x\n", s);
               	goto INTR_CHK_ERR;
	      }	
 	   } 
	   /* status errors are mainly for internal firmware debug */
           if (espstat & STAT_ERR_MASK) {
		smsnap->cur_retry++;	
                /* ESP's status reg ERROR */
                if (espstat & ESP_STAT_PERR) { /* parity error */
                        EPRINTF("sm_intr: SCSI bus parity error\n");
                        smsnap->cur_err = SE_PARITY;
                }
                /* if mismatch, the 'DMA" direction is wrong */
                if ((((dvma->ctl_stat & DVMA_WRITE) == ZERO) &&
                      (smsnap->sync_chk == SM_RECV_DATA)) ||
                      ((dvma->ctl_stat & DVMA_WRITE) &&
                      (smsnap->sync_chk == SM_SEND_DATA))) {
                        EPRINTF2("sm_intr: wrong dma dir= %x, stat= %x\n",
				smsnap->sync_chk, dvma->ctl_stat);
                        smsnap->cur_err = SE_DIRERR;
                }
                if ((ESP_RD.fifo_flag & KEEP_5BITS) == MAX_FIFO_FLAG) {
                        /* fifo overwritten */
                        EPRINTF1("sm_intr: fifo_flag ERROR= %x\n",
                                ESP_RD.fifo_flag);
                        /* the 'TOP of FIFO' had been overwritten */
                        smsnap->cur_err = SE_FIFOVER;
                }
#ifdef notdef
                /* If none of the above, GROSS error is from cmd overflown */
		/* top_of_cmd had been overwritten */
                /* ignore this for P4-ESP for timing reason */
                if (smsnap->cur_err == ZERO) {
                        EPRINTF1("sm_intr: cur_cmd= %x\n", ESP_RD.cmd);
                        smsnap->cur_err = SE_CMDOVER;
                }
#endif notdef
                if (smsnap->cur_err) {
			EPRINTF1("sm_intr: esp stat_err= %x\n", espstat);
                        goto INTR_CHK_ERR;
		}
           }
        }
 
INTR_CHK_STAT:
	/* CORRECTON for an ESP ERRATA: ensure phase change detection reset */
	ESP_WR.cmd = CMD_NOP;
	smsnap->esp_stat &= ESP_PHASE_MASK;
	s = ESP_RD.stat;
	i = s & ESP_PHASE_MASK;
	if (i != smsnap->esp_stat) {
		DPRINTF2("sm_intr: phase changed, old= %x, new= %x\n", 
			smsnap->esp_stat, i);
		smsnap->esp_stat = i;
		espstat = s;
		i = ESP_RD.intr;
                if (i) {
                    DPRINTF2("sm_intr: INTR changed, old= %x, new= %x\n",
                        smsnap->esp_intr, i);
                    smsnap->esp_intr = i;
                }
	}
	DPRINTF2("sm_intr: ESP_int= %x, state= %x\n", 
		smsnap->esp_intr, smsnap->cur_state);
	switch (smsnap->esp_intr) {
	   case ESP_INT_DISCON:  	/* disconnect INT */
		switch (smsnap->cur_state) {
		    /* case STATE_MSG_DISCON not needed */
		    case STATE_COMP_REQ: /* cmd completed seuqence sent out */
		    case STATE_MSG_CMPT: /* command completed msg accepted */
			/* All ZERO= good, All other, bit 1= check cond, */
			/* bit 3= busy, bit 4=resev conflict are errors */
			if (smsnap->scsi_status & SCSI_BUSY_MASK) {
				DPRINTF1("sm_intr: BUS_ERR status= %x\n", 
					smsnap->scsi_status);
				smsnap->cur_err = SE_BUSTATERR;
			}
			smsnap->cur_state = STATE_FREE;
			break;
		    case STATE_SEL_REQ:	/* selection command timeout */
			smsnap->cur_err = SE_SELTOUT;
		    case STATE_FREE:	
			break;
		    default:	/* could be from target dropping BSY */
			EPRINTF("sm_intr: target drop busy\n");
			smsnap->cur_err = SE_PHASERR;
	        }
		break;	
	   case ESP_INT_FCMP:/* function complete INT */	
	   case ESP_INT_BUS:  /* from"select_seq","comp_seq","msg_accept" cmd */
	   case INT_OK_MASK:  /* from"select_seq","comp_seq","msg_accept" cmd */
		if (smsnap->cur_state == STATE_SEL_REQ) {
		    /* for initator, only SEL_SEQ has */
                    /* intermittent step indications: */
                    /* both sel w/wo ATN need 4 steps to complete OK */
                    /* only sel w/ATN/stop is 3 steps to complete OK */
		    /* both sel w/wo ATN need 4 steps to complete OK */
		    if ((smsnap->esp_step & KEEP_3BITS) != STEP_SEL_OK) {
			EPRINTF2("sm_intr: BAD_SEL, step= %x, stat= %x\n", 
				smsnap->esp_step, smsnap->esp_stat);
			if (smsnap->esp_stat != ESP_PHASE_STATUS) {	
				smsnap->cur_err = SE_SELTOUT;
				goto INTR_CHK_ERR;
			}
			ESP_WR.cmd = CMD_FLUSH;
			ESP_WR.cmd = CMD_NOP;
			/* ready for next phase */
		    }
		    smsnap->cur_state = STATE_SEL_DONE;
		}
		switch (smsnap->esp_stat) {   /* current phase */
		  case ESP_PHASE_DATA_IN:	/* either data in or out */
		  case ESP_PHASE_DATA_OUT:	/* either data in or out */
			DPRINTF1("sm_intr: data phase, pre_state= %x\n", 
				smsnap->cur_state);
			DEBUG_DELAY(1000000);
			/* if this phase came after a non-command phase */
			/* try to service this unexpected data phase */
			if (smsnap->cur_state != STATE_SEL_DONE) {
				DPRINTF("sm_intr: wrong data phase\n");
				if (++smsnap->cur_retry > MAX_RETRY) {
					EPRINTF("sm_intr: wrong data phase\n");
					smsnap->cur_err = SE_PHASERR;
					goto INTR_CHK_ERR;
				}
				ESP_WR.cmd = CMD_NOP;
				smsnap->esp_stat = ESP_RD.stat & ESP_PHASE_MASK;                                i = ESP_RD.intr;
                                DPRINTF2("sm_intr: stat= %x, intr = %x\n",
                                        smsnap->esp_stat, i);
                                if (i) {
                                        smsnap->esp_intr = i;
                                        goto INTR_CHK_STAT;
                                }
                                else    /* just service and ignore */
                                        goto INTR_RTN;
			}
			/* assume ALL cmd that has NO data, do not need DMA */
			if (smsnap->sync_chk == SM_NO_DATA) {
				DPRINTF1("sm_intr: spurious data phase, state= %x\n", 
					smsnap->cur_state);
				if (++smsnap->cur_retry > MAX_RETRY) {
					EPRINTF1("sm_intr: spurious data phase, state= %x\n", 
					smsnap->cur_state);
					smsnap->cur_err = SE_PHASERR;
					goto INTR_CHK_ERR;
				}
				/* try to fake one out a data xfr */
				ESP_WR.cmd = CMD_TRAN_PAD | CMD_DMA;
			}
			else { 	/* data transfer required */
				smsnap->cur_state = STATE_DATA_REQ;
				/* force async transfer */
				ESP_WR.sync_offset = ZERO; 
				/* enable dvma and R/W*/
				sm_dma_setup(h_sip);	
				/* add in per ESP errata note */
                        	ESP_WR.cmd = CMD_NOP;
				ESP_WR.cmd = CMD_TRAN_INFO | CMD_DMA;
			}	
			break;
		  case ESP_PHASE_STATUS:
			DPRINTF1("sm_intr: status phase, pre_state= %x\n", 
				smsnap->cur_state);
			if (sm_dma_cleanup(h_sip, espstat)) 
				goto INTR_CHK_ERR;
	  	 	smsnap->cur_state = STATE_COMP_REQ;
			DPRINTF("sm_intr: send complete_seq\n");
			ESP_WR.cmd = CMD_COMP_SEQ;
			break;
		  case ESP_PHASE_MSG_IN:
			DPRINTF2("sm_intr: msg_in phase, state= %x, flag= %x\n", 
				smsnap->cur_state, ESP_RD.fifo_flag);
			switch (smsnap->cur_state) {
			  case STATE_COMP_REQ:	/* two bytes */
				smsnap->scsi_status = ESP_RD.fifo_data; 
			  case STATE_MSGIN_REQ:
				smsnap->scsi_message = ESP_RD.fifo_data;
				break;
			  case STATE_DATA_REQ:
                                 DPRINTF2("sm_intr: msg_in dma_stat= %x, cnt= %x\n",
                                         dvma->ctl_stat, h_sip->cc);
                                 if (sm_dma_cleanup(h_sip, espstat))
                                         goto INTR_CHK_ERR;
                                 /* fall through */
                            default:
                                 /* get message byte */
                                 if (ESP_RD.fifo_flag) {
                                   if (ESP_RD.fifo_flag > 1) {
			        EPRINTF2("sm_intr: msg_in flag= %x, state= %x\n",
                                         ESP_RD.fifo_flag, smsnap->cur_state);
                                      EPRINTF2("sm_intr: stat= %x, intr= %x\n",
                                        smsnap->esp_stat, smsnap->esp_intr);
                                         while (ESP_RD.fifo_flag) {
                                                 i = ESP_RD.fifo_data;
                                                 EPRINTF1(" %x", i);
                                         }
                                         ESP_WR.cmd = CMD_NOP;
                                        goto INTR_CHK_STAT;
                                     }
                                     smsnap->scsi_message = ESP_RD.fifo_data;
                                 }
                                 else {
                                         ESP_WR.cmd = CMD_TRAN_INFO;
					 smsnap->cur_state = STATE_MSGIN_REQ;
                                         goto INTR_RTN;
				}
			}
			DPRINTF2("sm_intr: status= %x, message= %x\n",
				smsnap->scsi_status, smsnap->scsi_message);
			DPRINTF1("sm_intr: flag= %x\n", ESP_RD.fifo_flag);
			switch (smsnap->scsi_message) {
			   case SC_COMMAND_COMPLETE:
			   case SC_NO_OP:
				smsnap->cur_state = STATE_MSG_CMPT;
				/* save the scsi_status for now */
				/* a disconnect INT shuold show up */
				/* after sending out "msg_accpt" cmd */
				/* at that time, we are TOTALLY done */
				break;
			   case SC_PARITY:
				DPRINTF("sm_intr: PARITY_err msg\n");
				smsnap->cur_err  = SE_PARITY;
				smsnap->cur_state = STATE_MSG_PARITY;
				break;
			   case SC_DEVICE_RESET:
				DPRINTF("sm_intr: RESET_err msg\n");
				smsnap->cur_state = STATE_RESET;
				smsnap->cur_err  = SE_RESET;
				break;	
			   case SC_ABORT:
				DPRINTF("sm_intr: ABORT_err msg\n");
				smsnap->cur_state = STATE_MSG_ABORT;
				smsnap->cur_err = SE_MSGERR;
				break;	/* no need to send abort msg */
			   case SC_MSG_REJECT:
				DPRINTF("sm_intr: REJECT_err msg\n");
				smsnap->cur_state = STATE_MSG_REJECT;
				break;
				/* All other is ocnsidered ERROR for now */
			   case SC_DISCONNECT:
			   case SC_SAVE_DATA_PTR:
			   case SC_RESTORE_PTRS:
			   case SC_EXTENDED_MESSAGE:	
			   case SC_LINK_CMD_CPLT:
			   case SC_FLAG_LINK_CMD_CPLT: 
			   case SC_IDENTIFY:
			   case SC_DR_IDENTIFY:
			   default:
				EPRINTF1("sm_intr: unsupported message: %x\n", 
					smsnap->scsi_message);
				smsnap->cur_err = SE_MSGERR;
			}
			/* send a message accpt cmd */
			ESP_WR.cmd = CMD_MSG_ACPT;
			ESP_WR.cmd = CMD_NOP;		/* for ESP bug */
			break;	
	          case ESP_PHASE_MSG_OUT:	
		  case ESP_PHASE_COMMAND:
		  default: 	/* all others are bad, just set PHASE error */
			EPRINTF1("sm_intr: bad phase, pre_state: %x\n", 
				smsnap->cur_state);
			smsnap->cur_err = SE_PHASERR;
		}
		break;
	    case ESP_INT_RESET:	/* external scsi RESET interrupt received */
		DPRINTF1("sm_intr: SCSI bus_reset int, state= %x\n", 
			smsnap->cur_state);
		/* just set flag, and sm_reset() will be called later */
		smsnap->cur_err = SE_RESET;
		break;
	   case ESP_INT_ILLCMD:	/* illegal cmd */ 
	   case INT_ILL_BUS:		/* illegal bus status */ 
		DPRINTF1("sm_intr: illegal cmd= %x\n", ESP_RD.cmd);
		smsnap->cur_err = SE_ILLCMD;
		break;
	    case ESP_INT_RESEL:
            case ESP_INT_SELATN:	/* selected with ATN -- target cmd */ 
   	    case ESP_INT_SEL:		/* selected without ATN -- target cmd */
	    default:
		/* all others are not supported NOW */
		EPRINTF1("sm_intr: bad interrupt= %x\n", smsnap->esp_intr);
		smsnap->cur_err = SE_PHASERR;
	}
INTR_CHK_ERR:
	if (smsnap->cur_retry > MAX_RETRY) { 
                EPRINTF1("sm_intr: retry exhausted, phase= %x\n", 
                        smsnap->cur_state); 
                DEBUG_DELAY(10000000);
                smsnap->cur_err = SE_TOUTERR;
        }
	/* Just simply pass the status to upper level callers */
	if (smsnap->cur_err) {
#ifdef SMDEBUG
            if (smsnap->cur_err != SE_BUSTATERR) {
                EPRINTF2("sm_intr: ESP_ERR= %x, phase= %x\n",
                        smsnap->cur_err, smsnap->cur_state);
            }
#endif SMDEBUG
	    DEBUG_DELAY(1000000);		
 	    switch (smsnap->cur_err) {
		case SE_SELTOUT:   /* select/resel timeout */
		   ESP_WR.cmd = CMD_FLUSH;
                   ESP_WR.cmd = CMD_NOP;
		   break;	   /* return error, but NO reset */
		case SE_BUSTATERR: /* bus ERR, i.e, chk_cond, busy */
	  	   smsnap->cur_err = ZERO;
	  	   smsnap->cur_state = STATE_FREE;
		   break;	   /* return error, but NO reset */
	        case SE_MEMERR:	   /* memory exception error during DMA */
		case SE_DIRERR:    /* WRONG DMA direction, could trash MEM */
                case SE_DVMAERR:   /* DVMA Drain stuck */
                case SE_ILLCMD:	   /* illegal command detected */
		case SE_PARITY:	   /* either ESP or target parity ERR */
		case SE_FIFOVER:   /* ESP status err, fifo overflown */
		case SE_CMDOVER:   /* ESP status err, command overflown */ 
		case SE_RESET:	   /* reset detected */
		   /* software reset (NO external SCSI reset) */
		   sm_reset(h_sip, RESET_INT_ONLY);
		   break;
		case SE_TOUTERR:   /* DMA timed out */
		case SE_MSGERR:	   /* ESP has un-expected message */
		case SE_PHASERR:   /* ESP has un-expected phase */
		   /* possible HW error, do a HARD-SCSI reset */
		   sm_reset(h_sip, RESET_ALL_SCSI);
		   break;
		case SE_REQPEND:	/* DMA's request pending stuck */
                   toutcnt = (int)dvma->ctl_stat;
                   EPRINTF1("sm_intr: DVMA-scsi req_pend stuck, stat= %x\n", 
			toutcnt);
		   break;
		case SE_SPURINT:	
		   EPRINTF1("sm_intr: spurious esp_intr= %x\n",
                        smsnap->esp_intr);
                   smsnap->cur_err = ZERO;
		   goto INTR_RTN;
		default:
		   DPRINTF1("sm_intr: spurious esp_err= %x\n", smsnap->cur_err);
	    }
	    smsnap->cur_state = STATE_FREE;
	}
INTR_RTN:
	DPRINTF1("sm_intr: INTR_END, state= %x\n", smsnap->cur_state);
	DEBUG_DELAY(100000);
}
#endif (defined sun4) || (defined sun3x)

/* end of SCSI-ESP standalone boot driver (smboot.c) */
