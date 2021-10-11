#ident "@(#) dbri_mmcodec.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/limits.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/ioccom.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/file.h>

#include <sun/audioio.h>
#include <sun/isdnio.h>
#include <sun/dbriio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>
#include <sbusdev/dbrivar.h>
#include "audiodebug.h"


/* Global variables */
extern int Audio_debug;

int	Dbri_recv_bsize = DBRI_RECV_BSIZE;



/* Some useful defines - see the self documenting code */

#define	SB_PAL_LEN	8
#define	ULAW_ZERO	0xffffffff
#define	ALAW_ZERO	0xd5d5d5d5
#define	LINEAR_ZERO	0x00000000

#define	DUMMYDATA_LEN	(32)
#define	DUMMYDATA_CYCLE	(driver->ser_sts.pal_present ? driver->ser_sts.mmcodec_ts_start:CHI_DATA_MODE_LEN)
#define	DM_T_5_8_LEN	(32)
#define	DM_T_5_8_CYCLE	(driver->ser_sts.mmcodec_ts_start + 32)
#define	DM_R_5_6_LEN	(10)
#define	DM_R_5_6_CYCLE	(driver->ser_sts.mmcodec_ts_start + 32)
#define	DM_R_7_8_LEN	(16)
#define	DM_R_7_8_CYCLE	(driver->ser_sts.mmcodec_ts_start + 48)

/* XMIT cycles can not start at zero */
#define	CM_T_1_4_LEN	(32)
#define	CM_T_1_4_CYCLE	(driver->ser_sts.pal_present ? driver->ser_sts.mmcodec_ts_start:CHI_CTRL_MODE_LEN)
#define	CM_R_1_2_LEN	(16)
#define	CM_R_1_2_CYCLE	(driver->ser_sts.mmcodec_ts_start + 0)
#define	CM_R_3_4_LEN	(16)
#define	CM_R_3_4_CYCLE	(driver->ser_sts.mmcodec_ts_start + 16)
#define	CM_R_7_LEN	(8)
#define	CM_R_7_CYCLE	(driver->ser_sts.mmcodec_ts_start + 48)

/* Masks for reading Control Mode data back from the MMCODEC */
#define	CM_R_1_2_MASK	0x063f
#define	CM_R_3_4_MASK	0x3f03
#define	CM_R_7_MANU(a)	((a & 0xf0) >> 4)
#define	CM_R_7_REV(a)	(a & 0x0f)

/* parameters to mmcodec_setup_chi */
#define	SETUPCHI_CTRL_MODE	1
#define	SETUPCHI_CODEC_MASTER	2
#define	SETUPCHI_DBRI_MASTER	3

/* parameters to mmcodec_set_spkrmute */
#define	SPKRMUTE_SET		1
#define	SPKRMUTE_GET		2
#define	SPKRMUTE_CLEAR		3
#define	SPKRMUTE_TOGGLE		4

#define	MMCODEC_SLEEPPRI	(PZERO-1)
#define	MMCODEC_TIMEOUT		(1 * hz)	/* 3 secs must be too long */
#define	SB_BUTTON_REPEAT	(hz / 6)	/* 6 times / second repeat */
#define	SB_BUTTON_START		(hz)		/* repeat after 1 second */
int	sb_button_repeat = 0;

#define	MMCODEC_NOFS_THRESHOLD	(10)
int	audio_error_debug = 0;
#define	eprintf		if (audio_error_debug) printf


/* SB PAL bit definitions */
#define	VMINUS_BUTTON	0x01
#define	VPLUS_BUTTON	0x02
#define	MUTE_BUTTON	0x04
#define	HPHONE_IN	0x08
#define	MIKE_IN		0x10
#define	ID_MASK		0xe0
#define	PAL_ID		0xa0

void
mmcodec_reset(driver)
	dbri_dev_t	*driver;
{
	dbri_reg_t	*chip = driver->chip;
	u_char		temp;

	/*
	 * After DBRI has been reset, and before we have done any
	 * write to the pio pins, we can read them to find out the
	 * status of the CODEC's currently "on the system". If there
	 * is a Speakerbox plugged in, then the external power down
	 * pin will be pulled high. If there is an onboard MMCODEC,
	 * then the internal power down pin will be pulled high. If
	 * there is both, the Speakerbox will take precedence.
	 */
	temp = chip->n.pio;
	aprintf("mmcodec_reset: reg2 initially 0x%x\n", (unsigned int)temp);
	if (temp & SCHI_SET_PDN) {
		driver->ser_sts.pal_present = TRUE;
		driver->ser_sts.mmcodec_ts_start = SB_PAL_LEN;
	} else if (temp & SCHI_SET_INT_PDN) {
		driver->ser_sts.pal_present = FALSE;
		driver->ser_sts.mmcodec_ts_start = 0;
	} else {
		(void) printf("Audio: no Audio device found\n");
		driver->ser_sts.chi_state = CHI_NO_DEVICES;
		driver->chip->n.pio = 0;
		/*
		 * You can't unplug an onboard CODEC, so we know
		 * that if there's no device, it *should* be an SB
		 */
		driver->ser_sts.pal_present = TRUE;
		driver->ser_sts.mmcodec_ts_start = SB_PAL_LEN;

		return;
	}
	driver->chip->n.pio = SCHI_ENA_ALL | SCHI_SET_DATA_MODE |
		SCHI_SET_INT_PDN | SCHI_CLR_RESET | SCHI_SET_PDN;
	driver->ser_sts.chi_state = CHI_POWERED_DOWN;

	return;
} /* mmcodec_reset */


unsigned int
reverse_bit_order(word, len)
	unsigned int	word;
	int		len;
{
	int		bit;
	int		count;
	unsigned int	new_word = 0;

	len--;
	for (count = 0; count <= len; count++) {
		bit = ((word >> count) & 0x1);
		new_word += (bit << (len - count));
	}

	return (new_word);
} /* reverse_bit_order */

int
mmcodec_getdev(driver)
	dbri_dev_t	*driver;
{
	if (driver->ser_sts.pal_present) {
		return (AUDIO_DEV_SPEAKERBOX);
	} else {
		return (AUDIO_DEV_CODEC);
	}
} /* mmcodec_getdev */


void
mmcodec_init_pipes(driver)
	dbri_dev_t	*driver;
{
	int		tmp;
	unsigned int	pipe;
	pipe_tab_t	*pipep;
	dbri_chip_cmd_t	cmd;
	int		pal_value;
	mmcodec_data_t	mmdata;
	mmcodec_ctrl_t	mmctrl;

	aprintf("mmcodec_init_pipes: driver=0x%x\n", (unsigned int)driver);
	driver->chip->n.sts |= DBRI_STS_C;	/* Enable CHI intrfce */

	/* Setup CHI base pipe */
	pipep = &driver->ptab[DBRI_PIPE_CHI];
	pipep->dts.word32 = 0;
	pipep->in_tsd.word32 = 0;
	pipep->out_tsd.word32 = 0;

	/* Setup dts command words */
	pipep->dts.r.cmd = DBRI_OPCODE_DTS;		/* DTS command */
	pipep->dts.r.vi = DBRI_DTS_VI;			/* input tsd */
	pipep->dts.r.vo = DBRI_DTS_VO;			/* output tsd */
	pipep->dts.r.id = DBRI_DTS_ID;			/* Add timeslot */
	pipep->dts.r.oldin = DBRI_PIPE_CHI;		/* old input pipe */
	pipep->dts.r.oldout = DBRI_PIPE_CHI;		/* old output pipe */
	pipep->dts.r.pipe = DBRI_PIPE_CHI;		/* pipe 16 */

	pipep->in_tsd.chi.mode = DBRI_DTS_ALTERNATE;	/* alternate */
	pipep->in_tsd.chi.next = DBRI_PIPE_CHI;		/* next is 16 */

	pipep->out_tsd.chi.mode = DBRI_DTS_ALTERNATE;	/* alternamte */
	pipep->out_tsd.chi.next = DBRI_PIPE_CHI;	/* next pipe 16 */

	cmd.cmd_wd1 = pipep->dts.word32;		/* DTS command */
	cmd.cmd_wd2 = pipep->in_tsd.word32;
	cmd.cmd_wd3 = pipep->out_tsd.word32;
	dbri_command(driver, cmd);

	/*
	 * Issue SDP's for pipes 17, 18, and 19 - Data Mode pipes for, transmit
	 * timeslots 5-8 (32 bits), receive timeslots 5-6 (16 bits), and
	 * receive timeslots 7-8 (16 bits) respectively
	 */
	pipep = &driver->ptab[DBRI_PIPE_DM_T_5_8];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_D |		/* transmit pipe */
			DBRI_SDP_PTR |		/* ptr to TMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_DM_T_5_8;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	pipep = &driver->ptab[DBRI_PIPE_DM_R_5_6];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_PTR |		/* ptr to RMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_DM_R_5_6;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	pipep = &driver->ptab[DBRI_PIPE_DM_R_7_8];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_PTR |		/* ptr to RMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_DM_R_7_8;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	/*
	 * Now issue SDP's for pipes 20, 21, and 22 - Control Mode pipes for,
	 * Transmit Timeslots 1-4 (32 bits), Receive timeslots 1-2 (16 bits),
	 * and Receive timeslots 3-4 (16 bits) respectively
	 */
	pipep = &driver->ptab[DBRI_PIPE_CM_T_1_4];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_D |		/* transmit pipe */
			DBRI_SDP_PTR |		/* ptr to TMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_CM_T_1_4;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	pipep = &driver->ptab[DBRI_PIPE_CM_R_1_2];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_PTR |		/* ptr to RMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_CM_R_1_2;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	pipep = &driver->ptab[DBRI_PIPE_CM_R_3_4];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_PTR |		/* ptr to RMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_CM_R_3_4;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	pipep = &driver->ptab[DBRI_PIPE_CM_R_7];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_PTR |		/* ptr to RMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_CM_R_7;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	/*
	 * Set up pipe to transmit dummy data, when we're not transmitting
	 * real data. Since DBRI drives the CHIDX line low (zero) when
	 * there is no pipe defined for a time slot, it looks like
	 * zeroes to the CODEC. In ulaw, zero is actually all ones so
	 * the zeroes sent by DBRI causes popping and clicking
	 */
	pipe = DBRI_PIPE_DUMMYDATA;
	pipep = &driver->ptab[pipe];
	pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
			DBRI_SDP_CHNG |		/* report any change */
			DBRI_SDP_FIXED |	/* fixed data */
			DBRI_SDP_D |		/* transmit pipe */
			DBRI_SDP_PTR |		/* ptr to TMD */
			DBRI_SDP_CLR |		/* clear pipe */
			DBRI_PIPE_DUMMYDATA;	/* pipe number */
	cmd.cmd_wd1 = pipep->sdp;
	cmd.cmd_wd2 = 0;
	dbri_command(driver, cmd);

	/*
	 * Set up data mode control information
	 */

	mmdata.word32 = 0;

	/*
	 * Whatever we initialize the MMCODEC to here must match with
	 * what dbri_attach set's up. Default is DBRI_DEFAULT_GAIN, with
	 * speaker and microphones as ports
	 */
	mmdata.r.om0 = ~MMCODEC_OM0_ENABLE & 1;
	mmdata.r.om1 = ~MMCODEC_OM1_ENABLE & 1;
	mmdata.r.sm = MMCODEC_SM;		/* enable speaker */
	mmdata.r.ovr = MMCODEC_OVR_CLR;		/* clear overflow conditions */
	mmdata.r.is = MMCODEC_IS_MIC;

	/* Calculate default output attenuation */
	tmp = MMCODEC_MAX_ATEN -
		(DBRI_DEFAULT_GAIN * (MMCODEC_MAX_ATEN+1) / (AUDIO_MAX_GAIN+1));
	mmdata.r.lo = tmp;
	mmdata.r.ro = tmp;

	/* Calculate default input gain */
	tmp = DBRI_DEFAULT_GAIN * (MMCODEC_MAX_GAIN+1) / (AUDIO_MAX_GAIN+1);
	mmdata.r.lg = tmp;
	mmdata.r.rg = tmp;

	/* Calculate default monitor gain */
	mmdata.r.ma = MMCODEC_MA_MAX_ATEN;	/* monitor attenuation */

	/*
	 * We create a standard for handing data for fixed pipes; the
	 * data found in the pipe table will be "normal" so we can look
	 * at it and manipulate it without hurting our brains, however
	 * the data sent to the dbri_command routine will be reversed so
	 * it doesn't have to know about it.
	 */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];
	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe); /* SSP command */
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	dbri_command(driver, cmd);

	/*
	 * Set up control mode initial values
	 */

	mmctrl.word32[0] = 0;

	mmctrl.r.dcb = 0;
	mmctrl.r.sre = ~MMCODEC_SRE & 1;
	mmctrl.r.vs0 = MMCODEC_VS0;
	mmctrl.r.vs1 = MMCODEC_VS1;
	mmctrl.r.dfr = MMCODEC_DFR_8000;	/* 8 KHz */
	mmctrl.r.st = MMCODEC_ST_MONO;
	mmctrl.r.df = MMCODEC_DF_ULAW;		/* uLAW encoding */
	mmctrl.r.mck = MMCODEC_MCK_XTAL1;	/* Clock is XTAL1 */
