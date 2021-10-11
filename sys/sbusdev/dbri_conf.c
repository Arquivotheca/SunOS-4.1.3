#ident "@(#) dbri_conf.c 1.1@(#) Copyright (c) 1991-92 Sun Microsystems, Inc."

/*
 * Sun DBRI Dual Basic Rate Controller configuration file
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/ioccom.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <sun/audioio.h>
#include <sun/isdnio.h>
#include <sun/dbriio.h>
#include <sbusdev/audiovar.h>
#include <sbusdev/dbri_reg.h>
#include <sbusdev/mmcodec_reg.h>
#include <sbusdev/dbrivar.h>


/*
 * DBRI ISDN global variables that are patchable by the end user through
 * kadb.
 */
int	Default_T101_timer = 6000;	/* NT T101 timer in milliseconds */
					/* Range: 5-30 seconds */
int	Default_T102_timer = 33;	/* NT T102 timer in milliseconds */
					/* Range: 25-100 milliseconds */
int	Default_T103_timer = 6000;	/* TE T103 timer in milliseconds */
					/* Range: 5-30 seconds */
int	Default_T104_timer = 600;	/* TE T104 timer in milliseconds */
					/* Range: 0,500-1000 milliseconds */
enum isdn_ph_param_asmb Default_asmb = ISDN_TE_PH_PARAM_ASMB_CTS2;
int	Default_power = TRUE;		/* Power on by deafult */
int	Keepalive_timer = 4000;	/* DBRI Keep-alive timer in milliseconds */

int	Nstreams = (int)DBRI_NSTREAM;

/*
 * Dbri_panic - If TRUE, a fatal error causes a system panic, otherwise
 * the unit causing the error is disabled and the system will continue
 * without it.
 *
 * Dbri_debug - If non-zero, this driver will ensure that some percentage
 * of cpu time is spent printing DBRI-specific debugging messages to the
 * console.
 *
 * Dbri_fcscheck - If TRUE, received HDLC packets have their FCS re-checked
 * in software to be doubly sure of no packets problems between the BRI
 * wire and the CPU.
 */
int	Dbri_debug = 0;
int	Dbri_panic = FALSE;
#if defined(DBRI_SWFCS)
int	Dbri_fcscheck = TRUE;
#endif


/*
 * Port_tab - This table is indexed by port number. The 1st field in each
 *	entry, the "port", must match the array index of the entry.
 */
dbri_port_tab_t Port_tab[] = {
	/* ISDN port,		DBRI xmit chan,	DBRI recv chan    */

	{ISDN_PORT_NONE,	DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_NONSTD,		DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_HOST,		DBRI_HOST_CHNL,		DBRI_HOST_CHNL},

	/* Management ports */
	{ISDN_CTLR_MGT,		DBRI_DBRIMGT,	DBRI_DBRIMGT},	/* XXX? */
	{ISDN_TE_MGT,		DBRI_TEMGT,	DBRI_TEMGT},	/* XXX? */
	{ISDN_NT_MGT,		DBRI_NTMGT,	DBRI_NTMGT},	/* XXX? */

	/* No support for PRI */
	{ISDN_PRI_MGT,		DBRI_NO_CHNL,	DBRI_NO_CHNL},

	/* NT channel defines */
	{ISDN_NT_D,		DBRI_NT_D_OUT,		DBRI_NT_D_IN},
	{ISDN_NT_B1,		DBRI_NT_B1_OUT,		DBRI_NT_B1_IN},
	{ISDN_NT_B2,		DBRI_NT_B2_OUT,		DBRI_NT_B2_IN},
	{ISDN_NT_H,		DBRI_NT_H_OUT,		DBRI_NT_H_IN},
	{ISDN_NT_S,		DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_NT_Q,		DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_NT_B1_56,		DBRI_NT_B1_56_OUT,	DBRI_NT_B1_56_IN},
	{ISDN_NT_B2_56,		DBRI_NT_B2_56_OUT,	DBRI_NT_B2_56_IN},
	{ISDN_NT_B1_TR,		DBRI_NT_B1_TR_OUT,	DBRI_NT_B1_TR_IN},
	{ISDN_NT_B2_TR,		DBRI_NT_B2_TR_OUT,	DBRI_NT_B2_TR_IN},

	/* TE channel defines */
	{ISDN_TE_D,		DBRI_TE_D_OUT,		DBRI_TE_D_IN},
	{ISDN_TE_B1,		DBRI_TE_B1_OUT,		DBRI_TE_B1_IN},
	{ISDN_TE_B2,		DBRI_TE_B2_OUT,		DBRI_TE_B2_IN},
	{ISDN_TE_H,		DBRI_TE_H_OUT,		DBRI_TE_H_IN},
	{ISDN_TE_S,		DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_TE_Q,		DBRI_NO_CHNL,		DBRI_NO_CHNL},
	{ISDN_TE_B1_56,		DBRI_TE_B1_56_OUT,	DBRI_TE_B1_56_IN},
	{ISDN_TE_B2_56,		DBRI_TE_B2_56_OUT,	DBRI_TE_B2_56_IN},
	{ISDN_TE_B1_TR,		DBRI_TE_B1_TR_OUT,	DBRI_TE_B1_TR_IN},
	{ISDN_TE_B2_TR,		DBRI_TE_B2_TR_OUT,	DBRI_TE_B2_TR_IN},

	/* Codec defines */
	{ISDN_CHI,		DBRI_AUDIO_OUT,	DBRI_AUDIO_IN},


	/*
	 * XXX - As items are added to isdn_port_t in isdnio.h they also
	 * need to be added here
	 */
	{ISDN_LAST_PORT,	DBRI_NO_CHNL,	DBRI_NO_CHNL}
};

