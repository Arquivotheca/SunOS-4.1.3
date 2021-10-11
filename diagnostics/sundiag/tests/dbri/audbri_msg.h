#ident   "@(#)audbri_msg.h 1.1 92/07/30 SMI"

/*
 * Copyright (C) 1992 by Sun Microsystems, Inc.
 */


/*
 *  Error message declarations for the DBRI/MMCODEC audio tests.
 */

extern char	*test_usage_msg;

extern char *altth_msg;
extern char *vdown_msg;
extern char *vup_msg;
extern char *mute_msg;
extern char *unmute_msg;
extern char *ctlpass_msg;
extern char *dump_msg;
extern char *marker_msg;

extern char *clip_msg;
extern char *noclip_msg;
extern char *lclip_msg;
extern char *rclip_msg;
extern char *left_msg;
extern char *right_msg;

extern char	*failed_msg;
extern char	*err_limit_msg;
extern char	*ref_msg;
extern char	*rms_msg;
extern char	*tmprms_msg;
extern char	*tmpmag_msg;
extern char	*fft_msg;
extern char	*rrms_msg;
extern char	*rfft_msg;
extern char	*au_msg;
extern char	*pgain_msg;
extern char	*rgain_msg;
extern char	*sr_msg;
extern char	*drain_msg;
extern char	*aioread_msg;
extern char	*aiowrite_msg;
extern char	*crystal_msg;

extern char	*er_device;
extern char	*er_open;
extern char	*er_mmap;
extern char	*er_setitimer;
extern char	*er_stat;
extern char	*er_truncate;
extern char	*er_ioctl;
extern char	*er_write;
extern char	*er_read;
extern char	*er_aiowrite;
extern char	*er_aioread;
extern char	*er_aiowritep;
extern char	*er_aioreadp;
extern char	*er_aiowait;
extern char	*er_readf;
extern char	*er_mem;
extern char	*er_ur;
extern char	*er_or;
extern char	*er_auclip;
extern char	*er_aucrystal;
extern char	*er_notclip;
extern char	*er_badlbtype;
extern char	*er_notvalid;
extern char	*er_lnotvalid;
extern char	*er_rnotvalid;
extern char	*er_fftmag;
extern char	*er_rms;
extern char	*er_fft;
extern char	*er_refspec;
extern char	*er_afile;
extern char	*er_precision;
extern char	*er_encoding;

/*
 *  Error codes
 */
#define TEST_ERROR	1
#define TOO_MANY_ERRORS	2

#define	ERSYS		10
#define	ERIOCTL		11
#define	ERWRITE		12
#define	ERREAD		13
#define	ERSEEK		14
#define	ERLOOP		15
#define	ERDATA		16

#define ERAUDIO		30