#if CHI_DATA_MODE_LEN == 64
	mmctrl.r.bsel = MMCODEC_BSEL_64;
#elif CHI_DATA_MODE_LEN == 128
	mmctrl.r.bsel = MMCODEC_BSEL_128;
#elif CHI_DATA_MODE_LEN == 256
	mmctrl.r.bsel = MMCODEC_BSEL_256;
#else
	print error... illegal CHI_DATA_MODE_LEN value;
#endif
	mmctrl.r.xclk = MMCODEC_XCLK;		/* CODEC is CHI master */
	mmctrl.r.xen = MMCODEC_XEN;		/* enable serial xmit */
	mmctrl.r.enl = ~MMCODEC_ENL & 1;	/* disable loopback */

	/* We currently don't use Time Slots 5, 6, or 8, in control mode */

	pipe = DBRI_PIPE_CM_T_1_4;
	pipep = &driver->ptab[pipe];
	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe); /* SSP command */
	pipep->ssp = mmctrl.word32[0];
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, CM_T_1_4_LEN);
	dbri_command(driver, cmd);

	if (driver->ser_sts.pal_present) {

		/* Issue SDP's for ID Pal pipes here */

		pipep = &driver->ptab[DBRI_PIPE_SB_PAL_T];
		pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
				DBRI_SDP_CHNG |		/* report any change */
				DBRI_SDP_FIXED |	/* fixed data */
				DBRI_SDP_D |		/* transmit pipe */
				DBRI_SDP_PTR |		/* ptr to TMD */
				DBRI_SDP_CLR |		/* clear pipe */
				DBRI_PIPE_SB_PAL_T;	/* pipe number */
		cmd.cmd_wd1 = pipep->sdp;
		cmd.cmd_wd2 = 0;
		dbri_command(driver, cmd);

		pal_value = 0; 		/* LED off initially */
		cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(DBRI_PIPE_SB_PAL_T);
		pipep->ssp = pal_value;
		cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, SB_PAL_LEN);
		dbri_command(driver, cmd);

		pipep = &driver->ptab[DBRI_PIPE_SB_PAL_R];
		pipep->sdp = DBRI_CMD_SDP |		/* SDP Command */
				DBRI_SDP_CHNG |		/* report any change */
				DBRI_SDP_FIXED |	/* fixed data */
				DBRI_SDP_PTR |		/* ptr to RMD */
				DBRI_SDP_CLR |		/* clear pipe */
				DBRI_PIPE_SB_PAL_R;	/* pipe number */
		cmd.cmd_wd1 = pipep->sdp;
		cmd.cmd_wd2 = 0;
		dbri_command(driver, cmd);

		/*
		 * Set initial value to compare against; assume the
		 * headphones and mike aren't plugged in so we get
		 * avail_ports set properly
		 */
		pal_value = PAL_ID;
		pipep->ssp = pal_value;
	}

	/*
	 * XXX - Once we find out whether or not we're a Speakerbox
	 * or running on Sunergy, fix the avail_ports field here
	 */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	if (sb_button_repeat == 0)
		sb_button_repeat = SB_BUTTON_REPEAT;

	return;
} /* mmcodec_init_pipes */


/*
 * The dbri_setup_dts command won't work for regular overhead kind
 * of pipes since it wants to get all the information from the channel
 * table, and we don't want to add these to it. This is a much simpler
 * version of the routine.
 */
void
mmcodec_do_dts(driver, pipe, oldpipe, nextpipe, dir, cycle, length)
	dbri_dev_t	*driver;
	unsigned int	pipe, oldpipe, nextpipe;
	int		dir, cycle, length;
{
	dbri_chip_cmd_t	cmd;
	pipe_tab_t	*pp, *op, *np;

	pp = &driver->ptab[pipe];
	np = &driver->ptab[nextpipe];
	op = &driver->ptab[oldpipe];

	pp->dts.r.cmd = DBRI_OPCODE_DTS;		/* DTS command */
	pp->dts.r.id = DBRI_DTS_ID;			/* Add timeslot */
	pp->dts.r.pipe = pipe;				/* pipe # */
	pp->in_tsd.word32 = 0;
	pp->out_tsd.word32 = 0;

	if (dir == DBRI_IN) {
		op->in_tsd.r.next = pipe;
		np->dts.r.oldin = pipe;

		pp->dts.r.vi = DBRI_DTS_VI;
		pp->dts.r.oldin = oldpipe;
		pp->in_tsd.r.len = length;
		pp->in_tsd.r.cycle = cycle;
		pp->in_tsd.r.mode = DBRI_DTS_SINGLE;
		pp->in_tsd.r.next = nextpipe;
	} else {
		op->out_tsd.r.next = pipe;
		np->dts.r.oldout = pipe;

		pp->dts.r.vo = DBRI_DTS_VO;
		pp->dts.r.oldout = oldpipe;
		pp->out_tsd.r.len = length;
		pp->out_tsd.r.cycle = cycle;
		pp->out_tsd.r.mode = DBRI_DTS_SINGLE;
		pp->out_tsd.r.next = nextpipe;
	}

	cmd.cmd_wd1 = pp->dts.word32;
	cmd.cmd_wd2 = pp->in_tsd.word32;
	cmd.cmd_wd3 = pp->out_tsd.word32;
	dbri_command(driver, cmd);

} /* mmcodec_do_dts */


void
mmcodec_dts_datamode_pipes(driver)
	dbri_dev_t	*driver;
{
	aprintf("mmcodec_dts_datamode_pipes: 0x%x\n", (unsigned int)driver);
	/*
	 * This set's up the pipes so that the linked list is the
	 * same all the time. Transmit side looks like 16-[23]-17-16, and
	 * the receive side looks like 16-[24]-18-19-16
	 * We can't use the dbri_setup_dts routine since these don't/can't
	 * show up in the Channel table, but code is stolen from there
	 * It still needs to reflect the list in the pipe table though
	 * so that the above routine (and others) continue to work
	 *
	 * The SBPal pipes are DTS's both here and in mmcodec_dts_cntlmode.
	 * In theory, they could be done once in the beginning, like
	 * in init_pipes and then never touched again, but that would mean
	 * that the datamode and control mode frame lengths were the same.
	 * (The only difference between this code and the cntlmode code
	 * is the Transmit pipe cycle value)
	 */
	if (driver->ser_sts.pal_present) {
		mmcodec_do_dts(driver, DBRI_PIPE_SB_PAL_T,
		/* old, next */	DBRI_PIPE_CHI, DBRI_PIPE_CHI,
		/* dir, c, l */	DBRI_OUT, CHI_DATA_MODE_LEN, SB_PAL_LEN);

		mmcodec_do_dts(driver, DBRI_PIPE_SB_PAL_R,
		/* old, next */	DBRI_PIPE_CHI, DBRI_PIPE_CHI,
		/* dir, c, l */	DBRI_IN, 0, SB_PAL_LEN);
	}

	mmcodec_do_dts(driver, DBRI_PIPE_DM_T_5_8,
		((unsigned int) (driver->ser_sts.pal_present ?
		DBRI_PIPE_SB_PAL_T : DBRI_PIPE_CHI)),		/* old */
		DBRI_PIPE_CHI,					/* next */
		DBRI_OUT, DM_T_5_8_CYCLE, DM_T_5_8_LEN);	/* dir, c, l */

#ifdef	notdef
	/*
	 * XXX5 6/15/92 - not using this pipe continuously reduces the
	 * load on DBRI and hopefully less receiver overflow errors
	 */
	mmcodec_do_dts(driver, DBRI_PIPE_DM_R_5_6,
		((unsigned int) (driver->ser_sts.pal_present ?
		DBRI_PIPE_SB_PAL_R : DBRI_PIPE_CHI)),		/* old */
		DBRI_PIPE_CHI,					/* next */
		DBRI_IN, DM_R_5_6_CYCLE, DM_R_5_6_LEN);		/* dir, c, l */

	/*
	 * XXX5 5/7/92 - not using this pipe causes machines with a 16.67Mhz
	 * SBus to *not* hang on CHI receiver errors; revisit later
	 */
	mmcodec_do_dts(driver, DBRI_PIPE_DM_R_7_8,
		DBRI_PIPE_DM_R_5_6, DBRI_PIPE_CHI,		/* old, next */
		DBRI_IN, DM_R_7_8_CYCLE, DM_R_7_8_LEN);		/* dir, c, l */
#endif	notdef

	return;
} /* mmcodec_dts_datamode_pipes */


void
mmcodec_dts_cntlmode_pipes(driver)
	dbri_dev_t	*driver;
{
	aprintf("mmcodec_dts_cntlmode_pipes: 0x%x\n", (unsigned int)driver);
	/*
	 * This set's up the pipes so that the linked list is the
	 * same all the time. Transmit side looks like 16-[23]-20-16, and
	 * the receive side looks like 16-[24]-21-22-16
	 * We can't use the dbri_setup_dts routine since these don't/can't
	 * show up in the Channel table, but code is stolen from there
	 * It still needs to reflect the list in the pipe table though
	 * so that the above routine (and others) continue to work
	 */

	if (driver->ser_sts.pal_present) {
		mmcodec_do_dts(driver, DBRI_PIPE_SB_PAL_T,
		/* old, next */	DBRI_PIPE_CHI, DBRI_PIPE_CHI,
		/* dir, c, l */	DBRI_OUT, CHI_CTRL_MODE_LEN, SB_PAL_LEN);

		mmcodec_do_dts(driver, DBRI_PIPE_SB_PAL_R,
		/* old, next */	DBRI_PIPE_CHI, DBRI_PIPE_CHI,
		/* dir, c, l */	DBRI_IN, 0, SB_PAL_LEN);
	}

	mmcodec_do_dts(driver, DBRI_PIPE_CM_T_1_4,
		((unsigned int) (driver->ser_sts.pal_present ?
		DBRI_PIPE_SB_PAL_T : DBRI_PIPE_CHI)),		/* old */
		DBRI_PIPE_CHI,					/* next */
		DBRI_OUT, CM_T_1_4_CYCLE, CM_T_1_4_LEN);	/* dir, c, l */


	mmcodec_do_dts(driver, DBRI_PIPE_CM_R_1_2,
		((unsigned int) (driver->ser_sts.pal_present ?
		DBRI_PIPE_SB_PAL_R : DBRI_PIPE_CHI)),		/* old */
		DBRI_PIPE_CHI,					/* next */
		DBRI_IN, CM_R_1_2_CYCLE, CM_R_1_2_LEN);		/* dir, c, l */

	mmcodec_do_dts(driver, DBRI_PIPE_CM_R_3_4,
	/* old, next */	DBRI_PIPE_CM_R_1_2, DBRI_PIPE_CHI,
	/* dir, c, l */	DBRI_IN, CM_R_3_4_CYCLE, CM_R_3_4_LEN);

	mmcodec_do_dts(driver, DBRI_PIPE_CM_R_7,
	/* old, next */	DBRI_PIPE_CM_R_3_4, DBRI_PIPE_CHI,
	/* dir, c, l */	DBRI_IN, CM_R_7_CYCLE, CM_R_7_LEN);

	return;
} /* mmcodec_dts_cntlmode_pipes */