/*
 * Chan_tab - This table is indexed by channel number. The 1st field in each
 *	entry, the "channel", must match the array index of the entry.
 */

dbri_chan_tab_t Chan_tab[] = {
	/*
	 * DBRI chan,	ISDN port,	base pipe,		cycle, len,
	 *		direction,	mode,			samples, chans
	 *		input as,	output as,		control as,
	 *		audtype,	minor device,		isdn type,
	 *		numbufs,	name string,		input_size,
	 */
	/* TE channel defines */
	{DBRI_TE_D_OUT,	ISDN_TE_D,	DBRI_PIPE_TE_D_OUT,	17, 2,
			DBRI_OUT,	DBRI_MODE_HDLC_D,	2000, 1,
			DBRI_TE_D_IN,	DBRI_TE_D_OUT,		DBRI_TEMGT,
			AUDTYPE_BOTH,	DBRI_MINOR_TE_D,	ISDN_TYPE_TE,
			DBRI_CMDPOOL,	"DBRI_TE_D_OUT",	0,
	},

	{DBRI_TE_D_IN,	ISDN_TE_D,	DBRI_PIPE_TE_D_IN,	17, 2,
			DBRI_IN,	DBRI_MODE_HDLC,		2000, 1,
			DBRI_TE_D_IN,	DBRI_TE_D_OUT,		DBRI_TEMGT,
			AUDTYPE_BOTH,	DBRI_MINOR_NODEV,	ISDN_TYPE_TE,
			DBRI_RECBUFS,	"DBRI_TE_D_IN",		256,
	},

	{DBRI_TE_B1_OUT, ISDN_TE_B1,	DBRI_PIPE_TE_D_OUT,	0, 8,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_B1_IN,	DBRI_TE_B1_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_RW,	ISDN_TYPE_TE,
			DBRI_CMDPOOL,	"DBRI_TE_B1_OUT",	0,
	},

	{DBRI_TE_B1_IN,	ISDN_TE_B1,	DBRI_PIPE_TE_D_IN,	0, 8,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_B1_IN,	DBRI_TE_B1_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_RO,	ISDN_TYPE_TE,
			DBRI_RECBUFS,	"DBRI_TE_B1_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_TE_B2_OUT, ISDN_TE_B2,	DBRI_PIPE_TE_D_OUT,	8, 8,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_B2_IN,	DBRI_TE_B2_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_RW,	ISDN_TYPE_TE,
			DBRI_CMDPOOL,	"DBRI_TE_B2_OUT",	0,
	},

	{DBRI_TE_B2_IN,	ISDN_TE_B2,	DBRI_PIPE_TE_D_IN,	8, 8,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_B2_IN,	DBRI_TE_B2_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_RO,	ISDN_TYPE_TE,
			DBRI_RECBUFS,	"DBRI_TE_B2_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_TE_H_OUT,	ISDN_TE_H,	DBRI_PIPE_TE_D_OUT,	0, 16,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_H_IN,	DBRI_TE_H_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_H_RW,	ISDN_TYPE_TE,
			DBRI_CMDPOOL,	"DBRI_TE_H_OUT",	0,
	},

	{DBRI_TE_H_IN,	ISDN_TE_H,	DBRI_PIPE_TE_D_IN,	0, 16,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_TE_H_IN,	DBRI_TE_H_OUT,		DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_H_RO,	ISDN_TYPE_TE,
			DBRI_RECBUFS,	"DBRI_TE_H_IN",		DBRI_RECV_BSIZE,
	},

	{DBRI_TE_B1_56_OUT, ISDN_TE_B1_56, DBRI_PIPE_TE_D_OUT,	0, 7,
			DBRI_OUT,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_TE_B1_56_IN, DBRI_TE_B1_56_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_56_RW,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B1_56_OUT",	0,
	},

	{DBRI_TE_B1_56_IN, ISDN_TE_B1_56, DBRI_PIPE_TE_D_IN,	0, 7,
			DBRI_IN,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_TE_B1_56_IN, DBRI_TE_B1_56_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_56_RO,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B1_56_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_TE_B2_56_OUT, ISDN_TE_B2_56, DBRI_PIPE_TE_D_OUT,	8, 7,
			DBRI_OUT,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_TE_B2_56_IN, DBRI_TE_B2_56_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_56_RW,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B2_56_OUT",	0,
	},

	{DBRI_TE_B2_56_IN, ISDN_TE_B2_56, DBRI_PIPE_TE_D_IN,	8, 7,
			DBRI_IN,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_TE_B2_56_IN, DBRI_TE_B2_56_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_56_RO,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B2_56_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_TE_B1_TR_OUT, ISDN_TE_B1_TR, DBRI_PIPE_TE_D_OUT,	0, 8,
			DBRI_OUT,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_TE_B1_TR_IN, DBRI_TE_B1_TR_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_TR_RW,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B1_TR_OUT",	0,
	},

	{DBRI_TE_B1_TR_IN, ISDN_TE_B1,	DBRI_PIPE_TE_D_IN,	0, 8,
			DBRI_IN,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_TE_B1_TR_IN, DBRI_TE_B1_TR_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B1_TR_RO,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B1_TR_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_TE_B2_TR_OUT, ISDN_TE_B2_TR, DBRI_PIPE_TE_D_OUT,	8, 8,
			DBRI_OUT,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_TE_B2_TR_IN, DBRI_TE_B2_TR_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_TR_RW,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B2_TR_OUT",	0,
	},

	{DBRI_TE_B2_TR_IN, ISDN_TE_B2,	DBRI_PIPE_TE_D_IN,	8, 8,
			DBRI_IN,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_TE_B2_TR_IN, DBRI_TE_B2_TR_OUT,	DBRI_TEMGT,
			AUDTYPE_DATA,	DBRI_MINOR_TE_B2_TR_RO,	ISDN_TYPE_TE,
			0,		"DBRI_TE_B2_TR_IN",	DBRI_RECV_BSIZE,
	},

	/* NT channel defines */
	{DBRI_NT_D_OUT,	ISDN_NT_D,	DBRI_PIPE_NT_D_OUT,	17, 2,
			DBRI_OUT,	DBRI_MODE_HDLC,		2000, 1,
			DBRI_NT_D_IN,	DBRI_NT_D_OUT,		DBRI_NTMGT,
			AUDTYPE_BOTH,	DBRI_MINOR_NT_D,	ISDN_TYPE_NT,
			DBRI_CMDPOOL,	"DBRI_NT_D_OUT",	0,
	},

	{DBRI_NT_D_IN,	ISDN_NT_D,	DBRI_PIPE_NT_D_IN,	17, 2,
			DBRI_IN,	DBRI_MODE_HDLC,		2000, 1,
			DBRI_NT_D_IN,	DBRI_NT_D_OUT,		DBRI_NTMGT,
			AUDTYPE_BOTH,	DBRI_MINOR_NODEV,	ISDN_TYPE_NT,
			DBRI_RECBUFS,	"DBRI_NT_D_IN",		256,
	},

	{DBRI_NT_B1_OUT, ISDN_NT_B1,	DBRI_PIPE_NT_D_OUT,	0, 8,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_B1_IN,	DBRI_NT_B1_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_RW,	ISDN_TYPE_NT,
			DBRI_CMDPOOL,	"DBRI_NT_B1_OUT",	0,
	},

	{DBRI_NT_B1_IN,	ISDN_NT_B1,	DBRI_PIPE_NT_D_IN,	0, 8,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_B1_IN,	DBRI_NT_B1_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_RO,	ISDN_TYPE_NT,
			DBRI_RECBUFS,	"DBRI_NT_B1_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_NT_B2_OUT, ISDN_NT_B2,	DBRI_PIPE_NT_D_OUT,	8, 8,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_B2_IN,	DBRI_NT_B2_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_RW,	ISDN_TYPE_NT,
			DBRI_CMDPOOL,	"DBRI_NT_B2_OUT",	0,
	},

	{DBRI_NT_B2_IN,	ISDN_NT_B2,	DBRI_PIPE_NT_D_IN,	8, 8,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_B2_IN,	DBRI_NT_B2_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_RO,	ISDN_TYPE_NT,
			DBRI_RECBUFS,	"DBRI_NT_B2_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_NT_H_OUT,	ISDN_NT_H,	DBRI_PIPE_NT_D_OUT,	0, 16,
			DBRI_OUT,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_H_IN,	DBRI_NT_H_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_H_RW, 	ISDN_TYPE_NT,
			DBRI_CMDPOOL,	"DBRI_NT_H_OUT",	0,
	},

	{DBRI_NT_H_IN,	ISDN_NT_H,	DBRI_PIPE_NT_D_IN,	0, 16,
			DBRI_IN,	DBRI_MODE_HDLC,		8000, 1,
			DBRI_NT_H_IN,	DBRI_NT_H_OUT,		DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_H_RO,	ISDN_TYPE_NT,
			DBRI_RECBUFS,	"DBRI_NT_H_IN",		DBRI_RECV_BSIZE,
	},

	{DBRI_NT_B1_56_OUT, ISDN_NT_B1_56, DBRI_PIPE_NT_D_OUT,	0, 7,
			DBRI_OUT,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_NT_B1_56_IN, DBRI_NT_B1_56_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_56_RW,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B1_56_OUT",	0,
	},

	{DBRI_NT_B1_56_IN, ISDN_NT_B1_56, DBRI_PIPE_NT_D_IN,	0, 7,
			DBRI_IN,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_NT_B1_56_IN, DBRI_NT_B1_56_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_56_RO,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B1_56_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_NT_B2_56_OUT, ISDN_NT_B2_56, DBRI_PIPE_NT_D_OUT,	8, 7,
			DBRI_OUT,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_NT_B2_56_IN, DBRI_NT_B2_56_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_56_RW,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B2_56_OUT",	0,
	},

	{DBRI_NT_B2_56_IN, ISDN_NT_B2_56, DBRI_PIPE_NT_D_IN,	8, 7,
			DBRI_IN,	DBRI_MODE_HDLC,		7000, 1,
			DBRI_NT_B2_56_IN, DBRI_NT_B2_56_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_56_RO,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B2_56_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_NT_B1_TR_OUT, ISDN_NT_B1_TR, DBRI_PIPE_NT_D_OUT,	0, 8,
			DBRI_OUT,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_NT_B1_TR_IN, DBRI_NT_B1_TR_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_TR_RW,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B1_TR_OUT",	0,
	},

	{DBRI_NT_B1_TR_IN, ISDN_NT_B1_TR, DBRI_PIPE_NT_D_IN,	0, 8,
			DBRI_IN,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_NT_B1_TR_IN, DBRI_NT_B1_TR_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B1_TR_RO,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B1_TR_IN",	DBRI_RECV_BSIZE,
	},

	{DBRI_NT_B2_TR_OUT, ISDN_NT_B2_TR, DBRI_PIPE_NT_D_OUT,	8, 8,
			DBRI_OUT,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_NT_B2_TR_IN, DBRI_NT_B2_TR_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_TR_RW,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B2_TR_OUT",	0,
	},

	{DBRI_NT_B2_TR_IN, ISDN_NT_B2_TR, DBRI_PIPE_NT_D_IN,	8, 8,
			DBRI_IN,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_NT_B2_TR_IN, DBRI_NT_B2_TR_OUT,	DBRI_NTMGT,
			AUDTYPE_DATA,	DBRI_MINOR_NT_B2_TR_RO,	ISDN_TYPE_NT,
			0,		"DBRI_NT_B2_TR_IN",	DBRI_RECV_BSIZE,
	},

	/* CHI channel defines */
	{DBRI_AUDIO_OUT, ISDN_CHI,	DBRI_PIPE_CHI,		64, 8,
			DBRI_OUT,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_AUDIO_IN,	DBRI_AUDIO_OUT,		DBRI_AUDIOCTL,
			AUDTYPE_DATA,	DBRI_MINOR_AUDIO_RW,	ISDN_TYPE_OTHER,
			DBRI_CMDPOOL,	"DBRI_AUDIO_OUT",	0,
	},

	{DBRI_AUDIO_IN,	ISDN_CHI, 	DBRI_PIPE_CHI,		0, 8,
			DBRI_IN,	DBRI_MODE_XPRNT,	8000, 1,
			DBRI_AUDIO_IN,	DBRI_AUDIO_OUT,		DBRI_AUDIOCTL,
			AUDTYPE_DATA,	DBRI_MINOR_AUDIO_RO,	ISDN_TYPE_OTHER,
			DBRI_RECBUFS,	"DBRI_AUDIO_IN",	DBRI_RECV_BSIZE,
	},

	/* Management Ports */
	/*
	 * DBRI chan,	ISDN port,	base pipe,		cycle, len,
	 *		direction,	mode,			samples, chans
	 *		input as,	output as,		control as,
	 *		audtype,	minor device,		isdn type,
	 */
	{DBRI_AUDIOCTL, ISDN_PORT_NONE, DBRI_PIPE_CHI,		0, 0,
			DBRI_NO_IO,	DBRI_MODE_UNKNOWN,	0, 0,
			DBRI_AUDIO_IN,	DBRI_AUDIO_OUT,		DBRI_AUDIOCTL,
			AUDTYPE_CONTROL, DBRI_MINOR_AUDIOCTL,	ISDN_TYPE_OTHER,
			0,		"DBRI_AUDIOCTL",	0,
	},

	{DBRI_TEMGT,	ISDN_TE_MGT,	DBRI_PIPE_TE_D_IN,	0, 0,
			DBRI_NO_IO,	DBRI_MODE_UNKNOWN,	0, 0,
			DBRI_TE_D_IN,	DBRI_TE_D_OUT,		DBRI_TEMGT,
			AUDTYPE_CONTROL,DBRI_MINOR_TEMGT,	ISDN_TYPE_OTHER,
			0,		"DBRI_TEMGT",		0,
	},

	{DBRI_NTMGT,	ISDN_NT_MGT,	DBRI_PIPE_NT_D_IN,	0, 0,
			DBRI_NO_IO,	DBRI_MODE_UNKNOWN,	0, 0,
			DBRI_NT_D_IN,	DBRI_NT_D_OUT,		DBRI_NTMGT,
			AUDTYPE_CONTROL,DBRI_MINOR_NTMGT,	ISDN_TYPE_OTHER,
			0,		"DBRI_NTMGT",		0,
	},

	{DBRI_DBRIMGT,	ISDN_CTLR_MGT,	DBRI_BAD_PIPE,		0, 0,
			DBRI_NO_IO,	DBRI_MODE_UNKNOWN,	0, 0,
			DBRI_DBRIMGT,	DBRI_DBRIMGT,		DBRI_DBRIMGT,
			AUDTYPE_CONTROL,DBRI_MINOR_DBRIMGT,	ISDN_TYPE_OTHER,
			0,		"DBRI_DBRIMGT",		0,
	},

	/*
	 * XXX - user could insert new channels here but they need to
	 * insert into table above also.
	 */

	/*
	 * XXX - need to add other channels used for mmcodec such as
	 * control channels in control mode.
	 */

	/*
	 * XXX - also need some extra channels for the mmcodec data and
	 * control pipes as they can also use a base pipe and bit offsets.
	 */
};