/*
 * mmcodec_setup_chi - Setup CHI bus appropriately; this is hardcoded for
 * a single MMCODEC on the CHI bus, with or without a Speakerbox PAL.
 */
void
mmcodec_setup_chi(driver, how)
	dbri_dev_t	*driver;
	int		how;
{
	dbri_chip_cmd_t	cmd;
	serial_status_t	*serialp;
	unsigned int	chicm;

	/*
	 * The first thing to do when changing clock parameters (according
	 * to Harry French) is to disable CHI so that everything is reset.
	 * Spec says "does not drive CHIDX or CHIDR when XEN is 0".
	 */
	serialp = &driver->ser_sts;
	serialp->chi_cdm = (DBRI_CMD_CDM |
			    DBRI_CDM_XCE |	/* transmit on rising edge */
			    DBRI_CDM_REN);	/* receive enable */

	cmd.cmd_wd1 = serialp->chi_cdm;			/* CDM command */
	dbri_command(driver, cmd);

	/* Set up parameters for CHI and CDM commands */
	/*
	 * XXX4 - bug#15, bpf must always be zero; shouldn't hurt V5?
	 */
	switch (how) {
	case SETUPCHI_CTRL_MODE:
	case SETUPCHI_DBRI_MASTER:
		chicm = (256*6)/CHI_CTRL_MODE_LEN;
		break;

	case SETUPCHI_CODEC_MASTER:
		chicm = 1;		/* CHICK is an input NOT 8kHz */
		break;

	default:
		(void) printf("audio: invalid setup!\n");
		break;
	}

	/* issue CHI command */
	serialp->chi_cmd =	DBRI_CMD_CHI |		/* CHI cmd */
				DBRI_CHI_CHICM(chicm) |	/* clock mode */
				DBRI_CHI_INT |		/* report now */
				DBRI_CHI_CHIL |		/* gen CHIL ints */
							/* no open drain */
							/* FS sampled falling */
				DBRI_CHI_FD |		/* FS driven rising */
				DBRI_CHI_BPF(0);	/* bpf always 0 */


	cmd.cmd_wd1 = serialp->chi_cmd;			/* CHI command */
	dbri_command(driver, cmd);

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 			/* PAUSE command */
	dbri_command(driver, cmd);

	return;
} /* mmcodec_setup_chi */

void
mmcodec_enable_chi(driver)
	dbri_dev_t	*driver;
{
	dbri_chip_cmd_t	cmd;
	serial_status_t	*serialp;

	serialp = &driver->ser_sts;
	serialp->chi_cdm = (DBRI_CMD_CDM | 	/* CDM command */
						/* xmit on CHIDX */
						/* rcv data on CHIDR */
						/* falling recv data */
						/* recv one clock per bit */
						/* strobe on even clk */
						/* xmit one clock per bit */
			    DBRI_CDM_XCE |	/* transmit on rising edge */
			    DBRI_CDM_XEN |	/* transmit enable */
			    DBRI_CDM_REN);	/* receive enable */

	cmd.cmd_wd1 = serialp->chi_cdm;			/* CDM command */
	dbri_command(driver, cmd);

	return;
} /* mmcodec_enable_chi */

void
mmcodec_watchdog(driver)
	dbri_dev_t	*driver;
{
	dbri_stream_t	*dsp;
	pipe_tab_t	*pipep;
	struct iocblk	*iocp;
	int		s;

	s = splr(driver->intrlevel);
	printf("audio %d: Unable to communicate with speakerbox.\n",
		driver->ser_sts.chi_state);
	driver->ser_sts.chi_state = CHI_NO_DEVICES;
	driver->chip->n.pio = SCHI_ENA_ALL | SCHI_SET_DATA_MODE |
		SCHI_SET_INT_PDN | SCHI_CLR_RESET | SCHI_SET_PDN;

	dsp = &driver->ioc[(int)DBRI_AUDIO_IN];
	pipep = &driver->ptab[dsp->pipe];
	if ((dsp->pipe != DBRI_BAD_PIPE) && ISPIPEINUSE(pipep)) {
		if (dsp->mp != NULL) {
			dsp->mp->b_datap->db_type = M_IOCNAK;
			iocp = (struct iocblk *)dsp->mp->b_rptr;
			iocp->ioc_rval = -1;
			iocp->ioc_error =  EIO;
			qreply(dsp->as.writeq, dsp->mp);
			dsp->mp = NULL;
		}
	}
	dsp = &driver->ioc[(int)DBRI_AUDIO_OUT];
	pipep = &driver->ptab[dsp->pipe];
	if ((dsp->pipe != DBRI_BAD_PIPE) && ISPIPEINUSE(pipep)) {
		if (dsp->mp != NULL) {
			dsp->mp->b_datap->db_type = M_IOCNAK;
			iocp = (struct iocblk *)dsp->mp->b_rptr;
			iocp->ioc_rval = -1;
			iocp->ioc_error =  EIO;
			qreply(dsp->as.writeq, dsp->mp);
			dsp->mp = NULL;
		}
	}

#ifndef	NOTYET
	/*
	 * Only wakeup opens, not initial setup (from startup audio) or
	 * ioctls. If you "wakeup too early", dbri will panic a C2
	 * (see bugid# 1092706)
	 */
	if (driver->ser_sts.chi_flags & CHI_FLAG_OPEN_SLEEP) {
		driver->ser_sts.chi_flags &= ~(CHI_FLAG_OPEN_SLEEP);
		wakeup((caddr_t)driver);
	}
#endif	!NOTYET
	splx(s);
	return;
} /* mmcodec_watchdog */


/*
 * Start the process of putting the MMCODEC into Control Mode; we have to
 * wait for interrupts from DBRI before things can finish, so the rest of
 * this is driven by the state machine at interrupt level. Check out
 * dbri_fxdt_intr, to see how it works
 */
void
mmcodec_setup_ctrl_mode(driver)
	dbri_dev_t	*driver;
{
	int		s;
	u_char		temp;
	unsigned int	pipe;
	pipe_tab_t	*pipep;
	dbri_chip_cmd_t	cmd;
	mmcodec_data_t	mmdata;
	mmcodec_ctrl_t	mmctrl;

	aprintf("mmcodec_setup_ctrl_mode %d:\n",
		driver->ser_sts.chi_state);

	/*
	 * 1) Volume attenuation should be increased to Max; mute
	 * 	all outputs (if CODEC is already powered down (or
	 *	previously not there), or all outputs are already
	 *	muted, we don't need to do this)
	 */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];
	mmdata.word32 = pipep->ssp;
	if ((driver->ser_sts.chi_state == CHI_POWERED_DOWN) ||
		(driver->ser_sts.chi_state == CHI_NO_DEVICES) ||
		(!mmdata.r.om1 && !mmdata.r.om0 && !mmdata.r.sm)) {

		timeout((int(*)())mmcodec_watchdog, (caddr_t)driver,
		    MMCODEC_TIMEOUT);
	} else if (driver->ser_sts.chi_state != CHI_GOING_TO_CTRL_MODE) {

		mmcodec_do_dts(driver, DBRI_PIPE_DM_R_5_6,
		((unsigned int) (driver->ser_sts.pal_present ?
		DBRI_PIPE_SB_PAL_R : DBRI_PIPE_CHI)),		/* old */
		DBRI_PIPE_CHI,					/* next */
		DBRI_IN, DM_R_5_6_CYCLE, DM_R_5_6_LEN);		/* dir, c, l */

		mmdata.r.lo = MMCODEC_MAX_DEV_ATEN;
		mmdata.r.ro = MMCODEC_MAX_DEV_ATEN;
		mmdata.r.sm = ~MMCODEC_SM & 1;
		mmdata.r.om0 = ~MMCODEC_OM0_ENABLE & 1;
		mmdata.r.om1 = ~MMCODEC_OM1_ENABLE & 1;
		/*
		 * Be sure not to save these new values in pipep->ssp so
		 * that we can restore them to their original values later
		 */
		driver->ser_sts.chi_state = CHI_GOING_TO_CTRL_MODE;
		cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
		cmd.cmd_wd2 = reverse_bit_order(mmdata.word32, DM_T_5_8_LEN);
		ATRACEI(mmcodec_setup_ctrl_mode, 'SSP ', cmd.cmd_wd2);
		dbri_command(driver, cmd);
		cmd.cmd_wd1 = DBRI_CMD_PAUSE;
		dbri_command(driver, cmd);
		timeout((int(*)())mmcodec_watchdog, (caddr_t)driver,
		    MMCODEC_TIMEOUT);
		return;
	}
	/*
	 * 2) Stop receiving audio data
	 *	SDP NULL has been issued previously to now
	 */
	/*
	 * 3) Set D/C low to indicate control mode
	 */
	temp = driver->chip->n.pio;
	aprintf("mmcodec_setup_ctrl_mode: reg2 initially 0x%x\n",
		(unsigned int)temp);
	if (driver->ser_sts.pal_present) {
		driver->chip->n.pio = SCHI_ENA_ALL | SCHI_SET_CTRL_MODE |
		    SCHI_SET_INT_PDN | SCHI_CLR_RESET | SCHI_CLR_PDN;
	} else {
		driver->chip->n.pio = SCHI_ENA_ALL | SCHI_SET_CTRL_MODE |
		    SCHI_CLR_INT_PDN | SCHI_CLR_RESET | SCHI_SET_PDN;
	}

	/*
	 * 4) Wait 12 SCLKS or 12/(5.5125*64) ms
	 */
	DELAY(34); /* param is an int */
	driver->ser_sts.chi_state = CHI_IN_CTRL_MODE;
	temp = driver->chip->n.pio;
	aprintf("mmcodec_setup_ctrl_mode: reg2 finally 0x%x\n",
		(unsigned int)temp);

	/*
	 * 6) Send Control information to MMCODEC w/ DCB bit low
	 */
	pipe = DBRI_PIPE_CM_T_1_4;
	pipep = &driver->ptab[pipe];
	mmctrl.word32[0] = pipep->ssp;
	mmctrl.r.dcb = 0;
	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe); /* SSP command */
	pipep->ssp = mmctrl.word32[0];
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, CM_T_1_4_LEN);
	ATRACEI(mmcodec_setup_ctrl_mode, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);
	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	s = splr(driver->intrlevel);
	mmcodec_dts_cntlmode_pipes(driver);

	/*
	 * 5) Start driving FSYNC and SCLK with DBRI
	 */
	mmcodec_setup_chi(driver, SETUPCHI_CTRL_MODE);
	driver->ser_sts.chi_state = CHI_WAIT_DCB_LOW;
	mmcodec_enable_chi(driver);
	(void) splx(s);

	return;
} /* mmcodec_setup_ctrl_mode */

/*
 * The first 194 frames of data mode is when the CODEC does it's
 * auto-calibration, so data isn't valid for recording and the CODEC
 * won't play any data, until these are over. This routine gets
 * called after auto cal to send or receive data
 */
void
mmcodec_data_mode_ok(driver)
	dbri_dev_t	*driver;
{
	unsigned int		pipe;
	pipe_tab_t		*pipep;
	dbri_chip_cmd_t		cmd;
	dbri_stream_t		*dsp;
	union dbri_dtscmd	dts;
	union dbri_dtstsd	out_tsd;
	int			s;

	/*
	 * One of the few routines called from timeout, need to block intrs
	 */
	s = splr(driver->intrlevel);
	aprintf("mmcodec_data_mode_ok: state=%d\n", driver->ser_sts.chi_state);
	if (driver->ser_sts.chi_state != CHI_CODEC_CALIBRATION) {
		return;
	}

	/* Now we can unmute outputs */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe]; /* ctrl in data stream */
	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(mmcodec_data_mode_ok, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd); /* Set short pipe data */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	/*
	 * NOW restart any pipes that were stopped while we reconfig'd
	 * the codec.
	 */
	dsp = &driver->ioc[(int)DBRI_AUDIO_IN];
	pipep = &driver->ptab[dsp->pipe];
	if ((dsp->pipe != DBRI_BAD_PIPE) && ISPIPEINUSE(pipep)) {
		aprintf("mmcodec_data_mode_ok: dts recv pipe %d\n", dsp->pipe);
		(void) dbri_setup_dts(pipep->as, dsp->pipe, DBRI_AUDIO_IN);
		cmd.cmd_wd1 = pipep->dts.word32;
		cmd.cmd_wd2 = pipep->in_tsd.word32;
		cmd.cmd_wd3 = pipep->out_tsd.word32;
		dbri_command(driver, cmd);

		/* XXX - call dbri_start? call before DTS? */

		if (dsp->mp != NULL) {
			ATRACEI(mmcodec_data_mode_ok, 'Rrpy', dsp->pipe);
			dsp->mp->b_datap->db_type = M_IOCACK;
			qreply(dsp->as.writeq, dsp->mp);
			dsp->mp = NULL;
			audio_sendsig(dsp->as.control_as);
		}
	}

	dsp = &driver->ioc[(int)DBRI_AUDIO_OUT];
	pipep = &driver->ptab[dsp->pipe];
	if ((dsp->pipe != DBRI_BAD_PIPE) && ISPIPEINUSE(pipep)) {
		aprintf("mmcodec_data_mode_ok: dts xmit pipe %d\n", dsp->pipe);
		(void) dbri_setup_dts(pipep->as, dsp->pipe, DBRI_AUDIO_OUT);
		cmd.cmd_wd1 = pipep->dts.word32;
		cmd.cmd_wd2 = pipep->in_tsd.word32;
		cmd.cmd_wd3 = pipep->out_tsd.word32;
		dbri_command(driver, cmd);

		/* XXX - call dbri_start? call before DTS? */

		if (dsp->mp != NULL) {
			ATRACEI(mmcodec_data_mode_ok, 'Trpy', dsp->pipe);
			dsp->mp->b_datap->db_type = M_IOCACK;
			qreply(dsp->as.writeq, dsp->mp);
			dsp->mp = NULL;
			audio_sendsig(dsp->as.control_as);
		}
	} else {
		/*
		 * Set up dummy data pipe to send zeroes; (XXX - this
		 * just screws up 8bit stereo even more.) The way we
		 * get away with this is by not putting any values in
		 * the pipe table, thereby hiding pipe 26 from the rest
		 * of the driver. When a *real* data pipe get's defined
		 * in dbri_setup_dts, it will just get plopped in the
		 * linked list where 26 was, and 26 will be out
		 *
		 * Dummy data is always 32 bits, regardless of current
		 * MMCODEC format, for simplicity
		 *
		 * BTW, I *really* don't like this - this really sucks
		 * but got any other ideas? (Besides rewriting from scratch)
		 */
		aprintf("mmcodec_data_mode_ok: dts dummy pipe\n");
		dts.word32 = 0;
		out_tsd.word32 = 0;
		dts.r.cmd = DBRI_OPCODE_DTS;
		dts.r.id = DBRI_DTS_ID;
		dts.r.pipe = DBRI_PIPE_DUMMYDATA;
		dts.r.vo = DBRI_DTS_VO;
		dts.r.oldout = ((unsigned int)
		    (driver->ser_sts.pal_present ?
		    DBRI_PIPE_SB_PAL_T : DBRI_PIPE_CHI));
		out_tsd.r.len = DUMMYDATA_LEN;
		out_tsd.r.cycle = DUMMYDATA_CYCLE;
		out_tsd.r.mode = DBRI_DTS_SINGLE;
		out_tsd.r.next = DBRI_PIPE_DM_T_5_8;

		cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(DBRI_PIPE_DUMMYDATA);
		cmd.cmd_wd2 = reverse_bit_order(driver->ser_sts.chi_dd_val,
		    DUMMYDATA_LEN);
		dbri_command(driver, cmd);

		cmd.cmd_wd1 = dts.word32;
		cmd.cmd_wd2 = 0;
		cmd.cmd_wd3 = out_tsd.word32;
		dbri_command(driver, cmd);
	}

	/*
	 * Only wakeup opens, not initial setup (from startup audio) or
	 * ioctls. If you "wakeup too early", dbri will panic a C2
	 * (see bugid# 1092706)
	 */
	if (driver->ser_sts.chi_flags & CHI_FLAG_OPEN_SLEEP) {
		driver->ser_sts.chi_flags &= ~(CHI_FLAG_OPEN_SLEEP);
		wakeup((caddr_t)driver);
		aprintf("mmcodec_data_mode_ok: wakeup!\n");
	}

	driver->ser_sts.chi_state = CHI_IN_DATA_MODE;

	splx(s);
	return;

} /* mmcodec_data_mode_ok */

void
mmcodec_setup_data_mode(driver)
	dbri_dev_t	*driver;
{
	u_char		temp;
	dbri_stream_t	*dsp;
	int		mute_time;

	untimeout((int(*)())mmcodec_watchdog, (caddr_t)driver);

	aprintf("mmcodec_setup_data_mode: driver=0x%x\n", (unsigned int)driver);

	/*
	 * 13) Start sending data
	 */
	mmcodec_dts_datamode_pipes(driver);

	mmcodec_enable_chi(driver);

	/*
	 * 14) Set D/C pin high to indicate data mode
	 */
	temp = driver->chip->n.pio;
	aprintf("mmcodec_setup_data_mode: reg2 initially 0x%x\n",
		(unsigned int)temp);
	driver->chip->n.pio |= SCHI_SET_DATA_MODE;
	temp = driver->chip->n.pio;
	aprintf("mmcodec_setup_data_mode: reg2 finally 0x%x\n",
		(unsigned int)temp);

	/*
	 * Calculate how long auto-calibration is suppoosed to take
	 * calc is 1/<sample_rate>*194 frames which turns out to
	 * either be 1, 2, or 3 clock ticks - pick which one it is
	 *
	 * XXX - fix this - it assumes 10ms clock ticks
	 */
	dsp = &driver->ioc[(int)DBRI_AUDIO_IN];
	if (dsp->as.info.sample_rate >= 22050)
		mute_time = 1;
	else if (dsp->as.info.sample_rate >= 11025)
		mute_time = 2;
	else
		mute_time = 3;
	aprintf("mmcodec_setup_data_mode: mute time is %d\n", mute_time);
	timeout((int(*)())mmcodec_data_mode_ok, (caddr_t)driver, mute_time);

	/* reset back to zero */
	driver->ser_sts.chi_nofs = 0;
	return;
} /* mmcodec_setup_data_mode */


void
mmcodec_startup_audio(driver)
	dbri_dev_t	*driver;
{
	pipe_tab_t	*pipep;
	aud_stream_t	*as;

	pipep = &driver->ptab[DBRI_PIPE_CHI];
	if (!ISPIPEINUSE(pipep)) {
		mmcodec_init_pipes(driver);
	}
	/*
	 * Always start up as ulaw; AUDIO_IN is as good an "as" as any
	 */
	as = &driver->ioc[(int)DBRI_AUDIO_IN].as;
	if (driver->ser_sts.chi_state == CHI_POWERED_DOWN) {
		mmcodec_set_audio_config(driver, as,
			8000, 1, 8, AUDIO_ENCODING_ULAW);
	}
} /* mmcodec_startup_audio */


/*
 * audio_con_stream - connect an audio stream; could be play or record
 */
int
audio_con_stream(driver, as, error)
	dbri_dev_t	*driver;
	aud_stream_t	*as;
	int		*error;
#ifdef	NOTYET
{
#ifdef	lint
	as = as;
#endif	lint

	/* See if we timed out */
	if (driver->ser_sts.chi_state == CHI_NO_DEVICES) {
		/*
		 * Start things back up, but still return an error;
		 */
		mmcodec_set_audio_config(driver, as,
					8000, 1, 8, AUDIO_ENCODING_ULAW);
		*error = ENODEV;
		return (DBRI_BADCONN);
	} else {
		return (DBRI_GOODCONN);
	}

} /* audio_con_stream */
#else
{
	int		skip = FALSE;

	aprintf("audio_con_stream:\n");
	/*
	 * If both a reader and a writer open at the same time, we
	 * need to make sure the following is only done once ... eg: if
	 * reader is sleeping, waiting for completion, then the writer
	 * should not call "set_audio_config", but should go to sleep
	 * waiting for the reader's sleep to wake up and then they can
	 * both continue.
	 */
	if ((as->play_as->info.open && as->record_as->info.open) &&
			(as->play_as->readq != as->record_as->readq)) {
		skip = TRUE;
	}
	if (!skip) {
		mmcodec_set_audio_config(driver, as,
					    8000, 1, 8, AUDIO_ENCODING_ULAW);

		driver->ser_sts.chi_flags |= CHI_FLAG_OPEN_SLEEP;
		(void) sleep((caddr_t)driver, MMCODEC_SLEEPPRI);
		aprintf("audio_con_stream:sleep WOKEUP!\n");
	} else {
		aprintf("audio_con_stream: skipping config\n");
	}

	/* See if we timed out */
	if (driver->ser_sts.chi_state == CHI_NO_DEVICES) {
		*error = ENODEV;
		return (DBRI_BADCONN);
	} else {
		return (DBRI_GOODCONN);
	}

} /* audio_con_stream */
#endif NOTYET


/*
 * audio_xcon_stream - disconnect an audio stream; dbri_disconnect has
 * already issued SDP to stop and deleted the pipe - we just cleanup for audio
 * if this is the "last close"
 */
/*ARGSUSED*/
void
audio_xcon_stream(driver, as)
	dbri_dev_t	*driver;
	aud_stream_t	*as;
{
#ifdef	NOTYET
	unsigned int	pipe;
	pipe_tab_t	*ipipep, *opipep;


	/*
	 * XXX - should not reset if there is a serial to serial connection
	 * left (except that'll be mulaw anyway?)
	 */
	/*
	 * The "open" flag doesn't get reset until after audio_close
	 * has finished so we can't check that to determine if both
	 * play and record are now closed.
	 */
	pipe = driver->ioc[(int)DBRI_AUDIO_IN].pipe;
	ipipep = &driver->ptab[pipe];
	pipe = driver->ioc[(int)DBRI_AUDIO_OUT].pipe;
	opipep = &driver->ptab[pipe];
	if (!ISPIPEINUSE(ipipep) && !ISPIPEINUSE(opipep)) {
		/*
		 * XXX - We could put a check here to not do this if already
		 * mulaw, but this is what puts things back together if
		 * the speakerbox gets unplugged;
		 */
		mmcodec_set_audio_config(driver, as,
					8000, 1, 8, AUDIO_ENCODING_ULAW);
	}
#endif	NOTYET
	dbri_stream_t		*dsp;
	dbri_chip_cmd_t		cmd;
	union dbri_dtscmd	dts;
	union dbri_dtstsd	out_tsd;

	/*
	 * XXX - should not reset if there is a serial to serial connection
	 * left (except that'll be mulaw anyway?)
	 */
	dsp = &driver->ioc[(int)DBRI_AUDIO_OUT];
	if (dsp == (dbri_stream_t *)as) {
		/*
		 * Closing the audio out stream; I wonder if we want to
		 * do this for B Channels as well?
		 */
		/*
		 * Set up dummy data pipe to send zeroes; (XXX - this
		 * just screws up 8bit stereo even more.) The way we
		 * get away with this is by not putting any values in
		 * the pipe table, thereby hiding pipe 26 from the rest
		 * of the driver. When a *real* data pipe get's defined
		 * in dbri_setup_dts, it will just get plopped in the
		 * linked list where 26 was, and 26 will be out
		 *
		 * Dummy data is always 32 bits, regardless of current
		 * MMCODEC format, for simplicity
		 *
		 * BTW, I *really* don't like this - this really sucks
		 * but got any other ideas? (Besides rewriting from scratch)
		 */
		dts.word32 = 0;
		out_tsd.word32 = 0;
		dts.r.cmd = DBRI_OPCODE_DTS;
		dts.r.id = DBRI_DTS_ID;
		dts.r.pipe = DBRI_PIPE_DUMMYDATA;
		dts.r.vo = DBRI_DTS_VO;
		dts.r.oldout = ((unsigned int)
		    (driver->ser_sts.pal_present ?
		    DBRI_PIPE_SB_PAL_T : DBRI_PIPE_CHI));
		out_tsd.r.len = DUMMYDATA_LEN;
		out_tsd.r.cycle = DUMMYDATA_CYCLE;
		out_tsd.r.mode = DBRI_DTS_SINGLE;
		out_tsd.r.next = DBRI_PIPE_DM_T_5_8;

		cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(DBRI_PIPE_DUMMYDATA);
		cmd.cmd_wd2 = reverse_bit_order(driver->ser_sts.chi_dd_val,
		    DUMMYDATA_LEN);
		dbri_command(driver, cmd);

		cmd.cmd_wd1 = dts.word32;
		cmd.cmd_wd2 = 0;
		cmd.cmd_wd3 = out_tsd.word32;
		dbri_command(driver, cmd);
	}

	return;
} /* audio_xcon_stream */


/*
 * mmcodec_check_audio_config - returns error if invalid configuration
 */
int
mmcodec_check_audio_config(driver, sample_rate, channels, precision, encoding)
	dbri_dev_t	*driver;
	unsigned int	sample_rate;
	unsigned int	channels;
	unsigned int	precision;
	unsigned int	encoding;
{
	mmcodec_ctrl_t	mmctrl;

	aprintf("mmcodec_check_audio_config: ");
	aprintf("sample_rate to %u ", sample_rate);
	aprintf("channels to %u ", channels);
	aprintf("precision to %u ", precision);
	aprintf("encoding to %u ", encoding);
	aprintf("\n");

	mmctrl.word32[0] = driver->ptab[DBRI_PIPE_CM_T_1_4].ssp;
	if (!Modify(encoding)) {
		if (mmctrl.r.df == MMCODEC_DF_ULAW)
			encoding = AUDIO_ENCODING_ULAW;
		else if (mmctrl.r.df == MMCODEC_DF_ALAW)
			encoding = AUDIO_ENCODING_ALAW;
		else
			encoding = AUDIO_ENCODING_LINEAR;
	}

	switch (encoding) {
	case AUDIO_ENCODING_ULAW:
	case AUDIO_ENCODING_ALAW:
		if (Modify(channels) && (channels != 1))
			return (EINVAL);
		if (Modify(precision) && (precision != 8))
			return (EINVAL);
		if (Modify(sample_rate) && (sample_rate != 8000))
			return (EINVAL);
		break;
	case AUDIO_ENCODING_LINEAR:
		if (Modify(channels) && (channels != 2) && (channels != 1))
			return (EINVAL);
		if (Modify(precision) && (precision != 16))
			return (EINVAL);
		if (Modify(sample_rate)) {
			/*
			 * Current thinking is that close only
			 * counts in horseshoes and handgrenades and
			 * audio API's - the driver wants it exact.
			 */
			switch (sample_rate) {
			case 8000:
			case 16000:
			case 18900:
			case 32000:
			case 37800:
			case 44100:
			case 48000:
			case 9600:
			/* semi-supported now means supported */
			case 5512:
			case 11025:
			case 27429:
			case 22050:
			case 33075:
			case 6615:
				break;
			default:
				return (EINVAL);
			}
		}
		break;
	default:
		return (EINVAL);
	} /* switch */

	return (0);
} /* mmcodec_check_audio_config */


/*
 * mmcodec_set_audio_config - Set the configuration of the "audio device" to
 * these parameters; although the encoding can imply the values for the rest
 * of the parameters, they need to be valid as these are used to fill in the
 * global values returned to user programs
 */
void
mmcodec_set_audio_config(driver, as, sample_rate, channels, precision, encoding)
	dbri_dev_t	*driver;
	aud_stream_t	*as;
	unsigned int	sample_rate;
	unsigned int	channels;
	unsigned int	precision;
	unsigned int	encoding;
{
	mmcodec_ctrl_t	mmctrl;
	dbri_stream_t	*iocp_in, *iocp_out;

	aprintf("mmcodec_set_audio_config: ");
	aprintf("sample_rate-%u ", sample_rate);
	aprintf("channels-%u ", channels);
	aprintf("precision-%u ", precision);
	aprintf("encoding-%u ", encoding);
	aprintf("\n");

	iocp_out = AsToDs(as->play_as);
	iocp_in = AsToDs(as->record_as);

	mmctrl.word32[0] = driver->ptab[DBRI_PIPE_CM_T_1_4].ssp;

	/*
	 * Change the input buffer size to reflect new format; for
	 * consistency, we try to keep it about around an 8th of a second
	 * but we can't go above DBRI_MAXPACKET
	 */
	if (encoding == AUDIO_ENCODING_ULAW) {
		mmctrl.r.df = MMCODEC_DF_ULAW;
		mmctrl.r.st = MMCODEC_ST_MONO;
		mmctrl.r.dfr = MMCODEC_DFR_8000;
		if (ISCONTROLSTREAM(as) &&
		    (driver->ser_sts.chi_flags & CHI_FLAG_SER_STS)) {
			/* serial to serial connection setup */
			mmctrl.r.xclk = ~MMCODEC_XCLK & 1;	/* no xmit clk*/
			mmctrl.r.bsel = MMCODEC_BSEL_256;
			mmctrl.r.mck = MMCODEC_MCK_MSTR;
		} else {			/* direct audio connection */
			mmctrl.r.xclk = MMCODEC_XCLK;
			mmctrl.r.bsel = MMCODEC_BSEL_128;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
		}
		iocp_in->as.input_size = Dbri_recv_bsize;
		driver->ser_sts.chi_dd_val = ULAW_ZERO;

	} else if (encoding == AUDIO_ENCODING_ALAW) {
		mmctrl.r.df = MMCODEC_DF_ALAW;
		mmctrl.r.st = MMCODEC_ST_MONO;
		mmctrl.r.dfr = MMCODEC_DFR_8000;
		mmctrl.r.mck = MMCODEC_MCK_XTAL1;
		iocp_in->as.input_size = Dbri_recv_bsize;
		driver->ser_sts.chi_dd_val = ALAW_ZERO;
	} else {
		/* XXX - add 8 bit stereo support sometime later */

		iocp_in->as.input_size = 
		    MIN((sample_rate * 2 * channels / 8), DBRI_MAXPACKET);
		iocp_in->as.input_size &= 0xfffffff0;
		driver->ser_sts.chi_dd_val = LINEAR_ZERO;

		mmctrl.r.df = MMCODEC_DF_16_BIT;
		if (channels == 2)
			mmctrl.r.st = MMCODEC_ST_STEREO;
		else
			mmctrl.r.st = MMCODEC_ST_MONO;
		switch (sample_rate) {
		case 16000:		/* G_722 */
			mmctrl.r.dfr = MMCODEC_DFR_16000;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		case 18900:		/* CDROM_XA_C */
			mmctrl.r.dfr = MMCODEC_DFR_18900;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 32000:		/* DAT_32 */
			mmctrl.r.dfr = MMCODEC_DFR_32000;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		case 37800:		/* CDROM_XA_AB */
			mmctrl.r.dfr = MMCODEC_DFR_37800;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 44100:		/* CD_DA */
			mmctrl.r.dfr = MMCODEC_DFR_44100;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 48000:		/* DAT_48 */
			mmctrl.r.dfr = MMCODEC_DFR_48000;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		case 9600:		/* SPEECHIO */
			mmctrl.r.dfr = MMCODEC_DFR_9600;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		case 8000:		/* ULAW and ALAW */
		default:
			mmctrl.r.dfr = MMCODEC_DFR_8000;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		/* The above are the *real* ones, these are fake */
		case 5513:
			mmctrl.r.dfr = MMCODEC_DFR_5513;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 11025:
			mmctrl.r.dfr = MMCODEC_DFR_11025;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 27429:
			mmctrl.r.dfr = MMCODEC_DFR_27429;
			mmctrl.r.mck = MMCODEC_MCK_XTAL1;
			break;
		case 22050:
			mmctrl.r.dfr = MMCODEC_DFR_22050;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 33075:
			mmctrl.r.dfr = MMCODEC_DFR_33075;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		case 6615:
			mmctrl.r.dfr = MMCODEC_DFR_6615;
			mmctrl.r.mck = MMCODEC_MCK_XTAL2;
			break;
		} /* switch */
	}
	aprintf("mmcodec_set_audio_config: mmctrl = 0x%x\n", mmctrl.word32[0]);
	driver->ptab[DBRI_PIPE_CM_T_1_4].ssp = mmctrl.word32[0];
	aprintf("changing input_size to %d\n", iocp_in->as.input_size);

	/*
	 * Update the current configuration of the audio device passed
	 * to user programs
	 */
	iocp_in->as.info.sample_rate =
			iocp_out->as.info.sample_rate = sample_rate;
	iocp_in->as.info.channels =
			iocp_out->as.info.channels = channels;
	iocp_in->as.info.precision =
			iocp_out->as.info.precision = precision;
	iocp_in->as.info.encoding =
			iocp_out->as.info.encoding = encoding;
	mmcodec_setup_ctrl_mode(driver);
	return;
} /* mmcodec_set_audio_config */


/*
 * Speaker Mute essentially determines how to set the LED more than
 * anything else. Since we have two (mutually exclusive) interests,
 * Speaker Muted and Output to Speaker enabled or disabled, and only one
 * bit (sm), this gets a litte tricky. So we use the hwi_state.output_muted
 * as the former and play.ports as the latter. Notice that the value of
 * 'sm' at any given time doesn't really tell us anything.
 */
int
mmcodec_set_spkrmute(driver, how)
	dbri_dev_t	*driver;
	int		how;
{
	int		pal_value;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;
	aud_stream_t	*as;

	as = &driver->ioc[(int)DBRI_AUDIO_OUT].as;
	pipep = &driver->ptab[DBRI_PIPE_DM_T_5_8];
	mmdata.word32 = pipep->ssp;
	/*
	 * We optimise a little bit here and if the "new" setting
	 * is the same as the existing one, don't issue the SSP
	 */
	switch (how) {
	case SPKRMUTE_SET:
		if (driver->hwi_state.output_muted == TRUE) {
			return (-1);	/* if already muted, do nothing */
		}
		mmdata.r.sm = ~MMCODEC_SM & 1;
		mmdata.r.om0 = ~MMCODEC_OM0_ENABLE & 1;
		mmdata.r.om1 = ~MMCODEC_OM1_ENABLE & 1;
		driver->hwi_state.output_muted = TRUE;
		break;
	case SPKRMUTE_GET:
		return ((int)driver->hwi_state.output_muted);
	case SPKRMUTE_CLEAR:
		if (driver->hwi_state.output_muted == FALSE) {
			return (-1);	/* if not muted, don't do anything */
		}
		if (as->info.port & AUDIO_SPEAKER) {
			mmdata.r.sm = MMCODEC_SM;
		}
		if (as->info.port & AUDIO_HEADPHONE) {
			mmdata.r.om1 = MMCODEC_OM1_ENABLE;
		}
		if (as->info.port & AUDIO_LINE_OUT) {
			mmdata.r.om0 = MMCODEC_OM0_ENABLE;
		}
		driver->hwi_state.output_muted = FALSE;
		break;
	case SPKRMUTE_TOGGLE:
		if (driver->hwi_state.output_muted == FALSE) {
			mmdata.r.sm = ~MMCODEC_SM & 1;
			mmdata.r.om0 = ~MMCODEC_OM0_ENABLE & 1;
			mmdata.r.om1 = ~MMCODEC_OM1_ENABLE & 1;
			driver->hwi_state.output_muted = TRUE;
		} else {
			if (as->info.port & AUDIO_SPEAKER) {
				mmdata.r.sm = MMCODEC_SM;
			}
			if (as->info.port & AUDIO_HEADPHONE) {
				mmdata.r.om1 = MMCODEC_OM1_ENABLE;
			}
			if (as->info.port & AUDIO_LINE_OUT) {
				mmdata.r.om0 = MMCODEC_OM0_ENABLE;
			}
			driver->hwi_state.output_muted = FALSE;
		}
		break;
	}

	/* XXX - technically, if sm didn't change we don't need to do this */
	cmd.cmd_wd1 =	DBRI_CMD_SSP | DBRI_SSP_PIPE(DBRI_PIPE_DM_T_5_8);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(mmcodec_set_spkrmute, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);

	/*
	 * Set the mute LED appropriately; notice we never save the current
	 * setting in the ssp area of pipe_tab_t; I *think* this is good
	 */
	if (driver->hwi_state.output_muted == TRUE) {
		pal_value = 1;
	} else {
		pal_value = 0;
	}

	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(DBRI_PIPE_SB_PAL_T);
	cmd.cmd_wd2 = reverse_bit_order((unsigned int)pal_value, SB_PAL_LEN);
	dbri_command(driver, cmd);

	cmd.cmd_wd1 = DBRI_CMD_PAUSE;
	dbri_command(driver, cmd);

	return (-1);		/* should not be used */
} /* mmcodec_set_spkrmute */


unsigned char
dbri_output_muted(driver, val)
	dbri_dev_t	*driver;
	unsigned char	val;
{
	if (val) {
		(void) mmcodec_set_spkrmute(driver, SPKRMUTE_SET);
	} else {
		(void) mmcodec_set_spkrmute(driver, SPKRMUTE_CLEAR);
	}
	return (driver->hwi_state.output_muted);
}

/*
 * Set output port to external jack or built-in speaker
 *
 * Notice the way the "sm" field is being overloaded here - it's both
 * a speaker muted bit, and an enable for the Speaker. If we call
 * mmcodec_set_spkrmute to set this bit, it's muting the speaker
 * and all the appropriate software things happen (LED turned on and
 * field in aud_state_t set correctly). If we set it to enable or
 * disable output to the speaker, none of this happens
 */
unsigned int
dbri_outport(driver, val)
	dbri_dev_t	*driver;
	unsigned int	val;
{
	unsigned int	pipe;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;

	aprintf("dbri_outport: driver=0x%x, val=%u\n", (unsigned int)driver,
		val);

	/* Pipes need to be setup prior to calling this routine */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe]; /* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	/*
	 * On MMCODEC om0 is AUDIO_LINE_OUT, om1 is AUDIO_HEADPHONE,
	 * and sm is effectively AUDIO_SPEAKER
	 */
	if (driver->hwi_state.output_muted == FALSE) {
		/* disable everything, then selectively enable */
		mmdata.r.sm = ~MMCODEC_SM & 1;
		mmdata.r.om0 = ~MMCODEC_OM0_ENABLE & 1;
		mmdata.r.om1 = ~MMCODEC_OM1_ENABLE & 1;
		if (val & AUDIO_SPEAKER)
			mmdata.r.sm = MMCODEC_SM;
		if (val & AUDIO_HEADPHONE)
			mmdata.r.om1 = MMCODEC_OM1_ENABLE;
		if (val & AUDIO_LINE_OUT)
			mmdata.r.om0 = MMCODEC_OM0_ENABLE;
	
		cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
		pipep->ssp = mmdata.word32;
		cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
		ATRACEI(dbri_outport, 'SSP ', cmd.cmd_wd2);
		dbri_command(driver, cmd); /* Set short pipe data */
	}

	return (val);
} /* dbri_outport */


/* Set input port to line in jack or microphone */
unsigned int
dbri_inport(driver, val)
	dbri_dev_t	*driver;
	unsigned int	val;
{
	unsigned int	pipe;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;

	aprintf("dbri_inport: driver=0x%x, val=%u\n", (unsigned int)driver,
		val);
	/* Pipes need to be setup prior to calling this routine */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];		/* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	/*
	 * Again we have an either/or situation, but on input this
	 * makes more sense.
	 */
	mmdata.r.is = (val == AUDIO_MICROPHONE) ? MMCODEC_IS_MIC
						: MMCODEC_IS_LINE;

	cmd.cmd_wd1 = DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(dbri_inport, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd); /* Set short pipe data */

	return (val);
} /* dbri_inport */

/*
 * XXX - in all the routines, dbri_play_gain, dbri_record_gain, and
 * in mmcodec_change_volume, the code mistakenly uses 0 (zero) instead
 * of using AUDIO_MIN_GAIN
 */
/*
 * Convert play gain to chip values and load them.  Return the closest
 * appropriate gain value. Use the balance field to determine what
 * to put in each of the left and right attenuation fields
 */
unsigned int
dbri_play_gain(driver, val, balance)
	dbri_dev_t	*driver;
	unsigned int	val;
	unsigned char	balance;
{
	unsigned int	pipe;
	unsigned int	la, ra, tmp;
	int		r, l;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;

	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];		/* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	r = l = val;
	if (balance < AUDIO_MID_BALANCE)
		r = MAX(0, (int)(val - ((AUDIO_MID_BALANCE - balance) <<
		    AUDIO_BALANCE_SHIFT)));
	else if (balance > AUDIO_MID_BALANCE)
		l = MAX(0, (int)(val - ((balance - AUDIO_MID_BALANCE) <<
		    AUDIO_BALANCE_SHIFT)));

	if (l == 0) {
		la = MMCODEC_MAX_DEV_ATEN;
	} else {
		la = MMCODEC_MAX_ATEN -
		    (l * (MMCODEC_MAX_ATEN+1) / (AUDIO_MAX_GAIN+1));
	}
	if (r == 0) {
		ra = MMCODEC_MAX_DEV_ATEN;
	} else {
		ra = MMCODEC_MAX_ATEN -
		    (r * (MMCODEC_MAX_ATEN+1) / (AUDIO_MAX_GAIN+1));
	}

	mmdata.r.lo = la;
	mmdata.r.ro = ra;

	cmd.cmd_wd1 =	DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(dbri_play_gain, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);		/* Set short pipe data */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	/*
	 * We end up returning a value slightly different than the one
	 * passed in - *most* applications expect this; the tmp variable
	 * is only needed for the debugging printf
	 */
	if ((val == 0) || (val == AUDIO_MAX_GAIN)) {
		tmp = val;
	} else {
		if (l == val) {
			tmp = ((MMCODEC_MAX_ATEN - la) * (AUDIO_MAX_GAIN+1) /
			    (MMCODEC_MAX_ATEN+1));
		} else if (r == val) {
			tmp = ((MMCODEC_MAX_ATEN - ra) * (AUDIO_MAX_GAIN+1) /
			    (MMCODEC_MAX_ATEN+1));
		}
	}
	aprintf("dbri_play_gain:v=%u, b=%u, l=%d, r=%d, la=%u, ra=%u, tmp=%u\n",
		val, balance, l, r, la, ra, tmp);
	return (tmp);
} /* dbri_play_gain */

/*
 * Increase or Decrease the play volume, *ONE* *device* unit at a time
 */
void
mmcodec_change_volume(driver, dir)
	dbri_dev_t	*driver;
	unsigned int	dir;
{
	unsigned int	pipe;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;
	aud_stream_t	*as_output;
	unsigned char	balance;

	as_output = &driver->ioc[(int)DBRI_AUDIO_OUT].as;
	balance = as_output->info.balance;
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];		/* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	/*
	 * Volume up button pressed - decrease attenuation; Volume down
	 * button pressed - increase attenuation. Handle left and right
	 * attenuation individually since balance may have them a different
	 * values; find out which one is the "correct" one and which one is
	 * compensating for balance
	 */
	aprintf("IN: output now %u %u\n",
	    mmdata.r.lo, mmdata.r.ro);
	if (balance <= AUDIO_MID_BALANCE) {
		if (dir == VPLUS_BUTTON) {
			if (mmdata.r.lo == 0)
				return;
			if (mmdata.r.lo == MMCODEC_MAX_DEV_ATEN)
				mmdata.r.lo = MMCODEC_MAX_ATEN - 1;
			else
				mmdata.r.lo--;
			mmdata.r.ro = MIN(MMCODEC_MAX_ATEN,
			    (int)(mmdata.r.lo + (AUDIO_MID_BALANCE - balance)));

		} else if (dir == VMINUS_BUTTON) {
			if (mmdata.r.lo == MMCODEC_MAX_DEV_ATEN)
				return;
			mmdata.r.lo++;
			mmdata.r.ro = MIN(MMCODEC_MAX_ATEN,
			    (int)(mmdata.r.lo + (AUDIO_MID_BALANCE - balance)));

			if (mmdata.r.lo == MMCODEC_MAX_ATEN)
				mmdata.r.lo = MMCODEC_MAX_DEV_ATEN;
			if (mmdata.r.ro == MMCODEC_MAX_ATEN)
				mmdata.r.ro = MMCODEC_MAX_DEV_ATEN;

		} else {
			return;
		}
		if (mmdata.r.lo == 0) {
			as_output->info.gain = AUDIO_MAX_GAIN;
		} else if (mmdata.r.lo == MMCODEC_MAX_DEV_ATEN) {
			as_output->info.gain = 0;
		} else {
			as_output->info.gain =
			    ((MMCODEC_MAX_ATEN - mmdata.r.lo) *
			    (AUDIO_MAX_GAIN+1) / (MMCODEC_MAX_ATEN+1));
		}
	} else if (balance > AUDIO_MID_BALANCE) {
		if (dir == VPLUS_BUTTON) {
			if (mmdata.r.ro == 0)
				return;
			if (mmdata.r.ro == MMCODEC_MAX_DEV_ATEN)
				mmdata.r.ro = MMCODEC_MAX_ATEN - 1;
			else
				mmdata.r.ro--;
			mmdata.r.lo = MIN(MMCODEC_MAX_ATEN,
			    (int)(mmdata.r.ro + (balance - AUDIO_MID_BALANCE)));

		} else if (dir == VMINUS_BUTTON) {
			if (mmdata.r.ro == MMCODEC_MAX_DEV_ATEN)
				return;
			mmdata.r.ro++;
			mmdata.r.lo = MIN(MMCODEC_MAX_ATEN,
			    (int)(mmdata.r.ro + (balance - AUDIO_MID_BALANCE)));

			if (mmdata.r.ro == MMCODEC_MAX_ATEN)
				mmdata.r.ro = MMCODEC_MAX_DEV_ATEN;
			if (mmdata.r.lo == MMCODEC_MAX_ATEN)
				mmdata.r.lo = MMCODEC_MAX_DEV_ATEN;

		} else {
			return;
		}
		if (mmdata.r.ro == 0) {
			as_output->info.gain = AUDIO_MAX_GAIN;
		} else if (mmdata.r.ro == MMCODEC_MAX_DEV_ATEN) {
			as_output->info.gain = 0;
		} else {
			as_output->info.gain =
			    ((MMCODEC_MAX_ATEN - mmdata.r.ro) *
			    (AUDIO_MAX_GAIN+1) / (MMCODEC_MAX_ATEN+1));
		}
	} else {
		return;
	}

	aprintf("mmcodec_change_volume: output now %u %u\n",
	    mmdata.r.lo, mmdata.r.ro);
	cmd.cmd_wd1 =	DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(mmcodec_change_volume, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);		/* Set short pipe data */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	return;
} /* mmcodec_change_volume */

/*
 * Convert record gain to chip values and load them.
 * Return the closest appropriate gain value.
 */
unsigned int
dbri_record_gain(driver, val, balance)
	dbri_dev_t	*driver;
	unsigned int	val;
	unsigned char	balance;
{
	unsigned int	pipe;
	unsigned int	lg, rg, tmp;
	int		l, r;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;

	/* Pipes need to be setup prior to calling this routine */
	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];		/* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	l = r = val;
	if (balance < AUDIO_MID_BALANCE)
		r = MAX(0, (int)(val - ((AUDIO_MID_BALANCE - balance) <<
		    AUDIO_BALANCE_SHIFT)));
	else if (balance > AUDIO_MID_BALANCE)
		l = MAX(0, (int)(val - ((balance - AUDIO_MID_BALANCE) <<
		    AUDIO_BALANCE_SHIFT)));

	lg = l * (MMCODEC_MAX_GAIN+1) / (AUDIO_MAX_GAIN+1);
	rg = r * (MMCODEC_MAX_GAIN+1) / (AUDIO_MAX_GAIN+1);

	mmdata.r.lg = lg;
	mmdata.r.rg = rg;

	cmd.cmd_wd1 =	DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(dbri_record_gain, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);		/* Set short pipe data */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	/*
	 * We end up returning a value slightly different than the one
	 * passed in - *most* applications expect this; the tmp variable
	 * is only needed for the debugging printf
	 */
	if (l == val) {
		tmp = ((lg+1) * AUDIO_MAX_GAIN) / (MMCODEC_MAX_GAIN+1);
	} else if (r == val) {
		tmp = ((rg+1) * AUDIO_MAX_GAIN) / (MMCODEC_MAX_GAIN+1);
	}
	aprintf("dbri_record_gain: v=%u,b=%u,l=%d,r=%d,lg=%u,rg=%u,tmp=%u\n",
		val, balance, l, r, lg, rg, tmp);
	return (tmp);
} /* dbri_record_gain */

/*
 * Convert monitor gain to chip values and load them.
 * Return the closest appropriate gain value.
 */
unsigned int
dbri_monitor_gain(driver, val)
	dbri_dev_t	*driver;
	unsigned int	val;
{
	unsigned int	pipe;
	int		aten, tmp;
	pipe_tab_t	*pipep;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;

	pipe = DBRI_PIPE_DM_T_5_8;
	pipep = &driver->ptab[pipe];		/* ctrl in data stream */
	mmdata.word32 = pipep->ssp;

	aten = MMCODEC_MA_MAX_ATEN -
		(val * (MMCODEC_MA_MAX_ATEN+1) / (AUDIO_MAX_GAIN+1));
	aprintf("dbri_monitor_gain: val=%u, attenuation becomes %d\n", val,
		aten);
	mmdata.r.ma = aten;

	cmd.cmd_wd1 =	DBRI_CMD_SSP | DBRI_SSP_PIPE(pipe);
	pipep->ssp = mmdata.word32;
	cmd.cmd_wd2 = reverse_bit_order(pipep->ssp, DM_T_5_8_LEN);
	ATRACEI(dbri_monitor_gain, 'SSP ', cmd.cmd_wd2);
	dbri_command(driver, cmd);		/* Set short pipe data */

	cmd.cmd_wd1 = DBRI_CMD_PAUSE; 		/* PAUSE command */
	dbri_command(driver, cmd);

	/*
	 * We end up returning a value slightly different than the one
	 * passed in - *most* applications expect this; the tmp variable
	 * is only needed for the debugging printf
	 */
	if (val == AUDIO_MAX_GAIN) {
		tmp = AUDIO_MAX_GAIN;
	} else {
		tmp = (MMCODEC_MAX_GAIN - aten) * (AUDIO_MAX_GAIN+1) /
		    (MMCODEC_MAX_GAIN+1);
	}
	aprintf("dbri_monitor_gain: val=%u, aten becomes %u, returns %u\n",
		val, aten, tmp);
	return (tmp);
} /* dbri_monitor_gain */


void
mmcodec_volume_timer(driver)
	dbri_dev_t	*driver;
{
	int		done;
	pipe_tab_t	*pipep;
	aud_stream_t	*ctrl_as;

	pipep = &driver->ptab[DBRI_PIPE_SB_PAL_R];
	done = FALSE;
	if (pipep->ssp & VPLUS_BUTTON) {
		mmcodec_change_volume(driver, VPLUS_BUTTON);
		done = TRUE;
	} else if (pipep->ssp & VMINUS_BUTTON) {
		mmcodec_change_volume(driver, VMINUS_BUTTON);
		done = TRUE;
	}
	if (done) {
		ctrl_as = &driver->ioc[(int)DBRI_AUDIOCTL].as;
		audio_sendsig(ctrl_as);
		timeout((int(*)())mmcodec_volume_timer, (caddr_t)driver,
		    sb_button_repeat);
	}
} /* mmcodec_volume_timer */

/*
 * dbri_fxdt_intr - handle interrupts from fixed pipes
 */
void
dbri_fxdt_intr(driver, intr)
	dbri_dev_t		*driver;
	dbri_intrq_ent_t	intr;
{
	unsigned int	pipe, t14d, r12d, r34d, tmp;
	int		change;
	pipe_tab_t	*pipep;
	mmcodec_ctrl_t	mmctrl;
	mmcodec_data_t	mmdata;
	dbri_chip_cmd_t	cmd;
	aud_stream_t	*ctrl_as, *as_input, *as_output;

	aprintf("dbri_fxdt_intr: channel %d, data 0x%x\n",
		intr.f.channel, (unsigned int)intr.code_fxdt.changed_data);
	switch (intr.f.channel) {
	case DBRI_PIPE_SB_PAL_R:
		pipe = intr.f.channel;
		pipep = &driver->ptab[intr.f.channel];
		tmp = reverse_bit_order(intr.code_fxdt.changed_data,
						SB_PAL_LEN);
		aprintf("PAL RECV: 0x%x\n", tmp);
		/* Ignore changes to zero from CODEC handshake */
		if (tmp == 0)
			break;
		/* Check for no change; DBRI-V1 bug #110 and V3 #7 and V5 #? */
		if (pipep->ssp == tmp)
			break;
		if ((tmp & ID_MASK) != PAL_ID) {
			/*
			 * XXX - we get constant changes on this pipe
			 * suggesting no speakerbox - why?
			 */
			aprintf("Audio: ID 0x%x, SpeakerBox disconnected?\n",
			    tmp);
			break;
		}
		change = FALSE;
		as_input = &driver->ioc[(int)DBRI_AUDIO_IN].as;
		as_output = &driver->ioc[(int)DBRI_AUDIO_OUT].as;
		ctrl_as = &driver->ioc[(int)DBRI_AUDIOCTL].as;
		if ((tmp & MIKE_IN) != (pipep->ssp & MIKE_IN)) {
			aprintf("mike status change\n");
			if (tmp & MIKE_IN) {
				as_input->info.avail_ports |= AUDIO_MICROPHONE;
			} else {
				as_input->info.avail_ports &= ~AUDIO_MICROPHONE;
			}
			change = TRUE;
		}
		if ((tmp & HPHONE_IN) != (pipep->ssp & HPHONE_IN)) {
			aprintf("headphone status change\n");
			if (tmp & HPHONE_IN) {
				as_output->info.avail_ports |= AUDIO_HEADPHONE;
			} else {
				as_output->info.avail_ports &= ~AUDIO_HEADPHONE;
			}
			change = TRUE;
		}

		/*
		 * Current button policy is:
		 * Mute reduces volume alot. If you hit either
		 * volume button while muted, it will "un-mute" the speaker
		 * Leaving button pressed continues to affect volume
		 *
		 * Since pressing the buttons really fast can leave a bunch
		 * of outstanding timeout requests, everytime we see a new
		 * button press, turn off the one from before. It's a good
		 * thing we can have as many untimeouts as we want ....
		 */
		pipep->ssp = tmp;
		if (pipep->ssp & VPLUS_BUTTON) {
			mmcodec_change_volume(driver, VPLUS_BUTTON);
			(void) mmcodec_set_spkrmute(driver, SPKRMUTE_CLEAR);
			change = TRUE;
			untimeout((int(*)())mmcodec_volume_timer,
				    (caddr_t)driver);
			timeout((int(*)())mmcodec_volume_timer,
				(caddr_t)driver, SB_BUTTON_START);
		} else if (pipep->ssp & VMINUS_BUTTON) {
			mmcodec_change_volume(driver, VMINUS_BUTTON);
			(void) mmcodec_set_spkrmute(driver, SPKRMUTE_CLEAR);
			change = TRUE;
			untimeout((int(*)())mmcodec_volume_timer,
				    (caddr_t)driver);
			timeout((int(*)())mmcodec_volume_timer,
				(caddr_t)driver, SB_BUTTON_START);
		} else if (pipep->ssp & MUTE_BUTTON) {
			(void) mmcodec_set_spkrmute(driver, SPKRMUTE_TOGGLE);
			change = TRUE;
		}
		/*
		 * change means two things - one is that a signal needs to
		 * be sent up the control device, and the other is that a
		 * button was pushed down; button UP events mean untimeout
		 */
		if (change) {
			/*
			 * audio_sendsig takes care of the fact that
			 * the control device may not be open
			 */
			audio_sendsig(ctrl_as);
		} else {
			untimeout((int(*)())mmcodec_volume_timer,
				    (caddr_t)driver);
		}
		break;

	case DBRI_PIPE_CM_R_7:
		pipep = &driver->ptab[DBRI_PIPE_CM_R_7];
		tmp = reverse_bit_order(intr.code_fxdt.changed_data,
						CM_R_7_LEN);
		/*
		 * We get transitions to zero when we first start up, and
		 * due to bug (that isn't a bug anymore) in the Analog CODEC,
		 * we have to ignore tranisitions when we're going to data mode
		 * from control mode (bit's after DCB are invalid)
		 */
		if ((tmp != 0) &&
			(driver->ser_sts.chi_state == CHI_WAIT_DCB_LOW) &&
			!(driver->ser_sts.chi_flags & CHI_FLAG_CODEC_RPTD)) {

			pipep->ssp = tmp;
			(void) printf("MMCODEC: manufacturer id %u, rev %u\n",
				CM_R_7_MANU(pipep->ssp),
				CM_R_7_REV(pipep->ssp));
			driver->ser_sts.chi_flags |= CHI_FLAG_CODEC_RPTD;
		}
		break;

	case DBRI_PIPE_CHI:
	case DBRI_PIPE_DM_T_5_8:
	case DBRI_PIPE_CM_T_1_4:
	case DBRI_PIPE_SB_PAL_T:
		aprintf("dbri_fxdt_intr: XMIT? pipe %d, data 0x%x\n",
			intr.f.channel, (unsigned int)intr.f.field);
		break;

	case DBRI_PIPE_DM_R_5_6:
		pipe = intr.f.channel;
		pipep = &driver->ptab[intr.f.channel];
		/*
		 * XX5 - non intuitive code alert
		 * the receive pipe length for this pipe is only 10, but
		 * we reverse 16 bits - this allows us to put the correct
		 * data into word16 - just don't look at the other 6 bits
		 * since they're not valid
		 */
		tmp = reverse_bit_order(intr.code_fxdt.changed_data, 16);
		/* Check for no change; DBRI-V1 bug #110 and V3 #7 and V5 #? */
		if (pipep->ssp == tmp)
			break;
		pipep->ssp = tmp;
		/*
		 * Once at max attenuation and outputs have been muted,
		 * we can go into control mode. Note that the CODEC may
		 * not really be at max attenuation since they reflect
		 * it in the data stream before it actually happens, but
		 * we should at least be muted which is what we care about
		 */
		aprintf("fxdt_intr: DM_R %d, val=0x%x\n", pipe, tmp);
		if (driver->ser_sts.chi_state == CHI_GOING_TO_CTRL_MODE) {
			mmdata.word16[0] = tmp;
			if (!mmdata.r.om1 && !mmdata.r.om0 && !mmdata.r.sm)
				mmcodec_setup_ctrl_mode(driver);
		}
		break;

	case DBRI_PIPE_DM_R_7_8:
		/* This pipe is no longer used */
		break;

	case DBRI_PIPE_CM_R_1_2:
	case DBRI_PIPE_CM_R_3_4:
		pipe = intr.f.channel;
		pipep = &driver->ptab[intr.f.channel];
		pipep->ssp = reverse_bit_order(intr.code_fxdt.changed_data, 16);
		if (pipe == DBRI_PIPE_CM_R_1_2)
			pipep->ssp &= CM_R_1_2_MASK;
		else if (pipe == DBRI_PIPE_CM_R_3_4)
			pipep->ssp &= CM_R_3_4_MASK;
		else
			return;

		t14d = driver->ptab[DBRI_PIPE_CM_T_1_4].ssp;
		r12d = driver->ptab[DBRI_PIPE_CM_R_1_2].ssp;
		r34d = driver->ptab[DBRI_PIPE_CM_R_3_4].ssp;
		aprintf("t14d=0x%x, r12d=0x%x, r34d=0x%x\n", t14d, r12d, r34d);

		/*
		 * 7) Wait for DCB from MMCODEC to go low
		 * Must check all bits as they come back to make sure
		 * they're *all* correct
		 */
		if (driver->ser_sts.chi_state == CHI_WAIT_DCB_LOW) {
			if (((t14d & CM_R_3_4_MASK) != r34d) ||
			    (((t14d >> 16) & CM_R_1_2_MASK) != r12d)) {
				aprintf("dcb not low yet\n");
				return;
			}
			aprintf("dbri_fxdt_intr: have DCB low\n");

			/*
			 * 8) Set DCB bit to MMCODEC high
			 */
			mmctrl.word32[0] = t14d;
			mmctrl.r.dcb = MMCODEC_DCB;

			pipe = DBRI_PIPE_CM_T_1_4;
			pipep = &driver->ptab[pipe];
			cmd.cmd_wd1 =	DBRI_CMD_SSP | 	DBRI_SSP_PIPE(pipe);
			pipep->ssp = mmctrl.word32[0];
			cmd.cmd_wd2 = reverse_bit_order(pipep->ssp,
							CM_T_1_4_LEN);
			ATRACEI(dbri_fxdt_intr, 'SSP ', cmd.cmd_wd2);
			dbri_command(driver, cmd);

			driver->ser_sts.chi_state = CHI_WAIT_DCB_HIGH;

		/*
		 * 9) Wait for DCB from MMCODEC to go high
		 * Due to Analog devices chip bug (which isn't a bug cause
		 * we changed the spec for them), all bits after DCB are
		 * invalid so we can't check for them - must isolate DCB bit.
		 */
		} else if (driver->ser_sts.chi_state == CHI_WAIT_DCB_HIGH) {
			mmctrl.word16[0] = r12d;
			if (!mmctrl.r.dcb) {
				/*
				 * XXX - this is actually very bad, if
				 * we're going from low to high, we're
				 * only going to get one interrupt on
				 * #21, so if we're not straight now, we
				 * never will be; Unfortunately, DBRI
				 * randomly gives us extra fxdt interrupts
				 * so this is not true.
				 */
				aprintf("dcb not high yet!!\n");
				return;
			}
			aprintf("dbri_fxdt_intr: have DCB high\n");
			/*
			 * 10) Discontinue xmitting and receiving control info
			 */
			/*
			 * 11) Stop driving FSYNC and SCLK
			 */
			driver->ser_sts.chi_state = CHI_GOING_TO_DATA_MODE;

			/*
			 * XXX This is not quite right for monitor pipes 
			 * There could be a unidirectional audio stream as well 
			 * as a bidirectional audio stream, thus the check 
			 * should be modified.
			 */

			as_input = &driver->ioc[(int)DBRI_AUDIO_IN].as;
			as_output = &driver->ioc[(int)DBRI_AUDIO_OUT].as;
			if (driver->ser_sts.chi_flags & CHI_FLAG_SER_STS) {
			 	mmcodec_setup_chi(driver, SETUPCHI_DBRI_MASTER);
			} else { 	/* /dev/audio opened directly  */
				mmcodec_setup_chi(driver,SETUPCHI_CODEC_MASTER);
			}
		} else {
			aprintf("dbri_fxdt_intr: CM R Intr state=%u\n",
				driver->ser_sts.chi_state);
		}
		break;

	default:
		(void) printf("dbri_fxdt_intr: DEFAULT! pipe=%d, data=0x%x\n",
			intr.f.channel, (unsigned int)intr.f.field);
		break;

	} /* switch on channel */

	return;
} /* dbri_fxdt_intr */


/*
 * dbri_chil_intr - handle interrupts from the CHI serial interface; this
 * routine only gets called if the channel indicates CHI (36)
 */
void
dbri_chil_intr(driver, intr)
	dbri_dev_t		*driver;
	dbri_intrq_ent_t	intr;
{
	serial_status_t	*serialp;
	dbri_stream_t	*dsp;
	dbri_chip_cmd_t	cmd;

	serialp = &driver->ser_sts;

	aprintf("dbri_chil_intr: state=%d, channel=%d, field=0x%x\n",
		driver->ser_sts.chi_state, intr.f.channel, intr.f.field);
	if (intr.code_chil.overflow) {
		eprintf("CHI receiver overflow/error\n");
		/*
		 * must disable and then reenable the CHI receiver
		 * to get things going again
		 */
		cmd.cmd_wd1 = serialp->chi_cdm;
		cmd.cmd_wd1 &= ~(DBRI_CDM_REN);
		dbri_command(driver, cmd);
		cmd.cmd_wd1 = serialp->chi_cdm;
		dbri_command(driver, cmd);

		dsp = &driver->ioc[(int)DBRI_AUDIO_IN];
		dsp->audio_uflow = TRUE;
	}
	switch (driver->ser_sts.chi_state) {
	case CHI_GOING_TO_DATA_MODE:
		/*
		 * Second CHIL - to setup data mode again
		 */
		driver->ser_sts.chi_state = CHI_CODEC_CALIBRATION;
		mmcodec_setup_data_mode(driver);
		break;
	case CHI_WAIT_DCB_LOW:
		/*
		 * First CHIL - to setup control mode
		 */
		break;
	default:
		/*
		 * XXX5 - V5 keeps issuing CHIL interrupts forever when
		 * the SB gets unplugged - must issue a CHI command to
		 * disable it; Since the CODEC handshake changes Clock
		 * masters twice (one to DBRI master and one to MMCODEC
		 * master), we could *potentially* get a "CHIL nofs"
		 * interrupt for each if each change between the 2 left
		 * no frame sync for 125us. So we don't know whether the
		 * SB is actually unplugged until we get the 3rd "CHIL nofs".
		 * This assumes a "quick" system - we have noticed that
		 * on a busy system, the hand shake can take a long time
		 * so we come up with some arbitrarily high number (like 10)
		 * to wait ....
		 * Issuing the CHI command with the interrupt bit off disables
		 * the interrupts ONCE dbri has actually executed it, hence
		 * the check for state.
		 */
		if ((intr.code_chil.nofs) &&
		    (driver->ser_sts.chi_state != CHI_NO_DEVICES)) {
		
			driver->ser_sts.chi_nofs++;
			if (driver->ser_sts.chi_nofs > MMCODEC_NOFS_THRESHOLD) {
				(void) printf("Audio: speakerbox unplugged\n");
				cmd.cmd_wd1 = serialp->chi_cmd;
				cmd.cmd_wd1 &= ~(DBRI_CHI_CHIL);
				dbri_command(driver, cmd);
				driver->ser_sts.chi_state = CHI_NO_DEVICES;
				/* Don't drive PIO lines - use defaults */
				driver->chip->n.pio = 0;
				/*
				 * XXX5 - if we're in the middle of a write,
				 * we should send up an M_ERROR
				 */
			}
		}
		break;
	} /* switch */

	return;
} /* dbri_chil_intr */


unsigned int
get_aud_pipe_cycle(as, pipe, channel)
	aud_stream_t	*as;
	unsigned int	pipe;
	dbri_chnl_t	channel;
{
	unsigned int	cycle;
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;

#ifdef	lint
	pipe = pipe;
	driver = driver;
#endif	lint

	cycle = -1;		/* a very large number */
	/*
	 * We must return different values depending on the presence of
	 * speakerbox and whether or not this is an xmit or recv pipe
	 */
	switch (channel) {
	case DBRI_AUDIO_OUT:
		cycle = ((unsigned int)
		    (driver->ser_sts.pal_present ?
		    SB_PAL_LEN : CHI_DATA_MODE_LEN));
		break;
	case DBRI_AUDIO_IN:
		cycle = (unsigned int) driver->ser_sts.mmcodec_ts_start;
		break;
	default:
		(void) printf("get_aud_pipe_cycle: bad channel!!\n");
		break;
	} /* switch */
	return (cycle);
} /* get_aud_pipe_cycle */


unsigned int
get_aud_pipe_length(as, pipe, channel)
	aud_stream_t	*as;
	unsigned int	pipe;
	dbri_chnl_t	channel;
{
	unsigned int	length;
	dbri_dev_t	*driver = (dbri_dev_t *)as->v->devdata;
	mmcodec_ctrl_t	mmctrl;

#ifdef	lint
	pipe = pipe;
	channel = channel;
#endif	lint

	length = -1;		/* only to be compatible with cycle above */
	mmctrl.word32[0] = driver->ptab[DBRI_PIPE_CM_T_1_4].ssp;

	/*
	 * As I see it, we return either 8, 16, or 32 and nothing else
	 * Right? 8 bit stereo, is uninteresting as Ben says ....  Notice
	 * length is *not* direction dependent.
	 */
	if ((mmctrl.r.df == MMCODEC_DF_ULAW) ||
	    (mmctrl.r.df == MMCODEC_DF_ALAW)) {
		length = 8;
	} else if (mmctrl.r.df == MMCODEC_DF_16_BIT) {
		if (mmctrl.r.st == MMCODEC_ST_MONO)
			length = 16;
		else
			length = 32;
	}

	return (length);
} /* get_aud_pipe_length */
